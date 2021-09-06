/*++
	Copyright (c) Microsoft Corporation. All Rights Reserved.
	Sample code. Dealpoint ID #843729.

	Module Name:

		rmiinternal.c

	Abstract:

		Contains Synaptics initialization code

	Environment:

		Kernel mode

	Revision History:

--*/

#include <Cross Platform Shim\compat.h>
#include <spb.h>
#include <rmi4\f01\function01.h>
#include <rmi4\f12\function12.h>
#include <rmi4\f1a\function1a.h>
#include <rmi4\rmiinternal.h>
#include <rmiinternal.tmh>

NTSTATUS
RmiServiceInterrupts(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN WDFQUEUE PingPongQueue
)
{
	NTSTATUS status = STATUS_NO_DATA_DETECTED;
	RMI4_CONTROLLER_CONTEXT* controller;

	controller = (RMI4_CONTROLLER_CONTEXT*)ControllerContext;

	//
	// Grab a waitlock to ensure the ISR executes serially and is 
	// protected against power state transitions
	//
	WdfWaitLockAcquire(controller->ControllerLock, NULL);

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

			goto exit;
		}
	}

	int i;
	for (i = 0; i < controller->FunctionCount; i++)
	{
		if (controller->InterruptStatus & controller->FunctionInterruptMasks[i])
		{
			switch (controller->Descriptors[i].Number)
			{
			case 0x12:
			{
				status = RmiServiceInterruptF12(ControllerContext, SpbContext, PingPongQueue);
				if (!NT_SUCCESS(status))
				{
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_INTERRUPT,
						"Error servicing F12 interrupt - 0x%08lX",
						status);

					goto exit;
				}

				break;
			}
			case 0x1A:
			{
				status = RmiServiceInterruptF1A(ControllerContext, SpbContext, PingPongQueue);
				if (!NT_SUCCESS(status))
				{
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_INTERRUPT,
						"Error servicing F1A interrupt - 0x%08lX",
						status);

					goto exit;
				}

				break;
			}
			case 0x01:
			{
				status = RmiServiceInterruptF01(ControllerContext, SpbContext, PingPongQueue);
				if (!NT_SUCCESS(status))
				{
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_INTERRUPT,
						"Error servicing F01 interrupt - 0x%08lX",
						status);

					goto exit;
				}

				break;
			}
			default:
			{
				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_INTERRUPT,
					"Interrupt not handled yet, must extend driver - $%02X",
					controller->Descriptors[i].Number);

				break;
			}
			}

			controller->InterruptStatus &= ~(controller->FunctionInterruptMasks[i]);
		}
	}

	if (controller->InterruptStatus != 0)
	{
		Trace(
			TRACE_LEVEL_WARNING,
			TRACE_INTERRUPT,
			"Ignoring following interrupt flags - 0x%08lX",
			controller->InterruptStatus);

		controller->InterruptStatus = 0;
	}

exit:

	WdfWaitLockRelease(controller->ControllerLock);

	return status;
}

NTSTATUS
RmiChangePage(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN int DesiredPage
)
/*++

  Routine Description:

	This utility function changes the current register address page.

  Arguments:

	ControllerContext - A pointer to the current touch controller context
	SpbContext - A pointer to the current i2c context
	DesiredPage - The page the caller expects to be mapped in

  Return Value:

	NTSTATUS indicating success or failure

--*/
{
	BYTE page;
	NTSTATUS status;

	//
	// If we're on this page already return success
	//
	if (ControllerContext->CurrentPage == DesiredPage)
	{
		status = STATUS_SUCCESS;
	}
	else
	{
		page = (BYTE)DesiredPage;

		status = SpbWriteDataSynchronously(
			SpbContext,
			RMI4_PAGE_SELECT_ADDRESS,
			&page,
			sizeof(BYTE));

		if (NT_SUCCESS(status))
		{
			ControllerContext->CurrentPage = DesiredPage;
		}
	}

	return status;
}

int
RmiGetFunctionIndex(
	IN RMI4_FUNCTION_DESCRIPTOR* FunctionDescriptors,
	IN int FunctionCount,
	IN int FunctionDesired
)
/*++

  Routine Description:

	Returns the descriptor table index that corresponds to the
	desired RMI function.

  Arguments:

	FunctionDescriptors - A pointer to the touch controllers
	full list of function descriptors

	FunctionCount - The count of function descriptors contained
	in the above FunctionDescriptors list

	FunctionDesired - The RMI function number (note they are always
	in hexadecimal the RMI4 specification)

  Return Value:

	NTSTATUS indicating success or failure

--*/
{
	UCHAR i;

	for (i = 0; i < FunctionCount; i++)
	{
		//
		// Break if we found the index
		//
		if (FunctionDescriptors[i].Number == FunctionDesired)
		{
			break;
		}
	}

	//
	// Return the count if the index wasn't found
	//
	return i;
}

UCHAR
RmiGetInterruptMask(
	IN UCHAR interruptSrc,
	IN UCHAR interruptCount
)
{
	UCHAR ii;

	UCHAR interruptOff = interruptCount % 8;
	UCHAR interruptMask = 0;

	for (ii = interruptOff; ii < (interruptSrc + interruptOff); ii++) {
		interruptMask |= 1 << ii;
	}

	return interruptMask;
}

NTSTATUS
RmiConfigureFunctions(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
/*++

  Routine Description:

	RMI4 devices such as this Synaptics touch controller are organized
	as collections of logical functions. Discovered functions must be
	configured, which is done in this function (things like sleep
	timeouts, interrupt enables, report rates, etc.)

  Arguments:

	ControllerContext - A pointer to the current touch controller
	context

	SpbContext - A pointer to the current i2c context

  Return Value:

	NTSTATUS indicating success or failure

--*/
{
	NTSTATUS status = STATUS_SUCCESS;
	int function;
	UCHAR Number;

	for (function = 0; function < ControllerContext->FunctionCount; function++)
	{
		Number = ControllerContext->Descriptors[function].Number;

		switch (Number)
		{
		case 0x12:
		{
			status = RmiConfigureF12(
				ControllerContext,
				SpbContext
			);

			break;
		}
		case 0x1A:
		{
			status = RmiConfigureF1A(
				ControllerContext,
				SpbContext
			);

			break;
		}
		case 0x01:
		{
			status = RmiConfigureF01(
				ControllerContext,
				SpbContext
			);

			break;
		}
		default:
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Unknown function $%x",
				Number);

			status = STATUS_SUCCESS;
		}
		}

		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Could not configure function $%x - 0x%08lX",
				Number,
				status);

			goto exit;
		}
	}

exit:
	return status;
}

NTSTATUS
RmiBuildFunctionsTable(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
/*++

  Routine Description:

	RMI4 devices such as this Synaptics touch controller are organized
	as collections of logical functions. When initially communicating
	with the chip, a driver must build a table of available functions,
	as is done in this routine.

  Arguments:

	ControllerContext - A pointer to the current touch controller context

	SpbContext - A pointer to the current i2c context

  Return Value:

	NTSTATUS indicating success or failure

--*/
{
	UCHAR address;
	UCHAR interruptCount;
	int function;
	int page;
	NTSTATUS status;

	// Make sure we don't have any previous descriptor data available.
	RtlZeroMemory(
		ControllerContext->Descriptors,
		sizeof(ControllerContext->Descriptors));
	RtlZeroMemory(
		ControllerContext->FunctionOnPage,
		sizeof(ControllerContext->FunctionOnPage));
	RtlZeroMemory(
		ControllerContext->FunctionInterruptMasks,
		sizeof(ControllerContext->FunctionInterruptMasks));

	//
	// First function is at a fixed address 
	//
	function = 0;
	address = RMI4_FIRST_FUNCTION_ADDRESS;
	interruptCount = 0;
	page = 0;

	//
	// Discover chip functions one by one
	//
	do
	{
		//
		// Read function descriptor
		//
		status = SpbReadDataSynchronously(
			SpbContext,
			address,
			&ControllerContext->Descriptors[function],
			sizeof(RMI4_FUNCTION_DESCRIPTOR));

		if (!(NT_SUCCESS(status)))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"RmiBuildFunctionsTable: Error returned from SPB/I2C read attempt %d - 0x%08lX",
				function,
				status);
			goto exit;
		}

		RMI4_FUNCTION_DESCRIPTOR Descriptor = ControllerContext->Descriptors[function];

		//
		// If we've exhausted functions on this page, look for more functions
		// on the next register page
		//
		if (Descriptor.Number == 0)
		{
			page++;
			address = RMI4_FIRST_FUNCTION_ADDRESS;

			status = RmiChangePage(
				ControllerContext,
				SpbContext,
				page);

			if (!NT_SUCCESS(status))
			{
				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_INIT,
					"RmiBuildFunctionsTable: Error attempting to change page - 0x%08lX",
					status);
				goto exit;
			}
		}
		//
		// Descriptor stored, look for next or terminator
		//
		else
		{
			ControllerContext->FunctionOnPage[function] = page;
			ControllerContext->FunctionInterruptMasks[function] = RmiGetInterruptMask(
				Descriptor.VersionIrq.IrqCount,
				interruptCount);

			interruptCount += Descriptor.VersionIrq.IrqCount;

			Trace(
				TRACE_LEVEL_VERBOSE,
				TRACE_INIT,
				"RmiBuildFunctionsTable: Discovered Function $%x:\n"
				"- Query Base: 0x%02X\n"
				"- Command Base: 0x%02X\n"
				"- Control Base: 0x%02X\n"
				"- Data Base: 0x%02X\n"
				"- Interrupt Count: %d\n"
				"- Function Version: %d\n"
				"- Interrupt Mask: 0x%02X",
				Descriptor.Number,
				Descriptor.QueryBase,
				Descriptor.CommandBase,
				Descriptor.ControlBase,
				Descriptor.DataBase,
				Descriptor.VersionIrq.IrqCount,
				Descriptor.VersionIrq.FuncVer,
				ControllerContext->FunctionInterruptMasks[function]);

			function++;
			address = address - sizeof(RMI4_FUNCTION_DESCRIPTOR);
		}

	} while (
		(page < RMI4_MAX_FUNCTIONS) &&
		(function < RMI4_MAX_FUNCTIONS));

	//
	// If we swept the address space without finding an "end function"
	// or maxed-out the total number of functions supported by the 
	// driver, note the error and exit.
	//
	if (function >= RMI4_MAX_FUNCTIONS)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error, encountered more than %d functions, must extend driver",
			RMI4_MAX_FUNCTIONS);

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}
	if (address <= 0)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error, did not find terminator function 0, address down to %d",
			address);

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	//
	// Note the total number of functions that exist
	//
	ControllerContext->FunctionCount = function;

	Trace(
		TRACE_LEVEL_VERBOSE,
		TRACE_INIT,
		"Discovered %d RMI functions total",
		function);

exit:

	return status;
}

NTSTATUS
RmiReadRegisterDescriptor(
	IN SPB_CONTEXT* Context,
	IN UCHAR Address,
	IN PRMI_REGISTER_DESCRIPTOR Rdesc
)
{
	NTSTATUS Status;

	BYTE size_presence_reg = 0;
	BYTE buf[35];
	int presense_offset = 1;
	BYTE* struct_buf;
	int reg;
	int offset = 0;
	int map_offset = 0;
	int i;
	int b;

	Status = SpbReadDataSynchronously(
		Context,
		Address,
		&size_presence_reg,
		sizeof(BYTE)
	);

	if (!NT_SUCCESS(Status)) goto i2c_read_fail;

	++Address;

	if (size_presence_reg < 0 || size_presence_reg > 35)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"size_presence_reg has invalid size, either less than 0 or larger than 35");
		Status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	memset(buf, 0, sizeof(buf));

	/*
	* The presence register contains the size of the register structure
	* and a bitmap which identified which packet registers are present
	* for this particular register type (ie query, control, or data).
	*/
	Status = SpbReadDataSynchronously(
		Context,
		Address,
		buf,
		size_presence_reg
	);
	if (!NT_SUCCESS(Status)) goto i2c_read_fail;
	++Address;

	if (buf[0] == 0)
	{
		presense_offset = 3;
		Rdesc->StructSize = buf[1] | (buf[2] << 8);
	}
	else
	{
		Rdesc->StructSize = buf[0];
	}

	for (i = presense_offset; i < size_presence_reg; i++)
	{
		for (b = 0; b < 8; b++)
		{
			if (buf[i] & (0x1 << b)) bitmap_set(Rdesc->PresenceMap, map_offset, 1);
			++map_offset;
		}
	}

	Rdesc->NumRegisters = (UINT8)bitmap_weight(Rdesc->PresenceMap, RMI_REG_DESC_PRESENSE_BITS);
	Rdesc->Registers = ExAllocatePoolWithTag(
		NonPagedPoolNx,
		Rdesc->NumRegisters * sizeof(RMI_REGISTER_DESC_ITEM),
		TOUCH_POOL_TAG_F12
	);

	if (Rdesc->Registers == NULL)
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto exit;
	}

	/*
	* Allocate a temporary buffer to hold the register structure.
	* I'm not using devm_kzalloc here since it will not be retained
	* after exiting this function
	*/
	struct_buf = ExAllocatePoolWithTag(
		NonPagedPoolNx,
		Rdesc->StructSize,
		TOUCH_POOL_TAG_F12
	);

	if (struct_buf == NULL)
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto exit;
	}

	/*
	* The register structure contains information about every packet
	* register of this type. This includes the size of the packet
	* register and a bitmap of all subpackets contained in the packet
	* register.
	*/
	Status = SpbReadDataSynchronously(
		Context,
		Address,
		struct_buf,
		Rdesc->StructSize
	);

	if (!NT_SUCCESS(Status)) goto free_buffer;

	reg = find_first_bit(Rdesc->PresenceMap, RMI_REG_DESC_PRESENSE_BITS);
	for (i = 0; i < Rdesc->NumRegisters; i++)
	{
		PRMI_REGISTER_DESC_ITEM item = &Rdesc->Registers[i];
		int reg_size = struct_buf[offset];

		++offset;
		if (reg_size == 0)
		{
			reg_size = struct_buf[offset] |
				(struct_buf[offset + 1] << 8);
			offset += 2;
		}

		if (reg_size == 0)
		{
			reg_size = struct_buf[offset] |
				(struct_buf[offset + 1] << 8) |
				(struct_buf[offset + 2] << 16) |
				(struct_buf[offset + 3] << 24);
			offset += 4;
		}

		item->Register = (USHORT)reg;
		item->RegisterSize = reg_size;

		map_offset = 0;

		do {
			for (b = 0; b < 7; b++) {
				if (struct_buf[offset] & (0x1 << b))
					bitmap_set(item->SubPacketMap, map_offset, 1);
				++map_offset;
			}
		} while (struct_buf[offset++] & 0x80);

		item->NumSubPackets = (BYTE)bitmap_weight(item->SubPacketMap, RMI_REG_DESC_SUBPACKET_BITS);

		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_INIT,
			"%s: reg: %d reg size: %ld subpackets: %d",
			__func__,
			item->Register, item->RegisterSize, item->NumSubPackets
		);

		reg = find_next_bit(Rdesc->PresenceMap, RMI_REG_DESC_PRESENSE_BITS, reg + 1);
	}

free_buffer:
	ExFreePoolWithTag(
		struct_buf,
		TOUCH_POOL_TAG_F12
	);

exit:
	return Status;

i2c_read_fail:
	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"Failed to read general info register - 0x%08lX",
		Status);
	goto exit;
}

size_t
RmiRegisterDescriptorCalcSize(
	IN PRMI_REGISTER_DESCRIPTOR Rdesc
)
{
	PRMI_REGISTER_DESC_ITEM item;
	int i;
	size_t size = 0;

	for (i = 0; i < Rdesc->NumRegisters; i++)
	{
		item = &Rdesc->Registers[i];
		size += item->RegisterSize;
	}
	return size;
}

const PRMI_REGISTER_DESC_ITEM RmiGetRegisterDescItem(
	PRMI_REGISTER_DESCRIPTOR Rdesc,
	USHORT reg
)
{
	PRMI_REGISTER_DESC_ITEM item;
	int i;

	for (i = 0; i < Rdesc->NumRegisters; i++)
	{
		item = &Rdesc->Registers[i];
		if (item->Register == reg) return item;
	}

	return NULL;
}

UINT8 RmiGetRegisterIndex(
	PRMI_REGISTER_DESCRIPTOR Rdesc,
	USHORT reg
)
{
	UINT8 i;

	for (i = 0; i < Rdesc->NumRegisters; i++)
	{
		if (Rdesc->Registers[i].Register == reg) return i;
	}

	return Rdesc->NumRegisters;
}