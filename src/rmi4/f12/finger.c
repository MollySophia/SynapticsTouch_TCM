// Copyright (c) Microsoft Corporation. All Rights Reserved. 
// Copyright (c) Bingxing Wang. All Rights Reserved. 
// Copyright (c) LumiaWOA authors. All Rights Reserved. 

#include <Cross Platform Shim\compat.h>
#include <controller.h>
#include <HidCommon.h>
#include <spb.h>
#include <rmi4\rmiinternal.h>
#include <rmi4\f12\controlregisters.h>
#include <rmi4\f12\finger.h>
#include <finger.tmh>

USHORT
RmiGetFingerCountFromControllerF12(
	IN VOID* ControllerContext
)
{
	RMI4_CONTROLLER_CONTEXT* controller;
	controller = (RMI4_CONTROLLER_CONTEXT*)ControllerContext;

	UINT8 Data1Offset = RmiGetRegisterIndex(&controller->DataRegDesc, 1);
	if (Data1Offset == controller->DataRegDesc.NumRegisters)
	{
		return 0;
	}
	USHORT Data1Size = (USHORT)controller->DataRegDesc.Registers[Data1Offset].RegisterSize;

	USHORT SimpleSize = sizeof(RMI4_F12_FINGER_SIMPLE_DATA_REGISTER);
	USHORT ZSize = sizeof(RMI4_F12_FINGER_3D_DATA_REGISTER);
	USHORT WSize = sizeof(RMI4_F12_FINGER_W_DATA_REGISTER);
	USHORT ZWSize = sizeof(RMI4_F12_FINGER_3D_W_DATA_REGISTER);

	USHORT FingerCount = 0;

	if (Data1Size % ZWSize == 0)
	{
		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_INTERRUPT,
			"Data register reports simple, z and w data");

		FingerCount = Data1Size / ZWSize;
	}
	else if (Data1Size % WSize == 0)
	{
		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_INTERRUPT,
			"Data register reports simple and w data");

		FingerCount = Data1Size / WSize;
	}
	else if (Data1Size % ZSize == 0)
	{
		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_INTERRUPT,
			"Data register reports simple and z data");

		FingerCount = Data1Size / ZSize;
	}
	else if (Data1Size % SimpleSize == 0)
	{
		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_INTERRUPT,
			"Data register reports simple data");

		FingerCount = Data1Size / SimpleSize;
	}
	else
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected - Data 1 size is not as expected: %d",
			Data1Size);
	}

	return FingerCount;
}

NTSTATUS
RmiGetFingerStatusFromControllerF12(
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
	RMI4_CONTROLLER_CONTEXT* controller;

	int index, i, x, y;
	PVOID controllerData = NULL;
	controller = (RMI4_CONTROLLER_CONTEXT*)ControllerContext;

	UINT8 Data1Offset = RmiGetRegisterIndex(&controller->DataRegDesc, 1);
	if (Data1Offset == controller->DataRegDesc.NumRegisters)
	{
		status = STATUS_SUCCESS;
		goto exit;
	}
	USHORT Data1Size = (USHORT)controller->DataRegDesc.Registers[Data1Offset].RegisterSize;

	if (controller->MaxFingers == 0)
	{
		status = STATUS_SUCCESS;
		goto exit;
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
		Data1Size,
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
		controller->Descriptors[index].DataBase + (BYTE)Data1Offset,
		controllerData,
		Data1Size
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Error reading finger status data - 0x%08lX",
			status);

		goto free_buffer;
	}

	BYTE ObjectTypeAndStatus = 0;
	BYTE X_MSB = 0;
	BYTE X_LSB = 0;
	BYTE Y_MSB = 0;
	BYTE Y_LSB = 0;

	for (i = 0; i < controller->MaxFingers; i++)
	{
		switch (Data1Size / controller->MaxFingers)
		{
			case sizeof(RMI4_F12_FINGER_SIMPLE_DATA_REGISTER) :
			{
				PRMI4_F12_FINGER_SIMPLE_DATA_REGISTER controllerFingerData = (PRMI4_F12_FINGER_SIMPLE_DATA_REGISTER)controllerData;

				ObjectTypeAndStatus = controllerFingerData[i].ObjectTypeAndStatus;
				X_MSB = controllerFingerData[i].X_MSB;
				X_LSB = controllerFingerData[i].X_LSB;
				Y_MSB = controllerFingerData[i].Y_MSB;
				Y_LSB = controllerFingerData[i].Y_LSB;
				break;
			}
			case sizeof(RMI4_F12_FINGER_3D_DATA_REGISTER) :
			{
				PRMI4_F12_FINGER_3D_DATA_REGISTER controllerFingerData = (PRMI4_F12_FINGER_3D_DATA_REGISTER)controllerData;

				ObjectTypeAndStatus = controllerFingerData[i].ObjectTypeAndStatus;
				X_MSB = controllerFingerData[i].X_MSB;
				X_LSB = controllerFingerData[i].X_LSB;
				Y_MSB = controllerFingerData[i].Y_MSB;
				Y_LSB = controllerFingerData[i].Y_LSB;
				break;
			}
			case sizeof(RMI4_F12_FINGER_W_DATA_REGISTER) :
			{
				PRMI4_F12_FINGER_W_DATA_REGISTER controllerFingerData = (PRMI4_F12_FINGER_W_DATA_REGISTER)controllerData;

				ObjectTypeAndStatus = controllerFingerData[i].ObjectTypeAndStatus;
				X_MSB = controllerFingerData[i].X_MSB;
				X_LSB = controllerFingerData[i].X_LSB;
				Y_MSB = controllerFingerData[i].Y_MSB;
				Y_LSB = controllerFingerData[i].Y_LSB;
				break;
			}
			case sizeof(RMI4_F12_FINGER_3D_W_DATA_REGISTER) :
			{
				PRMI4_F12_FINGER_3D_W_DATA_REGISTER controllerFingerData = (PRMI4_F12_FINGER_3D_W_DATA_REGISTER)controllerData;

				ObjectTypeAndStatus = controllerFingerData[i].ObjectTypeAndStatus;
				X_MSB = controllerFingerData[i].X_MSB;
				X_LSB = controllerFingerData[i].X_LSB;
				Y_MSB = controllerFingerData[i].Y_MSB;
				Y_LSB = controllerFingerData[i].Y_LSB;
				break;
			}
		}

		switch (ObjectTypeAndStatus)
		{
		case RMI4_F12_OBJECT_FINGER:
			Data->FingerStates[i] = RMI4_FINGER_STATE_PRESENT_WITH_ACCURATE_POS;
			Data->PenStates[i] = RMI4_PEN_STATE_NOT_PRESENT;
			Data->PuckStates[i] = RMI4_PUCK_STATE_NOT_PRESENT;

			Trace(
				TRACE_LEVEL_INFORMATION,
				TRACE_INTERRUPT,
				"Finger[%d] status is finger",
				i);

			break;
		case RMI4_F12_OBJECT_PALM:
			Data->FingerStates[i] = RMI4_FINGER_STATE_NOT_PRESENT;
			Data->PenStates[i] = RMI4_PEN_STATE_NOT_PRESENT;
			Data->PuckStates[i] = RMI4_PUCK_STATE_PRESENT_WITH_ACCURATE_POS;

			Trace(
				TRACE_LEVEL_INFORMATION,
				TRACE_INTERRUPT,
				"Finger[%d] status is palm",
				i);

			break;
		case RMI4_F12_OBJECT_HOVERING_FINGER:
			Data->FingerStates[i] = RMI4_FINGER_STATE_PRESENT_WITH_ACCURATE_POS;
			Data->PenStates[i] = RMI4_PEN_STATE_NOT_PRESENT;
			Data->PuckStates[i] = RMI4_PUCK_STATE_NOT_PRESENT;

			Trace(
				TRACE_LEVEL_INFORMATION,
				TRACE_INTERRUPT,
				"Finger[%d] status is hovering finger",
				i);

			break;
		case RMI4_F12_OBJECT_GLOVED_FINGER:
			Data->FingerStates[i] = RMI4_FINGER_STATE_PRESENT_WITH_ACCURATE_POS;
			Data->PenStates[i] = RMI4_PEN_STATE_NOT_PRESENT;
			Data->PuckStates[i] = RMI4_PUCK_STATE_NOT_PRESENT;

			Trace(
				TRACE_LEVEL_INFORMATION,
				TRACE_INTERRUPT,
				"Finger[%d] status is gloved finger",
				i);

			break;
		case RMI4_F12_OBJECT_ACTIVE_STYLUS:
			Data->FingerStates[i] = RMI4_FINGER_STATE_NOT_PRESENT;
			Data->PenStates[i] = RMI4_PEN_STATE_PRESENT_WITH_TIP;
			Data->PuckStates[i] = RMI4_PUCK_STATE_NOT_PRESENT;

			Trace(
				TRACE_LEVEL_INFORMATION,
				TRACE_INTERRUPT,
				"Finger[%d] status is active stylus",
				i);

			break;
		case RMI4_F12_OBJECT_STYLUS:
			Data->FingerStates[i] = RMI4_FINGER_STATE_NOT_PRESENT;
			Data->PenStates[i] = RMI4_PEN_STATE_PRESENT_WITH_TIP;
			Data->PuckStates[i] = RMI4_PUCK_STATE_NOT_PRESENT;

			Trace(
				TRACE_LEVEL_INFORMATION,
				TRACE_INTERRUPT,
				"Finger[%d] status is stylus",
				i);

			break;
		case RMI4_F12_OBJECT_ERASER:
			Data->FingerStates[i] = RMI4_FINGER_STATE_NOT_PRESENT;
			Data->PenStates[i] = RMI4_PEN_STATE_PRESENT_WITH_ERASER;
			Data->PuckStates[i] = RMI4_PUCK_STATE_NOT_PRESENT;

			Trace(
				TRACE_LEVEL_INFORMATION,
				TRACE_INTERRUPT,
				"Finger[%d] status is eraser",
				i);

			break;
		case RMI4_F12_OBJECT_NONE:
			Data->FingerStates[i] = RMI4_FINGER_STATE_NOT_PRESENT;
			Data->PenStates[i] = RMI4_PEN_STATE_NOT_PRESENT;
			Data->PuckStates[i] = RMI4_PUCK_STATE_NOT_PRESENT;

			Trace(
				TRACE_LEVEL_VERBOSE,
				TRACE_INTERRUPT,
				"Finger[%d] status is none",
				i);

			break;
		default:
			Data->FingerStates[i] = RMI4_FINGER_STATE_NOT_PRESENT;
			Data->PenStates[i] = RMI4_PEN_STATE_NOT_PRESENT;
			Data->PuckStates[i] = RMI4_PUCK_STATE_NOT_PRESENT;

			Trace(
				TRACE_LEVEL_INFORMATION,
				TRACE_INTERRUPT,
				"Finger[%d] status is unknown: %d",
				i,
				ObjectTypeAndStatus);

			break;
		}

		x = (X_MSB << 8) | X_LSB;
		y = (Y_MSB << 8) | Y_LSB;

		Data->Positions[i].X = x;
		Data->Positions[i].Y = y;

		if (ObjectTypeAndStatus != (BYTE)RMI4_F12_OBJECT_NONE)
		{
			Trace(
				TRACE_LEVEL_VERBOSE,
				TRACE_INTERRUPT,
				"Finger[%d] X: %d, Y: %d",
				i,
				x,
				y);
		}
	}

free_buffer:
	ExFreePoolWithTag(
		controllerData,
		TOUCH_POOL_TAG_F12
	);

exit:
	return status;
}