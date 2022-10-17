/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved.
    Copyright (c) Bingxing Wang. All Rights Reserved.
    Copyright (c) LumiaWoA authors. All Rights Reserved.

	Module Name:

		init.c

	Abstract:

		Contains Synaptics initialization code

	Environment:

		Kernel mode

	Revision History:

--*/

#include <Cross Platform Shim\compat.h>
#include <spb.h>
#include <tcm/touch_tcm.h>
#include <init.tmh>

NTSTATUS
TchStartDevice(
	IN VOID* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
/*++

  Routine Description:

	This routine is called in response to the KMDF prepare hardware call
	to initialize the touch controller for use.

  Arguments:

	ControllerContext - A pointer to the current touch controller
	context

	SpbContext - A pointer to the current i2c context

  Return Value:

	NTSTATUS indicating success or failure

--*/
{
	NTSTATUS status = STATUS_SUCCESS;
	TCM_CONTROLLER_CONTEXT* controller = (TCM_CONTROLLER_CONTEXT*)ControllerContext;

	if (controller == NULL) {
		return STATUS_INVALID_PARAMETER;
	}

	status = TcmReadMessage(controller,
						SpbContext,
						(PREPORT_CONTEXT)NULL);
	if(!NT_SUCCESS(status)) {
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read Identification packet - 0x%08lX",
			status);
		return STATUS_UNSUCCESSFUL;
	}
	
	controller->ControllerState.Power = TCM_POWER_ON;
	controller->ControllerState.Init = TRUE;

	status = TcmGetIcInfo(controller,
		SpbContext);

	if (!NT_SUCCESS(status)) {
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to get ic info - 0x%08lX",
			status);
		return STATUS_UNSUCCESSFUL;
	}

	status = TcmGetReportConfig(controller,
		SpbContext);

	if (!NT_SUCCESS(status) || controller->ConfigData.DataLength == 0) {
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to get report config - 0x%08lX",
			status);
		return STATUS_UNSUCCESSFUL;
	}

	return status;
}

NTSTATUS
TchStopDevice(
	IN VOID* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
/*++

Routine Description:

	This routine cleans up the device that is stopped.

Argument:

	ControllerContext - Touch controller context

	SpbContext - A pointer to the current i2c context

Return Value:

	NTSTATUS indicating sucess or failure
--*/
{
	(void*)ControllerContext;
	(void*)SpbContext;
	// TCM_CONTROLLER_CONTEXT* controller;

	// UNREFERENCED_PARAMETER(SpbContext);

	// controller = (TCM_CONTROLLER_CONTEXT*)ControllerContext;

	return STATUS_SUCCESS;
}

NTSTATUS
TchAllocateContext(
	OUT VOID** ControllerContext,
	IN WDFDEVICE FxDevice
)
/*++

Routine Description:

	This routine allocates a controller context.

Argument:

	ControllerContext - Touch controller context
	FxDevice - Framework device object

Return Value:

	NTSTATUS indicating sucess or failure
--*/
{
	TCM_CONTROLLER_CONTEXT* context;
	NTSTATUS status;
	
	context = ExAllocatePoolWithTag(
		NonPagedPoolNx,
		sizeof(TCM_CONTROLLER_CONTEXT),
		TOUCH_POOL_TAG);

	if (NULL == context)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not allocate controller context!");

		status = STATUS_UNSUCCESSFUL;
		goto exit;
	}

	RtlZeroMemory(context, sizeof(TCM_CONTROLLER_CONTEXT));
	context->FxDevice = FxDevice;

	//
	// Get Touch settings and populate context
	//
	//TchGetTouchSettings(&context->TouchSettings);

	//
	// Allocate a WDFWAITLOCK for guarding access to the
	// controller HW and driver controller context
	//
	status = WdfWaitLockCreate(
		WDF_NO_OBJECT_ATTRIBUTES,
		&context->ControllerLock);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not create lock - 0x%08lX",
			status);

		TchFreeContext(context);
		goto exit;

	}

	context->ResponseSignal = ExAllocatePoolWithTag(
		NonPagedPool,
		sizeof(TCM_SIGNAL_OBJ),
		TOUCH_POOL_TAG);

	if (NULL == context->ResponseSignal)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not allocate response signal obj!");

		status = STATUS_UNSUCCESSFUL;
		goto exit;
	}

	KeInitializeEvent(&context->ResponseSignal->Event, SynchronizationEvent, FALSE);

	context->DeviceAddr = 0x20;
	context->MaxFingers = MAX_FINGER;
	context->GesturesEnabled = FALSE;

	*ControllerContext = context;

exit:

	return status;
}

NTSTATUS
TchFreeContext(
	IN VOID* ControllerContext
)
/*++

Routine Description:

	This routine frees a controller context.

Argument:

	ControllerContext - Touch controller context

Return Value:

	NTSTATUS indicating sucess or failure
--*/
{
	TCM_CONTROLLER_CONTEXT* controller;

	controller = (TCM_CONTROLLER_CONTEXT*)ControllerContext;

	if (controller != NULL)
	{

		if (controller->ControllerLock != NULL)
		{
			WdfObjectDelete(controller->ControllerLock);
		}

		ExFreePoolWithTag(controller, TOUCH_POOL_TAG);
	}

	return STATUS_SUCCESS;
}