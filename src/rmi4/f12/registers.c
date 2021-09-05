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
#include <registers.tmh>

NTSTATUS
RmiReadControlRegister(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN USHORT ControlRegister,
	_In_reads_bytes_(BufferLength) PVOID Buffer,
	IN ULONG BufferLength
)
{
	return RmiReadFunctionControlRegister(
		ControllerContext,
		SpbContext,
		ControllerContext->F12ControlRegDesc,
		RMI4_F12_2D_TOUCHPAD_SENSOR,
		ControlRegister,
		Buffer,
		BufferLength
	);
}

NTSTATUS
RmiReadDataRegister(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN USHORT DataRegister,
	_In_reads_bytes_(BufferLength) PVOID Buffer,
	IN ULONG BufferLength
)
{
	return RmiReadFunctionDataRegister(
		ControllerContext,
		SpbContext,
		ControllerContext->F12DataRegDesc,
		RMI4_F12_2D_TOUCHPAD_SENSOR,
		DataRegister,
		Buffer,
		BufferLength
	);
}

NTSTATUS
RmiWriteControlRegister(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN USHORT ControlRegister,
	IN PVOID Buffer,
	IN ULONG BufferLength
)
{
	return RmiWriteFunctionControlRegister(
		ControllerContext,
		SpbContext,
		ControllerContext->F12ControlRegDesc,
		RMI4_F12_2D_TOUCHPAD_SENSOR,
		ControlRegister,
		Buffer,
		BufferLength
	);
}