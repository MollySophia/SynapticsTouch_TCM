// Copyright (c) Microsoft Corporation. All Rights Reserved. 
// Copyright (c) Bingxing Wang. All Rights Reserved. 
// Copyright (c) LumiaWOA authors. All Rights Reserved. 

#include <Cross Platform Shim\compat.h>
#include <controller.h>
#include <HidCommon.h>
#include <report.h>
#include <spb.h>
#include <rmi4\rmiinternal.h>
#include <rmi4\rmiregisters.h>
#include <rmi4\f12\registers.h>
#include <rmi4\f12\controlregisters.h>
#include <rmi4\f12\function12.h>
#include <rmi4\f12\data6\activepen.h>
#include <rmi4\f12\data1\finger.h>
#include <function12.tmh>

NTSTATUS
RmiSetReportingFlagsF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN UCHAR NewMode,
	OUT UCHAR* OldMode
)
/*++

	Routine Description:

		Changes the F12 Reporting Mode on the controller as specified

	Arguments:

		ControllerContext - Touch controller context

		SpbContext - A pointer to the current i2c context

		NewMode - Either RMI_F12_REPORTING_MODE_CONTINUOUS
				  or RMI_F12_REPORTING_MODE_REDUCED

		OldMode - Old value of reporting mode

	Return Value:

		NTSTATUS indicating success or failure

--*/
{
	RMI4_F12_FINGER_REPORT_REGISTER reportingControl = { 0 };
	NTSTATUS status;

	//
	// Read Device Control register
	//
	status = RmiReadControlRegister(
		ControllerContext,
		SpbContext,
		F12_2D_CTRL20,
		&reportingControl,
		sizeof(reportingControl)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not read F12_2D_CTRL20 register - 0x%08lX",
			status);

		goto exit;
	}

	if (OldMode)
	{
		*OldMode = reportingControl.ReportingFlags;
	}

	//
	// Assign new value
	//
	reportingControl.SuppressXCoordinate = 0;
	reportingControl.SuppressYCoordinate = 0;
	reportingControl.ReportingFlags = NewMode;

	//
	// Write setting back to the controller
	//
	status = RmiWriteControlRegister(
		ControllerContext,
		SpbContext,
		F12_2D_CTRL20,
		&reportingControl,
		sizeof(RMI4_F12_FINGER_REPORT_REGISTER)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write F12_2D_CTRL20 register - %X",
			status);

		goto exit;
	}

	if ((NewMode & RMI4_F12_REPORTING_WAKEUP_GESTURE_MODE) == RMI4_F12_REPORTING_WAKEUP_GESTURE_MODE)
	{
		ControllerContext->GesturesEnabled = TRUE;
	}
	else
	{
		ControllerContext->GesturesEnabled = FALSE;
	}

exit:
	return status;
}

NTSTATUS
RmiGetReportingConfigurationF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	OUT PRMI4_F12_OBJECT_REPORT_ENABLE_REGISTER ControlRegisterData
)
{
	NTSTATUS status;

	NT_ASSERT(ControlRegisterData != NULL);

	//
	// Read Device Control register
	//
	status = RmiReadControlRegister(
		ControllerContext,
		SpbContext,
		F12_2D_CTRL23,
		ControlRegisterData,
		sizeof(RMI4_F12_OBJECT_REPORT_ENABLE_REGISTER)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not read F12_2D_CTRL23 register - 0x%08lX",
			status);

		goto exit;
	}

exit:

	return status;
}

NTSTATUS
RmiSetReportingConfigurationF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PRMI4_F12_OBJECT_REPORT_ENABLE_REGISTER ControlRegisterData
)
{
	NTSTATUS status;

	//
	// Write setting back to the controller
	//
	status = RmiWriteControlRegister(
		ControllerContext,
		SpbContext,
		F12_2D_CTRL23,
		ControlRegisterData,
		sizeof(RMI4_F12_OBJECT_REPORT_ENABLE_REGISTER)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write F12_2D_CTRL23 register - %X",
			status);

		goto exit;
	}

exit:

	return status;
}

NTSTATUS
RmiConfigureControlRegisterF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	USHORT RegisterIndex
)
{
	NTSTATUS status = STATUS_SUCCESS;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"Configuring $12 Control Register F12_2D_CTRL%d",
		RegisterIndex);

	switch (RegisterIndex)
	{
	case F12_2D_CTRL20:
		status = RmiSetReportingFlagsF12(
			ControllerContext,
			SpbContext,
			RMI4_F12_REPORTING_CONTINUOUS_MODE,
			NULL
		);
		break;
	case F12_2D_CTRL23:
		status = RmiConfigureReportingF12(
			ControllerContext,
			SpbContext
		);
		break;
	default:
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Skipped configuring $12 Control Register F12_2D_CTRL%d as the driver does not support it.",
			RegisterIndex);
	}
	
	return status;
}

NTSTATUS
RmiConfigureReportingF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	NTSTATUS status;
	RMI4_F12_OBJECT_REPORT_ENABLE_REGISTER ControlRegisterData = { 0 };
	RtlZeroMemory(&ControlRegisterData, sizeof(RMI4_F12_OBJECT_REPORT_ENABLE_REGISTER));

	/*
		Default config on Lumia 950s (Retail) is:
			- Reported Object Count: 10
			- Finger reporting: ON
			- Palm reporting: ON
			- Report Gloved Finger as Finger: ON
			- Everything else: OFF

		Turn on every object type and turn off reporting as finger,
		let that be handled by us instead.
	*/

	ControlRegisterData.ReportedObjectCount = 10;

	ControlRegisterData.FingerReportingEnabled = 1;
	ControlRegisterData.ActiveStylusReportingEnabled = 1;
	ControlRegisterData.CoverReportingEnabled = 1;
	ControlRegisterData.EraserReportingEnabled = 1;
	ControlRegisterData.GlovedFingerReportingEnabled = 1;
	ControlRegisterData.HoveringFingerReportingEnabled = 1;
	ControlRegisterData.PalmReportingEnabled = 1;
	ControlRegisterData.SmallObjectReportingEnabled = 1;
	ControlRegisterData.StylusReportingEnabled = 1;
	ControlRegisterData.UnclassifiedObjectReportingEnabled = 1;
	ControlRegisterData.NarrowObjectReportingEnabled = 1;
	ControlRegisterData.HandEdgeReportingEnabled = 1;

	ControlRegisterData.ReportActiveStylusAsFinger = 0;
	ControlRegisterData.ReportCoverAsFinger = 0;
	ControlRegisterData.ReportEraserAsFinger = 0;
	ControlRegisterData.ReportGlovedFingerAsFinger = 0;
	ControlRegisterData.ReportHoveringFingerAsFinger = 0;
	ControlRegisterData.ReportPalmAsFinger = 0;
	ControlRegisterData.ReportSmallObjectAsFinger = 0;
	ControlRegisterData.ReportStylusAsFinger = 0;
	ControlRegisterData.ReportUnclassifiedObjectAsFinger = 0;
	ControlRegisterData.ReportNarrowObjectSwipeAsFinger = 0;
	ControlRegisterData.ReportHandEdgeAsFinger = 0;

	status = RmiSetReportingConfigurationF12(
		ControllerContext,
		SpbContext,
		&ControlRegisterData
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"RmiConfigureReportingF12 - 0x%08lX",
			status);

		goto exit;
	}

exit:
	return status;
}

NTSTATUS
RmiDiscoverControlRegistersF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	NTSTATUS status = STATUS_SUCCESS;
	int index;

	/*
		Lumia 950s (Retail) should have
		the following control registers:

		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL8 at 0x15 with a size of 14 // ok
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL9 at 0x16 with a size of 21 // incomplete by one
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL10 at 0x17 with a size of 7 // ok
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL11 at 0x18 with a size of 21
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL12 at 0x19 with a size of 4
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL15 at 0x1A with a size of 7 // ok
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL18 at 0x1B with a size of 30 // one too much
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL20 at 0x1C with a size of 3 // ok
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL22 at 0x1D with a size of 1
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL23 at 0x1E with a size of 5 // ok
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL24 at 0x1F with a size of 6
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL25 at 0x20 with a size of 9
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL26 at 0x21 with a size of 1
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL27 at 0x22 with a size of 5
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL28 at 0x23 with a size of 1 // Report enable/RPT
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL29 at 0x24 with a size of 16
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL30 at 0x25 with a size of 16
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL33 at 0x26 with a size of 28
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL34 at 0x27 with a size of 16
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL35 at 0x28 with a size of 19
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL36 at 0x29 with a size of 16
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL37 at 0x2A with a size of 16
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL38 at 0x2B with a size of 16
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL39 at 0x2C with a size of 16
		SynapticsTouch: Discovered $12 Control Register F12_2D_CTRL40 at 0x2D with a size of 16
	*/

	//
	// Find RMI F12 function
	//
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F12_2D_TOUCHPAD_SENSOR);

	if (index == ControllerContext->FunctionCount)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Set ReportingMode failure - RMI Function 12 missing");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	PRMI_REGISTER_DESC_ITEM item;
	for (int i = 0; i < ControllerContext->F12ControlRegDesc.NumRegisters; i++)
	{
		item = &ControllerContext->F12ControlRegDesc.Registers[i];

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Discovered $12 Control Register F12_2D_CTRL%d at 0x%X with a size of %d",
			item->Register,
			ControllerContext->Descriptors[index].ControlBase + i,
			item->RegisterSize
		);

		status = RmiConfigureControlRegisterF12(
			ControllerContext,
			SpbContext,
			item->Register
		);

		if (!NT_SUCCESS(status)) {

			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Failed to configure $12 Control Register F12_2D_CTRL%d - 0x%08lX",
				item->Register,
				status);
			//goto exit;
			status = STATUS_SUCCESS;
		}
	}

exit:
	return status;
}

NTSTATUS
RmiDiscoverDataRegistersF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext
)
{
	NTSTATUS status = STATUS_SUCCESS;
	int index;

	/*
		Lumia 950s (Retail) should have
		the following data registers:

		1 - ok
		2
		4  (GESTURE_REPORT_DATA)
		13
		15 (FINGER_REPORT_DATA)
	*/

	//
	// Find RMI F12 function
	//
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F12_2D_TOUCHPAD_SENSOR);

	if (index == ControllerContext->FunctionCount)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Set ReportingMode failure - RMI Function 12 missing");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	PRMI_REGISTER_DESC_ITEM item;
	for (int i = 0; i < ControllerContext->F12DataRegDesc.NumRegisters; i++)
	{
		item = &ControllerContext->F12DataRegDesc.Registers[i];

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Discovered $12 Data Register F12_2D_DATA%d at 0x%X with a size of %d",
			item->Register,
			ControllerContext->Descriptors[index].DataBase + i,
			item->RegisterSize
		);
	}

exit:
	return status;
}

NTSTATUS
RmiDiscoverQueryRegistersF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext
)
{
	NTSTATUS status = STATUS_SUCCESS;
	int index;

	//
	// Find RMI F12 function
	//
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F12_2D_TOUCHPAD_SENSOR);

	if (index == ControllerContext->FunctionCount)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Set ReportingMode failure - RMI Function 12 missing");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	PRMI_REGISTER_DESC_ITEM item;
	for (int i = 0; i < ControllerContext->F12QueryRegDesc.NumRegisters; i++)
	{
		item = &ControllerContext->F12QueryRegDesc.Registers[i];

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Discovered $12 Query Register F12_2D_QUERY%d at 0x%X with a size of %d",
			item->Register,
			ControllerContext->Descriptors[index].QueryBase + i,
			item->RegisterSize
		);
	}

exit:
	return status;
}

NTSTATUS
RmiConfigureF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	NTSTATUS status = STATUS_SUCCESS;
	int index;
	RMI4_F12_QUERY_0_REGISTER GeneralInformation = { 0 };

	//
	// Find 2D touch sensor function and configure it
	//
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F12_2D_TOUCHPAD_SENSOR);

	if (index == ControllerContext->FunctionCount)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected - RMI Function 12 missing");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	status = RmiChangePage(
		ControllerContext,
		SpbContext,
		ControllerContext->FunctionOnPage[index]);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not change register page");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Reading general info register");

	// Zeroth query register is general information about function $12
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].QueryBase,
		&GeneralInformation,
		sizeof(RMI4_F12_QUERY_0_REGISTER)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read general info register - 0x%08lX",
			status);
		goto exit;
	}

	ControllerContext->HasDribble = GeneralInformation.HasDribble;
	ControllerContext->HasRegisterDescriptors = GeneralInformation.HasRegisterDescriptors;

	if (!GeneralInformation.HasRegisterDescriptors)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Behavior of F12 without register descriptors is undefined. Is this a counterfeit IC? Assuming 10 fingers supported and only finger reporting available (Data 1)"
		);

		ControllerContext->MaxFingers = 10;

		goto exit;
	}

	// First query register is the size of the presence map for Query Registers
	// Second query register is the register presence map for Query Registers
	// Third query register is the size map for Query Registers
	status = RmiReadRegisterDescriptor(
		SpbContext,
		ControllerContext->Descriptors[index].QueryBase + 1,
		&ControllerContext->F12QueryRegDesc
	);

	if (!NT_SUCCESS(status)) {

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read the Query Register Descriptor - 0x%08lX",
			status);
		goto exit;
	}

	// Fourth query register is the size of the presence map for Control Registers
	// Fifth query register is the register presence map for Control Registers
	// Sixth query register is the size map for Control Registers
	status = RmiReadRegisterDescriptor(
		SpbContext,
		ControllerContext->Descriptors[index].QueryBase + 4,
		&ControllerContext->F12ControlRegDesc
	);

	if (!NT_SUCCESS(status)) {

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read the Control Register Descriptor - 0x%08lX",
			status);
		goto exit;
	}

	// Seventh query register is the size of the presence map for Control Registers
	// Eigth query register is the register presence map for Control Registers
	// Nineth query register is the size map for Control Registers
	status = RmiReadRegisterDescriptor(
		SpbContext,
		ControllerContext->Descriptors[index].QueryBase + 7,
		&ControllerContext->F12DataRegDesc
	);

	if (!NT_SUCCESS(status)) {

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read the Data Register Descriptor - 0x%08lX",
			status);
		goto exit;
	}

	// Retrieve the total size we need to query to get all of the data with every interrupt
	ControllerContext->PacketSize = RmiRegisterDescriptorCalcSize(
		&ControllerContext->F12DataRegDesc
	);

	// Get the accurate finger count from the controller
	USHORT FingerCount = RmiGetObjectCountFromControllerF12(ControllerContext);

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Finger Count: %d",
		FingerCount);

	ControllerContext->MaxFingers = (UCHAR)FingerCount;

	// Discover query registers
	status = RmiDiscoverQueryRegistersF12(
		ControllerContext
	);

	if (!NT_SUCCESS(status)) {

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to discover $12 Query Registers - 0x%08lX",
			status);
		goto exit;
	}

	// Discover control registers
	status = RmiDiscoverControlRegistersF12(
		ControllerContext,
		SpbContext
	);

	if (!NT_SUCCESS(status)) {

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to discover $12 Control Registers - 0x%08lX",
			status);
		goto exit;
	}

	// Discover data registers
	status = RmiDiscoverDataRegistersF12(
		ControllerContext
	);

	if (!NT_SUCCESS(status)) {

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to discover $12 Data Registers - 0x%08lX",
			status);
		goto exit;
	}

exit:
	return status;
}

NTSTATUS
RmiServiceWakeUpInterrupt(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN PHID_KEY_REPORT HidReport,
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
		HidReport->SystemPowerDown = 0;
		ControllerContext->EscapeStrokeOnce = FALSE;
		*PendingEnable = FALSE;
		ControllerContext->GesturesEnabled = FALSE;
	}
	else
	{
		HidReport->SystemPowerDown = 1;
		ControllerContext->EscapeStrokeOnce = TRUE;
		*PendingEnable = TRUE;
	}

	return status;
}

NTSTATUS
RmiServiceActivePenDataInterrupt(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN RMI4_DETECTED_PEN data,
	IN PREPORT_CONTEXT ReportContext
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
	static int invert = -1;
	status = STATUS_SUCCESS;

	if (data.Pen == 0) {
		if (ControllerContext->ActivePenPresent)
		{
			ControllerContext->ActivePenPresent = FALSE;
		}
		else
		{
			status = STATUS_NO_DATA_DETECTED;
		}

		goto exit;
	}

	USHORT X = (USHORT)data.X;
	USHORT Y = (USHORT)data.Y;

	if ((X == 0xFFFF) && (Y == 0xFFFF)) {
		if (ControllerContext->ActivePenPresent)
		{
			ControllerContext->ActivePenPresent = FALSE;

			status = ReportPen(
				ReportContext,
				FALSE,
				FALSE,
				FALSE,
				FALSE,
				FALSE,
				0,
				0,
				0,
				0,
				0
			);
		}
		else
		{
			status = STATUS_NO_DATA_DETECTED;
		}

		goto exit;
	}

	if (!ControllerContext->ActivePenPresent)
	{
		invert = -1;
	}

	if (invert != -1 && invert != data.Invert)
	{
		ControllerContext->ActivePenPresent = FALSE;

		status = ReportPen(
			ReportContext,
			FALSE,
			FALSE,
			FALSE,
			FALSE,
			FALSE,
			0,
			0,
			0,
			0,
			0
		);

		goto exit;
	}

	invert = data.Invert;

	//ControllerContext->PenBattery = data.Battery;
	//ControllerContext->PenId = data.PenId;

	status = ReportPen(
		ReportContext,
		data.Invert == FALSE && data.Pressure > 0,
		data.Barrel,
		data.Invert,
		data.Invert == TRUE && data.Pressure > 0,
		1,
		X,
		Y,
		data.Pressure,
		0,
		0
	);

	ControllerContext->ActivePenPresent = TRUE;

exit:
	return status;
}

NTSTATUS
TchServiceActivePenInterrupts(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PREPORT_CONTEXT ReportContext
)
{
	NTSTATUS status = STATUS_NO_DATA_DETECTED;
	RMI4_DETECTED_PEN data;

	RtlZeroMemory(&data, sizeof(data));

	//
	// See if new touch data is available
	//
	status = RmiGetPenStatusFromControllerF12(
		ControllerContext,
		SpbContext,
		&data
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_VERBOSE,
			TRACE_SAMPLES,
			"No active pen data to report - 0x%08lX",
			status);

		goto exit;
	}

	status = RmiServiceActivePenDataInterrupt(
		ControllerContext,
		data,
		ReportContext);

	//
	// Success indicates the report is ready to be sent, otherwise,
	// continue to service interrupts.
	//
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Error processing active pen event - 0x%08lX",
			status);
	}

exit:
	return status;
}

NTSTATUS
TchServiceWakeUpInterrupts(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PREPORT_CONTEXT ReportContext
)
{
	NTSTATUS status;

	HID_INPUT_REPORT HidReport;
	BOOLEAN PendingEnable = TRUE;

	UNREFERENCED_PARAMETER(SpbContext);

	status = STATUS_NO_DATA_DETECTED;

	if (!ControllerContext->GesturesEnabled)
	{
		goto exit;
	}

	while (PendingEnable == TRUE)
	{
		RtlZeroMemory(&HidReport, sizeof(HidReport));

		HidReport.ReportID = REPORTID_KEYPAD;

		status = RmiServiceWakeUpInterrupt(
			ControllerContext,
			&(HidReport.KeyReport),
			&PendingEnable);

		//
		// Success indicates the report is ready to be sent, otherwise,
		// continue to service interrupts.
		//
		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"Error processing wake up event - 0x%08lX",
				status);

			goto exit;
		}

		status = TchSendReport(ReportContext->PingPongQueue, &HidReport);

		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"Error send hid report for wake up event - 0x%08lX",
				status);

			goto exit;
		}
	}

exit:
	return status;
}

NTSTATUS
TchServiceObjectInterrupts(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PREPORT_CONTEXT ReportContext
)
{
	NTSTATUS status = STATUS_SUCCESS;
	DETECTED_OBJECTS data;

	RtlZeroMemory(&data, sizeof(data));

	//
	// See if new touch data is available
	//
	status = RmiGetObjectStatusFromControllerF12(
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

		goto exit;
	}

	status = ReportObjects(
		ReportContext,
		data);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_VERBOSE,
			TRACE_SAMPLES,
			"Error while reporting objects - 0x%08lX",
			status);

		goto exit;
	}

exit:
	return status;
}

NTSTATUS
RmiServiceInterruptF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PREPORT_CONTEXT ReportContext
)
{
	NTSTATUS status = STATUS_SUCCESS;

	TchServiceWakeUpInterrupts(ControllerContext, SpbContext, ReportContext);
	TchServiceActivePenInterrupts(ControllerContext, SpbContext, ReportContext);
	TchServiceObjectInterrupts(ControllerContext, SpbContext, ReportContext);

	return status;
}