#pragma once

#include <wdm.h>
#include <wdf.h>
#define RESHUB_USE_HELPER_ROUTINES
#include <reshub.h>
#include <report.h>

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
typedef struct _RMI4_F12_FINGER_SIMPLE_DATA_REGISTER {
	BYTE ObjectTypeAndStatus;
	BYTE X_LSB;
	BYTE X_MSB;
	BYTE Y_LSB;
	BYTE Y_MSB;
} RMI4_F12_FINGER_SIMPLE_DATA_REGISTER, * PRMI4_F12_FINGER_SIMPLE_DATA_REGISTER;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
typedef struct _RMI4_F12_FINGER_3D_DATA_REGISTER {
	BYTE ObjectTypeAndStatus;
	BYTE X_LSB;
	BYTE X_MSB;
	BYTE Y_LSB;
	BYTE Y_MSB;
	BYTE Z;
} RMI4_F12_FINGER_3D_DATA_REGISTER, * PRMI4_F12_FINGER_3D_DATA_REGISTER;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
typedef struct _RMI4_F12_FINGER_W_DATA_REGISTER {
	BYTE ObjectTypeAndStatus;
	BYTE X_LSB;
	BYTE X_MSB;
	BYTE Y_LSB;
	BYTE Y_MSB;
	BYTE wX;
	BYTE wY;
} RMI4_F12_FINGER_W_DATA_REGISTER, * PRMI4_F12_FINGER_W_DATA_REGISTER;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
typedef struct _RMI4_F12_FINGER_3D_W_DATA_REGISTER {
	BYTE ObjectTypeAndStatus;
	BYTE X_LSB;
	BYTE X_MSB;
	BYTE Y_LSB;
	BYTE Y_MSB;
	BYTE Z;
	BYTE wX;
	BYTE wY;
} RMI4_F12_FINGER_3D_W_DATA_REGISTER, * PRMI4_F12_FINGER_3D_W_DATA_REGISTER;
#pragma pack(pop)

USHORT
RmiGetObjectCountFromControllerF12(
	IN VOID* ControllerContext
);

NTSTATUS
RmiGetObjectStatusFromControllerF12(
	IN VOID* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN DETECTED_OBJECTS* Data
);