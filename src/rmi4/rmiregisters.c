#include <Cross Platform Shim\compat.h>
#include <spb.h>
#include <rmi4\f01\function01.h>
#include <rmi4\f12\function12.h>
#include <rmi4\f1a\function1a.h>
#include <rmi4\rmiinternal.h>
#include <rmi4\rmiregisters.h>
#include <rmiregisters.tmh>

NTSTATUS
RmiReadFunctionControlRegister(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN RMI_REGISTER_DESCRIPTOR RegisterDescriptor,
	IN int FunctionDesired,
	IN USHORT ControlRegister,
	_In_reads_bytes_(BufferLength) PVOID Buffer,
	IN ULONG BufferLength
)
{
	int index;
	NTSTATUS status;
	UINT8 indexCtrl;

	RtlZeroMemory(Buffer, BufferLength);

	//
	// Find RMI function
	//
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		FunctionDesired);

	if (index == ControllerContext->FunctionCount)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"RmiReadFunctionControlRegister failure - RMI Function $%x missing",
			FunctionDesired);

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

	if (ControllerContext->HasRegisterDescriptors)
	{
		indexCtrl = RmiGetRegisterIndex(&RegisterDescriptor, ControlRegister);

		if (indexCtrl == RegisterDescriptor.NumRegisters)
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Cannot find F%x_CTRL%d offset",
				FunctionDesired,
				ControlRegister);

			status = STATUS_INVALID_DEVICE_STATE;
			goto exit;
		}

		if (RegisterDescriptor.Registers[indexCtrl].RegisterSize < BufferLength)
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Unexpected F%x_CTRL%d size: %d",
				FunctionDesired,
				ControlRegister,
				RegisterDescriptor.Registers[indexCtrl].RegisterSize);

			//status = STATUS_INVALID_DEVICE_STATE;
			//goto exit;
		}

		//
		// Read Device Control register
		//
		status = SpbReadDataSynchronously(
			SpbContext,
			ControllerContext->Descriptors[index].ControlBase + indexCtrl,
			Buffer,
			min(RegisterDescriptor.Registers[indexCtrl].RegisterSize, BufferLength)
		);
	}
	else
	{
		//
		// Read Device Control register
		//
		status = SpbReadDataSynchronously(
			SpbContext,
			ControllerContext->Descriptors[index].ControlBase,
			Buffer,
			BufferLength
		);
	}

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not read F%x_CTRL%d register - %X",
			FunctionDesired,
			ControlRegister,
			status);

		goto exit;
	}

exit:
	return status;
}

NTSTATUS
RmiReadFunctionDataRegister(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN RMI_REGISTER_DESCRIPTOR RegisterDescriptor,
	IN int FunctionDesired,
	IN USHORT DataRegister,
	_In_reads_bytes_(BufferLength) PVOID Buffer,
	IN ULONG BufferLength
)
{
	int index;
	NTSTATUS status;
	UINT8 indexData;

	RtlZeroMemory(Buffer, BufferLength);

	//
	// Find RMI function
	//
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		FunctionDesired);

	if (index == ControllerContext->FunctionCount)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"RmiReadFunctionDataRegister failure - RMI Function $%x missing",
			FunctionDesired);

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

	if (ControllerContext->HasRegisterDescriptors)
	{
		indexData = RmiGetRegisterIndex(&RegisterDescriptor, DataRegister);

		if (indexData == RegisterDescriptor.NumRegisters)
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Cannot find F%x_DATA%d offset",
				FunctionDesired,
				DataRegister);

			status = STATUS_INVALID_DEVICE_STATE;
			goto exit;
		}

		if (RegisterDescriptor.Registers[indexData].RegisterSize < BufferLength)
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Unexpected F%x_DATA%d size: %d",
				FunctionDesired,
				DataRegister,
				RegisterDescriptor.Registers[indexData].RegisterSize);

			//status = STATUS_INVALID_DEVICE_STATE;
			//goto exit;
		}

		//
		// Read Device Control register
		//
		status = SpbReadDataSynchronously(
			SpbContext,
			ControllerContext->Descriptors[index].DataBase + indexData,
			Buffer,
			min(RegisterDescriptor.Registers[indexData].RegisterSize, BufferLength)
		);
	}
	else
	{
		//
		// Read Device Control register
		//
		status = SpbReadDataSynchronously(
			SpbContext,
			ControllerContext->Descriptors[index].DataBase,
			Buffer,
			BufferLength
		);
	}

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not read F%x_DATA%d register - %X",
			FunctionDesired,
			DataRegister,
			status);

		goto exit;
	}

exit:
	return status;
}

NTSTATUS
RmiWriteFunctionControlRegister(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN RMI_REGISTER_DESCRIPTOR RegisterDescriptor,
	IN int FunctionDesired,
	IN USHORT ControlRegister,
	IN PVOID Buffer,
	IN ULONG BufferLength
)
{
	int index;
	NTSTATUS status;
	UINT8 indexCtrl;

	//
	// Find RMI function
	//
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		FunctionDesired);

	if (index == ControllerContext->FunctionCount)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"RmiWriteFunctionControlRegister failure - RMI Function $%x missing",
			FunctionDesired);

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

	if (ControllerContext->HasRegisterDescriptors)
	{
		indexCtrl = RmiGetRegisterIndex(&RegisterDescriptor, ControlRegister);

		if (indexCtrl == RegisterDescriptor.NumRegisters)
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Cannot find F%x_CTRL%d offset",
				FunctionDesired,
				ControlRegister);

			status = STATUS_INVALID_DEVICE_STATE;
			goto exit;
		}

		if (RegisterDescriptor.Registers[indexCtrl].RegisterSize < BufferLength)
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Unexpected F%x_CTRL%d size: %d",
				FunctionDesired,
				ControlRegister,
				RegisterDescriptor.Registers[indexCtrl].RegisterSize);

			//status = STATUS_INVALID_DEVICE_STATE;
			//goto exit;
		}

		//
		// Write setting to the controller
		//
		status = SpbWriteDataSynchronously(
			SpbContext,
			ControllerContext->Descriptors[index].ControlBase + indexCtrl,
			Buffer,
			min(RegisterDescriptor.Registers[indexCtrl].RegisterSize, BufferLength)
		);
	}
	else
	{
		//
		// Write setting to the controller
		//
		status = SpbWriteDataSynchronously(
			SpbContext,
			ControllerContext->Descriptors[index].ControlBase,
			Buffer,
			BufferLength
		);
	}

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write F%x_CTRL%d register - %X",
			FunctionDesired,
			ControlRegister,
			status);

		goto exit;
	}

exit:
	return status;
}