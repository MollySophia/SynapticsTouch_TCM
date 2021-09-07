// Copyright (c) Microsoft Corporation. All Rights Reserved. 
// Copyright (c) LumiaWOA authors. All Rights Reserved. 

#include <Cross Platform Shim\compat.h>
#include <controller.h>
#include <HidCommon.h>
#include <spb.h>
#include <rmi4\rmiinternal.h>
#include <rmi4\f1a\function1a.h>
#include <function1a.tmh>

NTSTATUS
RmiConfigureF1A(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	UNREFERENCED_PARAMETER(SpbContext);

	int index;

	//
	// Find 0D capacitive button sensor function and configure it if it exists
	//
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F1A_0D_CAP_BUTTON_SENSOR);

	if (index != ControllerContext->FunctionCount)
	{
		ControllerContext->HasButtons = TRUE;

		//
		// TODO: Get configuration data from registry once Synaptics
		//       provides sane default values. Until then, assume the
		//       object is configured for the desired product scenario
		//       by default.
		//
	}

	return STATUS_SUCCESS;
}

NTSTATUS
RmiGetObjectsFromControllerF1A(
	IN VOID* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN RMI4_DETECTED_BUTTONS* Data,
	IN BOOLEAN ReversedKeys
)
/*++

Routine Description:

	This routine services capacitive button (F$1A) interrupts, it reads
	button data and fills a HID keyboard report with the relevant information

Arguments:

	ControllerContext - Touch controller context
	SpbContext - A pointer to the current i2c context
	HidReport- A HID report buffer to be filled with button data

Return Value:

	NTSTATUS, where success indicates the request memory was updated with
	button press information.
--*/
{
	NTSTATUS status;
	RMI4_CONTROLLER_CONTEXT* controller;
	RMI4_F1A_DATA_REGISTERS dataF1A;

	int index;

	controller = (RMI4_CONTROLLER_CONTEXT*)ControllerContext;

	//
	// If the controller doesn't support buttons, ignore this interrupt
	//
	if (controller->HasButtons == FALSE)
	{
		status = STATUS_NOT_IMPLEMENTED;
		goto exit;
	}

	//
	// Get the the key press/release information from the controller
	//
	index = RmiGetFunctionIndex(
		controller->Descriptors,
		controller->FunctionCount,
		RMI4_F1A_0D_CAP_BUTTON_SENSOR);

	if (index == controller->FunctionCount)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected - RMI Function 1A missing");
		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	status = RmiChangePage(
		ControllerContext,
		SpbContext,
		controller->FunctionOnPage[index]);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not change register page");
		goto exit;
	}

	//
	// Read button press/release data
	// 
	status = SpbReadDataSynchronously(
		SpbContext,
		controller->Descriptors[index].DataBase,
		&dataF1A,
		sizeof(dataF1A));

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Error reading finger status data - STATUS:%X",
			status);

		goto exit;
	}

	for (int i = 0; i < MAX_BUTTONS; i++)
	{
		if (ReversedKeys)
		{
			Data->ButtonStates[i] = ((dataF1A.Raw >> i) & 0x1);
		}
		else
		{
			Data->ButtonStates[i] = ((dataF1A.Raw >> (MAX_BUTTONS - i - 1)) & 0x1);
		}
	}

exit:
	return status;
}

NTSTATUS
TchServiceButtonsInterrupts(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PREPORT_CONTEXT ReportContext
)
{
	NTSTATUS status = STATUS_NO_DATA_DETECTED;
	RMI4_DETECTED_BUTTONS data;

	RtlZeroMemory(&data, sizeof(RMI4_DETECTED_BUTTONS));

	//
	// See if new button data is available
	//
	status = RmiGetObjectsFromControllerF1A(
		ControllerContext,
		SpbContext,
		&data,
		ControllerContext->InterruptStatus & RMI4_INTERRUPT_BIT_0D_CAP_BUTTON_REVERSED
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_VERBOSE,
			TRACE_SAMPLES,
			"No button data to report - 0x%08lX",
			status);

		goto exit;
	}

	status = ReportKeypad(
		ReportContext, 
		data.ButtonStates[0], 
		data.ButtonStates[1], 
		data.ButtonStates[2]);

	//
	// Success indicates the report is ready to be sent, otherwise,
	// continue to service interrupts.
	//
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Error reporting button event - 0x%08lX",
			status);
	}

exit:
	return status;
}

NTSTATUS
RmiServiceInterruptF1A(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PREPORT_CONTEXT ReportContext
)
{
	NTSTATUS status = STATUS_SUCCESS;

	TchServiceButtonsInterrupts(ControllerContext, SpbContext, ReportContext);

	return status;
}