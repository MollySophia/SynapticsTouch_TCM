// Copyright (c) Microsoft Corporation. All Rights Reserved. 
// Copyright (c) Bingxing Wang. All Rights Reserved. 
// Copyright (c) LumiaWOA authors. All Rights Reserved. 

#include <Cross Platform Shim\compat.h>
#include <controller.h>
#include <HidCommon.h>
#include <spb.h>
#include <rmi4\rmiinternal.h>
#include <rmi4\f12\controlregisters.h>
#include <rmi4\f12\activepen.h>
#include <activepen.tmh>

NTSTATUS
RmiGetPenStatusFromControllerF12(
	IN VOID* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN RMI4_DETECTED_OBJECTS* Data
)
{
	NTSTATUS status;
	RMI4_CONTROLLER_CONTEXT* controller;

	int index;
	PVOID controllerData = NULL;
	controller = (RMI4_CONTROLLER_CONTEXT*)ControllerContext;

	UINT8 Data6Offset = RmiGetRegisterIndex(&controller->DataRegDesc, 6);
	if (Data6Offset == controller->DataRegDesc.NumRegisters)
	{
		return 0;
	}
	USHORT Data6Size = (USHORT)controller->DataRegDesc.Registers[Data6Offset].RegisterSize;

	if (Data6Size != sizeof(RMI4_F12_16BIT_PRESSURE_STYLUS_DATA_REGISTER) &&
		Data6Size != sizeof(RMI4_F12_8BIT_PRESSURE_STYLUS_DATA_REGISTER))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected - Data 6 size is not as expected: %d",
			Data6Size);

		return STATUS_SUCCESS;
	}

	//
	// Locate RMI data base address of 2D touch function
	//
	index = RmiGetFunctionIndex(
		controller->Descriptors,
		controller->FunctionCount,
		RMI4_F12_2D_TOUCHPAD_SENSOR);

	if (index == controller->FunctionCount)
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
		controller->FunctionOnPage[index]);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not change register page");

		goto exit;
	}

	controllerData = ExAllocatePoolWithTag(
		NonPagedPoolNx,
		Data6Size,
		TOUCH_POOL_TAG_F12
	);

	if (controllerData == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto exit;
	}

	// 
	// Packets we need is determined by context
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		controller->Descriptors[index].DataBase + (BYTE)Data6Offset,
		controllerData,
		Data6Size
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Error reading pen status data - 0x%08lX",
			status);

		goto free_buffer;
	}

	if (Data6Size == sizeof(RMI4_F12_16BIT_PRESSURE_STYLUS_DATA_REGISTER))
	{
		PRMI4_F12_16BIT_PRESSURE_STYLUS_DATA_REGISTER controllerPenData = (PRMI4_F12_16BIT_PRESSURE_STYLUS_DATA_REGISTER)controllerData;

		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_INTERRUPT,
			"Alt Pen Data:\n"
			"Pen: %d\n"
			"Invert: %d\n"
			"Barrel: %d\n"
			"X: %d\n"
			"Y: %d\n"
			"Pressure: %d\n"
			"Battery: %d\n"
			"PenId: %d",
			controllerPenData->Pen,
			controllerPenData->Invert,
			controllerPenData->Barrel,
			controllerPenData->X_MSB,
			controllerPenData->Y_MSB,
			controllerPenData->Pressure_MSB,
			controllerPenData->Battery,
			controllerPenData->PenId_24_31);

		// Translate structure
		Data->ActivePenState.Pen = controllerPenData->Pen;
		Data->ActivePenState.Invert = controllerPenData->Invert;
		Data->ActivePenState.Barrel = controllerPenData->Barrel;
		Data->ActivePenState.X = (controllerPenData->X_MSB << 8) | controllerPenData->X_LSB;
		Data->ActivePenState.Y = (controllerPenData->Y_MSB << 8) | controllerPenData->Y_LSB;
		Data->ActivePenState.Pressure = (controllerPenData->Pressure_MSB << 8) | controllerPenData->Pressure_LSB;
		Data->ActivePenState.Battery = controllerPenData->Battery;
		Data->ActivePenState.PenId = (controllerPenData->PenId_0_7 << 24) | 
			(controllerPenData->PenId_8_15 << 16) | 
			(controllerPenData->PenId_16_23 << 8) | 
			controllerPenData->PenId_24_31;
	}
	else if (Data6Size == sizeof(RMI4_F12_8BIT_PRESSURE_STYLUS_DATA_REGISTER))
	{
		PRMI4_F12_8BIT_PRESSURE_STYLUS_DATA_REGISTER controllerPenData = (PRMI4_F12_8BIT_PRESSURE_STYLUS_DATA_REGISTER)controllerData;

		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_INTERRUPT,
			"Alt Pen Data:\n"
			"Pen: %d\n"
			"Invert: %d\n"
			"Barrel: %d\n"
			"X: %d\n"
			"Y: %d\n"
			"Pressure: %d\n"
			"Battery: %d\n"
			"PenId: %d",
			controllerPenData->Pen,
			controllerPenData->Invert,
			controllerPenData->Barrel,
			controllerPenData->X_MSB,
			controllerPenData->Y_MSB,
			controllerPenData->Pressure,
			controllerPenData->Battery,
			controllerPenData->PenId_24_31);

		// Translate structure
		Data->ActivePenState.Pen = controllerPenData->Pen;
		Data->ActivePenState.Invert = controllerPenData->Invert;
		Data->ActivePenState.Barrel = controllerPenData->Barrel;
		Data->ActivePenState.X = (controllerPenData->X_MSB << 8) | controllerPenData->X_LSB;
		Data->ActivePenState.Y = (controllerPenData->Y_MSB << 8) | controllerPenData->Y_LSB;
		Data->ActivePenState.Pressure = controllerPenData->Pressure;
		Data->ActivePenState.Battery = controllerPenData->Battery;
		Data->ActivePenState.PenId = (controllerPenData->PenId_0_7 << 24) |
			(controllerPenData->PenId_8_15 << 16) |
			(controllerPenData->PenId_16_23 << 8) |
			controllerPenData->PenId_24_31;
	}

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INTERRUPT,
		"Active Pen Data:\n"
		"Pen: %d\n"
		"Invert: %d\n"
		"Barrel: %d\n"
		"X: %d\n"
		"Y: %d\n"
		"Pressure: %d\n"
		"Battery: %d\n"
		"PenId: %d",
		Data->ActivePenState.Pen,
		Data->ActivePenState.Invert,
		Data->ActivePenState.Barrel,
		Data->ActivePenState.X,
		Data->ActivePenState.Y,
		Data->ActivePenState.Pressure,
		Data->ActivePenState.Battery,
		Data->ActivePenState.PenId);

free_buffer:
	ExFreePoolWithTag(
		controllerData,
		TOUCH_POOL_TAG_F12
	);

exit:
	return status;
}