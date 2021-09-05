#pragma once

#include <wdm.h>
#include <wdf.h>
#include <rmi4\rmiinternal.h>

// Ignore warning C4152: nonstandard extension, function/data pointer conversion in expression
#pragma warning (disable : 4152)

// Ignore warning C4201: nonstandard extension used : nameless struct/union
#pragma warning (disable : 4201)

// Ignore warning C4201: nonstandard extension used : bit field types other than in
#pragma warning (disable : 4214)

// Ignore warning C4324: 'xxx' : structure was padded due to __declspec(align())
#pragma warning (disable : 4324)

NTSTATUS
RmiReadFunctionControlRegister(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN RMI_REGISTER_DESCRIPTOR RegisterDescriptor,
	IN int FunctionDesired,
	IN USHORT ControlRegister,
	_In_reads_bytes_(BufferLength) PVOID Buffer,
	IN ULONG BufferLength
);

NTSTATUS
RmiReadFunctionDataRegister(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN RMI_REGISTER_DESCRIPTOR RegisterDescriptor,
	IN int FunctionDesired,
	IN USHORT DataRegister,
	_In_reads_bytes_(BufferLength) PVOID Buffer,
	IN ULONG BufferLength
);

NTSTATUS
RmiWriteFunctionControlRegister(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN RMI_REGISTER_DESCRIPTOR RegisterDescriptor,
	IN int FunctionDesired,
	IN USHORT ControlRegister,
	IN PVOID Buffer,
	IN ULONG BufferLength
);