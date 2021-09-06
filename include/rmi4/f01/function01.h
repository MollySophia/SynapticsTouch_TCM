/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved.
    Copyright (c) LumiaWOA Authors. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name:

        function01.h

    Abstract:

        Contains function definitions used internally
        by the F01 digitizer function.

    Environment:

        Kernel mode

    Revision History:

--*/

#pragma once

#include <wdm.h>
#include <wdf.h>
#include <rmi4\rmiinternal.h>

typedef struct _RMI4_F01_CTRL_REGISTERS
{
    union
    {
        BYTE All;
        struct
        {
            BYTE SleepMode        : 2; // 0..1
            BYTE NoSleep          : 1; // 2
            BYTE Unknown0         : 1; // 3
            BYTE Reserved0        : 1; // 4
            BYTE ChargerConnected : 1; // 5
            BYTE ReportRate       : 1; // 6
            BYTE Configured       : 1; // 7
        };
    } DeviceControl;
    BYTE InterruptEnable;
    BYTE DozeInterval;
    BYTE DozeThreshold;
    BYTE DozeHoldoff;
} RMI4_F01_CTRL_REGISTERS;

#define RMI4_F01_DEVICE_CONTROL_SLEEP_MODE_OPERATING  0
#define RMI4_F01_DEVICE_CONTROL_SLEEP_MODE_SLEEPING   1

typedef struct _RMI4_F01_DATA_REGISTERS
{
    union {
        BYTE All;
        struct
        {
            BYTE Status : 4;
            BYTE Reserved0 : 2;
            BYTE FlashProg : 1;
            BYTE Unconfigured : 1;
        };
    } DeviceStatus;
    BYTE InterruptStatus[1];
} RMI4_F01_DATA_REGISTERS;

#define RMI4_F01_DATA_STATUS_NO_ERROR             0
#define RMI4_F01_DATA_STATUS_RESET_OCCURRED       1
#define RMI4_F01_DATA_STATUS_INVALID_CONFIG       2
#define RMI4_F01_DATA_STATUS_DEVICE_FAILURE       3
#define RMI4_F01_DATA_STATUS_CONFIG_CRC_FAILURE   4
#define RMI4_F01_DATA_STATUS_FW_CRC_FAILURE       5
#define RMI4_F01_DATA_STATUS_CRC_IN_PROGRESS      6

typedef struct _RMI4_F01_COMMAND_REGISTERS
{
    BYTE Reset;
} RMI4_F01_COMMAND_REGISTERS;

NTSTATUS
RmiChangeChargerConnectedState(
    IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
    IN SPB_CONTEXT* SpbContext,
    IN UCHAR ChargerConnectedState
);

NTSTATUS
RmiChangeSleepState(
    IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
    IN SPB_CONTEXT* SpbContext,
    IN UCHAR SleepState
);

NTSTATUS
RmiCheckInterrupts(
    IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
    IN SPB_CONTEXT* SpbContext,
    IN ULONG* InterruptStatus
);

NTSTATUS
RmiConfigureF01(
    IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
    IN SPB_CONTEXT* SpbContext
);

NTSTATUS
RmiGetFirmwareVersion(
    IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
    IN SPB_CONTEXT* SpbContext
);

VOID
RmiConvertF01ToPhysical(
    IN RMI4_F01_CTRL_REGISTERS_LOGICAL* Logical,
    IN RMI4_F01_CTRL_REGISTERS* Physical
);

NTSTATUS
RmiSetInterruptEnable(
    IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
    IN SPB_CONTEXT* SpbContext,
    IN UCHAR InterruptEnable
);

UCHAR
RmiGetFunctionInterruptMask(
    IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
    IN int FunctionDesired
);

NTSTATUS
RmiConfigureInterruptEnable(
    IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
    IN SPB_CONTEXT* SpbContext
);

NTSTATUS
RmiServiceInterruptF01(
    IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
    IN SPB_CONTEXT* SpbContext,
    IN WDFQUEUE PingPongQueue
);