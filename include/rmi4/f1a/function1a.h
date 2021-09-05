/*++
	Copyright (c) Microsoft Corporation. All Rights Reserved.
	Copyright (c) LumiaWOA Authors. All Rights Reserved.
	Sample code. Dealpoint ID #843729.

	Module Name:

		function1a.h

	Abstract:

		Contains function definitions used internally
		by the F1A digitizer function.

	Environment:

		Kernel mode

	Revision History:

--*/

#pragma once

#include <wdm.h>
#include <wdf.h>
#include <rmi4\rmiinternal.h>

#define RMI4_INTERRUPT_BIT_0D_CAP_BUTTON          0x10
#define RMI4_INTERRUPT_BIT_0D_CAP_BUTTON_REVERSED 0x20

//
// Function $1A - 0-D Capacitive Button Sensors
//

typedef struct _RMI4_F1A_QUERY_REGISTERS
{
	struct
	{
		BYTE MaxButtonCount : 2;
		BYTE Reserved0 : 5;
	};
	struct
	{
		BYTE HasGenControl : 1;
		BYTE HasIntEnable : 1;
		BYTE HasMultiButSel : 1;
		BYTE HasTxRxMapping : 1;
		BYTE HasPerButThresh : 1;
		BYTE HasRelThresh : 1;
		BYTE HasStrongButHyst : 1;
		BYTE HasFiltStrength : 1;
	};
} RMI4_F1A_QUERY_REGISTERS;

typedef struct _RMI4_F1A_CTRL_REGISTERS
{
	struct
	{
		BYTE MultiButtonRpt : 2;
		BYTE FilterMode : 2;
		BYTE Reserved0 : 4;
	};
	struct
	{
		BYTE IntEnBtn0 : 1;
		BYTE IntEnBtn1 : 1;
		BYTE IntEnBtn2 : 1;
		BYTE IntEnBtn3 : 1;
		BYTE Reserved1 : 4;
	};
	struct
	{
		BYTE MultiBtn0 : 1;
		BYTE MultiBtn1 : 1;
		BYTE MultiBtn2 : 1;
		BYTE MultiBtn3 : 1;
		BYTE Reserved2 : 4;
	};
	BYTE PhysicalTx0;
	BYTE PhysicalRx0;
	BYTE PhysicalTx1;
	BYTE PhysicalRx1;
	BYTE PhysicalTx2;
	BYTE PhysicalRx2;
	BYTE PhysicalTx3;
	BYTE PhysicalRx3;
	BYTE Threshold0;
	BYTE Threshold1;
	BYTE Threshold2;
	BYTE Threshold3;
	BYTE ReleaseThreshold;
	BYTE StrongButtonHyst;
	BYTE FilterStrength;
} RMI4_F1A_CTRL_REGISTERS;

typedef struct _RMI4_F1A_DATA_REGISTERS
{
	union
	{
		struct
		{
			BYTE Button0 : 1;
			BYTE Button1 : 1;
			BYTE Button2 : 1;
			BYTE Button3 : 1;
			BYTE Reserved0 : 4;
		};
		BYTE Raw;
	};
} RMI4_F1A_DATA_REGISTERS;


NTSTATUS
RmiConfigureF1A(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
);

NTSTATUS
RmiGetObjectsFromControllerF1A(
	IN VOID* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN RMI4_DETECTED_BUTTONS* Data,
	IN BOOLEAN ReversedKeys
);

NTSTATUS
RmiServiceInterruptF1A(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN WDFQUEUE PingPongQueue
);