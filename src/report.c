/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved.
    Copyright (c) Bingxing Wang. All Rights Reserved.
    Copyright (c) LumiaWoA authors. All Rights Reserved.

	Module Name:

		report.c

	Abstract:

		Contains Synaptics specific code for reporting samples

	Environment:

		Kernel mode

	Revision History:

--*/

#include <Cross Platform Shim\compat.h>
#include <controller.h>
#include <hid.h>
#include <HidCommon.h>
#include <spb.h>
#include <rmi4\rmiinternal.h>
#include <rmi4\f01\function01.h>
#include <rmi4\f12\function12.h>
#include <rmi4\f1a\function1a.h>
#include <report.tmh>

const USHORT gOEMVendorID = 0x7379;    // "sy"
const USHORT gOEMProductID = 0x726D;    // "rm"
const USHORT gOEMVersionID = 3708;

const PWSTR gpwstrManufacturerID = L"Synaptics";
const PWSTR gpwstrProductID = L"3708";
const PWSTR gpwstrSerialNumber = L"4";


VOID
RmiUpdateLocalPenCache(
	IN RMI4_DETECTED_OBJECTS* Data,
	IN RMI4_PEN_CACHE* Cache
)
/*++

Routine Description:

	This routine takes raw data reported by the Synaptics hardware and
	parses it to update a local cache of finger states. This routine manages
	removing lifted touches from the cache, and manages a map between the
	order of reported touches in hardware, and the order the driver should
	use in reporting.

Arguments:

	Data - A pointer to the new data returned from hardware
	Cache - A data structure holding various current finger state info

Return Value:

	None.

--*/
{
	int i, j;

	//
	// When hardware was last read, if any slots reported as lifted, we
	// must clean out the slot and old touch info. There may be new
	// finger data using the slot.
	//
	for (i = 0; i < RMI4_MAX_TOUCHES; i++)
	{
		//
		// Sweep for a slot that needs to be cleaned
		//
		if (!(Cache->PenSlotDirty & (1 << i)))
		{
			continue;
		}

		NT_ASSERT(Cache->PenDownCount > 0);

		//
		// Find the slot in the reporting list 
		//
		for (j = 0; j < RMI4_MAX_TOUCHES; j++)
		{
			if (Cache->PenDownOrder[j] == i)
			{
				break;
			}
		}

		NT_ASSERT(j != RMI4_MAX_TOUCHES);

		//
		// Remove the slot. If the finger lifted was the last in the list,
		// we just decrement the list total by one. If it was not last, we
		// shift the trailing list items up by one.
		//
		for (; (j < Cache->PenDownCount - 1) && (j < RMI4_MAX_TOUCHES - 1); j++)
		{
			Cache->PenDownOrder[j] = Cache->PenDownOrder[j + 1];
		}
		Cache->PenDownCount--;

		//
		// Finished, clobber the dirty bit
		//
		Cache->PenSlotDirty &= ~(1 << i);
	}

	//
	// Cache the new set of finger data reported by hardware
	//
	for (i = 0; i < RMI4_MAX_TOUCHES; i++)
	{
		//
		// Take actions when a new contact is first reported as down
		//
		if ((Data->PenStates[i] != RMI4_PEN_STATE_NOT_PRESENT) &&
			((Cache->PenSlotValid & (1 << i)) == 0) &&
			(Cache->PenDownCount < RMI4_MAX_TOUCHES))
		{
			Cache->PenSlotValid |= (1 << i);
			Cache->PenDownOrder[Cache->PenDownCount++] = i;
		}

		//
		// Ignore slots with no new information
		//
		if (!(Cache->PenSlotValid & (1 << i)))
		{
			continue;
		}

		//
		// When finger is down, update local cache with new information from
		// the controller. When finger is up, we'll use last cached value
		//
		Cache->PenSlot[i].fingerStatus = (UCHAR)Data->PenStates[i];
		if (Cache->PenSlot[i].fingerStatus)
		{
			Cache->PenSlot[i].x = Data->Positions[i].X;
			Cache->PenSlot[i].y = Data->Positions[i].Y;
		}

		//
		// If a finger lifted, note the slot is now inactive so that any
		// cached data is cleaned out before we read hardware again.
		//
		if (Cache->PenSlot[i].fingerStatus == RMI4_PEN_STATE_NOT_PRESENT)
		{
			Cache->PenSlotDirty |= (1 << i);
			Cache->PenSlotValid &= ~(1 << i);
		}
	}

	//
	// Get current scan time (in 100us units)
	//
	ULONG64 QpcTimeStamp;
	Cache->ScanTime = KeQueryInterruptTimePrecise(&QpcTimeStamp) / 1000;
}

VOID
RmiFillNextPenHidReportFromCache(
	IN PHID_PEN_REPORT HidReport,
	IN RMI4_PEN_CACHE* Cache,
	IN PTOUCH_SCREEN_PROPERTIES Props,
	IN int* PensReported,
	IN int PensTotal
)
/*++

Routine Description:

	This routine fills a HID report with the next touch entries in
	the local device finger cache.

	The routine also adjusts X/Y coordinates to match the desired display
	coordinates.

Arguments:

	HidReport - pointer to the HID report structure to fill
	Cache - pointer to the local device finger cache
	Props - information on how to adjust X/Y coordinates to match the display
	TouchesReported - On entry, the number of touches (against total) that
		have already been reported. As touches are transferred from the local
		device cache to a HID report, this number is incremented.
	TouchesTotal - total number of touches in the touch cache

Return Value:

	None.

--*/
{
	int currentFingerIndex;
	int fingersToReport = min(PensTotal - *PensReported, 1);
	USHORT SctatchX = 0, ScratchY = 0;

	//
	// There are only 16-bits for ScanTime, truncate it
	//
	//HidReport->ScanTime = Cache->ScanTime & 0xFFFF;

	//
	// Only five fingers supported yet
	//
	for (currentFingerIndex = 0; currentFingerIndex < fingersToReport; currentFingerIndex++)
	{
		int currentlyReporting = Cache->PenDownOrder[*PensReported];

		SctatchX = (USHORT)Cache->PenSlot[currentlyReporting].x;
		ScratchY = (USHORT)Cache->PenSlot[currentlyReporting].y;

		//
		// Perform per-platform x/y adjustments to controller coordinates
		//
		TchTranslateToDisplayCoordinates(
			&SctatchX,
			&ScratchY,
			Props);

		HidReport->X = SctatchX;
		HidReport->Y = ScratchY;

		if (Cache->PenSlot[currentlyReporting].fingerStatus)
		{
			HidReport->InRange = 1;
			HidReport->TipSwitch = FINGER_STATUS;

			if (Cache->PenSlot[currentlyReporting].fingerStatus == RMI4_PEN_STATE_PRESENT_WITH_ERASER)
			{
				HidReport->Eraser = 1;
			}
		}

		(*PensReported)++;
	}
}

NTSTATUS
RmiServicePenDataInterrupt(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN RMI4_DETECTED_OBJECTS data,
	IN PHID_PEN_REPORT HidReport,
	IN UCHAR InputMode,
	OUT BOOLEAN* PendingPens
)
/*++

Routine Description:

	Called when a touch interrupt needs service.

Arguments:

	ControllerContext - Touch controller context
	SpbContext - A pointer to the current SPB context (I2C, etc)
	HidReport- Buffer to fill with a hid report if touch data is available
	InputMode - Specifies mouse, single-touch, or multi-touch reporting modes
	PendingTouches - Notifies caller if there are more touches to report, to
		complete reporting the full state of fingers on the screen

Return Value:

	NTSTATUS indicating whether or not the current hid report buffer was filled

	PendingTouches also indicates whether the caller should expect more than
		one request to be completed to indicate the full state of fingers on
		the screen
--*/
{
	NTSTATUS status;

	UNREFERENCED_PARAMETER(InputMode);

	status = STATUS_SUCCESS;
	NT_ASSERT(PendingPens != NULL);
	*PendingPens = FALSE;

	/*//
	// If no touches are unreported in our cache, read the next set of touches
	// from hardware.
	//
	if (ControllerContext->PensReported == ControllerContext->PensTotal)
	{
		//
		// Process the new touch data by updating our cached state
		//
		//
		RmiUpdateLocalPenCache(
			&data,
			&ControllerContext->PenCache);

		//
		// Prepare to report touches via HID reports
		//
		ControllerContext->PensReported = 0;
		ControllerContext->PensTotal =
			ControllerContext->PenCache.PenDownCount;

		//
		// If no touches are present return that no data needed to be reported
		//
		if (ControllerContext->PensTotal == 0)
		{
			status = STATUS_NO_DATA_DETECTED;
			goto exit;
		}
	}

	RtlZeroMemory(HidReport, sizeof(HID_PEN_REPORT));

	//
	// Fill report with the next cached touches
	//
	RmiFillNextPenHidReportFromCache(
		HidReport,
		&ControllerContext->PenCache,
		&ControllerContext->Props,
		&ControllerContext->PensReported,
		ControllerContext->PensTotal);

	//
	// Update the caller if we still have outstanding touches to report
	//
	if (ControllerContext->PensReported < ControllerContext->PensTotal)
	{
		*PendingPens = TRUE;
	}
	else
	{
		*PendingPens = FALSE;
	}*/

	RtlZeroMemory(HidReport, sizeof(HID_PEN_REPORT));

	if (data.ActivePenState.Pen == 0) {
		if (ControllerContext->PenPresent)
		{
			ControllerContext->PenPresent = FALSE;
		}

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"No active pen data",
			status);

		goto exit;
	}

	USHORT ScratchX = (USHORT)data.ActivePenState.X;
	USHORT ScratchY = (USHORT)data.ActivePenState.Y;

	if ((ScratchX == 0xFFFF) && (ScratchY == 0xFFFF)) {
		if (ControllerContext->PenPresent)
		{
			ControllerContext->PenPresent = FALSE;
		}

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Active pen in range but no valid x & y",
			status);

		goto exit;
	}

	//
	// Perform per-platform x/y adjustments to controller coordinates
	//
	TchTranslateToDisplayCoordinates(
		&ScratchX,
		&ScratchY,
		&ControllerContext->Props);

	//ControllerContext->PenBattery = data.ActivePenState.Battery;
	//ControllerContext->PenId = data.ActivePenState.PenId;

	HidReport->TipSwitch = data.ActivePenState.Pressure > 0;
	HidReport->Eraser = data.ActivePenState.Invert;
	HidReport->BarrelSwitch = data.ActivePenState.Barrel;

	HidReport->X = ScratchX;
	HidReport->Y = ScratchY;
	HidReport->TipPressure = data.ActivePenState.Pressure;

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INTERRUPT,
		"Active pen: "
		"Status = %d, "
		"Invert = %d, "
		"Barrel = %d, "
		"X = %d, "
		"Y = %d, "
		"Pressure = %d",
		data.ActivePenState.Pen,
		data.ActivePenState.Invert,
		data.ActivePenState.Barrel,
		ScratchX, ScratchY, data.ActivePenState.Pressure);

	ControllerContext->PenPresent = TRUE;

exit:
	return status;
}

VOID
RmiUpdateLocalFingerCache(
	IN RMI4_DETECTED_OBJECTS *Data,
	IN RMI4_FINGER_CACHE *Cache
)
/*++

Routine Description:

	This routine takes raw data reported by the Synaptics hardware and
	parses it to update a local cache of finger states. This routine manages
	removing lifted touches from the cache, and manages a map between the
	order of reported touches in hardware, and the order the driver should
	use in reporting.

Arguments:

	Data - A pointer to the new data returned from hardware
	Cache - A data structure holding various current finger state info

Return Value:

	None.

--*/
{
	int i, j;

	//
	// When hardware was last read, if any slots reported as lifted, we
	// must clean out the slot and old touch info. There may be new
	// finger data using the slot.
	//
	for (i=0; i<RMI4_MAX_TOUCHES; i++)
	{
		//
		// Sweep for a slot that needs to be cleaned
		//
		if (!(Cache->FingerSlotDirty & (1 << i)))
		{
			continue;
		}

		NT_ASSERT(Cache->FingerDownCount > 0);

		//
		// Find the slot in the reporting list 
		//
		for (j=0; j<RMI4_MAX_TOUCHES; j++)
		{
			if (Cache->FingerDownOrder[j] == i)
			{
				break;
			}
		}

		NT_ASSERT(j != RMI4_MAX_TOUCHES);

		//
		// Remove the slot. If the finger lifted was the last in the list,
		// we just decrement the list total by one. If it was not last, we
		// shift the trailing list items up by one.
		//
		for (; (j<Cache->FingerDownCount-1) && (j<RMI4_MAX_TOUCHES-1); j++)
		{
			Cache->FingerDownOrder[j] = Cache->FingerDownOrder[j+1];
		}
		Cache->FingerDownCount--;

		//
		// Finished, clobber the dirty bit
		//
		Cache->FingerSlotDirty &= ~(1 << i);
	}

	//
	// Cache the new set of finger data reported by hardware
	//
	for (i=0; i<RMI4_MAX_TOUCHES; i++)
	{
		//
		// Take actions when a new contact is first reported as down
		//
		if ((Data->FingerStates[i] != RMI4_FINGER_STATE_NOT_PRESENT) &&
			((Cache->FingerSlotValid & (1 << i)) == 0) &&
			(Cache->FingerDownCount < RMI4_MAX_TOUCHES))
		{
			Cache->FingerSlotValid |= (1 << i);
			Cache->FingerDownOrder[Cache->FingerDownCount++] = i;
		}

		//
		// Ignore slots with no new information
		//
		if (!(Cache->FingerSlotValid & (1 << i)))
		{
			continue;
		}

		//
		// When finger is down, update local cache with new information from
		// the controller. When finger is up, we'll use last cached value
		//
		Cache->FingerSlot[i].fingerStatus = (UCHAR)Data->FingerStates[i];
		if (Cache->FingerSlot[i].fingerStatus)
		{
			Cache->FingerSlot[i].x = Data->Positions[i].X;
			Cache->FingerSlot[i].y = Data->Positions[i].Y;
		}

		//
		// If a finger lifted, note the slot is now inactive so that any
		// cached data is cleaned out before we read hardware again.
		//
		if (Cache->FingerSlot[i].fingerStatus == RMI4_FINGER_STATE_NOT_PRESENT)
		{
			Cache->FingerSlotDirty |= (1 << i);
			Cache->FingerSlotValid &= ~(1 << i);
		}
	}

	//
	// Get current scan time (in 100us units)
	//
	ULONG64 QpcTimeStamp;
	Cache->ScanTime = KeQueryInterruptTimePrecise(&QpcTimeStamp) / 1000;
}

VOID
RmiFillNextHidReportFromCache(
	IN PHID_TOUCH_REPORT HidReport,
	IN RMI4_FINGER_CACHE *Cache,
	IN PTOUCH_SCREEN_PROPERTIES Props,
	IN int *TouchesReported,
	IN int TouchesTotal
)
/*++

Routine Description:

	This routine fills a HID report with the next touch entries in
	the local device finger cache.

	The routine also adjusts X/Y coordinates to match the desired display
	coordinates.

Arguments:

	HidReport - pointer to the HID report structure to fill
	Cache - pointer to the local device finger cache
	Props - information on how to adjust X/Y coordinates to match the display
	TouchesReported - On entry, the number of touches (against total) that
		have already been reported. As touches are transferred from the local
		device cache to a HID report, this number is incremented.
	TouchesTotal - total number of touches in the touch cache

Return Value:

	None.

--*/
{
	int currentFingerIndex;
	int fingersToReport = min(TouchesTotal - *TouchesReported, 2);
	USHORT SctatchX = 0, ScratchY = 0;

	//
	// There are only 16-bits for ScanTime, truncate it
	//
	//HidReport->ScanTime = Cache->ScanTime & 0xFFFF;

	//
	// No button in our context
	// 
	//HidReport->IsButtonClicked = FALSE;

	//
	// Report the count
	// We're sending touches using hybrid mode with 5 fingers in our
	// report descriptor. The first report must indicate the
	// total count of touch fingers detected by the digitizer.
	// The remaining reports must indicate 0 for the count.
	// The first report will have the TouchesReported integer set to 0
	// The others will have it set to something else.
	//
	if (*TouchesReported == 0)
	{
		HidReport->ContactCount = (UCHAR)TouchesTotal;
	}
	else
	{
		HidReport->ContactCount = 0;
	}

	//
	// Only five fingers supported yet
	//
	for (currentFingerIndex = 0; currentFingerIndex < fingersToReport; currentFingerIndex++)
	{
		int currentlyReporting = Cache->FingerDownOrder[*TouchesReported];

		HidReport->Contacts[currentFingerIndex].ContactID = (UCHAR)currentlyReporting;
		SctatchX = (USHORT)Cache->FingerSlot[currentlyReporting].x;
		ScratchY = (USHORT)Cache->FingerSlot[currentlyReporting].y;
		HidReport->Contacts[currentFingerIndex].Confidence = 1;

		//
		// Perform per-platform x/y adjustments to controller coordinates
		//
		TchTranslateToDisplayCoordinates(
			&SctatchX,
			&ScratchY,
			Props);

		HidReport->Contacts[currentFingerIndex].X = SctatchX;
		HidReport->Contacts[currentFingerIndex].Y = ScratchY;

		if (Cache->FingerSlot[currentlyReporting].fingerStatus)
		{
			HidReport->Contacts[currentFingerIndex].TipSwitch = FINGER_STATUS;
		}

		(*TouchesReported)++;
	}
}

NTSTATUS
RmiServiceTouchDataInterrupt(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN RMI4_DETECTED_OBJECTS data,
	IN PHID_TOUCH_REPORT HidReport,
	IN UCHAR InputMode,
	OUT BOOLEAN* PendingTouches
)
/*++

Routine Description:

	Called when a touch interrupt needs service.

Arguments:

	ControllerContext - Touch controller context
	SpbContext - A pointer to the current SPB context (I2C, etc)
	HidReport- Buffer to fill with a hid report if touch data is available
	InputMode - Specifies mouse, single-touch, or multi-touch reporting modes
	PendingTouches - Notifies caller if there are more touches to report, to
		complete reporting the full state of fingers on the screen

Return Value:

	NTSTATUS indicating whether or not the current hid report buffer was filled

	PendingTouches also indicates whether the caller should expect more than
		one request to be completed to indicate the full state of fingers on
		the screen
--*/
{
	NTSTATUS status;

	UNREFERENCED_PARAMETER(InputMode);

	status = STATUS_SUCCESS;
	NT_ASSERT(PendingTouches != NULL);
	*PendingTouches = FALSE;

	//
	// If no touches are unreported in our cache, read the next set of touches
	// from hardware.
	//
	if (ControllerContext->TouchesReported == ControllerContext->TouchesTotal)
	{
		//
		// Process the new touch data by updating our cached state
		//
		//
		RmiUpdateLocalFingerCache(
			&data,
			&ControllerContext->Cache);

		//
		// Prepare to report touches via HID reports
		//
		ControllerContext->TouchesReported = 0;
		ControllerContext->TouchesTotal =
			ControllerContext->Cache.FingerDownCount;

		//
		// If no touches are present return that no data needed to be reported
		//
		if (ControllerContext->TouchesTotal == 0)
		{
			status = STATUS_NO_DATA_DETECTED;
			goto exit;
		}
	}

	RtlZeroMemory(HidReport, sizeof(HID_TOUCH_REPORT));

	//
	// Fill report with the next cached touches
	//
	RmiFillNextHidReportFromCache(
		HidReport,
		&ControllerContext->Cache,
		&ControllerContext->Props,
		&ControllerContext->TouchesReported,
		ControllerContext->TouchesTotal);

	//
	// Update the caller if we still have outstanding touches to report
	//
	if (ControllerContext->TouchesReported < ControllerContext->TouchesTotal)
	{
		*PendingTouches = TRUE;
	}
	else
	{
		*PendingTouches = FALSE;
	}

exit:

	return status;
}

NTSTATUS
RmiServiceWakeUpInterrupt(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN PHID_KEY_REPORT HidReport,
	IN UCHAR InputMode,
	OUT BOOLEAN* PendingEnable
)
/*++

Routine Description:

	Called when a touch interrupt needs service.

Arguments:

	ControllerContext - Touch controller context
	SpbContext - A pointer to the current SPB context (I2C, etc)
	HidReport- Buffer to fill with a hid report if touch data is available
	InputMode - Specifies mouse, single-touch, or multi-touch reporting modes
	PendingTouches - Notifies caller if there are more touches to report, to
		complete reporting the full state of fingers on the screen

Return Value:

	NTSTATUS indicating whether or not the current hid report buffer was filled

	PendingTouches also indicates whether the caller should expect more than
		one request to be completed to indicate the full state of fingers on
		the screen
--*/
{
	NTSTATUS status;

	UNREFERENCED_PARAMETER(InputMode);

	status = STATUS_SUCCESS;
	NT_ASSERT(PendingEnable != NULL);
	*PendingEnable = FALSE;

	RtlZeroMemory(HidReport, sizeof(HID_KEY_REPORT));

	//
	// There are only 16-bits for ScanTime, truncate it
	//
	//HidReport->ScanTime = Cache->ScanTime & 0xFFFF;

	if (ControllerContext->EscapeStrokeOnce)
	{
		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_INTERRUPT,
			"Wake Up State - 0");

		HidReport->SystemPowerDown = 0;
		ControllerContext->EscapeStrokeOnce = FALSE;
		*PendingEnable = FALSE;
	}
	else
	{
		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_INTERRUPT,
			"Wake Up State - 1");

		HidReport->SystemPowerDown = 1;
		ControllerContext->EscapeStrokeOnce = TRUE;
		*PendingEnable = TRUE;
	}

	return status;
}

NTSTATUS
RmiServiceButtonDataInterrupt(
	IN RMI4_DETECTED_BUTTONS data,
	IN PHID_KEY_REPORT HidReport
)
{
	NTSTATUS status;
	
	status = STATUS_SUCCESS;

	RtlZeroMemory(HidReport, sizeof(HID_KEY_REPORT));

	HidReport->ACBack = data.ButtonStates[0];
	HidReport->Start = data.ButtonStates[1];
	HidReport->ACSearch = data.ButtonStates[2];

	return status;
}

NTSTATUS
TchServiceInterrupts(
	IN VOID *ControllerContext,
	IN SPB_CONTEXT *SpbContext,
	IN PHID_INPUT_REPORT HidReport,
	IN UCHAR InputMode,
	IN BOOLEAN *ServicingComplete
)
/*++

Routine Description:

	This routine is called in response to an interrupt. The driver will
	service chip interrupts, and if data is available to report to HID,
	fill the Request object buffer with a HID report.

Arguments:

	ControllerContext - Touch controller context
	SpbContext - A pointer to the current i2c context
	HidReport - Pointer to a HID_INPUT_REPORT structure to report to the OS
	InputMode - Specifies mouse, single-touch, or multi-touch reporting modes
	ServicingComplete - Notifies caller if there are more reports needed to
		complete servicing interrupts coming from the hardware.

Return Value:

	NTSTATUS indicating whether or not the current HidReport has been filled

	ServicingComplete indicates whether or not a new report buffer is required
		to complete interrupt processing.
--*/
{
	NTSTATUS status = STATUS_NO_DATA_DETECTED;
	RMI4_CONTROLLER_CONTEXT* controller;
	RMI4_DETECTED_OBJECTS data;
	RMI4_DETECTED_BUTTONS data2;

	controller = (RMI4_CONTROLLER_CONTEXT*) ControllerContext;

	NT_ASSERT(ServicingComplete != NULL);

	//
	// Grab a waitlock to ensure the ISR executes serially and is 
	// protected against power state transitions
	//
	WdfWaitLockAcquire(controller->ControllerLock, NULL);

	RtlZeroMemory(&data, sizeof(data));
	RtlZeroMemory(&data2, sizeof(data2));

	//
	// Check the interrupt source if no interrupts are pending processing
	//
	if (controller->InterruptStatus == 0)
	{
		status = RmiCheckInterrupts(
			controller,
			SpbContext,
			&controller->InterruptStatus);

		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"Error servicing interrupts - 0x%08lX",
				status);

			*ServicingComplete = FALSE;
			goto exit;
		}
	}

	//
	// Driver only services 0D cap button and 2D touch messages currently
	//
	if (controller->InterruptStatus &
		~(RMI4_INTERRUPT_BIT_0D_CAP_BUTTON | RMI4_INTERRUPT_BIT_0D_CAP_BUTTON_REVERSED | RMI4_INTERRUPT_BIT_2D_TOUCH))
	{
		Trace(
			TRACE_LEVEL_WARNING,
			TRACE_INTERRUPT,
			"Ignoring following interrupt flags - 0x%08lX",
			controller->InterruptStatus &
			~(RMI4_INTERRUPT_BIT_0D_CAP_BUTTON | RMI4_INTERRUPT_BIT_0D_CAP_BUTTON_REVERSED |
				RMI4_INTERRUPT_BIT_2D_TOUCH));

		//
		// Mask away flags we don't service
		//
		controller->InterruptStatus &=
			(RMI4_INTERRUPT_BIT_0D_CAP_BUTTON | RMI4_INTERRUPT_BIT_0D_CAP_BUTTON_REVERSED |
				RMI4_INTERRUPT_BIT_2D_TOUCH);
	}

	//
	// RmiServiceXXX routine will change status to STATUS_SUCCESS if there
	// is a HID report to process.
	//
	status = STATUS_UNSUCCESSFUL;

	BOOLEAN pendingTouches = FALSE;
	BOOLEAN pendingPens = FALSE;
	BOOLEAN pendingKeys = FALSE;

	if (controller->TouchesReported == controller->TouchesTotal ||
		controller->PensReported == controller->PensTotal)
	{
		//
		// See if new touch data is available
		//
		status = RmiGetObjectsFromControllerF12(
			ControllerContext,
			SpbContext,
			&data
		);

		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_VERBOSE,
				TRACE_SAMPLES,
				"No object data to report - 0x%08lX",
				status);

			goto otherInterrupts;
		}
	}

	//
	// Service a touch data event if indicated by hardware 
	//
	if (controller->InterruptStatus & RMI4_INTERRUPT_BIT_2D_TOUCH)
	{
		if (controller->GesturesEnabled)
		{
			HidReport->ReportID = REPORTID_KEYPAD;
			status = RmiServiceWakeUpInterrupt(
				ControllerContext,
				&(HidReport->KeyReport),
				InputMode,
				&pendingKeys);

			//
			// Success indicates the report is ready to be sent, otherwise,
			// continue to service interrupts.
			//
			if (NT_SUCCESS(status))
			{
				goto exit2D;
			}
			else
			{
				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_INTERRUPT,
					"Error processing wake up event - 0x%08lX",
					status);
			}
		}

		HidReport->ReportID = REPORTID_FINGER;
		status = RmiServiceTouchDataInterrupt(
			ControllerContext,
			data,
			&(HidReport->TouchReport),
			InputMode,
			&pendingTouches);

		//
		// Success indicates the report is ready to be sent, otherwise,
		// continue to service interrupts.
		//
		if (NT_SUCCESS(status))
		{
			goto exit2D;
		}
		else
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"Error processing touch event - 0x%08lX",
				status);
		}

		HidReport->ReportID = REPORTID_STYLUS;
		status = RmiServicePenDataInterrupt(
			ControllerContext,
			data,
			&(HidReport->PenReport),
			InputMode,
			&pendingPens);

		//
		// Success indicates the report is ready to be sent, otherwise,
		// continue to service interrupts.
		//
		if (NT_SUCCESS(status))
		{
			goto exit2D;
		}
		else
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"Error processing pen event - 0x%08lX",
				status);
		}
	}

otherInterrupts:

	//
	// Service a touch data event if indicated by hardware 
	//
	if (controller->InterruptStatus & RMI4_INTERRUPT_BIT_0D_CAP_BUTTON ||
		controller->InterruptStatus & RMI4_INTERRUPT_BIT_0D_CAP_BUTTON_REVERSED)
	{
		//
		// See if new button data is available
		//
		status = RmiGetObjectsFromControllerF1A(
			ControllerContext,
			SpbContext,
			&data2,
			controller->InterruptStatus & RMI4_INTERRUPT_BIT_0D_CAP_BUTTON_REVERSED
		);

		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_VERBOSE,
				TRACE_SAMPLES,
				"No button data to report - 0x%08lX",
				status);

			goto otherInterrupts2;
		}

		HidReport->ReportID = REPORTID_KEYPAD;
		status = RmiServiceButtonDataInterrupt(
			data2,
			&(HidReport->KeyReport));

		//
		// Success indicates the report is ready to be sent, otherwise,
		// continue to service interrupts.
		//
		if (NT_SUCCESS(status))
		{
			controller->InterruptStatus &= ~RMI4_INTERRUPT_BIT_0D_CAP_BUTTON;
			controller->InterruptStatus &= ~RMI4_INTERRUPT_BIT_0D_CAP_BUTTON_REVERSED;
		}
		else
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"Error processing button event - 0x%08lX",
				status);
		}
	}

otherInterrupts2:

	//
	// Add servicing for additional touch interrupts here
	//

	goto exit;

exit2D:

	//
	// If there are more touches to report, servicing is incomplete
	//
	if (pendingTouches == FALSE && pendingPens == FALSE && pendingKeys == FALSE)
	{
		controller->InterruptStatus &= ~RMI4_INTERRUPT_BIT_2D_TOUCH;
	}

	goto exit;

exit:

	//
	// Indicate whether or not we're done servicing interrupts
	//
	if (controller->InterruptStatus == 0)
	{
		*ServicingComplete = TRUE;
	}
	else
	{
		*ServicingComplete = FALSE;
	}

	WdfWaitLockRelease(controller->ControllerLock);

	return status;
}
