// Copyright (c) Microsoft Corporation. All Rights Reserved. 
// Copyright (c) Bingxing Wang. All Rights Reserved. 
// Copyright (c) LumiaWOA authors. All Rights Reserved. 

#include <Cross Platform Shim\compat.h>
#include <spb.h>
#include <rmi4\rmiinternal.h>
#include <rmi4\f01\function01.h>
#include <function01.tmh>


#pragma warning(push)
#pragma warning(disable:4242) // Conversion, possible loss of data

//
// The logical values come from the registry and are hence DWORDs but the 
// physical registers are only 8 bits wide so we use the lower 8 bits of the
// logical value.
//
#define LOGICAL_TO_PHYSICAL(LOGICAL_VALUE) ((LOGICAL_VALUE) & 0xff)

NTSTATUS
RmiChangeChargerConnectedState(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN UCHAR ChargerConnectedState
)
{
	RMI4_F01_CTRL_REGISTERS* controlF01;
	UCHAR deviceControl = { 0 };
	int index;
	NTSTATUS status;

	controlF01 = (RMI4_F01_CTRL_REGISTERS*)&deviceControl;

	//
	// Find RMI device control function housing charger connected settings
	// 
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F01_RMI_DEVICE_CONTROL);

	if (index == ControllerContext->FunctionCount)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_POWER,
			"Power change failure - RMI Function 01 missing");

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
			TRACE_POWER,
			"Could not change register page");

		goto exit;
	}

	//
	// Read Device Control register
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase,
		&deviceControl,
		sizeof(deviceControl)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_POWER,
			"Could not read charger connected register - 0x%08lX",
			status);

		goto exit;
	}

	//
	// Assign new sleep state
	//
	controlF01->DeviceControl.ChargerConnected = ChargerConnectedState;

	//
	// Write setting back to the controller
	//
	status = SpbWriteDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase,
		&deviceControl,
		sizeof(deviceControl)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_POWER,
			"Could not write charger connected register - %X",
			status);

		goto exit;
	}

exit:

	return status;
}

NTSTATUS
RmiChangeSleepState(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN UCHAR SleepState
)
/*++

Routine Description:

   Changes the SleepMode bits on the controller as specified

Arguments:

   ControllerContext - Touch controller context

   SpbContext - A pointer to the current i2c context

   SleepState - Either RMI4_F11_DEVICE_CONTROL_SLEEP_MODE_OPERATING
				or RMI4_F11_DEVICE_CONTROL_SLEEP_MODE_SLEEPING

Return Value:

   NTSTATUS indicating success or failure

--*/
{
	RMI4_F01_CTRL_REGISTERS* controlF01;
	UCHAR deviceControl = { 0 };
	int index;
	NTSTATUS status;

	controlF01 = (RMI4_F01_CTRL_REGISTERS*)&deviceControl;

	//
	// Find RMI device control function housing sleep settings
	// 
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F01_RMI_DEVICE_CONTROL);

	if (index == ControllerContext->FunctionCount)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_POWER,
			"Power change failure - RMI Function 01 missing");

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
			TRACE_POWER,
			"Could not change register page");

		goto exit;
	}

	//
	// Read Device Control register
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase,
		&deviceControl,
		sizeof(deviceControl)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_POWER,
			"Could not read sleep register - 0x%08lX",
			status);

		goto exit;
	}

	//
	// Assign new sleep state
	//
	controlF01->DeviceControl.SleepMode = SleepState;

	//
	// Write setting back to the controller
	//
	status = SpbWriteDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase,
		&deviceControl,
		sizeof(deviceControl)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_POWER,
			"Could not write sleep register - %X",
			status);

		goto exit;
	}

exit:

	return status;
}

NTSTATUS
RmiSetInterruptEnable(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN UCHAR InterruptEnable
)
{
	RMI4_F01_CTRL_REGISTERS* controlF01;
	UCHAR deviceControl = { 0 };
	int index;
	NTSTATUS status;

	controlF01 = (RMI4_F01_CTRL_REGISTERS*)&deviceControl;

	//
	// Find RMI device control function housing sleep settings
	// 
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F01_RMI_DEVICE_CONTROL);

	if (index == ControllerContext->FunctionCount)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Power change failure - RMI Function 01 missing");

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
			TRACE_INTERRUPT,
			"Could not change register page");

		goto exit;
	}

	//
	// Read Device Control register
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase,
		&deviceControl,
		sizeof(deviceControl)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Could not read interrupt register - 0x%08lX",
			status);

		goto exit;
	}

	//
	// Assign new sleep state
	//
	controlF01->InterruptEnable = InterruptEnable;

	//
	// Write setting back to the controller
	//
	status = SpbWriteDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase,
		&deviceControl,
		sizeof(deviceControl)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Could not write interrupt register - %X",
			status);

		goto exit;
	}

exit:

	return status;
}

UCHAR
RmiGetFunctionInterruptMask(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN int FunctionDesired
)
{
	int index;
	UCHAR interruptMask = 0;

	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		FunctionDesired);

	if (index != ControllerContext->FunctionCount)
	{
		interruptMask = ControllerContext->FunctionInterruptMasks[index];
	}

	return interruptMask;
}

NTSTATUS
RmiConfigureInterruptEnable(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	NTSTATUS status;
	UCHAR InterruptEnable = 0;

	int function;

	for (function = 0; function < ControllerContext->FunctionCount; function++)
	{
		InterruptEnable |= ControllerContext->FunctionInterruptMasks[function];
	}

	status = RmiSetInterruptEnable(
		ControllerContext,
		SpbContext,
		InterruptEnable
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Could not write interrupt enable register - %X",
			status);

		goto exit;
	}

exit:
	return status;
}

NTSTATUS
RmiCheckInterrupts(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN ULONG* InterruptStatus
)
/*++

  Routine Description:

	This function handles controller interrupts. It currently only
	supports valid touch interrupts. Any other interrupt sources (such as
	device losing configuration or being reset) are unhandled, but noted
	in the controller context.

  Arguments:

	ControllerContext - A pointer to the current touch controller
	context

	SpbContext - A pointer to the current i2c context

  Return Value:

	NTSTATUS indicating success or failure

--*/
{
	RMI4_F01_DATA_REGISTERS data;
	int index;
	NTSTATUS status;

	RtlZeroMemory(&data, sizeof(data));
	*InterruptStatus = 0;

	//
	// Locate RMI data base address
	//
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F01_RMI_DEVICE_CONTROL);

	if (index == ControllerContext->FunctionCount)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected - RMI Function 01 missing");

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

	//
	// Read interrupt status registers
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].DataBase,
		&data,
		sizeof(data));

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Error reading interrupt status - 0x%08lX",
			status);

		goto exit;
	}

	//
	// Check for catastrophic failures, simply store in context for
	// debugging should these errors occur.
	//
	switch (data.DeviceStatus.Status)
	{
	case RMI4_F01_DATA_STATUS_NO_ERROR:
	{
		break;
	}
	case RMI4_F01_DATA_STATUS_RESET_OCCURRED:
	{
		ControllerContext->ResetOccurred = TRUE;
		break;
	}
	case RMI4_F01_DATA_STATUS_INVALID_CONFIG:
	{
		ControllerContext->InvalidConfiguration = TRUE;

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Received status code 2 - invalid configuration");

		break;
	}
	case RMI4_F01_DATA_STATUS_DEVICE_FAILURE:
	{
		ControllerContext->DeviceFailure = TRUE;

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Received status code 4 - device failure");

		break;
	}
	default:
	{
		ControllerContext->UnknownStatus = TRUE;
		ControllerContext->UnknownStatusMessage = data.DeviceStatus.Status;

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Received unknown status code - %d",
			ControllerContext->UnknownStatusMessage);

		break;
	}
	}

	//
	// If we're in flash programming mode, report an error
	//
	if (data.DeviceStatus.FlashProg)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Error, device status indicates chip in programming mode");

		goto exit;
	}

	//
	// If the chip has lost it's configuration, reconfigure
	//
	if (data.DeviceStatus.Unconfigured)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Error, device status indicates chip is unconfigured");

		//
		// Initialize RMI function control registers
		//
		status = RmiConfigureFunctions(
			ControllerContext,
			SpbContext);

		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"Could not reconfigure chip - 0x%08lX",
				status);

			goto exit;
		}
	}

	if (data.InterruptStatus[0])
	{
		*InterruptStatus = data.InterruptStatus[0] & 0xFF;
	}
	else
	{
		Trace(
			TRACE_LEVEL_VERBOSE,
			TRACE_INTERRUPT,
			"Unexpected -- no interrupt status bit set");
	}

exit:
	return status;
}

NTSTATUS
RmiConfigureF01(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	NTSTATUS status;
	int index;

	RMI4_F01_CTRL_REGISTERS controlF01 = { 0 };

	//
	// Find RMI device control function and configure it
	// 
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F01_RMI_DEVICE_CONTROL);

	if (index == ControllerContext->FunctionCount)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected - RMI Function 01 missing");

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

	RmiConvertF01ToPhysical(
		&ControllerContext->Config.DeviceSettings,
		&controlF01);

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Writing RMI F01 Ctrl settings");

	//
	// Write settings to controller
	//
	status = SpbWriteDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase,
		&controlF01,
		sizeof(controlF01)
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error writing RMI F01 Ctrl settings - 0x%08lX",
			status);
		goto exit;
	}

	//
	// Note whether the device configuration settings initialized the
	// controller in an operating state, to prevent a double-start from 
	// the D0 entry dispatch routine (TchWakeDevice)
	//
	if (RMI4_F01_DEVICE_CONTROL_SLEEP_MODE_OPERATING ==
		controlF01.DeviceControl.SleepMode)
	{
		ControllerContext->DevicePowerState = PowerDeviceD0;
	}
	else
	{
		ControllerContext->DevicePowerState = PowerDeviceD3;
	}

exit:
	return status;
}

NTSTATUS
RmiGetFirmwareVersion(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
/*++

  Routine Description:

	This function queries the firmware version of the current chip for
	debugging purposes.

  Arguments:

	ControllerContext - A pointer to the current touch controller context
	SpbContext - A pointer to the current i2c context

  Return Value:

	NTSTATUS indicating success or failure

--*/
{
	int index;
	NTSTATUS status;

	//
	// Find RMI device control function and configure it
	// 
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F01_RMI_DEVICE_CONTROL);

	if (index == ControllerContext->FunctionCount)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected - RMI Function 01 missing");

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
		"Reading RMI F01 Query registers");

	//
	// Store all F01 query registers, which contain the product ID
	//
	// TODO: Fix transfer size when SPB can support larger I2C 
	//       transactions
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].QueryBase,
		&ControllerContext->F01QueryRegisters,
		sizeof(BYTE) * FIELD_OFFSET(RMI4_F01_QUERY_REGISTERS, ProductID10));

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error reading RMI F01 Query registers - 0x%08lX",
			status);

		goto exit;
	}

exit:

	return status;
}

VOID
RmiConvertF01ToPhysical(
	IN RMI4_F01_CTRL_REGISTERS_LOGICAL* Logical,
	IN RMI4_F01_CTRL_REGISTERS* Physical
)
/*++

  Routine Description:

	Registry configuration values for F01 must be specified as
	4-byte REG_DWORD values logically, however the chip interprets these
	values as bits or bytes physically. This function converts
	the registry parameters into a structure that can be programmed
	into the controller's memory.

  Arguments:

	Logical - a pointer to the logical settings

	Physical - a pointer to the controller memory-mapped structure

  Return Value:

	None. Function may print warnings in the future when truncating.

--*/
{
	RtlZeroMemory(Physical, sizeof(RMI4_F01_CTRL_REGISTERS));

	//
	// Note that truncation of registry values is possible if 
	// the data was incorrectly provided by the OEM, we may
	// print warning messages in the future.
	// 

	Physical->DeviceControl.SleepMode = LOGICAL_TO_PHYSICAL(Logical->SleepMode);
	Physical->DeviceControl.NoSleep = LOGICAL_TO_PHYSICAL(Logical->NoSleep);
	Physical->DeviceControl.ReportRate = LOGICAL_TO_PHYSICAL(Logical->ReportRate);
	Physical->DeviceControl.Configured = LOGICAL_TO_PHYSICAL(Logical->Configured);

	Physical->InterruptEnable = LOGICAL_TO_PHYSICAL(Logical->InterruptEnable);
	Physical->DozeInterval = LOGICAL_TO_PHYSICAL(Logical->DozeInterval);
	Physical->DozeThreshold = LOGICAL_TO_PHYSICAL(Logical->DozeThreshold);
	Physical->DozeHoldoff = LOGICAL_TO_PHYSICAL(Logical->DozeHoldoff);
}

#pragma warning(pop)

NTSTATUS
RmiServiceInterruptF01(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PREPORT_CONTEXT ReportContext
)
{
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(ReportContext);

	status = RmiCheckInterrupts(
		ControllerContext,
		SpbContext,
		&ControllerContext->InterruptStatus);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Error servicing interrupts - 0x%08lX",
			status);

		goto exit;
	}

exit:
	return status;
}