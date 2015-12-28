/*
 * Rockchip RK3288 VPU codec driver
 *
 * Copyright (C) 2014 Rockchip Electronics Co., Ltd.
 *	Alpha Lin <Alpha.Lin@rock-chips.com>
 *	Jung Zhao <jung.zhao@rock-chips.com>
 *
 * Copyright (C) 2014 Google, Inc.
 *	Tomasz Figa <tfiga@chromium.org>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "rk3288_vpu_common.h"

#include <linux/types.h>
#include <linux/sort.h>

#include "rk3288_vpu_regs.h"
#include "rk3288_vpu_hw.h"

#define mask_2b         ((u32)0x00000003)
#define mask_3b         ((u32)0x00000007)
#define mask_4b         ((u32)0x0000000F)
#define mask_5b         ((u32)0x0000001F)
#define mask_6b         ((u32)0x0000003F)
#define mask_11b        ((u32)0x000007FF)
#define mask_14b        ((u32)0x00003FFF)
#define mask_16b        ((u32)0x0000FFFF)

#define H264_BYTE_STREAM           0x00
#define H264_NAL_UNIT              0x01

/* AXI bus read and write ID values used by HW. 0 - 255 */
#ifndef ENC8270_AXI_READ_ID
#define ENC8270_AXI_READ_ID                                0
#endif

#ifndef ENC8270_AXI_WRITE_ID
#define ENC8270_AXI_WRITE_ID                               0
#endif

/* End of "ASIC bus interface configuration values"                           */

/* ASIC internal clock gating control. 0 - disabled, 1 - enabled              */
#ifndef ENC8270_ASIC_CLOCK_GATING_ENABLED
#define ENC8270_ASIC_CLOCK_GATING_ENABLED                  0
#endif

#ifndef ENC8270_OUTPUT_SWAP_32
#define ENC8270_OUTPUT_SWAP_32                      1
#endif

/* The output data's 16-bit swap: 0 or 1
 * This defines the 16-bit endianess of the ASIC output data.
 */
#ifndef ENC8270_OUTPUT_SWAP_16
#define ENC8270_OUTPUT_SWAP_16                      1
#endif

/* The output data's 8-bit swap: 0 or 1
 * This defines the byte endianess of the ASIC output data.
 */
#ifndef ENC8270_OUTPUT_SWAP_8
#define ENC8270_OUTPUT_SWAP_8                       1
#endif

#ifndef ENC8270_BURST_LENGTH
#define ENC8270_BURST_LENGTH                               16
#endif

#ifndef ENC8270_BURST_INCR_TYPE_ENABLED
#define ENC8270_BURST_INCR_TYPE_ENABLED                    0
#endif

#ifndef ENC8270_BURST_DATA_DISCARD_ENABLED
#define ENC8270_BURST_DATA_DISCARD_ENABLED                 0
#endif

#ifndef ENC8270_INPUT_SWAP_32_YUV
#define ENC8270_INPUT_SWAP_32_YUV                   1
#endif

/* The input image's 16-bit swap: 0 or 1
 * This defines the 16-bit endianess of the ASIC input YUV
 */
#ifndef ENC8270_INPUT_SWAP_16_YUV
#define ENC8270_INPUT_SWAP_16_YUV                   1
#endif

/* The input image's 8-bit swap: 0 or 1
 * This defines the byte endianess of the ASIC input YUV
 */
#ifndef ENC8270_INPUT_SWAP_8_YUV
#define ENC8270_INPUT_SWAP_8_YUV                    1
#endif

/* H.264 motion estimation parameters */
static const u32 h264PrevModeFavor[52] = {
	7, 7, 8, 8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 16, 17, 18,
	19, 20, 21, 22, 24, 25, 27, 29, 30, 32, 34, 36, 38, 41, 43, 46,
	49, 51, 55, 58, 61, 65, 69, 73, 78, 82, 87, 93, 98, 104, 110,
	117, 124, 132, 140
};

static const u32 h264DiffMvPenalty[52] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 6, 6, 7, 8, 9, 10,
	11, 13, 14, 16, 18, 20, 23, 25, 29, 32, 36, 40, 45, 51, 57, 64,
	72, 81, 91
};

/* sqrt(2^((qp-12)/3))*8 */
static const u32 h264DiffMvPenalty_rk30[52] = {
	2, 2, 3, 3, 3, 4, 4, 4, 5, 6,
	6, 7, 8, 9, 10, 11, 13, 14, 16, 18,
	20, 23, 26, 29, 32, 36, 40, 45, 51, 57,
	64, 72, 81, 91, 102, 114, 128, 144, 161, 181,
	203, 228, 256, 287, 323, 362, 406, 456, 512, 575,
	645, 724
};

/* 31*sqrt(2^((qp-12)/3))/4 */
static const u32 h264DiffMvPenalty4p_rk30[52] = {
	2, 2, 2, 3, 3, 3, 4, 4, 5, 5,
	6, 7, 8, 9, 10, 11, 12, 14, 16, 17,
	20, 22, 25, 28, 31, 35, 39, 44, 49, 55,
	62, 70, 78, 88, 98, 110, 124, 139, 156, 175,
	197, 221, 248, 278, 312, 351, 394, 442, 496, 557,
	625, 701
};

static const u32 h264Intra16Favor[52] = {
	24, 24, 24, 26, 27, 30, 32, 35, 39, 43, 48, 53, 58, 64, 71, 78,
	85, 93, 102, 111, 121, 131, 142, 154, 167, 180, 195, 211, 229,
	248, 271, 296, 326, 361, 404, 457, 523, 607, 714, 852, 1034,
	1272, 1588, 2008, 2568, 3318, 4323, 5672, 7486, 9928, 13216,
	17648
};

static const u32 h264InterFavor[52] = {
	40, 40, 41, 42, 43, 44, 45, 48, 51, 53, 55, 60, 62, 67, 69, 72,
	78, 84, 90, 96, 110, 120, 135, 152, 170, 189, 210, 235, 265,
	297, 335, 376, 420, 470, 522, 572, 620, 670, 724, 770, 820,
	867, 915, 970, 1020, 1076, 1132, 1180, 1230, 1275, 1320, 1370
};

static const u32 h264SkipSadPenalty[52] = {
	255, 255, 255, 255, 255, 255, 255, 255, 255, 224,
	208, 192, 176, 160, 144, 128, 112, 96, 80, 64,
	56, 48, 44, 40, 36, 32, 28, 24, 22, 20,
	18, 16, 12, 11, 10, 9, 8, 7, 5, 5,
	4, 4, 3, 3, 2, 2, 1, 1, 1, 1,
	0, 0
};

/* Penalty factor in 1/256 units for skip mode, 2550/(qp-1)-50 */
static u32 h264SkipSadPenalty_rk30[52] = {
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 233, 205, 182, 163,
	146, 132, 120, 109, 100,  92,  84,  78,  71,  66,  61,  56,  52,  48,
	44,  41,  38,  35,  32,  30,  27,  25,  23,  21,  19,  17,  15,  14,
	12,  11,   9,   8,   7,   5,   4,   3,   2,   1
};

static const int h264ContextInitIntra[460][2] = {
	/* 0 -> 10 */
	{20, -15}, {2, 54}, {3, 74}, {20, -15},
	{2, 54}, {3, 74}, { -28, 127}, { -23, 104},
	{ -6, 53}, { -1, 54}, {7, 51},

	/* 11 -> 23 unsused for I */
	{0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0},

	/* 24 -> 39 */
	{0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0},

	/* 40 -> 53 */
	{0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0},

	/* 54 -> 59 */
	{0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0},

	/* 60 -> 69 */
	{0, 41}, {0, 63}, {0, 63}, {0, 63},
	{ -9, 83}, {4, 86}, {0, 97}, { -7, 72},
	{13, 41}, {3, 62},

	/* 70 -> 87 */
	{0, 11}, {1, 55}, {0, 69}, { -17, 127},
	{ -13, 102}, {0, 82}, { -7, 74}, { -21, 107},
	{ -27, 127}, { -31, 127}, { -24, 127}, { -18, 95},
	{ -27, 127}, { -21, 114}, { -30, 127}, { -17, 123},
	{ -12, 115}, { -16, 122},

	/* 88 -> 104 */
	{ -11, 115}, { -12, 63}, { -2, 68}, { -15, 84},
	{ -13, 104}, { -3, 70}, { -8, 93}, { -10, 90},
	{ -30, 127}, { -1, 74}, { -6, 97}, { -7, 91},
	{ -20, 127}, { -4, 56}, { -5, 82}, { -7, 76},
	{ -22, 125},

	/* 105 -> 135 */
	{ -7, 93}, { -11, 87}, { -3, 77}, { -5, 71},
	{ -4, 63}, { -4, 68}, { -12, 84}, { -7, 62},
	{ -7, 65}, {8, 61}, {5, 56}, { -2, 66},
	{1, 64}, {0, 61}, { -2, 78}, {1, 50},
	{7, 52}, {10, 35}, {0, 44}, {11, 38},
	{1, 45}, {0, 46}, {5, 44}, {31, 17},
	{1, 51}, {7, 50}, {28, 19}, {16, 33},
	{14, 62}, { -13, 108}, { -15, 100},

	/* 136 -> 165 */
	{ -13, 101}, { -13, 91}, { -12, 94}, { -10, 88},
	{ -16, 84}, { -10, 86}, { -7, 83}, { -13, 87},
	{ -19, 94}, {1, 70}, {0, 72}, { -5, 74},
	{18, 59}, { -8, 102}, { -15, 100}, {0, 95},
	{ -4, 75}, {2, 72}, { -11, 75}, { -3, 71},
	{15, 46}, { -13, 69}, {0, 62}, {0, 65},
	{21, 37}, { -15, 72}, {9, 57}, {16, 54},
	{0, 62}, {12, 72},

	/* 166 -> 196 */
	{24, 0}, {15, 9}, {8, 25}, {13, 18},
	{15, 9}, {13, 19}, {10, 37}, {12, 18},
	{6, 29}, {20, 33}, {15, 30}, {4, 45},
	{1, 58}, {0, 62}, {7, 61}, {12, 38},
	{11, 45}, {15, 39}, {11, 42}, {13, 44},
	{16, 45}, {12, 41}, {10, 49}, {30, 34},
	{18, 42}, {10, 55}, {17, 51}, {17, 46},
	{0, 89}, {26, -19}, {22, -17},

	/* 197 -> 226 */
	{26, -17}, {30, -25}, {28, -20}, {33, -23},
	{37, -27}, {33, -23}, {40, -28}, {38, -17},
	{33, -11}, {40, -15}, {41, -6}, {38, 1},
	{41, 17}, {30, -6}, {27, 3}, {26, 22},
	{37, -16}, {35, -4}, {38, -8}, {38, -3},
	{37, 3}, {38, 5}, {42, 0}, {35, 16},
	{39, 22}, {14, 48}, {27, 37}, {21, 60},
	{12, 68}, {2, 97},

	/* 227 -> 251 */
	{ -3, 71}, { -6, 42}, { -5, 50}, { -3, 54},
	{ -2, 62}, {0, 58}, {1, 63}, { -2, 72},
	{ -1, 74}, { -9, 91}, { -5, 67}, { -5, 27},
	{ -3, 39}, { -2, 44}, {0, 46}, { -16, 64},
	{ -8, 68}, { -10, 78}, { -6, 77}, { -10, 86},
	{ -12, 92}, { -15, 55}, { -10, 60}, { -6, 62},
	{ -4, 65},

	/* 252 -> 275 */
	{ -12, 73}, { -8, 76}, { -7, 80}, { -9, 88},
	{ -17, 110}, { -11, 97}, { -20, 84}, { -11, 79},
	{ -6, 73}, { -4, 74}, { -13, 86}, { -13, 96},
	{ -11, 97}, { -19, 117}, { -8, 78}, { -5, 33},
	{ -4, 48}, { -2, 53}, { -3, 62}, { -13, 71},
	{ -10, 79}, { -12, 86}, { -13, 90}, { -14, 97},

	/* 276 special case, bypass used */
	{0, 0},

	/* 277 -> 307 */
	{ -6, 93}, { -6, 84}, { -8, 79}, {0, 66},
	{ -1, 71}, {0, 62}, { -2, 60}, { -2, 59},
	{ -5, 75}, { -3, 62}, { -4, 58}, { -9, 66},
	{ -1, 79}, {0, 71}, {3, 68}, {10, 44},
	{ -7, 62}, {15, 36}, {14, 40}, {16, 27},
	{12, 29}, {1, 44}, {20, 36}, {18, 32},
	{5, 42}, {1, 48}, {10, 62}, {17, 46},
	{9, 64}, { -12, 104}, { -11, 97},

	/* 308 -> 337 */
	{ -16, 96}, { -7, 88}, { -8, 85}, { -7, 85},
	{ -9, 85}, { -13, 88}, {4, 66}, { -3, 77},
	{ -3, 76}, { -6, 76}, {10, 58}, { -1, 76},
	{ -1, 83}, { -7, 99}, { -14, 95}, {2, 95},
	{0, 76}, { -5, 74}, {0, 70}, { -11, 75},
	{1, 68}, {0, 65}, { -14, 73}, {3, 62},
	{4, 62}, { -1, 68}, { -13, 75}, {11, 55},
	{5, 64}, {12, 70},

	/* 338 -> 368 */
	{15, 6}, {6, 19}, {7, 16}, {12, 14},
	{18, 13}, {13, 11}, {13, 15}, {15, 16},
	{12, 23}, {13, 23}, {15, 20}, {14, 26},
	{14, 44}, {17, 40}, {17, 47}, {24, 17},
	{21, 21}, {25, 22}, {31, 27}, {22, 29},
	{19, 35}, {14, 50}, {10, 57}, {7, 63},
	{ -2, 77}, { -4, 82}, { -3, 94}, {9, 69},
	{ -12, 109}, {36, -35}, {36, -34},

	/* 369 -> 398 */
	{32, -26}, {37, -30}, {44, -32}, {34, -18},
	{34, -15}, {40, -15}, {33, -7}, {35, -5},
	{33, 0}, {38, 2}, {33, 13}, {23, 35},
	{13, 58}, {29, -3}, {26, 0}, {22, 30},
	{31, -7}, {35, -15}, {34, -3}, {34, 3},
	{36, -1}, {34, 5}, {32, 11}, {35, 5},
	{34, 12}, {39, 11}, {30, 29}, {34, 26},
	{29, 39}, {19, 66},

	/* 399 -> 435 */
	{31, 21}, {31, 31}, {25, 50},
	{ -17, 120}, { -20, 112}, { -18, 114}, { -11, 85},
	{ -15, 92}, { -14, 89}, { -26, 71}, { -15, 81},
	{ -14, 80}, {0, 68}, { -14, 70}, { -24, 56},
	{ -23, 68}, { -24, 50}, { -11, 74}, {23, -13},
	{26, -13}, {40, -15}, {49, -14}, {44, 3},
	{45, 6}, {44, 34}, {33, 54}, {19, 82},
	{ -3, 75}, { -1, 23}, {1, 34}, {1, 43},
	{0, 54}, { -2, 55}, {0, 61}, {1, 64},
	{0, 68}, { -9, 92},

	/* 436 -> 459 */
	{ -14, 106}, { -13, 97}, { -15, 90}, { -12, 90},
	{ -18, 88}, { -10, 73}, { -9, 79}, { -14, 86},
	{ -10, 73}, { -10, 70}, { -10, 69}, { -5, 66},
	{ -9, 64}, { -5, 58}, {2, 59}, {21, -10},
	{24, -11}, {28, -8}, {28, -1}, {29, 3},
	{29, 9}, {35, 20}, {29, 36}, {14, 67}
};


static const s32 h264ContextInit[3][460][2] = {
	/* cabac_init_idc == 0 */
	{
		/* 0 -> 10 */
		{20, -15}, {2, 54}, {3, 74}, {20, -15},
		{2, 54}, {3, 74}, { -28, 127}, { -23, 104},
		{ -6, 53}, { -1, 54}, {7, 51},

		/* 11 -> 23 */
		{23, 33}, {23, 2}, {21, 0}, {1, 9},
		{0, 49}, { -37, 118}, {5, 57}, { -13, 78},
		{ -11, 65}, {1, 62}, {12, 49}, { -4, 73},
		{17, 50},

		/* 24 -> 39 */
		{18, 64}, {9, 43}, {29, 0}, {26, 67},
		{16, 90}, {9, 104}, { -46, 127}, { -20, 104},
		{1, 67}, { -13, 78}, { -11, 65}, {1, 62},
		{ -6, 86}, { -17, 95}, { -6, 61}, {9, 45},

		/* 40 -> 53 */
		{ -3, 69}, { -6, 81}, { -11, 96}, {6, 55},
		{7, 67}, { -5, 86}, {2, 88}, {0, 58},
		{ -3, 76}, { -10, 94}, {5, 54}, {4, 69},
		{ -3, 81}, {0, 88},

		/* 54 -> 59 */
		{ -7, 67}, { -5, 74}, { -4, 74}, { -5, 80},
		{ -7, 72}, {1, 58},

		/* 60 -> 69 */
		{0, 41}, {0, 63}, {0, 63}, {0, 63},
		{ -9, 83}, {4, 86}, {0, 97}, { -7, 72},
		{13, 41}, {3, 62},

		/* 70 -> 87 */
		{0, 45}, { -4, 78}, { -3, 96}, { -27, 126},
		{ -28, 98}, { -25, 101}, { -23, 67}, { -28, 82},
		{ -20, 94}, { -16, 83}, { -22, 110}, { -21, 91},
		{ -18, 102}, { -13, 93}, { -29, 127}, { -7, 92},
		{ -5, 89}, { -7, 96}, { -13, 108}, { -3, 46},
		{ -1, 65}, { -1, 57}, { -9, 93}, { -3, 74},
		{ -9, 92}, { -8, 87}, { -23, 126}, {5, 54},
		{6, 60}, {6, 59}, {6, 69}, { -1, 48},
		{0, 68}, { -4, 69}, { -8, 88},

		/* 105 -> 165 */
		{ -2, 85}, { -6, 78}, { -1, 75}, { -7, 77},
		{2, 54}, {5, 50}, { -3, 68}, {1, 50},
		{6, 42}, { -4, 81}, {1, 63}, { -4, 70},
		{0, 67}, {2, 57}, { -2, 76}, {11, 35},
		{4, 64}, {1, 61}, {11, 35}, {18, 25},
		{12, 24}, {13, 29}, {13, 36}, { -10, 93},
		{ -7, 73}, { -2, 73}, {13, 46}, {9, 49},
		{ -7, 100}, {9, 53}, {2, 53}, {5, 53},
		{ -2, 61}, {0, 56}, {0, 56}, { -13, 63},
		{ -5, 60}, { -1, 62}, {4, 57}, { -6, 69},
		{4, 57}, {14, 39}, {4, 51}, {13, 68},
		{3, 64}, {1, 61}, {9, 63}, {7, 50},
		{16, 39}, {5, 44}, {4, 52}, {11, 48},
		{ -5, 60}, { -1, 59}, {0, 59}, {22, 33},
		{5, 44}, {14, 43}, { -1, 78}, {0, 60},
		{9, 69},

		/* 166 -> 226 */
		{11, 28}, {2, 40}, {3, 44}, {0, 49},
		{0, 46}, {2, 44}, {2, 51}, {0, 47},
		{4, 39}, {2, 62}, {6, 46}, {0, 54},
		{3, 54}, {2, 58}, {4, 63}, {6, 51},
		{6, 57}, {7, 53}, {6, 52}, {6, 55},
		{11, 45}, {14, 36}, {8, 53}, { -1, 82},
		{7, 55}, { -3, 78}, {15, 46}, {22, 31},
		{ -1, 84}, {25, 7}, {30, -7}, {28, 3},
		{28, 4}, {32, 0}, {34, -1}, {30, 6},
		{30, 6}, {32, 9}, {31, 19}, {26, 27},
		{26, 30}, {37, 20}, {28, 34}, {17, 70},
		{1, 67}, {5, 59}, {9, 67}, {16, 30},
		{18, 32}, {18, 35}, {22, 29}, {24, 31},
		{23, 38}, {18, 43}, {20, 41}, {11, 63},
		{9, 59}, {9, 64}, { -1, 94}, { -2, 89},
		{ -9, 108},

		/* 227 -> 275 */
		{ -6, 76}, { -2, 44}, {0, 45}, {0, 52},
		{ -3, 64}, { -2, 59}, { -4, 70}, { -4, 75},
		{ -8, 82}, { -17, 102}, { -9, 77}, {3, 24},
		{0, 42}, {0, 48}, {0, 55}, { -6, 59},
		{ -7, 71}, { -12, 83}, { -11, 87}, { -30, 119},
		{1, 58}, { -3, 29}, { -1, 36}, {1, 38},
		{2, 43}, { -6, 55}, {0, 58}, {0, 64},
		{ -3, 74}, { -10, 90}, {0, 70}, { -4, 29},
		{5, 31}, {7, 42}, {1, 59}, { -2, 58},
		{ -3, 72}, { -3, 81}, { -11, 97}, {0, 58},
		{8, 5}, {10, 14}, {14, 18}, {13, 27},
		{2, 40}, {0, 58}, { -3, 70}, { -6, 79},
		{ -8, 85},

		/* 276 special case, bypass used */
		{0, 0},

		/* 277 -> 337 */
		{ -13, 106}, { -16, 106}, { -10, 87}, { -21, 114},
		{ -18, 110}, { -14, 98}, { -22, 110}, { -21, 106},
		{ -18, 103}, { -21, 107}, { -23, 108}, { -26, 112},
		{ -10, 96}, { -12, 95}, { -5, 91}, { -9, 93},
		{ -22, 94}, { -5, 86}, {9, 67}, { -4, 80},
		{ -10, 85}, { -1, 70}, {7, 60}, {9, 58},
		{5, 61}, {12, 50}, {15, 50}, {18, 49},
		{17, 54}, {10, 41}, {7, 46}, { -1, 51},
		{7, 49}, {8, 52}, {9, 41}, {6, 47},
		{2, 55}, {13, 41}, {10, 44}, {6, 50},
		{5, 53}, {13, 49}, {4, 63}, {6, 64},
		{ -2, 69}, { -2, 59}, {6, 70}, {10, 44},
		{9, 31}, {12, 43}, {3, 53}, {14, 34},
		{10, 38}, { -3, 52}, {13, 40}, {17, 32},
		{7, 44}, {7, 38}, {13, 50}, {10, 57},
		{26, 43},

		/* 338 -> 398 */
		{14, 11}, {11, 14}, {9, 11}, {18, 11},
		{21, 9}, {23, -2}, {32, -15}, {32, -15},
		{34, -21}, {39, -23}, {42, -33}, {41, -31},
		{46, -28}, {38, -12}, {21, 29}, {45, -24},
		{53, -45}, {48, -26}, {65, -43}, {43, -19},
		{39, -10}, {30, 9}, {18, 26}, {20, 27},
		{0, 57}, { -14, 82}, { -5, 75}, { -19, 97},
		{ -35, 125}, {27, 0}, {28, 0}, {31, -4},
		{27, 6}, {34, 8}, {30, 10}, {24, 22},
		{33, 19}, {22, 32}, {26, 31}, {21, 41},
		{26, 44}, {23, 47}, {16, 65}, {14, 71},
		{8, 60}, {6, 63}, {17, 65}, {21, 24},
		{23, 20}, {26, 23}, {27, 32}, {28, 23},
		{28, 24}, {23, 40}, {24, 32}, {28, 29},
		{23, 42}, {19, 57}, {22, 53}, {22, 61},
		{11, 86},

		/* 399 -> 435 */
		{12, 40}, {11, 51}, {14, 59},
		{ -4, 79}, { -7, 71}, { -5, 69}, { -9, 70},
		{ -8, 66}, { -10, 68}, { -19, 73}, { -12, 69},
		{ -16, 70}, { -15, 67}, { -20, 62}, { -19, 70},
		{ -16, 66}, { -22, 65}, { -20, 63}, {9, -2},
		{26, -9}, {33, -9}, {39, -7}, {41, -2},
		{45, 3}, {49, 9}, {45, 27}, {36, 59},
		{ -6, 66}, { -7, 35}, { -7, 42}, { -8, 45},
		{ -5, 48}, { -12, 56}, { -6, 60}, { -5, 62},
		{ -8, 66}, { -8, 76},

		/* 436 -> 459 */
		{ -5, 85}, { -6, 81}, { -10, 77}, { -7, 81},
		{ -17, 80}, { -18, 73}, { -4, 74}, { -10, 83},
		{ -9, 71}, { -9, 67}, { -1, 61}, { -8, 66},
		{ -14, 66}, {0, 59}, {2, 59}, {21, -13},
		{33, -14}, {39, -7}, {46, -2}, {51, 2},
		{60, 6}, {61, 17}, {55, 34}, {42, 62},
	},

	/* cabac_init_idc == 1 */
	{
		/* 0 -> 10 */
		{20, -15}, {2, 54}, {3, 74}, {20, -15},
		{2, 54}, {3, 74}, { -28, 127}, { -23, 104},
		{ -6, 53}, { -1, 54}, {7, 51},

		/* 11 -> 23 */
		{22, 25}, {34, 0}, {16, 0}, { -2, 9},
		{4, 41}, { -29, 118}, {2, 65}, { -6, 71},
		{ -13, 79}, {5, 52}, {9, 50}, { -3, 70},
		{10, 54},

		/* 24 -> 39 */
		{26, 34}, {19, 22}, {40, 0}, {57, 2},
		{41, 36}, {26, 69}, { -45, 127}, { -15, 101},
		{ -4, 76}, { -6, 71}, { -13, 79}, {5, 52},
		{6, 69}, { -13, 90}, {0, 52}, {8, 43},

		/* 40 -> 53 */
		{ -2, 69}, { -5, 82}, { -10, 96}, {2, 59},
		{2, 75}, { -3, 87}, { -3, 100}, {1, 56},
		{ -3, 74}, { -6, 85}, {0, 59}, { -3, 81},
		{ -7, 86}, { -5, 95},

		/* 54 -> 59 */
		{ -1, 66}, { -1, 77}, {1, 70}, { -2, 86},
		{ -5, 72}, {0, 61},

		/* 60 -> 69 */
		{0, 41}, {0, 63}, {0, 63}, {0, 63},
		{ -9, 83}, {4, 86}, {0, 97}, { -7, 72},
		{13, 41}, {3, 62},

		/* 70 -> 104 */
		{13, 15}, {7, 51}, {2, 80}, { -39, 127},
		{ -18, 91}, { -17, 96}, { -26, 81}, { -35, 98},
		{ -24, 102}, { -23, 97}, { -27, 119}, { -24, 99},
		{ -21, 110}, { -18, 102}, { -36, 127}, {0, 80},
		{ -5, 89}, { -7, 94}, { -4, 92}, {0, 39},
		{0, 65}, { -15, 84}, { -35, 127}, { -2, 73},
		{ -12, 104}, { -9, 91}, { -31, 127}, {3, 55},
		{7, 56}, {7, 55}, {8, 61}, { -3, 53},
		{0, 68}, { -7, 74}, { -9, 88},

		/* 105 -> 165 */
		{ -13, 103}, { -13, 91}, { -9, 89}, { -14, 92},
		{ -8, 76}, { -12, 87}, { -23, 110}, { -24, 105},
		{ -10, 78}, { -20, 112}, { -17, 99}, { -78, 127},
		{ -70, 127}, { -50, 127}, { -46, 127}, { -4, 66},
		{ -5, 78}, { -4, 71}, { -8, 72}, {2, 59},
		{ -1, 55}, { -7, 70}, { -6, 75}, { -8, 89},
		{ -34, 119}, { -3, 75}, {32, 20}, {30, 22},
		{ -44, 127}, {0, 54}, { -5, 61}, {0, 58},
		{ -1, 60}, { -3, 61}, { -8, 67}, { -25, 84},
		{ -14, 74}, { -5, 65}, {5, 52}, {2, 57},
		{0, 61}, { -9, 69}, { -11, 70}, {18, 55},
		{ -4, 71}, {0, 58}, {7, 61}, {9, 41},
		{18, 25}, {9, 32}, {5, 43}, {9, 47},
		{0, 44}, {0, 51}, {2, 46}, {19, 38},
		{ -4, 66}, {15, 38}, {12, 42}, {9, 34},
		{0, 89},

		/* 166 -> 226 */
		{4, 45}, {10, 28}, {10, 31}, {33, -11},
		{52, -43}, {18, 15}, {28, 0}, {35, -22},
		{38, -25}, {34, 0}, {39, -18}, {32, -12},
		{102, -94}, {0, 0}, {56, -15}, {33, -4},
		{29, 10}, {37, -5}, {51, -29}, {39, -9},
		{52, -34}, {69, -58}, {67, -63}, {44, -5},
		{32, 7}, {55, -29}, {32, 1}, {0, 0},
		{27, 36}, {33, -25}, {34, -30}, {36, -28},
		{38, -28}, {38, -27}, {34, -18}, {35, -16},
		{34, -14}, {32, -8}, {37, -6}, {35, 0},
		{30, 10}, {28, 18}, {26, 25}, {29, 41},
		{0, 75}, {2, 72}, {8, 77}, {14, 35},
		{18, 31}, {17, 35}, {21, 30}, {17, 45},
		{20, 42}, {18, 45}, {27, 26}, {16, 54},
		{7, 66}, {16, 56}, {11, 73}, {10, 67},
		{ -10, 116},

		/* 227 -> 275 */
		{ -23, 112}, { -15, 71}, { -7, 61}, {0, 53},
		{ -5, 66}, { -11, 77}, { -9, 80}, { -9, 84},
		{ -10, 87}, { -34, 127}, { -21, 101}, { -3, 39},
		{ -5, 53}, { -7, 61}, { -11, 75}, { -15, 77},
		{ -17, 91}, { -25, 107}, { -25, 111}, { -28, 122},
		{ -11, 76}, { -10, 44}, { -10, 52}, { -10, 57},
		{ -9, 58}, { -16, 72}, { -7, 69}, { -4, 69},
		{ -5, 74}, { -9, 86}, {2, 66}, { -9, 34},
		{1, 32}, {11, 31}, {5, 52}, { -2, 55},
		{ -2, 67}, {0, 73}, { -8, 89}, {3, 52},
		{7, 4}, {10, 8}, {17, 8}, {16, 19},
		{3, 37}, { -1, 61}, { -5, 73}, { -1, 70},
		{ -4, 78},

		/* 276 special case, bypass used */
		{0, 0},

		/* 277 -> 337 */
		{ -21, 126}, { -23, 124}, { -20, 110}, { -26, 126},
		{ -25, 124}, { -17, 105}, { -27, 121}, { -27, 117},
		{ -17, 102}, { -26, 117}, { -27, 116}, { -33, 122},
		{ -10, 95}, { -14, 100}, { -8, 95}, { -17, 111},
		{ -28, 114}, { -6, 89}, { -2, 80}, { -4, 82},
		{ -9, 85}, { -8, 81}, { -1, 72}, {5, 64},
		{1, 67}, {9, 56}, {0, 69}, {1, 69},
		{7, 69}, { -7, 69}, { -6, 67}, { -16, 77},
		{ -2, 64}, {2, 61}, { -6, 67}, { -3, 64},
		{2, 57}, { -3, 65}, { -3, 66}, {0, 62},
		{9, 51}, { -1, 66}, { -2, 71}, { -2, 75},
		{ -1, 70}, { -9, 72}, {14, 60}, {16, 37},
		{0, 47}, {18, 35}, {11, 37}, {12, 41},
		{10, 41}, {2, 48}, {12, 41}, {13, 41},
		{0, 59}, {3, 50}, {19, 40}, {3, 66},
		{18, 50},

		/* 338 -> 398 */
		{19, -6}, {18, -6}, {14, 0}, {26, -12},
		{31, -16}, {33, -25}, {33, -22}, {37, -28},
		{39, -30}, {42, -30}, {47, -42}, {45, -36},
		{49, -34}, {41, -17}, {32, 9}, {69, -71},
		{63, -63}, {66, -64}, {77, -74}, {54, -39},
		{52, -35}, {41, -10}, {36, 0}, {40, -1},
		{30, 14}, {28, 26}, {23, 37}, {12, 55},
		{11, 65}, {37, -33}, {39, -36}, {40, -37},
		{38, -30}, {46, -33}, {42, -30}, {40, -24},
		{49, -29}, {38, -12}, {40, -10}, {38, -3},
		{46, -5}, {31, 20}, {29, 30}, {25, 44},
		{12, 48}, {11, 49}, {26, 45}, {22, 22},
		{23, 22}, {27, 21}, {33, 20}, {26, 28},
		{30, 24}, {27, 34}, {18, 42}, {25, 39},
		{18, 50}, {12, 70}, {21, 54}, {14, 71},
		{11, 83},

		/* 399 -> 435 */
		{25, 32}, {21, 49}, {21, 54},
		{ -5, 85}, { -6, 81}, { -10, 77}, { -7, 81},
		{ -17, 80}, { -18, 73}, { -4, 74}, { -10, 83},
		{ -9, 71}, { -9, 67}, { -1, 61}, { -8, 66},
		{ -14, 66}, {0, 59}, {2, 59}, {17, -10},
		{32, -13}, {42, -9}, {49, -5}, {53, 0},
		{64, 3}, {68, 10}, {66, 27}, {47, 57},
		{ -5, 71}, {0, 24}, { -1, 36}, { -2, 42},
		{ -2, 52}, { -9, 57}, { -6, 63}, { -4, 65},
		{ -4, 67}, { -7, 82},

		/* 436 -> 459 */
		{ -3, 81}, { -3, 76}, { -7, 72}, { -6, 78},
		{ -12, 72}, { -14, 68}, { -3, 70}, { -6, 76},
		{ -5, 66}, { -5, 62}, {0, 57}, { -4, 61},
		{ -9, 60}, {1, 54}, {2, 58}, {17, -10},
		{32, -13}, {42, -9}, {49, -5}, {53, 0},
		{64, 3}, {68, 10}, {66, 27}, {47, 57},
	},

	/* cabac_init_idc == 2 */
	{
		/* 0 -> 10 */
		{20, -15}, {2, 54}, {3, 74}, {20, -15},
		{2, 54}, {3, 74}, { -28, 127}, { -23, 104},
		{ -6, 53}, { -1, 54}, {7, 51},

		/* 11 -> 23 */
		{29, 16}, {25, 0}, {14, 0}, { -10, 51},
		{ -3, 62}, { -27, 99}, {26, 16}, { -4, 85},
		{ -24, 102}, {5, 57}, {6, 57}, { -17, 73},
		{14, 57},

		/* 24 -> 39 */
		{20, 40}, {20, 10}, {29, 0}, {54, 0},
		{37, 42}, {12, 97}, { -32, 127}, { -22, 117},
		{ -2, 74}, { -4, 85}, { -24, 102}, {5, 57},
		{ -6, 93}, { -14, 88}, { -6, 44}, {4, 55},

		/* 40 -> 53 */
		{ -11, 89}, { -15, 103}, { -21, 116}, {19, 57},
		{20, 58}, {4, 84}, {6, 96}, {1, 63},
		{ -5, 85}, { -13, 106}, {5, 63}, {6, 75},
		{ -3, 90}, { -1, 101},

		/* 54 -> 59 */
		{3, 55}, { -4, 79}, { -2, 75}, { -12, 97},
		{ -7, 50}, {1, 60},

		/* 60 -> 69 */
		{0, 41}, {0, 63}, {0, 63}, {0, 63},
		{ -9, 83}, {4, 86}, {0, 97}, { -7, 72},
		{13, 41}, {3, 62},

		/* 70 -> 104 */
		{7, 34}, { -9, 88}, { -20, 127}, { -36, 127},
		{ -17, 91}, { -14, 95}, { -25, 84}, { -25, 86},
		{ -12, 89}, { -17, 91}, { -31, 127}, { -14, 76},
		{ -18, 103}, { -13, 90}, { -37, 127}, {11, 80},
		{5, 76}, {2, 84}, {5, 78}, { -6, 55},
		{4, 61}, { -14, 83}, { -37, 127}, { -5, 79},
		{ -11, 104}, { -11, 91}, { -30, 127}, {0, 65},
		{ -2, 79}, {0, 72}, { -4, 92}, { -6, 56},
		{3, 68}, { -8, 71}, { -13, 98},

		/* 105 -> 165 */
		{ -4, 86}, { -12, 88}, { -5, 82}, { -3, 72},
		{ -4, 67}, { -8, 72}, { -16, 89}, { -9, 69},
		{ -1, 59}, {5, 66}, {4, 57}, { -4, 71},
		{ -2, 71}, {2, 58}, { -1, 74}, { -4, 44},
		{ -1, 69}, {0, 62}, { -7, 51}, { -4, 47},
		{ -6, 42}, { -3, 41}, { -6, 53}, {8, 76},
		{ -9, 78}, { -11, 83}, {9, 52}, {0, 67},
		{ -5, 90}, {1, 67}, { -15, 72}, { -5, 75},
		{ -8, 80}, { -21, 83}, { -21, 64}, { -13, 31},
		{ -25, 64}, { -29, 94}, {9, 75}, {17, 63},
		{ -8, 74}, { -5, 35}, { -2, 27}, {13, 91},
		{3, 65}, { -7, 69}, {8, 77}, { -10, 66},
		{3, 62}, { -3, 68}, { -20, 81}, {0, 30},
		{1, 7}, { -3, 23}, { -21, 74}, {16, 66},
		{ -23, 124}, {17, 37}, {44, -18}, {50, -34},
		{ -22, 127},

		/* 166 -> 226 */
		{4, 39}, {0, 42}, {7, 34}, {11, 29},
		{8, 31}, {6, 37}, {7, 42}, {3, 40},
		{8, 33}, {13, 43}, {13, 36}, {4, 47},
		{3, 55}, {2, 58}, {6, 60}, {8, 44},
		{11, 44}, {14, 42}, {7, 48}, {4, 56},
		{4, 52}, {13, 37}, {9, 49}, {19, 58},
		{10, 48}, {12, 45}, {0, 69}, {20, 33},
		{8, 63}, {35, -18}, {33, -25}, {28, -3},
		{24, 10}, {27, 0}, {34, -14}, {52, -44},
		{39, -24}, {19, 17}, {31, 25}, {36, 29},
		{24, 33}, {34, 15}, {30, 20}, {22, 73},
		{20, 34}, {19, 31}, {27, 44}, {19, 16},
		{15, 36}, {15, 36}, {21, 28}, {25, 21},
		{30, 20}, {31, 12}, {27, 16}, {24, 42},
		{0, 93}, {14, 56}, {15, 57}, {26, 38},
		{ -24, 127},

		/* 227 -> 275 */
		{ -24, 115}, { -22, 82}, { -9, 62}, {0, 53},
		{0, 59}, { -14, 85}, { -13, 89}, { -13, 94},
		{ -11, 92}, { -29, 127}, { -21, 100}, { -14, 57},
		{ -12, 67}, { -11, 71}, { -10, 77}, { -21, 85},
		{ -16, 88}, { -23, 104}, { -15, 98}, { -37, 127},
		{ -10, 82}, { -8, 48}, { -8, 61}, { -8, 66},
		{ -7, 70}, { -14, 75}, { -10, 79}, { -9, 83},
		{ -12, 92}, { -18, 108}, { -4, 79}, { -22, 69},
		{ -16, 75}, { -2, 58}, {1, 58}, { -13, 78},
		{ -9, 83}, { -4, 81}, { -13, 99}, { -13, 81},
		{ -6, 38}, { -13, 62}, { -6, 58}, { -2, 59},
		{ -16, 73}, { -10, 76}, { -13, 86}, { -9, 83},
		{ -10, 87},

		/* 276 special case, bypass used */
		{0, 0},

		/* 277 -> 337 */
		{ -22, 127}, { -25, 127}, { -25, 120}, { -27, 127},
		{ -19, 114}, { -23, 117}, { -25, 118}, { -26, 117},
		{ -24, 113}, { -28, 118}, { -31, 120}, { -37, 124},
		{ -10, 94}, { -15, 102}, { -10, 99}, { -13, 106},
		{ -50, 127}, { -5, 92}, {17, 57}, { -5, 86},
		{ -13, 94}, { -12, 91}, { -2, 77}, {0, 71},
		{ -1, 73}, {4, 64}, { -7, 81}, {5, 64},
		{15, 57}, {1, 67}, {0, 68}, { -10, 67},
		{1, 68}, {0, 77}, {2, 64}, {0, 68},
		{ -5, 78}, {7, 55}, {5, 59}, {2, 65},
		{14, 54}, {15, 44}, {5, 60}, {2, 70},
		{ -2, 76}, { -18, 86}, {12, 70}, {5, 64},
		{ -12, 70}, {11, 55}, {5, 56}, {0, 69},
		{2, 65}, { -6, 74}, {5, 54}, {7, 54},
		{ -6, 76}, { -11, 82}, { -2, 77}, { -2, 77},
		{25, 42},

		/* 338 -> 398 */
		{17, -13}, {16, -9}, {17, -12}, {27, -21},
		{37, -30}, {41, -40}, {42, -41}, {48, -47},
		{39, -32}, {46, -40}, {52, -51}, {46, -41},
		{52, -39}, {43, -19}, {32, 11}, {61, -55},
		{56, -46}, {62, -50}, {81, -67}, {45, -20},
		{35, -2}, {28, 15}, {34, 1}, {39, 1},
		{30, 17}, {20, 38}, {18, 45}, {15, 54},
		{0, 79}, {36, -16}, {37, -14}, {37, -17},
		{32, 1}, {34, 15}, {29, 15}, {24, 25},
		{34, 22}, {31, 16}, {35, 18}, {31, 28},
		{33, 41}, {36, 28}, {27, 47}, {21, 62},
		{18, 31}, {19, 26}, {36, 24}, {24, 23},
		{27, 16}, {24, 30}, {31, 29}, {22, 41},
		{22, 42}, {16, 60}, {15, 52}, {14, 60},
		{3, 78}, { -16, 123}, {21, 53}, {22, 56},
		{25, 61},

		/* 399 -> 435 */
		{21, 33}, {19, 50}, {17, 61},
		{ -3, 78}, { -8, 74}, { -9, 72}, { -10, 72},
		{ -18, 75}, { -12, 71}, { -11, 63}, { -5, 70},
		{ -17, 75}, { -14, 72}, { -16, 67}, { -8, 53},
		{ -14, 59}, { -9, 52}, { -11, 68}, {9, -2},
		{30, -10}, {31, -4}, {33, -1}, {33, 7},
		{31, 12}, {37, 23}, {31, 38}, {20, 64},
		{ -9, 71}, { -7, 37}, { -8, 44}, { -11, 49},
		{ -10, 56}, { -12, 59}, { -8, 63}, { -9, 67},
		{ -6, 68}, { -10, 79},

		/* 436 -> 459 */
		{ -3, 78}, { -8, 74}, { -9, 72}, { -10, 72},
		{ -18, 75}, { -12, 71}, { -11, 63}, { -5, 70},
		{ -17, 75}, { -14, 72}, { -16, 67}, { -8, 53},
		{ -14, 59}, { -9, 52}, { -11, 68}, {9, -2},
		{30, -10}, {31, -4}, {33, -1}, {33, 7},
		{31, 12}, {37, 23}, {31, 38}, {20, 64},
	}
};

#define CLIP3(v, min, max)  ((v) < (min) ? (min) : ((v) > (max) ? (max) : (v)))

static void rk3288_vpu_h264e_init_cabac_table(u32 *cabac_tbl,
					      u32 cabac_init_idc)
{
	const s32(*ctx)[460][2];
	int i, j, qp;
	u8 *table = (u8 *) cabac_tbl;

	for (qp = 0; qp < 52; qp++) { /* All QP values */
		for (j = 0; j < 2; j++) { /* Intra/Inter */
			if (j == 0)
				/*lint -e(545) */
				ctx = &h264ContextInitIntra;
			else
				/*lint -e(545) */
				ctx = &h264ContextInit[cabac_init_idc];

			for (i = 0; i < 460; i++) {
				s32 m = (s32) (*ctx)[i][0];
				s32 n = (s32) (*ctx)[i][1];

				s32 preCtxState = CLIP3(1, 126,
						       ((m * (s32) qp) >> 4) +
						       n);

				if (preCtxState <= 63) {
					table[qp * 464 * 2 + j * 464 + i] =
						(u8) ((63 - preCtxState) << 1);
				} else {
					table[qp * 464 * 2 + j * 464 + i] =
						(u8) (((preCtxState - 64) << 1)
						     | 1);
				}
			}
		}
	}
	/* SwapEndianess(contextTable, 52 * 2 * 464); */
}


static void rk3288_vpu_h264e_fill_params(struct rk3288_vpu_dev *vpu,
				struct rk3288_vpu_ctx *ctx)
{
	const struct rk3288_vpu_h264e_params *params = ctx->run.h264e.params;
	struct r3288_h264e_reg_params *reg_params =
		(struct r3288_h264e_reg_params *) ctx->hw.h264e.regs.cpu;
	s32 i, mbPerFrame;

	reg_params->asicCfgReg =
		((ENC8270_AXI_READ_ID & (255)) << 24) |
		((ENC8270_AXI_WRITE_ID & (255)) << 16) |
		((ENC8270_OUTPUT_SWAP_16 & (1)) << 15) |
		((ENC8270_BURST_LENGTH & (63)) << 8) |
		((ENC8270_BURST_INCR_TYPE_ENABLED & (1)) << 6) |
		((ENC8270_BURST_DATA_DISCARD_ENABLED & (1)) << 5) |
		((ENC8270_ASIC_CLOCK_GATING_ENABLED & (1)) << 4) |
		((ENC8270_OUTPUT_SWAP_32 & (1)) << 3) |
		((ENC8270_OUTPUT_SWAP_8 & (1)) << 1);

	reg_params->intra16Favor = h264Intra16Favor[params->Qp];
	reg_params->interFavor = h264InterFavor[params->Qp];

	reg_params->mbsInRow = (ctx->dst_fmt.width + 15) / 16;
	reg_params->mbsInCol = (ctx->dst_fmt.height + 15) / 16;

	reg_params->frameCodingType = params->frameCodingType;
	reg_params->frameNum = params->frameNum;

	reg_params->picInitQp = params->picInitQp;
	reg_params->sliceAlphaOffset = params->sliceAlphaOffset;
	reg_params->sliceBetaOffset = params->sliceBetaOffset;
	reg_params->chromaQpIndexOffset = params->chromaQpIndexOffset;
	reg_params->filterDisable = params->filterDisable;
	reg_params->idrPicId = params->idrPicId;
	reg_params->ppsId = params->ppsId;
	reg_params->frameNum = params->frameNum;
	reg_params->sliceSizeMbRows = params->sliceSizeMbRows;
	reg_params->h264Inter4x4Disabled = params->h264Inter4x4Disabled;
	reg_params->enableCabac = params->enableCabac;
	reg_params->transform8x8Mode = params->transform8x8Mode;
	reg_params->cabacInitIdc = params->cabacInitIdc;

	reg_params->qp = params->Qp;
	reg_params->madQpDelta = params->madQpDelta;
	reg_params->madThreshold = params->madThreshold;
	reg_params->qpMin = params->qpMin;
	reg_params->qpMax = params->qpMax;
	reg_params->cpDistanceMbs = params->cpDistanceMbs;

	for (i = 0; i < 10; i++) {
		reg_params->cpTarget[i] = params->cpTarget[i];
		if (i < 5)
			reg_params->targetError[i] = params->targetError[i];
		if (i < 7)
			reg_params->deltaQp[i] = params->deltaQp[i];
	}

	mbPerFrame = reg_params->mbsInRow * reg_params->mbsInCol;

	if (mbPerFrame > 3600)
		reg_params->disableQuarterPixelMv = 1;
	else
		reg_params->disableQuarterPixelMv = 0;

	reg_params->riceEnable = 0;
	reg_params->sizeTblPresent = 0;

	reg_params->inputChromaBaseOffset = 0;
	reg_params->inputLumaBaseOffset = 0;

	reg_params->pixelsOnRow = (ctx->src_fmt.width + 15) & (~15);

	if (ctx->dst_fmt.width & 0x0f)
		reg_params->xFill = (16 - (ctx->dst_fmt.width & 0x0f)) / 4;
	else
		reg_params->xFill = 0;

	if (ctx->dst_fmt.height & 0x0f)
		reg_params->yFill = 16 - (ctx->dst_fmt.height & 0x0f);
	else
		reg_params->yFill = 0;

	reg_params->inputImageFormat = 0;
	reg_params->inputImageRotation = 0;
	reg_params->constrainedIntraPrediction = 0;

	reg_params->h264StrmMode = 0; /* 0 is byte stream; 1 is nal unit */

	reg_params->firstFreeBit = 32;
	reg_params->strmStartLSB = 0;
	reg_params->strmStartMSB = 0 << 24 | 0 << 16 | 0 << 8 | 1;
	reg_params->outputStrmSize = vb2_plane_size(&ctx->run.dst->b, 0);

	reg_params->vsMode = 0;
	reg_params->vsNextLumaBase = 0;

	rk3288_vpu_h264e_init_cabac_table(ctx->hw.h264e.cabac_tbl.cpu,
					 reg_params->cabacInitIdc);
	reg_params->cabacCtxBase = ctx->hw.h264e.cabac_tbl.dma;

	reg_params->riceWriteBase = 0;

	reg_params->bMaskMsb = 0;
	reg_params->rMaskMsb = 0;
	reg_params->gMaskMsb = 0;

	reg_params->colorConversionCoeffA = 19589;
	reg_params->colorConversionCoeffB = 38443;
	reg_params->colorConversionCoeffC = 7504;
	reg_params->colorConversionCoeffE = 37008;
	reg_params->colorConversionCoeffF = 46740;
}

static inline unsigned int ref_luma_size(unsigned int w, unsigned int h)
{
	return round_up(w, MB_DIM) * round_up(h, MB_DIM);
}

int rk3288_vpu_h264e_init(struct rk3288_vpu_ctx *ctx)
{
	struct rk3288_vpu_dev *vpu = ctx->dev;
	size_t ref_buf_size;
	size_t height = ctx->src_fmt.height;
	size_t width = ctx->src_fmt.width;
	int ret;

	ret = rk3288_vpu_aux_buf_alloc(vpu, &ctx->hw.h264e.regs,
				       sizeof(struct r3288_h264e_reg_params));
	if (ret) {
		vpu_err("allocate h264e regs failed\n");
		goto err_regs_alloc;
	}

	ret = rk3288_vpu_aux_buf_alloc(vpu, &ctx->hw.h264e.cabac_tbl,
				      52 * 2 * 464);
	if (ret) {
		vpu_err("allocate h264e cabac_tbl failed\n");
		goto err_cabac_tbl_alloc;
	}

	ref_buf_size = ref_luma_size(width, height) * 3 / 2;
	ret = rk3288_vpu_aux_buf_alloc(vpu, &ctx->hw.h264e.ext_buf,
				       2 * ref_buf_size);
	if (ret) {
		vpu_err("allocate ext_buf failed\n");
		goto err_ext_buf_alloc;
	}

	return ret;

err_ext_buf_alloc:
	rk3288_vpu_aux_buf_free(vpu, &ctx->hw.h264e.cabac_tbl);
err_cabac_tbl_alloc:
	rk3288_vpu_aux_buf_free(vpu, &ctx->hw.h264e.regs);
err_regs_alloc:
	return ret;
}

void rk3288_vpu_h264e_exit(struct rk3288_vpu_ctx *ctx)
{
	struct rk3288_vpu_dev *vpu = ctx->dev;

	rk3288_vpu_aux_buf_free(vpu, &ctx->hw.vp8e.ext_buf);
	rk3288_vpu_aux_buf_free(vpu, &ctx->hw.h264e.cabac_tbl);
	rk3288_vpu_aux_buf_free(vpu, &ctx->hw.h264e.regs);
}

static void rk3288_vpu_h264e_set_buffers(struct rk3288_vpu_dev *vpu,
					struct rk3288_vpu_ctx *ctx)
{
	struct r3288_h264e_reg_params *reg_params =
		(struct r3288_h264e_reg_params *) ctx->hw.h264e.regs.cpu;
	dma_addr_t ref_buf_dma, rec_buf_dma;
	size_t rounded_size;
	dma_addr_t dst_dma;

	dst_dma = vb2_dma_contig_plane_dma_addr(&ctx->run.dst->b, 0);

	vepu_write_relaxed(vpu, dst_dma, VEPU_REG_ADDR_OUTPUT_STREAM);

	vepu_write_relaxed(vpu, 0, VEPU_REG_ADDR_OUTPUT_CTRL);

	rounded_size = ref_luma_size(ctx->src_fmt.width,
				    ctx->src_fmt.height);
	ref_buf_dma = rec_buf_dma = ctx->hw.vp8e.ext_buf.dma;
	if (ctx->hw.h264e.ref_rec_ptr)
		rec_buf_dma += rounded_size * 3 / 2;
	else
		ref_buf_dma += rounded_size * 3 / 2;
	ctx->hw.h264e.ref_rec_ptr ^= 1;

	vepu_write_relaxed(vpu, ref_buf_dma, VEPU_REG_ADDR_REF_LUMA);
	vepu_write_relaxed(vpu, ref_buf_dma + rounded_size,
			   VEPU_REG_ADDR_REF_CHROMA);
	vepu_write_relaxed(vpu, rec_buf_dma, VEPU_REG_ADDR_REC_LUMA);
	vepu_write_relaxed(vpu, rec_buf_dma + rounded_size,
			   VEPU_REG_ADDR_REC_CHROMA);

	vepu_write_relaxed(vpu, vb2_dma_contig_plane_dma_addr(
			   &ctx->run.src->b, PLANE_Y),
			   VEPU_REG_ADDR_IN_LUMA);
	vepu_write_relaxed(vpu, vb2_dma_contig_plane_dma_addr(
			   &ctx->run.src->b, PLANE_CB),
			   VEPU_REG_ADDR_IN_CB);
	vepu_write_relaxed(vpu, vb2_dma_contig_plane_dma_addr(
			   &ctx->run.src->b, PLANE_CR),
			   VEPU_REG_ADDR_IN_CR);

	vepu_write_relaxed(vpu, (reg_params->inputChromaBaseOffset << 29) |
			   (reg_params->inputLumaBaseOffset << 26) |
			   (reg_params->pixelsOnRow << 12) |
			   (reg_params->xFill << 10) |
			   (reg_params->yFill << 6) |
			   (reg_params->inputImageFormat << 2) |
			   (reg_params->inputImageRotation),
			   VEPU_REG_IN_IMG_CTRL);

}

static void rk3288_vpu_h264e_set_params(struct rk3288_vpu_dev *vpu,
					struct rk3288_vpu_ctx *ctx)
{
	struct r3288_h264e_reg_params *reg_params =
		(struct r3288_h264e_reg_params *) ctx->hw.h264e.regs.cpu;
	s32 scaler;

	u32 prev_mode_favor = h264PrevModeFavor[reg_params->qp];

	/* input format yuv, config regs 2 */
	vepu_write_relaxed(vpu, reg_params->asicCfgReg |
			   ((ENC8270_INPUT_SWAP_16_YUV & (1)) << 14) |
			   ((ENC8270_INPUT_SWAP_32_YUV & (1)) << 2) |
			   (ENC8270_INPUT_SWAP_8_YUV & (1)),
			   VEPU_REG_AXI_CTRL);

	/* H.264 control */
	vepu_write_relaxed(vpu, (reg_params->picInitQp << 26) |
			   ((reg_params->sliceAlphaOffset & mask_4b) << 22) |
			   ((reg_params->sliceBetaOffset & mask_4b) << 18) |
			   ((reg_params->chromaQpIndexOffset & mask_5b) << 13) |
			   (reg_params->filterDisable << 5) |
			   (reg_params->idrPicId << 1) | (0),
			   VEPU_REG_ENC_CTRL0);

	vepu_write_relaxed(vpu,	(reg_params->ppsId << 24) |
			   (prev_mode_favor << 16) | (reg_params->frameNum),
			   VEPU_REG_ENC_CTRL1);

	vepu_write_relaxed(vpu, (reg_params->sliceSizeMbRows << 23) |
			   (reg_params->disableQuarterPixelMv << 22) |
			   (reg_params->transform8x8Mode << 21) |
			   (reg_params->cabacInitIdc << 19) |
			   (reg_params->enableCabac << 18) |
			   (reg_params->h264Inter4x4Disabled << 17) |
			   (H264_BYTE_STREAM << 16) |
			   (h264Intra16Favor[reg_params->qp]),
			   VEPU_REG_ENC_CTRL2);

	/* If favor has not been set earlier by testId use default */
	scaler = max(1,
		     (s32)(200 /
		     (reg_params->mbsInRow + reg_params->mbsInCol)));
	reg_params->skipPenalty =
		min(255, (s32)h264SkipSadPenalty_rk30[reg_params->qp] * scaler);

	reg_params->splitMvMode = 1;

	reg_params->diffMvPenalty[0] = h264DiffMvPenalty4p_rk30[reg_params->qp];
	reg_params->diffMvPenalty[1] = h264DiffMvPenalty_rk30[reg_params->qp];
	reg_params->diffMvPenalty[2] = h264DiffMvPenalty_rk30[reg_params->qp];

	reg_params->splitPenalty[0] = 0;
	reg_params->splitPenalty[1] = 0;
	reg_params->splitPenalty[2] = 0;
	reg_params->splitPenalty[3] = 0;

	reg_params->zeroMvFavorDiv2 = 10;

	vepu_write_relaxed(vpu, (reg_params->splitMvMode << 30) |
			   reg_params->diffMvPenalty[1] |
			   (reg_params->diffMvPenalty[0] << 10) |
			   (reg_params->diffMvPenalty[2] << 20),
			   VEPU_REG_ENC_CTRL3);

	vepu_write_relaxed(vpu, reg_params->zeroMvFavorDiv2 << 28,
			   VEPU_REG_MVC_CTRL);

	vepu_write_relaxed(vpu, 0, VEPU_REG_ENC_CTRL5);

	vepu_write_relaxed(vpu, (reg_params->skipPenalty << 24) |
			   h264InterFavor[reg_params->qp], VEPU_REG_ENC_CTRL4);

	/* stream buffer limits */
	vepu_write_relaxed(vpu, reg_params->strmStartMSB,
			   VEPU_REG_STR_HDR_REM_MSB);
	vepu_write_relaxed(vpu, reg_params->strmStartLSB,
			   VEPU_REG_STR_HDR_REM_LSB);
	vepu_write_relaxed(vpu, reg_params->outputStrmSize,
			   VEPU_REG_STR_BUF_LIMIT);

	vepu_write_relaxed(vpu, ((reg_params->madQpDelta & 0xF) << 28) |
			   (reg_params->madThreshold << 22),
			   VEPU_REG_MAD_CTRL);

	/* video encoding rate control */
	vepu_write_relaxed(vpu, (reg_params->qp << 26) |
			   (reg_params->qpMax << 20) |
			   (reg_params->qpMin << 14) |
			   (reg_params->cpDistanceMbs),
			   VEPU_REG_QP_VAL);

	vepu_write_relaxed(vpu, (reg_params->cpTarget[0] << 16) |
			   (reg_params->cpTarget[1]),
			   VEPU_REG_CHECKPOINT(0));
	vepu_write_relaxed(vpu, (reg_params->cpTarget[2] << 16) |
			   (reg_params->cpTarget[3]),
			   VEPU_REG_CHECKPOINT(1));
	vepu_write_relaxed(vpu, (reg_params->cpTarget[4] << 16) |
			   (reg_params->cpTarget[5]),
			   VEPU_REG_CHECKPOINT(2));
	vepu_write_relaxed(vpu, (reg_params->cpTarget[6] << 16) |
			   (reg_params->cpTarget[7]),
			   VEPU_REG_CHECKPOINT(3));
	vepu_write_relaxed(vpu, (reg_params->cpTarget[8] << 16) |
			   (reg_params->cpTarget[9]),
			   VEPU_REG_CHECKPOINT(4));
	vepu_write_relaxed(vpu,
			   ((reg_params->targetError[0] & mask_16b) << 16) |
			   (reg_params->targetError[1] & mask_16b),
			   VEPU_REG_CHKPT_WORD_ERR(0));
	vepu_write_relaxed(vpu,
			   ((reg_params->targetError[2] & mask_16b) << 16) |
			   (reg_params->targetError[3] & mask_16b),
			   VEPU_REG_CHKPT_WORD_ERR(1));
	vepu_write_relaxed(vpu,
			   ((reg_params->targetError[4] & mask_16b) << 16) |
			   (reg_params->targetError[5] & mask_16b),
			   VEPU_REG_CHKPT_WORD_ERR(2));
	vepu_write_relaxed(vpu,
			   ((reg_params->deltaQp[0] & mask_4b) << 24) |
			   ((reg_params->deltaQp[1] & mask_4b) << 20) |
			   ((reg_params->deltaQp[2] & mask_4b) << 16) |
			   ((reg_params->deltaQp[3] & mask_4b) << 12) |
			   ((reg_params->deltaQp[4] & mask_4b) << 8) |
			   ((reg_params->deltaQp[5] & mask_4b) << 4) |
			   (reg_params->deltaQp[6] & mask_4b),
			   VEPU_REG_CHKPT_DELTA_QP);

	/* Stream start offset */
	vepu_write_relaxed(vpu, (reg_params->firstFreeBit << 23),
			   VEPU_REG_RLC_CTRL);
	vepu_write_relaxed(vpu, reg_params->vsNextLumaBase,
			   VEPU_REG_NEXT_PIC_BASE);
	vepu_write_relaxed(vpu, reg_params->vsMode << 30,
			   VEPU_REG_STAB_CTRL);
	vepu_write_relaxed(vpu, reg_params->cabacCtxBase,
			   VEPU_REG_ADDR_CABAC_TBL);
	vepu_write_relaxed(vpu, reg_params->riceWriteBase,
			   VEPU_REG_ADDR_MV_OUT);

	vepu_write_relaxed(vpu,
			   ((reg_params->colorConversionCoeffB &
			   mask_16b) << 16) |
			   (reg_params->colorConversionCoeffA & mask_16b),
			   VEPU_REG_RGB_YUV_COEFF(0));
	vepu_write_relaxed(vpu,
			   ((reg_params->colorConversionCoeffE &
			   mask_16b) << 16) |
			   (reg_params->colorConversionCoeffC & mask_16b),
			   VEPU_REG_RGB_YUV_COEFF(1));
	vepu_write_relaxed(vpu, ((reg_params->bMaskMsb & mask_5b) << 26) |
			   ((reg_params->gMaskMsb & mask_5b) << 21) |
			   ((reg_params->rMaskMsb & mask_5b) << 16) |
			   (reg_params->colorConversionCoeffF & mask_16b),
			   VEPU_REG_RGB_MASK_MSB);
}

void rk3288_vpu_h264e_run(struct rk3288_vpu_ctx *ctx)
{
	struct rk3288_vpu_dev *vpu = ctx->dev;
	struct r3288_h264e_reg_params *reg_params =
		(struct r3288_h264e_reg_params *) ctx->hw.h264e.regs.cpu;
	u32 reg;

	/*
	 * Program the hardware.
	 */
	rk3288_vpu_power_on(vpu);

	rk3288_vpu_h264e_fill_params(vpu, ctx);

	/* common register control */
	vepu_write_relaxed(vpu, (1 << 31) |
			   (reg_params->riceEnable << 30) |
			   (reg_params->sizeTblPresent << 29) |
			   (reg_params->mbsInRow << 19) |
			   (reg_params->mbsInCol << 10) |
			   (reg_params->frameCodingType << 3) |
			   VEPU_REG_ENC_CTRL_ENC_MODE_H264, VEPU_REG_ENC_CTRL);

	rk3288_vpu_h264e_set_params(vpu, ctx);
	rk3288_vpu_h264e_set_buffers(vpu, ctx);

	/* Make sure that all registers are written at this point. */
	wmb();

	/* Set the watchdog. */
	schedule_delayed_work(&vpu->watchdog_work, msecs_to_jiffies(2000));

	/* Start the hardware. */
	reg = VEPU_REG_AXI_CTRL_OUTPUT_SWAP16 |
		VEPU_REG_AXI_CTRL_INPUT_SWAP16 |
		VEPU_REG_AXI_CTRL_BURST_LEN(16) |
		VEPU_REG_AXI_CTRL_GATE_BIT |
		VEPU_REG_AXI_CTRL_OUTPUT_SWAP32 |
		VEPU_REG_AXI_CTRL_INPUT_SWAP32 |
		VEPU_REG_AXI_CTRL_OUTPUT_SWAP8 |
		VEPU_REG_AXI_CTRL_INPUT_SWAP8;
	vepu_write(vpu, reg, VEPU_REG_AXI_CTRL);

	vepu_write(vpu, 0, VEPU_REG_INTERRUPT);

	reg = VEPU_REG_ENC_CTRL_NAL_MODE_BIT |
		VEPU_REG_ENC_CTRL_WIDTH(MB_WIDTH(ctx->src_fmt.width)) |
		VEPU_REG_ENC_CTRL_HEIGHT(MB_HEIGHT(ctx->src_fmt.height)) |
		VEPU_REG_ENC_CTRL_ENC_MODE_VP8 |
		VEPU_REG_ENC_CTRL_EN_BIT;

	if (to_vb2_v4l2_buffer(&ctx->run.dst->b)->flags &
			V4L2_BUF_FLAG_KEYFRAME)
		reg |= VEPU_REG_ENC_CTRL_KEYFRAME_BIT;

	vepu_write(vpu, reg, VEPU_REG_ENC_CTRL);
}

#ifndef RK3288_VPU_ENC_CTRL_H264_RET_PARAMS
#define RK3288_VPU_ENC_CTRL_H264_RET_PARAMS 3
#endif

void rk3288_vpu_h264e_done(struct rk3288_vpu_ctx *ctx,
			  enum vb2_buffer_state result)
{
	struct rk3288_vpu_dev *vpu = ctx->dev;
	struct rk3288_vpu_h264e_feedback *feedback =
		ctx->ctrls[RK3288_VPU_ENC_CTRL_H264_RET_PARAMS]->p_cur.p;
	u32 i, reg = VEPU_REG_CHECKPOINT(0);
	u32 cpt_prev = 0, overflow = 0;


	feedback->qpSum = (vepu_read(vpu, VEPU_REG_MAD_CTRL) &
			   0x001fffff) * 2;
	feedback->madCount = (vepu_read(vpu, VEPU_REG_MB_CTRL) &
			      0xffff0000) >> 16;
	feedback->rlcCount = (vepu_read(vpu, VEPU_REG_RLC_CTRL) &
			      0x007ffff) * 4;

	for (i = 0; i < 10; i++) {
		u32 cpt = ((vepu_read(vpu, reg) >> (16 - 16 * (i & 1))) &
			   0xffff) * 32;
		if (cpt < cpt_prev)
			overflow += (1 << 21);
		feedback->cp[i] = cpt + overflow;
		reg += (i & 1);
	}

	ctx->run.h264e.feedback = feedback;

	rk3288_vpu_run_done(ctx, result);
}
