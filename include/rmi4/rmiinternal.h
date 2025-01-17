/*++
	Copyright (c) Microsoft Corporation. All Rights Reserved.
	Sample code. Dealpoint ID #843729.

	Module Name:

		rmiinternal.h

	Abstract:

		Contains common types and defintions used internally
		by the multi touch screen driver.

	Environment:

		Kernel mode

	Revision History:

--*/

#pragma once

#include <wdm.h>
#include <wdf.h>
#include <controller.h>
#include <resolutions.h>
#include <Cross Platform Shim/bitops.h>
#include <Cross Platform Shim/hweight.h>
#include <report.h>

// Ignore warning C4152: nonstandard extension, function/data pointer conversion in expression
#pragma warning (disable : 4152)

// Ignore warning C4201: nonstandard extension used : nameless struct/union
#pragma warning (disable : 4201)

// Ignore warning C4201: nonstandard extension used : bit field types other than in
#pragma warning (disable : 4214)

// Ignore warning C4324: 'xxx' : structure was padded due to __declspec(align())
#pragma warning (disable : 4324)


//
// Defines from Synaptics RMI4 Data Sheet, please refer to
// the spec for details about the fields and values.
//

#define RMI4_FIRST_FUNCTION_ADDRESS       0xE9
#define RMI4_PAGE_SELECT_ADDRESS          0xFF

#define RMI4_F34_FLASH_MEMORY_MANAGEMENT  0x34
#define RMI4_F01_RMI_DEVICE_CONTROL       0x01
#define RMI4_F12_2D_TOUCHPAD_SENSOR		  0x12
#define RMI4_F54_TEST_REPORTING           0x54
#define RMI4_F1A_0D_CAP_BUTTON_SENSOR     0x1A
#define RMI4_F55_SENSOR_TUNING            0x55

#define RMI4_MAX_FUNCTIONS                10

typedef struct _RMI4_FUNCTION_DESCRIPTOR
{
	BYTE QueryBase;
	BYTE CommandBase;
	BYTE ControlBase;
	BYTE DataBase;
	union
	{
		BYTE All;
		struct
		{
			BYTE IrqCount : 3;
			BYTE Reserved0 : 2;
			BYTE FuncVer : 2;
			BYTE Reserved1 : 1;
		};
	} VersionIrq;
	BYTE Number;
} RMI4_FUNCTION_DESCRIPTOR;

//
// Function $01 - RMI Device Control
//

typedef struct _RMI4_F01_QUERY_REGISTERS
{
	BYTE ManufacturerID;
	union
	{
		BYTE All;
		struct
		{
			BYTE CustomMap : 1;
			BYTE NonCompliant : 1;
			BYTE Reserved0 : 1;
			BYTE HasSensorID : 1;
			BYTE Reserved1 : 1;
			BYTE HasAdjDoze : 1;
			BYTE HasAdJDozeHold : 1;
			BYTE Reserved2 : 1;
		};
	} ProductProperties;
	BYTE ProductInfo0;
	BYTE ProductInfo1;
	BYTE Date0;
	BYTE Date1;
	BYTE WaferLotId0Lo;
	BYTE WaferLotId0Hi;
	BYTE WaferLotId1Lo;
	BYTE WaferLotId1Hi;
	BYTE WaferLotId2Lo;
	BYTE ProductID1;
	BYTE ProductID2;
	BYTE ProductID3;
	BYTE ProductID4;
	BYTE ProductID5;
	BYTE ProductID6;
	BYTE ProductID7;
	BYTE ProductID8;
	BYTE ProductID9;
	BYTE ProductID10;
	BYTE Reserved21;
	BYTE SendorID;
	BYTE Reserved23;
	BYTE Reserved24;
	BYTE Reserved25;
	BYTE Reserved26;
	BYTE Reserved27;
	BYTE Reserved28;
	BYTE Reserved29;
	BYTE Reserved30;
	BYTE Reserved31;
} RMI4_F01_QUERY_REGISTERS;

//
// Logical structure for getting registry config settings
//
typedef struct _RM4_F01_CTRL_REGISTERS_LOGICAL
{
	UINT32 SleepMode;
	UINT32 NoSleep;
	UINT32 ReportRate;
	UINT32 Configured;
	UINT32 InterruptEnable;
	UINT32 DozeInterval;
	UINT32 DozeThreshold;
	UINT32 DozeHoldoff;
} RMI4_F01_CTRL_REGISTERS_LOGICAL;

#define RMI4_MILLISECONDS_TO_TENTH_MILLISECONDS(n) n/10
#define RMI4_SECONDS_TO_HALF_SECONDS(n) 2*n

//
// Function $11 - 2-D Touch Sensor
//

//
// Logical structure for getting registry config settings
//
typedef struct _RMI4_F11_CTRL_REGISTERS_LOGICAL
{
	UINT32 ReportingMode;
	UINT32 AbsPosFilt;
	UINT32 RelPosFilt;
	UINT32 RelBallistics;
	UINT32 Dribble;
	UINT32 PalmDetectThreshold;
	UINT32 MotionSensitivity;
	UINT32 ManTrackEn;
	UINT32 ManTrackedFinger;
	UINT32 DeltaXPosThreshold;
	UINT32 DeltaYPosThreshold;
	UINT32 Velocity;
	UINT32 Acceleration;
	UINT32 SensorMaxXPos;
	UINT32 SensorMaxYPos;
	UINT32 ZTouchThreshold;
	UINT32 ZHysteresis;
	UINT32 SmallZThreshold;
	UINT32 SmallZScaleFactor;
	UINT32 LargeZScaleFactor;
	UINT32 AlgorithmSelection;
	UINT32 WxScaleFactor;
	UINT32 WxOffset;
	UINT32 WyScaleFactor;
	UINT32 WyOffset;
	UINT32 XPitch;
	UINT32 YPitch;
	UINT32 FingerWidthX;
	UINT32 FingerWidthY;
	UINT32 ReportMeasuredSize;
	UINT32 SegmentationSensitivity;
	UINT32 XClipLo;
	UINT32 XClipHi;
	UINT32 YClipLo;
	UINT32 YClipHi;
	UINT32 MinFingerSeparation;
	UINT32 MaxFingerMovement;
} RMI4_F11_CTRL_REGISTERS_LOGICAL;

typedef struct _RMI4_DETECTED_PEN {
	BOOLEAN Pen;
	BOOLEAN Invert;
	BOOLEAN Barrel;
	UINT32 X;
	UINT32 Y;
	UINT16 Pressure;
	UCHAR Battery;
	DWORD PenId;
} RMI4_DETECTED_PEN, * PRMI4_DETECTED_PEN;

typedef struct _RMI4_DETECTED_BUTTONS
{
	BYTE ButtonStates[MAX_BUTTONS];
} RMI4_DETECTED_BUTTONS;

//
// Function $12 - 2-D Touch Sensor
//

#define RMI_REG_DESC_PRESENSE_BITS	(32 * BITS_PER_BYTE)
#define RMI_REG_DESC_SUBPACKET_BITS	(37 * BITS_PER_BYTE)

/* describes a single packet register */
typedef struct _RMI_REGISTER_DESC_ITEM {
	USHORT Register;
	ULONG RegisterSize;
	BYTE NumSubPackets;
	ULONG SubPacketMap[BITS_TO_LONGS(RMI_REG_DESC_SUBPACKET_BITS)];
} RMI_REGISTER_DESC_ITEM, * PRMI_REGISTER_DESC_ITEM;

/*
* describes the packet registers for a particular type
* (ie query, control, data)
*/
typedef struct _RMI_REGISTER_DESCRIPTOR {
	ULONG StructSize;
	ULONG PresenceMap[BITS_TO_LONGS(RMI_REG_DESC_PRESENSE_BITS)];
	UINT8 NumRegisters;
	RMI_REGISTER_DESC_ITEM* Registers;
} RMI_REGISTER_DESCRIPTOR, * PRMI_REGISTER_DESCRIPTOR;

//
// Function $XX - Add new support functions here
//


//
// Driver structures
//

typedef struct _RMI4_CONFIGURATION
{
	RMI4_F01_CTRL_REGISTERS_LOGICAL DeviceSettings;
	RMI4_F11_CTRL_REGISTERS_LOGICAL TouchSettings;
	UINT32 PepRemovesVoltageInD3;
} RMI4_CONFIGURATION;

typedef enum _RMI4_REPORTED_BUTTON
{
	BUTTON_NONE = 0,
	BUTTON_SEARCH,
	BUTTON_START,
	BUTTON_BACK,
	BUTTON_UNKNOWN
} RMI4_REPORTED_BUTTON;

typedef struct _RMI4_BUTTON_INFO
{
	RMI4_REPORTED_BUTTON ReportedButton;
	UCHAR buttonStatus;
} RMI4_BUTTON_INFO;

typedef struct _RMI4_CONTROLLER_CONTEXT
{
	WDFDEVICE FxDevice;
	WDFWAITLOCK ControllerLock;

	//
	// Controller state
	//
	int FunctionCount;
	RMI4_FUNCTION_DESCRIPTOR Descriptors[RMI4_MAX_FUNCTIONS];
	int FunctionOnPage[RMI4_MAX_FUNCTIONS];
	UCHAR FunctionInterruptMasks[RMI4_MAX_FUNCTIONS];
	int CurrentPage;

	ULONG InterruptStatus;
	BOOLEAN HasButtons;
	BOOLEAN ResetOccurred;
	BOOLEAN InvalidConfiguration;
	BOOLEAN DeviceFailure;
	BOOLEAN UnknownStatus;
	BYTE UnknownStatusMessage;
	RMI4_F01_QUERY_REGISTERS F01QueryRegisters;

	//
	// Power state
	//
	DEVICE_POWER_STATE DevicePowerState;

	//
	// Register configuration programmed to chip
	//
	TOUCH_SCREEN_SETTINGS TouchSettings;
	RMI4_CONFIGURATION Config;

	BOOLEAN ActivePenPresent;
	BOOLEAN PenPresent;

	//
	// RMI4 F12 state
	//

	BOOLEAN HasDribble;
	BOOLEAN HasRegisterDescriptors;
	RMI_REGISTER_DESCRIPTOR F12QueryRegDesc;
	RMI_REGISTER_DESCRIPTOR F12ControlRegDesc;
	RMI_REGISTER_DESCRIPTOR F12DataRegDesc;
	size_t PacketSize;

	BYTE MaxFingers;

	BOOLEAN GesturesEnabled;
} RMI4_CONTROLLER_CONTEXT;

NTSTATUS
RmiBuildFunctionsTable(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
);

int
RmiGetFunctionIndex(
	IN RMI4_FUNCTION_DESCRIPTOR* FunctionDescriptors,
	IN int FunctionCount,
	IN int FunctionDesired
);

NTSTATUS
RmiChangePage(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN int DesiredPage
);

NTSTATUS
RmiReadRegisterDescriptor(
	IN SPB_CONTEXT* Context,
	IN UCHAR Address,
	IN PRMI_REGISTER_DESCRIPTOR Rdesc
);

size_t
RmiRegisterDescriptorCalcSize(
	IN PRMI_REGISTER_DESCRIPTOR Rdesc
);

const PRMI_REGISTER_DESC_ITEM RmiGetRegisterDescItem(
	PRMI_REGISTER_DESCRIPTOR Rdesc,
	USHORT reg
);

UINT8 RmiGetRegisterIndex(
	PRMI_REGISTER_DESCRIPTOR Rdesc,
	USHORT reg
);

NTSTATUS
RmiConfigureFunctions(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
);

NTSTATUS
RmiServiceInterrupts(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PREPORT_CONTEXT ReportContext
);