// Copyright (c) Microsoft Corporation. All Rights Reserved. 
// Copyright (c) Bingxing Wang. All Rights Reserved. 
// Copyright (c) LumiaWOA authors. All Rights Reserved. 

#include <Cross Platform Shim\compat.h>
#include <controller.h>
#include <HidCommon.h>
#include <spb.h>
#include <rmi4\rmiinternal.h>
#include <rmi4\rmiregisters.h>
#include <rmi4\f12\registers.h>
#include <rmi4\f12\controlregisters.h>
#include <rmi4\f12\function12.h>
#include <rmi4\f12\activepen.h>
#include <rmi4\f12\finger.h>
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
			UINT8 indexCtrl = RmiGetRegisterIndex(&ControllerContext->F12ControlRegDesc, i);

			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Discovered $12 Control Register F12_2D_CTRL%d at 0x%X with a size of %d",
				i,
				ControllerContext->Descriptors[index].ControlBase + indexCtrl,
				ControllerContext->F12ControlRegDesc.Registers[indexCtrl].RegisterSize
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
			UINT8 indexData = RmiGetRegisterIndex(&ControllerContext->F12DataRegDesc, i);

			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Discovered $12 Data Register F12_2D_DATA%d at 0x%X with a size of %d",
				i,
				ControllerContext->Descriptors[index].DataBase + indexData,
				ControllerContext->F12DataRegDesc.Registers[indexData].RegisterSize
			);
		}
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

	queryF12Addr += 3;

	status = RmiReadRegisterDescriptor(
		SpbContext,
		queryF12Addr,
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
	queryF12Addr += 3;

	status = RmiReadRegisterDescriptor(
		SpbContext,
		queryF12Addr,
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

	queryF12Addr += 3;

	ControllerContext->PacketSize = RmiRegisterDescriptorCalcSize(
		&ControllerContext->F12DataRegDesc
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

	indexQuery4 = RmiGetRegisterIndex(&ControllerContext->F12QueryRegDesc, 4);

	if (indexQuery4 == ControllerContext->F12QueryRegDesc.NumRegisters)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_QUERY4 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->F12QueryRegDesc.Registers[indexQuery4].RegisterSize != sizeof(ControlRegisterData->Size))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_QUERY4 register size: %d", ControllerContext->F12QueryRegDesc.Registers[indexQuery4].RegisterSize);

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
			"Could not read F12_2D_QUERY4 register - 0x%08lX",
			status);

		goto exit;
	}

	if (ControlRegisterData->Size > sizeof(ControlRegisterData->Data))
		ControlRegisterData->Size = sizeof(ControlRegisterData->Data);

	indexQuery5 = RmiGetRegisterIndex(&ControllerContext->F12QueryRegDesc, 5);

	if (indexQuery5 == ControllerContext->F12QueryRegDesc.NumRegisters)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_QUERY5 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->F12QueryRegDesc.Registers[indexQuery5].RegisterSize < ControlRegisterData->Size)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_QUERY5 register size: %d", ControllerContext->F12QueryRegDesc.Registers[indexQuery5].RegisterSize);

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}
	else if (ControllerContext->F12QueryRegDesc.Registers[indexQuery5].RegisterSize > ControlRegisterData->Size)
	{
		Trace(
			TRACE_LEVEL_WARNING,
			TRACE_INIT,
			"Higher F12_2D_QUERY5 register size: %d. Must expand driver!", ControllerContext->F12QueryRegDesc.Registers[indexQuery5].RegisterSize);
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
			"Could not read F12_2D_QUERY5 register - 0x%08lX",
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

	indexQuery7 = RmiGetRegisterIndex(&ControllerContext->F12QueryRegDesc, 7);

	if (indexQuery7 == ControllerContext->F12QueryRegDesc.NumRegisters)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_QUERY7 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->F12QueryRegDesc.Registers[indexQuery7].RegisterSize != sizeof(ControlRegisterData->Size))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_QUERY7 register size: %d", ControllerContext->F12QueryRegDesc.Registers[indexQuery7].RegisterSize);

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
			"Could not read F12_2D_QUERY7 register - 0x%08lX",
			status);

		goto exit;
	}

	if (ControlRegisterData->Size > sizeof(ControlRegisterData->Data))
		ControlRegisterData->Size = sizeof(ControlRegisterData->Data);

	indexQuery8 = RmiGetRegisterIndex(&ControllerContext->F12QueryRegDesc, 8);

	if (indexQuery8 == ControllerContext->F12QueryRegDesc.NumRegisters)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_QUERY8 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->F12QueryRegDesc.Registers[indexQuery8].RegisterSize < ControlRegisterData->Size)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_QUERY8 register size: %d", ControllerContext->F12QueryRegDesc.Registers[indexQuery8].RegisterSize);

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}
	else if (ControllerContext->F12QueryRegDesc.Registers[indexQuery8].RegisterSize > ControlRegisterData->Size)
	{
		Trace(
			TRACE_LEVEL_WARNING,
			TRACE_INIT,
			"Higher F12_2D_QUERY8 register size: %d. Must expand driver!", ControllerContext->F12QueryRegDesc.Registers[indexQuery8].RegisterSize);
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
			"Could not read F12_2D_QUERY8 register - 0x%08lX",
			status);

		goto exit;
	}

exit:
	return status;
}

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

	status = STATUS_SUCCESS;
	NT_ASSERT(PendingPens != NULL);
	*PendingPens = FALSE;

	//
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
	}

exit:
	return status;
}

VOID
RmiUpdateLocalFingerCache(
	IN RMI4_DETECTED_OBJECTS* Data,
	IN RMI4_FINGER_CACHE* Cache
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
		if (!(Cache->FingerSlotDirty & (1 << i)))
		{
			continue;
		}

		NT_ASSERT(Cache->FingerDownCount > 0);

		//
		// Find the slot in the reporting list 
		//
		for (j = 0; j < RMI4_MAX_TOUCHES; j++)
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
		for (; (j < Cache->FingerDownCount - 1) && (j < RMI4_MAX_TOUCHES - 1); j++)
		{
			Cache->FingerDownOrder[j] = Cache->FingerDownOrder[j + 1];
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
	for (i = 0; i < RMI4_MAX_TOUCHES; i++)
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
	IN RMI4_FINGER_CACHE* Cache,
	IN PTOUCH_SCREEN_PROPERTIES Props,
	IN int* TouchesReported,
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
	IN PHID_PEN_REPORT HidReport
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

	RtlZeroMemory(HidReport, sizeof(HID_PEN_REPORT));

	if (data.Pen == 0) {
		if (ControllerContext->PenPresent)
		{
			ControllerContext->PenPresent = FALSE;
		}
		else
		{
			status = STATUS_NO_DATA_DETECTED;
		}

		goto exit;
	}

	USHORT ScratchX = (USHORT)data.X;
	USHORT ScratchY = (USHORT)data.Y;

	if ((ScratchX == 0xFFFF) && (ScratchY == 0xFFFF)) {
		if (ControllerContext->PenPresent)
		{
			ControllerContext->PenPresent = FALSE;
		}
		else
		{
			status = STATUS_NO_DATA_DETECTED;
		}

		goto exit;
	}

	if (!ControllerContext->PenPresent)
	{
		invert = -1;
	}

	if (invert != -1 && invert != data.Invert)
	{
		ControllerContext->PenPresent = FALSE;
		goto exit;
	}

	invert = data.Invert;

	//
	// Perform per-platform x/y adjustments to controller coordinates
	//
	TchTranslateToDisplayCoordinates(
		&ScratchX,
		&ScratchY,
		&ControllerContext->Props);

	//ControllerContext->PenBattery = data.Battery;
	//ControllerContext->PenId = data.PenId;

	HidReport->InRange = 1;
	HidReport->TipSwitch = data.Invert == FALSE && data.Pressure > 0;
	HidReport->Eraser = data.Invert == TRUE && data.Pressure > 0;
	HidReport->Invert = data.Invert;
	HidReport->BarrelSwitch = data.Barrel;

	HidReport->X = ScratchX;
	HidReport->Y = ScratchY;
	HidReport->TipPressure = data.Pressure;

	ControllerContext->PenPresent = TRUE;

exit:
	return status;
}

NTSTATUS
TchServicePenInterrupts(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN WDFQUEUE PingPongQueue
)
{
	NTSTATUS status = STATUS_NO_DATA_DETECTED;

	HID_INPUT_REPORT HidReport;
	BOOLEAN PendingPens = TRUE;

	RMI4_DETECTED_OBJECTS data;

	RtlZeroMemory(&data, sizeof(data));

	//
	// If no touches are unreported in our cache, read the next set of touches
	// from hardware.
	//
	if (ControllerContext->PensReported == ControllerContext->PensTotal)
	{
		//
		// See if new touch data is available
		//
		status = RmiGetFingerStatusFromControllerF12(
			ControllerContext,
			SpbContext,
			&data
		);

		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_VERBOSE,
				TRACE_SAMPLES,
				"No pen data to report - 0x%08lX",
				status);

			goto exit;
		}
	}

	while (PendingPens == TRUE)
	{
		RtlZeroMemory(&HidReport, sizeof(HidReport));

		HidReport.ReportID = REPORTID_STYLUS;

		status = RmiServicePenDataInterrupt(
			ControllerContext,
			data,
			&(HidReport.PenReport),
			&PendingPens);

		//
		// Success indicates the report is ready to be sent, otherwise,
		// continue to service interrupts.
		//
		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"Error processing pen event - 0x%08lX",
				status);

			goto exit;
		}

		status = TchSendReport(PingPongQueue, &HidReport);

		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"Error send hid report for pen event - 0x%08lX",
				status);

			goto exit;
		}
	}

exit:
	return status;
}

NTSTATUS
TchServiceActivePenInterrupts(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN WDFQUEUE PingPongQueue
)
{
	NTSTATUS status = STATUS_NO_DATA_DETECTED;
	RMI4_DETECTED_PEN data;

	HID_INPUT_REPORT HidReport;

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

	RtlZeroMemory(&HidReport, sizeof(HidReport));

	HidReport.ReportID = REPORTID_STYLUS;

	status = RmiServiceActivePenDataInterrupt(
		ControllerContext,
		data,
		&(HidReport.PenReport));

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

	status = TchSendReport(PingPongQueue, &HidReport);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Error send hid report for active pen event - 0x%08lX",
			status);

		goto exit;
	}

exit:
	return status;
}

NTSTATUS
TchServiceWakeUpInterrupts(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN WDFQUEUE PingPongQueue
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

		status = TchSendReport(PingPongQueue, &HidReport);

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
TchServiceTouchInterrupts(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN WDFQUEUE PingPongQueue
)
{
	NTSTATUS status;
	RMI4_DETECTED_OBJECTS data;

	HID_INPUT_REPORT HidReport;
	BOOLEAN PendingTouches = TRUE;

	status = STATUS_SUCCESS;

	RtlZeroMemory(&data, sizeof(data));

	//
	// If no touches are unreported in our cache, read the next set of touches
	// from hardware.
	//
	if (ControllerContext->TouchesReported == ControllerContext->TouchesTotal)
	{
		//
		// See if new touch data is available
		//
		status = RmiGetFingerStatusFromControllerF12(
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
	}

	while (PendingTouches == TRUE)
	{
		RtlZeroMemory(&HidReport, sizeof(HidReport));

		HidReport.ReportID = REPORTID_FINGER;

		status = RmiServiceTouchDataInterrupt(
			ControllerContext,
			data,
			&(HidReport.TouchReport),
			&PendingTouches);

		//
		// Success indicates the report is ready to be sent, otherwise,
		// continue to service interrupts.
		//
		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"Error processing touch event - 0x%08lX",
				status);
		}

		status = TchSendReport(PingPongQueue, &HidReport);

		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"Error send hid report for touch event - 0x%08lX",
				status);

			goto exit;
		}
	}

exit:
	return status;
}

NTSTATUS
RmiServiceInterruptF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN WDFQUEUE PingPongQueue
)
{
	NTSTATUS status = STATUS_SUCCESS;

	TchServiceWakeUpInterrupts(ControllerContext, SpbContext, PingPongQueue);
	TchServiceActivePenInterrupts(ControllerContext, SpbContext, PingPongQueue);
	TchServiceTouchInterrupts(ControllerContext, SpbContext, PingPongQueue);
	TchServicePenInterrupts(ControllerContext, SpbContext, PingPongQueue);

	return status;
}