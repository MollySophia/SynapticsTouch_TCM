// Copyright (c) Microsoft Corporation. All Rights Reserved. 
// Copyright (c) Bingxing Wang. All Rights Reserved. 
// Copyright (c) LumiaWOA authors. All Rights Reserved. 

#include <Cross Platform Shim\compat.h>
#include <controller.h>
#include <HidCommon.h>
#include <spb.h>
#include <rmi4\rmiinternal.h>
#include <rmi4\f12\controlregisters.h>
#include <rmi4\f12\function12.h>
#include <rmi4\f12\activepen.h>
#include <rmi4\f12\finger.h>
#include <function12.tmh>

NTSTATUS
RmiConfigureControlRegister(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN USHORT ControlRegister,
	IN PVOID Buffer,
	IN ULONG BufferLength
)
{
	int index;
	NTSTATUS status;
	UINT8 indexCtrl;

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

	indexCtrl = RmiGetRegisterIndex(&ControllerContext->ControlRegDesc, ControlRegister);

	if (indexCtrl == ControllerContext->ControlRegDesc.NumRegisters)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_CTRL%d offset",
			ControlRegister);

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->ControlRegDesc.Registers[indexCtrl].RegisterSize < BufferLength)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_CTRL%d register size: %d",
			ControlRegister,
			ControllerContext->ControlRegDesc.Registers[indexCtrl].RegisterSize);

		//status = STATUS_INVALID_DEVICE_STATE;
		//goto exit;
	}

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Writing F12_2D_CTRL%d",
		ControlRegister);

	//
	// Write setting to the controller
	//
	status = SpbWriteDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase + indexCtrl,
		Buffer,
		BufferLength
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write F12_2D_CTRL%d register - %X",
			ControlRegister,
			status);

		goto exit;
	}

exit:

	return status;
}

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
	int index;
	NTSTATUS status;
	UINT8 indexCtrl20;

	RtlZeroMemory(&reportingControl, sizeof(RMI4_F12_FINGER_REPORT_REGISTER));

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

	indexCtrl20 = RmiGetRegisterIndex(&ControllerContext->ControlRegDesc, F12_2D_CTRL20);

	if (indexCtrl20 == ControllerContext->ControlRegDesc.NumRegisters)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_CTRL20 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->ControlRegDesc.Registers[indexCtrl20].RegisterSize != sizeof(RMI4_F12_FINGER_REPORT_REGISTER))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_CTRL20 register size");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	//
	// Read Device Control register
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase + indexCtrl20,
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

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Writing F12_2D_CTRL20");

	//
	// Write setting back to the controller
	//
	status = SpbWriteDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase + indexCtrl20,
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
	int index;
	NTSTATUS status;
	UINT8 indexCtrl23;

	NT_ASSERT(ControlRegisterData != NULL);

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

	indexCtrl23 = RmiGetRegisterIndex(&ControllerContext->ControlRegDesc, F12_2D_CTRL23);

	if (indexCtrl23 == ControllerContext->ControlRegDesc.NumRegisters)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_CTRL23 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	//
	// Read Device Control register
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase + indexCtrl23,
		ControlRegisterData,
		min(ControllerContext->ControlRegDesc.Registers[indexCtrl23].RegisterSize, sizeof(RMI4_F12_OBJECT_REPORT_ENABLE_REGISTER))
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
	int index;
	NTSTATUS status;
	UINT8 indexCtrl23;

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

	indexCtrl23 = RmiGetRegisterIndex(&ControllerContext->ControlRegDesc, F12_2D_CTRL23);

	if (indexCtrl23 == ControllerContext->ControlRegDesc.NumRegisters)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_CTRL23 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Writing F12_2D_CTRL23");

	//
	// Write setting back to the controller
	//
	status = SpbWriteDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase + indexCtrl23,
		ControlRegisterData,
		min(ControllerContext->ControlRegDesc.Registers[indexCtrl23].RegisterSize, sizeof(RMI4_F12_OBJECT_REPORT_ENABLE_REGISTER))
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

	status = RmiGetReportingConfigurationF12(
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

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Current Controller Reporting Configuration:"
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Reported Object Count: %d",
		ControlRegisterData.ReportedObjectCount
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"--== Reported Objects ==--"
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Finger: %d",
		ControlRegisterData.FingerReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		" "
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Active Stylus: %d",
		ControlRegisterData.ActiveStylusReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Cover: %d",
		ControlRegisterData.CoverReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Eraser: %d",
		ControlRegisterData.EraserReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Gloved Finger: %d",
		ControlRegisterData.GlovedFingerReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hovering Finger: %d",
		ControlRegisterData.HoveringFingerReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Palm: %d",
		ControlRegisterData.PalmReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Small Object: %d",
		ControlRegisterData.SmallObjectReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Stylus: %d",
		ControlRegisterData.StylusReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Unclassified Object: %d",
		ControlRegisterData.UnclassifiedObjectReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hand Edge: %d",
		ControlRegisterData.HandEdgeReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Narrow Object Swipe: %d",
		ControlRegisterData.NarrowObjectReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		" "
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"--== Reported As Finger ==--"
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Active Stylus: %d",
		ControlRegisterData.ReportActiveStylusAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Cover: %d",
		ControlRegisterData.ReportCoverAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Eraser: %d",
		ControlRegisterData.ReportEraserAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Gloved Finger: %d",
		ControlRegisterData.ReportGlovedFingerAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hovering Finger: %d",
		ControlRegisterData.ReportHoveringFingerAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Palm: %d",
		ControlRegisterData.ReportPalmAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Small Object: %d",
		ControlRegisterData.ReportSmallObjectAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Stylus: %d",
		ControlRegisterData.ReportStylusAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Unclassified Object: %d",
		ControlRegisterData.ReportUnclassifiedObjectAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hand Edge: %d",
		ControlRegisterData.ReportHandEdgeAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Narrow Object Swipe: %d",
		ControlRegisterData.ReportNarrowObjectSwipeAsFinger
	);

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

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"New Controller Reporting Configuration:"
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Reported Object Count: %d",
		ControlRegisterData.ReportedObjectCount
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"--== Reported Objects ==--"
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Finger: %d",
		ControlRegisterData.FingerReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		" "
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Active Stylus: %d",
		ControlRegisterData.ActiveStylusReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Cover: %d",
		ControlRegisterData.CoverReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Eraser: %d",
		ControlRegisterData.EraserReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Gloved Finger: %d",
		ControlRegisterData.GlovedFingerReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hovering Finger: %d",
		ControlRegisterData.HoveringFingerReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Palm: %d",
		ControlRegisterData.PalmReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Small Object: %d",
		ControlRegisterData.SmallObjectReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Stylus: %d",
		ControlRegisterData.StylusReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Unclassified Object: %d",
		ControlRegisterData.UnclassifiedObjectReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hand Edge: %d",
		ControlRegisterData.HandEdgeReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Narrow Object Swipe: %d",
		ControlRegisterData.NarrowObjectReportingEnabled
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		" "
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"--== Reported As Finger ==--"
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Active Stylus: %d",
		ControlRegisterData.ReportActiveStylusAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Cover: %d",
		ControlRegisterData.ReportCoverAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Eraser: %d",
		ControlRegisterData.ReportEraserAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Gloved Finger: %d",
		ControlRegisterData.ReportGlovedFingerAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hovering Finger: %d",
		ControlRegisterData.ReportHoveringFingerAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Palm: %d",
		ControlRegisterData.ReportPalmAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Small Object: %d",
		ControlRegisterData.ReportSmallObjectAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Stylus: %d",
		ControlRegisterData.ReportStylusAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Unclassified Object: %d",
		ControlRegisterData.ReportUnclassifiedObjectAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hand Edge: %d",
		ControlRegisterData.ReportHandEdgeAsFinger
	);
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Narrow Object Swipe: %d",
		ControlRegisterData.ReportNarrowObjectSwipeAsFinger
	);

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
RmiConfigureControlRegistersF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	NTSTATUS status;
	int index;
	RMI4_F12_QUERY_5_REGISTER Query5Reg = { 0 };
	RtlZeroMemory(&Query5Reg, sizeof(RMI4_F12_QUERY_5_REGISTER));

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

	status = RmiGetSupportedControlRegistersF12(
		ControllerContext,
		SpbContext,
		&Query5Reg);

	if (!NT_SUCCESS(status)) {

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read the Query 5 Register - 0x%08lX",
			status);
		goto exit;
	}

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

	for (USHORT i = 0; i < (sizeof(Query5Reg.Data) - 1) * 8; i++)
	{
		DWORD RegisterMask = (1 << (i % 8));
		DWORD DataIndex = (i - (i % 8)) / 8 + 1;

		if ((Query5Reg.Data[DataIndex] & RegisterMask) == RegisterMask)
		{
			UINT8 indexCtrl = RmiGetRegisterIndex(&ControllerContext->ControlRegDesc, i);

			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Discovered $12 Control Register F12_2D_CTRL%d at 0x%X with a size of %d",
				i,
				ControllerContext->Descriptors[index].ControlBase + indexCtrl,
				ControllerContext->ControlRegDesc.Registers[indexCtrl].RegisterSize
			);

			status = RmiConfigureControlRegisterF12(
				ControllerContext,
				SpbContext,
				i
			);

			if (!NT_SUCCESS(status)) {

				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_INIT,
					"Failed to configure $12 Control Register F12_2D_CTRL%d - 0x%08lX",
					i,
					status);
				//goto exit;
				status = STATUS_SUCCESS;
			}
		}
	}

exit:
	return status;
}

NTSTATUS
RmiQueryDataRegistersF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	NTSTATUS status;
	int index;
	RMI4_F12_QUERY_8_REGISTER Query8Reg = { 0 };
	RtlZeroMemory(&Query8Reg, sizeof(RMI4_F12_QUERY_8_REGISTER));

	/*
		Lumia 950s (Retail) should have
		the following data registers:

		1 - ok
		2
		4  (GESTURE_REPORT_DATA)
		13
		15 (FINGER_REPORT_DATA)
	*/

	status = RmiGetSupportedDataRegistersF12(
		ControllerContext,
		SpbContext,
		&Query8Reg);

	if (!NT_SUCCESS(status)) {

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read the Query 8 Register - 0x%08lX",
			status);
		goto exit;
	}

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

	for (USHORT i = 0; i < (sizeof(Query8Reg.Data) - 1) * 8; i++)
	{
		DWORD RegisterMask = (1 << (i % 8));
		DWORD DataIndex = (i - (i % 8)) / 8 + 1;

		if ((Query8Reg.Data[DataIndex] & RegisterMask) == RegisterMask)
		{
			UINT8 indexData = RmiGetRegisterIndex(&ControllerContext->DataRegDesc, i);

			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Discovered $12 Data Register F12_2D_DATA%d at 0x%X with a size of %d",
				i,
				ControllerContext->Descriptors[index].DataBase + indexData,
				ControllerContext->DataRegDesc.Registers[indexData].RegisterSize
			);
		}
	}

exit:
	return status;
}

NTSTATUS
RmiWriteRegistersTestF12(
	IN SPB_CONTEXT* SpbContext
)
{
	NTSTATUS status = STATUS_SUCCESS;
	int i = 0;

	//BYTE Buffer0[] = { 0x3F };
	BYTE Buffer1[] = { 0x6F, 0x00, 0x6F, 0x00, 0x31, 0x05, 0x91, 0x09, 0x23, 0x1E };
	//BYTE Buffer2[] = { 0x80 }; // Configured
	BYTE Buffer3[] = { 0x01 };
	BYTE Buffer4[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x07 };
	BYTE Buffer5[] = { 0x05 };
	BYTE Buffer6[] = { 0x00 };
	BYTE Buffer7[] = { 0x10, 0x00 };
	BYTE Buffer8[] = { 0x02 };
	//BYTE Buffer9[] = { 0x04 }; // Charger Connected
	BYTE BufferA[] = { 0x09, 0x23, 0x1C };
	//BYTE BufferB[] = { 0x08 };

	// 0xE: ControlState
	// 0xF: InterruptEnable

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0F,
		Buffer0,
		sizeof(Buffer0)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x1A,
		Buffer1,
		sizeof(Buffer1)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0E,
		Buffer2,
		sizeof(Buffer2)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0F,
		Buffer3,
		sizeof(Buffer3)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x01,
		Buffer4,
		sizeof(Buffer4)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x01,
		Buffer5,
		sizeof(Buffer5)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x02,
		Buffer6,
		sizeof(Buffer6)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x03,
		Buffer7,
		sizeof(Buffer7)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x04,
		Buffer8,
		sizeof(Buffer8)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x2D,
		Buffer3,
		sizeof(Buffer3)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0F,
		Buffer0,
		sizeof(Buffer0)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x21,
		Buffer3,
		sizeof(Buffer3)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x1A,
		Buffer1,
		sizeof(Buffer1)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0E,
		Buffer2,
		sizeof(Buffer2)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0E,
		Buffer9,
		sizeof(Buffer9)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x1B,
		BufferA,
		sizeof(BufferA)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0F,
		BufferB,
		sizeof(BufferB)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0F,
		Buffer0,
		sizeof(Buffer0)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x2E,
		Buffer3,
		sizeof(Buffer3)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0F,
		Buffer0,
		sizeof(Buffer0)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x21,
		Buffer3,
		sizeof(Buffer3)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x1A,
		Buffer1,
		sizeof(Buffer1)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0E,
		Buffer2,
		sizeof(Buffer2)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0E,
		Buffer9,
		sizeof(Buffer9)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x1B,
		BufferA,
		sizeof(BufferA)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0F,
		BufferB,
		sizeof(BufferB)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0F,
		Buffer0,
		sizeof(Buffer0)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x2E,
		Buffer3,
		sizeof(Buffer3)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0F,
		Buffer0,
		sizeof(Buffer0)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x21,
		Buffer3,
		sizeof(Buffer3)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x1A,
		Buffer1,
		sizeof(Buffer1)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0E,
		Buffer2,
		sizeof(Buffer2)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	/*status = SpbWriteDataSynchronously(
		SpbContext,
		0x0E,
		Buffer9,
		sizeof(Buffer9)
	);*/

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"SpbWriteDataSynchronously i=%d", i++);

	status = SpbWriteDataSynchronously(
		SpbContext,
		0x1B,
		BufferA,
		sizeof(BufferA)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write register");

		goto exit;
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
	NTSTATUS status;
	int index;

	BYTE queryF12Addr = 0;
	char buf = 0;

	//
	// Testing Code
	//
	/*RmiWriteRegistersTestF12(
		SpbContext
	);*/

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

	// Retrieve base address for queries
	queryF12Addr = ControllerContext->Descriptors[index].QueryBase;
	status = SpbReadDataSynchronously(
		SpbContext,
		queryF12Addr,
		&buf,
		sizeof(char)
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

	++queryF12Addr;

	/*if (!(buf & BIT(0)))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Behavior of F12 without register descriptors is undefined."
		);

		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}*/

	ControllerContext->HasDribble = !!(buf & BIT(3));

	status = RmiReadRegisterDescriptor(
		SpbContext,
		queryF12Addr,
		&ControllerContext->QueryRegDesc
	);

	if (!NT_SUCCESS(status)) {

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read the Query Register Descriptor - 0x%08lX",
			status);
		goto exit;
	}

	queryF12Addr += 3;

	status = RmiReadRegisterDescriptor(
		SpbContext,
		queryF12Addr,
		&ControllerContext->ControlRegDesc
	);

	if (!NT_SUCCESS(status)) {

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read the Control Register Descriptor - 0x%08lX",
			status);
		goto exit;
	}
	queryF12Addr += 3;

	status = RmiReadRegisterDescriptor(
		SpbContext,
		queryF12Addr,
		&ControllerContext->DataRegDesc
	);

	if (!NT_SUCCESS(status)) {

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read the Data Register Descriptor - 0x%08lX",
			status);
		goto exit;
	}
	queryF12Addr += 3;
	ControllerContext->PacketSize = RmiRegisterDescriptorCalcSize(
		&ControllerContext->DataRegDesc
	);

	status = RmiConfigureControlRegistersF12(
		ControllerContext,
		SpbContext
	);

	if (!NT_SUCCESS(status)) {

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to configure $12 Control Registers - 0x%08lX",
			status);
		//goto exit;
		status = STATUS_SUCCESS;
	}

	status = RmiQueryDataRegistersF12(
		ControllerContext,
		SpbContext
	);

	if (!NT_SUCCESS(status)) {

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to query $12 Data Registers - 0x%08lX",
			status);
		//goto exit;
		status = STATUS_SUCCESS;
	}


	USHORT FingerCount = RmiGetFingerCountFromControllerF12(ControllerContext);

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Finger Count: %d",
		FingerCount);

	ControllerContext->MaxFingers = (UCHAR)FingerCount;

exit:
	return status;

}

NTSTATUS
RmiGetObjectsFromControllerF12(
	IN VOID* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN RMI4_DETECTED_OBJECTS* Data
)
/*++

Routine Description:

	This routine reads raw touch messages from hardware. If there is
	no touch data available (if a non-touch interrupt fired), the
	function will not return success and no touch data was transferred.

Arguments:

	ControllerContext - Touch controller context
	SpbContext - A pointer to the current i2c context
	Data - A pointer to any returned F11 touch data

Return Value:

	NTSTATUS, where only success indicates data was returned

--*/
{
	NTSTATUS status;

	status = RmiGetFingerStatusFromControllerF12(
		ControllerContext,
		SpbContext,
		Data
		);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not get finger statuses from controller $12");

		goto exit;
	}

	status = RmiGetPenStatusFromControllerF12(
		ControllerContext,
		SpbContext,
		Data
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not get pen statuses from controller $12");

		goto exit;
	}

exit:
	return status;
}

NTSTATUS
RmiGetSupportedControlRegistersF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	OUT PRMI4_F12_QUERY_5_REGISTER ControlRegisterData
)
{
	int index;
	NTSTATUS status;
	UINT8 indexQuery4;
	UINT8 indexQuery5;

	NT_ASSERT(ControlRegisterData != NULL);

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

	indexQuery4 = RmiGetRegisterIndex(&ControllerContext->QueryRegDesc, 4);

	if (indexQuery4 == ControllerContext->QueryRegDesc.NumRegisters)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_Query4 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->QueryRegDesc.Registers[indexQuery4].RegisterSize != sizeof(ControlRegisterData->Size))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_Query4 register size: %d", ControllerContext->QueryRegDesc.Registers[indexQuery4].RegisterSize);

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	//
	// Read Device Control register
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].QueryBase + indexQuery4,
		&(ControlRegisterData->Size),
		sizeof(ControlRegisterData->Size)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not read F12_2D_Query4 register - 0x%08lX",
			status);

		goto exit;
	}

	if (ControlRegisterData->Size > sizeof(ControlRegisterData->Data))
		ControlRegisterData->Size = sizeof(ControlRegisterData->Data);

	indexQuery5 = RmiGetRegisterIndex(&ControllerContext->QueryRegDesc, 5);

	if (indexQuery5 == ControllerContext->QueryRegDesc.NumRegisters)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_Query5 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->QueryRegDesc.Registers[indexQuery5].RegisterSize < ControlRegisterData->Size)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_Query5 register size: %d", ControllerContext->QueryRegDesc.Registers[indexQuery5].RegisterSize);

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}
	else if (ControllerContext->QueryRegDesc.Registers[indexQuery5].RegisterSize > ControlRegisterData->Size)
	{
		Trace(
			TRACE_LEVEL_WARNING,
			TRACE_INIT,
			"Higher F12_2D_Query5 register size: %d. Must expand driver!", ControllerContext->QueryRegDesc.Registers[indexQuery5].RegisterSize);
	}

	//
	// Read Device Control register
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].QueryBase + indexQuery5,
		&(ControlRegisterData->Data),
		ControlRegisterData->Size
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not read F12_2D_Query5 register - 0x%08lX",
			status);

		goto exit;
	}

exit:
	return status;
}

NTSTATUS
RmiGetSupportedDataRegistersF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	OUT PRMI4_F12_QUERY_8_REGISTER ControlRegisterData
)
{
	int index;
	NTSTATUS status;
	UINT8 indexQuery7;
	UINT8 indexQuery8;

	NT_ASSERT(ControlRegisterData != NULL);

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

	indexQuery7 = RmiGetRegisterIndex(&ControllerContext->QueryRegDesc, 7);

	if (indexQuery7 == ControllerContext->QueryRegDesc.NumRegisters)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_Query7 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->QueryRegDesc.Registers[indexQuery7].RegisterSize != sizeof(ControlRegisterData->Size))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_Query7 register size: %d", ControllerContext->QueryRegDesc.Registers[indexQuery7].RegisterSize);

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	//
	// Read Device Control register
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].QueryBase + indexQuery7,
		&(ControlRegisterData->Size),
		sizeof(ControlRegisterData->Size)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not read F12_2D_Query7 register - 0x%08lX",
			status);

		goto exit;
	}

	if (ControlRegisterData->Size > sizeof(ControlRegisterData->Data))
		ControlRegisterData->Size = sizeof(ControlRegisterData->Data);

	indexQuery8 = RmiGetRegisterIndex(&ControllerContext->QueryRegDesc, 8);

	if (indexQuery8 == ControllerContext->QueryRegDesc.NumRegisters)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_Query8 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->QueryRegDesc.Registers[indexQuery8].RegisterSize < ControlRegisterData->Size)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_Query8 register size: %d", ControllerContext->QueryRegDesc.Registers[indexQuery8].RegisterSize);

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}
	else if (ControllerContext->QueryRegDesc.Registers[indexQuery8].RegisterSize > ControlRegisterData->Size)
	{
		Trace(
			TRACE_LEVEL_WARNING,
			TRACE_INIT,
			"Higher F12_2D_Query8 register size: %d. Must expand driver!", ControllerContext->QueryRegDesc.Registers[indexQuery8].RegisterSize);
	}

	//
	// Read Device Control register
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].QueryBase + indexQuery8,
		&(ControlRegisterData->Data),
		ControlRegisterData->Size
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not read F12_2D_Query8 register - 0x%08lX",
			status);

		goto exit;
	}

exit:
	return status;
}