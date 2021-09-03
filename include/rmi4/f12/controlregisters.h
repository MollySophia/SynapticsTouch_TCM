/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Copyright (c) LumiaWOA Authors. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name:

        controlregisters.h

    Abstract:

        Contains common types and definitions used internally
        by the F12 digitizer function.

    Environment:

        Kernel mode

    Revision History:

--*/

#pragma once

#include <wdm.h>
#include <wdf.h>

// Ignore warning C4152: nonstandard extension, function/data pointer conversion in expression
#pragma warning (disable : 4152)

// Ignore warning C4201: nonstandard extension used : nameless struct/union
#pragma warning (disable : 4201)

// Ignore warning C4201: nonstandard extension used : bit field types other than in
#pragma warning (disable : 4214)

// Ignore warning C4324: 'xxx' : structure was padded due to __declspec(align())
#pragma warning (disable : 4324)

#define F12_DATA1_BYTES_PER_OBJ			8

#define RMI_F12_REPORTING_MODE_CONTINUOUS  (0 << 0)
#define RMI_F12_REPORTING_MODE_REDUCED     (1 << 0)
#define RMI_F12_REPORTING_MODE_FS_CHANGE   (2 << 0)
#define RMI_F12_REPORTING_MODE_FP_CHANGE   (3 << 0)
#define RMI_F12_ABSOLUTE_POSITION_FILTER   (1 << 3)
#define RMI_F12_RELATIVE_POSITION_FILTER   (1 << 4)
#define RMI_F12_RELATIVE_BALLISTICS        (1 << 5)
#define RMI_F12_DRIBBLE                    (1 << 6)
#define RMI_F12_REPORTING_BEYOND_CLIP      (1 << 7)
#define RMI_F12_REPORTING_MODE_MASK         7

#define F12_2D_CTRL15   15
#define F12_2D_CTRL18   18
#define F12_2D_CTRL20   20
#define F12_2D_CTRL23   23
#define F12_2D_CTRL26   26
#define F12_2D_CTRL40   40

struct synaptics_rmi4_f12_ctrl_8 {
	union {
		struct {
			unsigned char max_x_coord_lsb;
			unsigned char max_x_coord_msb;
			unsigned char max_y_coord_lsb;
			unsigned char max_y_coord_msb;
			unsigned char rx_pitch_lsb;
			unsigned char rx_pitch_msb;
			unsigned char tx_pitch_lsb;
			unsigned char tx_pitch_msb;
			unsigned char low_rx_clip;
			unsigned char high_rx_clip;
			unsigned char low_tx_clip;
			unsigned char high_tx_clip;
			unsigned char num_of_rx;
			unsigned char num_of_tx;
		};
		unsigned char data[14];
	};
};

struct synaptics_rmi4_f12_ctrl_9 {
	union {
		struct {
			unsigned char touch_threshold;
			unsigned char lift_hysteresis;
			unsigned char small_z_scale_factor_lsb;
			unsigned char small_z_scale_factor_msb;
			unsigned char large_z_scale_factor_lsb;
			unsigned char large_z_scale_factor_msb;
			unsigned char small_large_boundary;
			unsigned char wx_scale;
			unsigned char wx_offset;
			unsigned char wy_scale;
			unsigned char wy_offset;
			unsigned char x_size_lsb;
			unsigned char x_size_msb;
			unsigned char y_size_lsb;
			unsigned char y_size_msb;
			unsigned char stability_threshold;
			unsigned char gloved_finger_threshold;
			unsigned char gloved_finger_landing_frames;
			unsigned char average_peak_ratio;
			unsigned char enable_thick_glove_detection;
		};
		unsigned char data[20]; // one more?
	};
};

struct synaptics_rmi4_f12_ctrl_10 {
	union {
		struct {
			unsigned char noise_floor;
			unsigned char min_peak_amp;
			unsigned char peak_merge_threshold;
			unsigned char drumming_acc_threshold;
			unsigned char min_drumming_separation;
			unsigned char noise_peak_filter;
			unsigned char reserved;
		};
		unsigned char data[7];
	};
};

struct synaptics_rmi4_f12_ctrl_15 {
	union {
		struct {
			unsigned char finger_amp_threshold;
			unsigned char small_finger_amp_threshold;
			unsigned char small_finger_border_size;
			unsigned char negative_finger_amp_threshold;
			unsigned char palm_amp_threshold;
			unsigned char palm_area;
			unsigned char negative_finger_low_ground_mass_threshold;
		};
		unsigned char data[7];
	};
};

struct synaptics_rmi4_f12_ctrl_18 {
	union {
		struct {
			unsigned char double_tap_x0_LSB;
			unsigned char double_tap_x0_MSB;
			unsigned char double_tap_y0_LSB;
			unsigned char double_tap_y0_MSB;
			unsigned char double_tap_x1_LSB;
			unsigned char double_tap_x1_MSB;
			unsigned char double_tap_y1_LSB;
			unsigned char double_tap_y1_MSB;
			unsigned char max_tap_time;
			unsigned char max_tap_distance;
			unsigned char swipe_x0_LSB;
			unsigned char swipe_x0_MSB;
			unsigned char swipe_y0_LSB;
			unsigned char swipe_y0_MSB;
			unsigned char swipe_x1_LSB;
			unsigned char swipe_x1_MSB;
			unsigned char swipe_y1_LSB;
			unsigned char swipe_y1_MSB;
			unsigned char swipe_min_disance;
			unsigned char swipe_min_speed;
			unsigned char circle_max_endpoint_gap;
			unsigned char circle_min_speed;
			unsigned char triangle_max_endpoint_gap;
			unsigned char triangle_min_speed;
			unsigned char triangle_min_angle;
			unsigned char up_vee : 1;
			unsigned char down_vee : 1;
			unsigned char left_vee : 1;
			unsigned char right_vee : 1;
			unsigned char f12_ctr18_05_b4__7 : 4;
			unsigned char vee_min_endpoint_gap;
			unsigned char vee_min_speed;
			unsigned char vee_angle_tolerance;
			unsigned char unicode_max_endpoint_gap;

			unsigned char unicode_min_speed;

			unsigned char f12_ctr18_06_orientation : 3;
			unsigned char f12_ctr18_06_b3__7 : 5;
		};
		unsigned char data[32]; // too big by 2
	};
};

#pragma pack(push)
#pragma pack(1)
typedef enum _RMI4_F12_REPORTING_FLAGS
{
	RMI4_F12_REPORTING_CONTINUOUS_MODE     = 0,
	RMI4_F12_REPORTING_REDUCED_MODE        = 1,
	RMI4_F12_REPORTING_WAKEUP_GESTURE_MODE = 2,
} RMI4_F12_REPORTING_FLAGS;
#pragma pack(pop)

typedef enum _RMI4_F12_OBJECT_TYPE {
	RMI4_F12_OBJECT_NONE = (BYTE)0x00,
	RMI4_F12_OBJECT_FINGER = (BYTE)0x01,
	RMI4_F12_OBJECT_ACTIVE_STYLUS = (BYTE)0x02,
	RMI4_F12_OBJECT_PALM = (BYTE)0x03,
	RMI4_F12_OBJECT_UNCLASSIFIED_OBJECT = (BYTE)0x04,
	RMI4_F12_OBJECT_HOVERING_FINGER = (BYTE)0x05,
	RMI4_F12_OBJECT_GLOVED_FINGER = (BYTE)0x06,
	RMI4_F12_OBJECT_NARROW_OBJECT = (BYTE)0x07,
	RMI4_F12_OBJECT_HAND_EDGE = (BYTE)0x08,

	RMI4_F12_OBJECT_COVER = (BYTE)0x0A,
	RMI4_F12_OBJECT_STYLUS = (BYTE)0x0B,
	RMI4_F12_OBJECT_ERASER = (BYTE)0x0C,
	RMI4_F12_OBJECT_SMALL_OBJECT = (BYTE)0x0D,
} RMI4_F12_OBJECT_TYPE;

typedef struct _RMI4_F12_QUERY_5_REGISTER {
	union {
		BYTE Data[7];
		struct {
			BYTE Size;
#pragma pack(push)
#pragma pack(1)
			struct {
				BYTE ctrl0_is_present : 1;
				BYTE ctrl1_is_present : 1;
				BYTE ctrl2_is_present : 1;
				BYTE ctrl3_is_present : 1;
				BYTE ctrl4_is_present : 1;
				BYTE ctrl5_is_present : 1;
				BYTE ctrl6_is_present : 1;
				BYTE ctrl7_is_present : 1;
			};
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
			struct {
				BYTE ctrl8_is_present : 1;
				BYTE ctrl9_is_present : 1;
				BYTE ctrl10_is_present : 1;
				BYTE ctrl11_is_present : 1;
				BYTE ctrl12_is_present : 1;
				BYTE ctrl13_is_present : 1;
				BYTE ctrl14_is_present : 1;
				BYTE ctrl15_is_present : 1;
			};
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
			struct {
				BYTE ctrl16_is_present : 1;
				BYTE ctrl17_is_present : 1;
				BYTE ctrl18_is_present : 1;
				BYTE ctrl19_is_present : 1;
				BYTE ctrl20_is_present : 1;
				BYTE ctrl21_is_present : 1;
				BYTE ctrl22_is_present : 1;
				BYTE ctrl23_is_present : 1;
			};
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
			struct {
				BYTE ctrl24_is_present : 1;
				BYTE ctrl25_is_present : 1;
				BYTE ctrl26_is_present : 1;
				BYTE ctrl27_is_present : 1;
				BYTE ctrl28_is_present : 1;
				BYTE ctrl29_is_present : 1;
				BYTE ctrl30_is_present : 1;
				BYTE ctrl31_is_present : 1;
			};
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
			struct {
				BYTE ctrl32_is_present : 1;
				BYTE ctrl33_is_present : 1;
				BYTE ctrl34_is_present : 1;
				BYTE ctrl35_is_present : 1;
				BYTE ctrl36_is_present : 1;
				BYTE ctrl37_is_present : 1;
				BYTE ctrl38_is_present : 1;
				BYTE ctrl39_is_present : 1;
			};
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
			struct {
				BYTE ctrl40_is_present : 1;
				BYTE ctrl41_is_present : 1;
				BYTE ctrl42_is_present : 1;
				BYTE ctrl43_is_present : 1;
				BYTE ctrl44_is_present : 1;
				BYTE ctrl45_is_present : 1;
				BYTE ctrl46_is_present : 1;
				BYTE ctrl47_is_present : 1;
			};
#pragma pack(pop)
		};
	};
} RMI4_F12_QUERY_5_REGISTER, * PRMI4_F12_QUERY_5_REGISTER;

typedef struct _RMI4_F12_QUERY_8_REGISTER {
	union {
		BYTE Data[4];
		struct {
			BYTE Size;
#pragma pack(push)
#pragma pack(1)
			struct {
				BYTE data0_is_present : 1;
				BYTE data1_is_present : 1;
				BYTE data2_is_present : 1;
				BYTE data3_is_present : 1;
				BYTE data4_is_present : 1;
				BYTE data5_is_present : 1;
				BYTE data6_is_present : 1;
				BYTE data7_is_present : 1;
			};
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
			struct {
				BYTE data8_is_present : 1;
				BYTE data9_is_present : 1;
				BYTE data10_is_present : 1;
				BYTE data11_is_present : 1;
				BYTE data12_is_present : 1;
				BYTE data13_is_present : 1;
				BYTE data14_is_present : 1;
				BYTE data15_is_present : 1;
			};
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
			struct {
				BYTE data16_is_present : 1;
				BYTE data17_is_present : 1;
				BYTE data18_is_present : 1;
				BYTE data19_is_present : 1;
				BYTE data20_is_present : 1;
				BYTE data21_is_present : 1;
				BYTE data22_is_present : 1;
				BYTE data23_is_present : 1;
			};
#pragma pack(pop)
		};
	};
} RMI4_F12_QUERY_8_REGISTER, * PRMI4_F12_QUERY_8_REGISTER;

typedef struct _RMI4_F12_FINGER_REPORT_REGISTER
{
	union {
		BYTE Data[3];
		struct
		{
			BYTE SuppressXCoordinate;
			BYTE SuppressYCoordinate;
			BYTE ReportingFlags;
		};
	};
} RMI4_F12_FINGER_REPORT_REGISTER, * PRMI4_F12_FINGER_REPORT_REGISTER;

typedef struct _RMI4_F12_OBJECT_REPORT_ENABLE_REGISTER
{
	union {
		BYTE Data[5];
		struct
		{
			struct
			{
				BYTE FingerReportingEnabled : 1;
				BYTE ActiveStylusReportingEnabled : 1;
				BYTE PalmReportingEnabled : 1;
				BYTE UnclassifiedObjectReportingEnabled : 1;
				BYTE HoveringFingerReportingEnabled : 1;
				BYTE GlovedFingerReportingEnabled : 1;
				BYTE NarrowObjectReportingEnabled : 1;
				BYTE HandEdgeReportingEnabled : 1;
			};

			BYTE ReportedObjectCount;

			struct
			{
				BYTE Padding1 : 1;
				BYTE ReportActiveStylusAsFinger : 1;
				BYTE ReportPalmAsFinger : 1;
				BYTE ReportUnclassifiedObjectAsFinger : 1;
				BYTE ReportHoveringFingerAsFinger : 1;
				BYTE ReportGlovedFingerAsFinger : 1;
				BYTE ReportNarrowObjectSwipeAsFinger : 1;
				BYTE ReportHandEdgeAsFinger : 1;
			};

			struct
			{
				BYTE CoverReportingEnabled : 1;
				BYTE StylusReportingEnabled : 1;
				BYTE EraserReportingEnabled : 1;
				BYTE SmallObjectReportingEnabled : 1;
				BYTE Padding2 : 4;
			};

			struct
			{
				BYTE ReportCoverAsFinger : 1;
				BYTE ReportStylusAsFinger : 1;
				BYTE ReportEraserAsFinger : 1;
				BYTE ReportSmallObjectAsFinger : 1;
				BYTE Padding3 : 4;
			};
		};
	};
} RMI4_F12_OBJECT_REPORT_ENABLE_REGISTER, * PRMI4_F12_OBJECT_REPORT_ENABLE_REGISTER;