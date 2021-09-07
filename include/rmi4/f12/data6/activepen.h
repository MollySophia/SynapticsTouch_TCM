#pragma once

#include <wdm.h>
#include <wdf.h>
#define RESHUB_USE_HELPER_ROUTINES
#include <reshub.h>

// Ignore warning C4152: nonstandard extension, function/data pointer conversion in expression
#pragma warning (disable : 4152)

// Ignore warning C4201: nonstandard extension used : nameless struct/union
#pragma warning (disable : 4201)

// Ignore warning C4201: nonstandard extension used : bit field types other than in
#pragma warning (disable : 4214)

// Ignore warning C4324: 'xxx' : structure was padded due to __declspec(align())
#pragma warning (disable : 4324)

#pragma pack(push)
#pragma pack(1)
typedef struct _RMI4_F12_8BIT_PRESSURE_STYLUS_DATA_REGISTER {
	BYTE Pen : 1;
	BYTE Invert : 1;
	BYTE Barrel : 1;
	BYTE Reserved : 5;
	BYTE X_LSB;
	BYTE X_MSB;
	BYTE Y_LSB;
	BYTE Y_MSB;
	BYTE Pressure;
	BYTE Battery;
	BYTE PenId_0_7;
	BYTE PenId_8_15;
	BYTE PenId_16_23;
	BYTE PenId_24_31;
} RMI4_F12_8BIT_PRESSURE_STYLUS_DATA_REGISTER, * PRMI4_F12_8BIT_PRESSURE_STYLUS_DATA_REGISTER;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
typedef struct _RMI4_F12_16BIT_PRESSURE_STYLUS_DATA_REGISTER {
	BYTE Pen : 1;
	BYTE Invert : 1;
	BYTE Barrel : 1;
	BYTE Reserved : 5;
	BYTE X_LSB;
	BYTE X_MSB;
	BYTE Y_LSB;
	BYTE Y_MSB;
	BYTE Pressure_LSB;
	BYTE Pressure_MSB;
	BYTE Battery;
	BYTE PenId_0_7;
	BYTE PenId_8_15;
	BYTE PenId_16_23;
	BYTE PenId_24_31;
} RMI4_F12_16BIT_PRESSURE_STYLUS_DATA_REGISTER, * PRMI4_F12_16BIT_PRESSURE_STYLUS_DATA_REGISTER;
#pragma pack(pop)

NTSTATUS
RmiGetPenStatusFromControllerF12(
	IN VOID* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN RMI4_DETECTED_PEN* Data
);