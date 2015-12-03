/*
 * Copyright (C) Fuzhou Rockchip Electronics Co.Ltd
 * Author:Mark Yao <mark.yao@rock-chips.com>
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

#ifndef _ROCKCHIP_DRM_VOP_H
#define _ROCKCHIP_DRM_VOP_H

/* rk3288 register definition */
#define RK3288_REG_CFG_DONE			0x0000
#define RK3288_VERSION_INFO			0x0004
#define RK3288_SYS_CTRL			0x0008
#define RK3288_SYS_CTRL1			0x000c
#define RK3288_DSP_CTRL0			0x0010
#define RK3288_DSP_CTRL1			0x0014
#define RK3288_DSP_BG				0x0018
#define RK3288_MCU_CTRL			0x001c
#define RK3288_INTR_CTRL0			0x0020
#define RK3288_INTR_CTRL1			0x0024
#define RK3288_WIN0_CTRL0			0x0030
#define RK3288_WIN0_CTRL1			0x0034
#define RK3288_WIN0_COLOR_KEY			0x0038
#define RK3288_WIN0_VIR			0x003c
#define RK3288_WIN0_YRGB_MST			0x0040
#define RK3288_WIN0_CBR_MST			0x0044
#define RK3288_WIN0_ACT_INFO			0x0048
#define RK3288_WIN0_DSP_INFO			0x004c
#define RK3288_WIN0_DSP_ST			0x0050
#define RK3288_WIN0_SCL_FACTOR_YRGB		0x0054
#define RK3288_WIN0_SCL_FACTOR_CBR		0x0058
#define RK3288_WIN0_SCL_OFFSET			0x005c
#define RK3288_WIN0_SRC_ALPHA_CTRL		0x0060
#define RK3288_WIN0_DST_ALPHA_CTRL		0x0064
#define RK3288_WIN0_FADING_CTRL		0x0068
/* win1 register */
#define RK3288_WIN1_CTRL0			0x0070
#define RK3288_WIN1_CTRL1			0x0074
#define RK3288_WIN1_COLOR_KEY			0x0078
#define RK3288_WIN1_VIR			0x007c
#define RK3288_WIN1_YRGB_MST			0x0080
#define RK3288_WIN1_CBR_MST			0x0084
#define RK3288_WIN1_ACT_INFO			0x0088
#define RK3288_WIN1_DSP_INFO			0x008c
#define RK3288_WIN1_DSP_ST			0x0090
#define RK3288_WIN1_SCL_FACTOR_YRGB		0x0094
#define RK3288_WIN1_SCL_FACTOR_CBR		0x0098
#define RK3288_WIN1_SCL_OFFSET			0x009c
#define RK3288_WIN1_SRC_ALPHA_CTRL		0x00a0
#define RK3288_WIN1_DST_ALPHA_CTRL		0x00a4
#define RK3288_WIN1_FADING_CTRL		0x00a8
/* win2 register */
#define RK3288_WIN2_CTRL0			0x00b0
#define RK3288_WIN2_CTRL1			0x00b4
#define RK3288_WIN2_VIR0_1			0x00b8
#define RK3288_WIN2_VIR2_3			0x00bc
#define RK3288_WIN2_MST0			0x00c0
#define RK3288_WIN2_DSP_INFO0			0x00c4
#define RK3288_WIN2_DSP_ST0			0x00c8
#define RK3288_WIN2_COLOR_KEY			0x00cc
#define RK3288_WIN2_MST1			0x00d0
#define RK3288_WIN2_DSP_INFO1			0x00d4
#define RK3288_WIN2_DSP_ST1			0x00d8
#define RK3288_WIN2_SRC_ALPHA_CTRL		0x00dc
#define RK3288_WIN2_MST2			0x00e0
#define RK3288_WIN2_DSP_INFO2			0x00e4
#define RK3288_WIN2_DSP_ST2			0x00e8
#define RK3288_WIN2_DST_ALPHA_CTRL		0x00ec
#define RK3288_WIN2_MST3			0x00f0
#define RK3288_WIN2_DSP_INFO3			0x00f4
#define RK3288_WIN2_DSP_ST3			0x00f8
#define RK3288_WIN2_FADING_CTRL		0x00fc
/* win3 register */
#define RK3288_WIN3_CTRL0			0x0100
#define RK3288_WIN3_CTRL1			0x0104
#define RK3288_WIN3_VIR0_1			0x0108
#define RK3288_WIN3_VIR2_3			0x010c
#define RK3288_WIN3_MST0			0x0110
#define RK3288_WIN3_DSP_INFO0			0x0114
#define RK3288_WIN3_DSP_ST0			0x0118
#define RK3288_WIN3_COLOR_KEY			0x011c
#define RK3288_WIN3_MST1			0x0120
#define RK3288_WIN3_DSP_INFO1			0x0124
#define RK3288_WIN3_DSP_ST1			0x0128
#define RK3288_WIN3_SRC_ALPHA_CTRL		0x012c
#define RK3288_WIN3_MST2			0x0130
#define RK3288_WIN3_DSP_INFO2			0x0134
#define RK3288_WIN3_DSP_ST2			0x0138
#define RK3288_WIN3_DST_ALPHA_CTRL		0x013c
#define RK3288_WIN3_MST3			0x0140
#define RK3288_WIN3_DSP_INFO3			0x0144
#define RK3288_WIN3_DSP_ST3			0x0148
#define RK3288_WIN3_FADING_CTRL		0x014c
/* hwc register */
#define RK3288_HWC_CTRL0			0x0150
#define RK3288_HWC_CTRL1			0x0154
#define RK3288_HWC_MST				0x0158
#define RK3288_HWC_DSP_ST			0x015c
#define RK3288_HWC_SRC_ALPHA_CTRL		0x0160
#define RK3288_HWC_DST_ALPHA_CTRL		0x0164
#define RK3288_HWC_FADING_CTRL			0x0168
/* post process register */
#define RK3288_POST_DSP_HACT_INFO		0x0170
#define RK3288_POST_DSP_VACT_INFO		0x0174
#define RK3288_POST_SCL_FACTOR_YRGB		0x0178
#define RK3288_POST_SCL_CTRL			0x0180
#define RK3288_POST_DSP_VACT_INFO_F1		0x0184
#define RK3288_DSP_HTOTAL_HS_END		0x0188
#define RK3288_DSP_HACT_ST_END			0x018c
#define RK3288_DSP_VTOTAL_VS_END		0x0190
#define RK3288_DSP_VACT_ST_END			0x0194
#define RK3288_DSP_VS_ST_END_F1		0x0198
#define RK3288_DSP_VACT_ST_END_F1		0x019c
/* register definition end */

#define RK3066_SYS_CTRL0		0x0
#define RK3066_SYS_CTRL1		0x4
#define RK3066_DSP_CTRL0		0x8
#define RK3066_DSP_CTRL1		0xc
#define RK3066_INT_STATUS		0x10
#define RK3066_MCU_CTRL			0x14
#define RK3066_BLEND_CTRL		0x18
#define RK3066_WIN0_COLOR_KEY		0x1c
#define RK3066_WIN1_COLOR_KEY		0x20
#define RK3066_WIN2_COLOR_KEY		0x24
#define RK3066_WIN0_YRGB_MST0		0x28
#define RK3066_WIN0_CBR_MST0		0x2c
#define RK3066_WIN0_YRGB_MST1		0x30
#define RK3066_WIN0_CBR_MST1		0x34
#define RK3066_WIN0_VIR			0x38
#define RK3066_WIN0_ACT_INFO		0x3c
#define RK3066_WIN0_DSP_INFO		0x40
#define RK3066_WIN0_DSP_ST		0x44
#define RK3066_WIN0_SCL_FACTOR_YRGB	0x48
#define RK3066_WIN0_SCL_FACTOR_CBR	0x4c
#define RK3066_WIN0_SCL_OFFSET		0x50
#define RK3066_WIN1_YRGB_MST		0x54
#define RK3066_WIN1_CBR_MST		0x58
#define RK3066_WIN1_VIR			0x5c
#define RK3066_WIN1_ACT_INFO		0x60
#define RK3066_WIN1_DSP_INFO		0x64
#define RK3066_WIN1_DSP_ST		0x68
#define RK3066_WIN1_SCL_FACTOR_YRGB	0x6c
#define RK3066_WIN1_SCL_FACTOR_CBR	0x70
#define RK3066_WIN1_SCL_OFFSET		0x74
#define RK3066_WIN2_MST			0x78
#define RK3066_WIN2_VIR			0x7c
#define RK3066_WIN2_DSP_INFO		0x80
#define RK3066_WIN2_DSP_ST		0x84
#define RK3066_HWC_MST			0x88
#define RK3066_HWC_DSP_ST		0x8c
#define RK3066_HWC_COLOR_LUT0		0x90
#define RK3066_HWC_COLOR_LUT1		0x94
#define RK3066_HWC_COLOR_LUT2		0x98
#define RK3066_DSP_HTOTAL_HS_END	0x9c
#define RK3066_DSP_HACT_ST_END		0xa0
#define RK3066_DSP_VTOTAL_VS_END	0xa4
#define RK3066_DSP_VACT_ST_END		0xa8
#define RK3066_DSP_VS_ST_END_F1		0xac
#define RK3066_DSP_VACT_ST_END_F1	0xb0
#define RK3066_REG_LOAD_EN		0xc0
#define RK3066_MCU_BYPASS_WPORT		0x100
#define RK3066_MCU_BYPASS_RPORT		0x200
#define RK3066_WIN2_LUT_ADDR		0x400
#define RK3066_DSP_LUT_ADDR		0x800

#define RK3036_SYS_CTRL		(0x00)
#define RK3036_DSP_CTRL0		(0x04)
#define RK3036_DSP_CTRL1		(0x08)
#define RK3036_INT_STATUS		(0x10)
#define RK3036_ALPHA_CTRL		(0x14)
#define RK3036_WIN0_COLOR_KEY		(0x18)
#define RK3036_WIN1_COLOR_KEY		(0x1C)
/* Layer Registers */
#define RK3036_WIN0_YRGB_MST		(0x20)
#define RK3036_WIN0_CBR_MST		(0x24)
#define RK3036_WIN1_MST		(0xa0)
#define RK3036_HWC_MST			(0x58)
#define RK3036_WIN1_VIR		(0x28)
#define RK3036_WIN0_VIR		(0x30)
#define RK3036_WIN0_ACT_INFO		(0x34)
#define RK3036_WIN1_ACT_INFO		(0xB4)
#define RK3036_WIN0_DSP_INFO		(0x38)
#define RK3036_WIN1_DSP_INFO		(0xB8)
#define RK3036_WIN0_DSP_ST		(0x3C)
#define RK3036_WIN1_DSP_ST		(0xBC)
#define RK3036_HWC_DSP_ST		(0x5C)
#define RK3036_WIN0_SCL_FACTOR_YRGB	(0x40)
#define RK3036_WIN0_SCL_FACTOR_CBR	(0x44)
#define RK3036_WIN1_SCL_FACTOR_YRGB	(0xC0)
#define RK3036_WIN0_SCL_OFFSET		(0x48)
#define RK3036_WIN1_SCL_OFFSET		(0xC8)
/* LUT Registers */
#define RK3036_WIN1_LUT_ADDR			(0x0400)
#define RK3036_HWC_LUT_ADDR			(0x0800)
/* Display Infomation Registers */
#define RK3036_DSP_HTOTAL_HS_END	(0x6C)
#define RK3036_DSP_HACT_ST_END		(0x70)
#define RK3036_DSP_VTOTAL_VS_END	(0x74)
#define RK3036_DSP_VACT_ST_END		(0x78)
#define RK3036_DSP_VS_ST_END_F1	(0x7C)
#define RK3036_DSP_VACT_ST_END_F1	(0x80)

/*BCSH Registers*/
#define RK3036_BCSH_CTRL			(0xD0)
#define RK3036_BCSH_COLOR_BAR			(0xD4)
#define RK3036_BCSH_BCS			(0xD8)
#define RK3036_BCSH_H				(0xDC)
/* Bus Register */
#define RK3036_AXI_BUS_CTRL		(0x2C)
#define RK3036_GATHER_TRANSFER		(0x84)
#define RK3036_VERSION_INFO		(0x94)
#define RK3036_REG_CFG_DONE		(0x90)


/* interrupt define */
#define DSP_HOLD_VALID_INTR		(1 << 0)
#define FS_INTR				(1 << 1)
#define LINE_FLAG_INTR			(1 << 2)
#define BUS_ERROR_INTR			(1 << 3)
#define HS_ERROR_INTR			(1 << 4)

#define INTR_MASK	(DSP_HOLD_VALID_INTR | FS_INTR | LINE_FLAG_INTR | \
				BUS_ERROR_INTR | HS_ERROR_INTR)

#define DSP_LINE_NUM(x)			(((x) & 0x1fff) << 12)
#define DSP_LINE_NUM_MASK		(0x1fff << 12)

/* src alpha ctrl define */
#define SRC_FADING_VALUE(x)		(((x) & 0xff) << 24)
#define SRC_GLOBAL_ALPHA(x)		(((x) & 0xff) << 16)
#define SRC_FACTOR_M0(x)		(((x) & 0x7) << 6)
#define SRC_ALPHA_CAL_M0(x)		(((x) & 0x1) << 5)
#define SRC_BLEND_M0(x)			(((x) & 0x3) << 3)
#define SRC_ALPHA_M0(x)			(((x) & 0x1) << 2)
#define SRC_COLOR_M0(x)			(((x) & 0x1) << 1)
#define SRC_ALPHA_EN(x)			(((x) & 0x1) << 0)
/* dst alpha ctrl define */
#define DST_FACTOR_M0(x)		(((x) & 0x7) << 6)

/*
 * display output interface supported by rockchip lcdc
 */
#define ROCKCHIP_OUT_MODE_P888	0
#define ROCKCHIP_OUT_MODE_P666	1
#define ROCKCHIP_OUT_MODE_P565	2
/* for use special outface */
#define ROCKCHIP_OUT_MODE_AAAA	15

enum alpha_mode {
	ALPHA_STRAIGHT,
	ALPHA_INVERSE,
};

enum global_blend_mode {
	ALPHA_GLOBAL,
	ALPHA_PER_PIX,
	ALPHA_PER_PIX_GLOBAL,
};

enum alpha_cal_mode {
	ALPHA_SATURATION,
	ALPHA_NO_SATURATION,
};

enum color_mode {
	ALPHA_SRC_PRE_MUL,
	ALPHA_SRC_NO_PRE_MUL,
};

enum factor_mode {
	ALPHA_ZERO,
	ALPHA_ONE,
	ALPHA_SRC,
	ALPHA_SRC_INVERSE,
	ALPHA_SRC_GLOBAL,
};

enum scale_mode {
	SCALE_NONE = 0x0,
	SCALE_UP   = 0x1,
	SCALE_DOWN = 0x2
};

enum lb_mode {
	LB_YUV_3840X5 = 0x0,
	LB_YUV_2560X8 = 0x1,
	LB_RGB_3840X2 = 0x2,
	LB_RGB_2560X4 = 0x3,
	LB_RGB_1920X5 = 0x4,
	LB_RGB_1280X8 = 0x5
};

enum sacle_up_mode {
	SCALE_UP_BIL = 0x0,
	SCALE_UP_BIC = 0x1
};

enum scale_down_mode {
	SCALE_DOWN_BIL = 0x0,
	SCALE_DOWN_AVG = 0x1
};

#define FRAC_16_16(mult, div)    (((mult) << 16) / (div))
#define SCL_FT_DEFAULT_FIXPOINT_SHIFT	12
#define SCL_MAX_VSKIPLINES		4
#define MIN_SCL_FT_AFTER_VSKIP		1

static inline uint16_t scl_cal_scale(int src, int dst, int shift)
{
	return ((src * 2 - 3) << (shift - 1)) / (dst - 1);
}

#define GET_SCL_FT_BILI_DN(src, dst)	scl_cal_scale(src, dst, 12)
#define GET_SCL_FT_BILI_UP(src, dst)	scl_cal_scale(src, dst, 16)
#define GET_SCL_FT_BIC(src, dst)	scl_cal_scale(src, dst, 16)

static inline uint16_t scl_get_bili_dn_vskip(int src_h, int dst_h,
					     int vskiplines)
{
	int act_height;

	act_height = (src_h + vskiplines - 1) / vskiplines;

	return GET_SCL_FT_BILI_DN(act_height, dst_h);
}

static inline enum scale_mode scl_get_scl_mode(int src, int dst)
{
	if (src < dst)
		return SCALE_UP;
	else if (src > dst)
		return SCALE_DOWN;

	return SCALE_NONE;
}

static inline int scl_get_vskiplines(uint32_t srch, uint32_t dsth)
{
	uint32_t vskiplines;

	for (vskiplines = SCL_MAX_VSKIPLINES; vskiplines > 1; vskiplines /= 2)
		if (srch >= vskiplines * dsth * MIN_SCL_FT_AFTER_VSKIP)
			break;

	return vskiplines;
}

static inline int scl_vop_cal_lb_mode(int width, bool is_yuv)
{
	int lb_mode;

	if (width > 2560)
		lb_mode = LB_RGB_3840X2;
	else if (width > 1920)
		lb_mode = LB_RGB_2560X4;
	else if (!is_yuv)
		lb_mode = LB_RGB_1920X5;
	else if (width > 1280)
		lb_mode = LB_YUV_3840X5;
	else
		lb_mode = LB_YUV_2560X8;

	return lb_mode;
}

#endif /* _ROCKCHIP_DRM_VOP_H */
