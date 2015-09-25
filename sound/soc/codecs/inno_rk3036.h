/*
 * Driver of Inno Codec for rk3036 by Rockchip Inc.
 * Author: Zheng ShunQian<zhengsq@rock-chips.com>
 */

#ifndef _INNO_RK3036_CODEC_H
#define _INNO_RK3036_CODEC_H

/* codec registers */
#define INNO_REG_00	0x00
#define INNO_REG_01	0x0c
#define INNO_REG_02	0x10
#define INNO_REG_03	0x14
#define INNO_REG_04	0x88
#define INNO_REG_05	0x8c
#define INNO_REG_06	0x90
#define INNO_REG_07	0x94
#define INNO_REG_08	0x98
#define INNO_REG_09	0x9c
#define INNO_REG_10	0xa0

/* register bit filed */
#define INNO_REG_00_CSR_RESET		(0x0 << 0) /*codec system reset*/
#define INNO_REG_00_CSR_WORK		(0x1 << 0)
#define INNO_REG_00_CDCR_RESET		(0x0 << 1) /*codec digital core reset*/
#define INNO_REG_00_CDCR_WORK		(0x1 << 1)
#define INNO_REG_00_PRB_DISABLE		(0x0 << 6) /*power reset bypass*/
#define INNO_REG_00_PRB_ENABLE		(0x1 << 6)

#define INNO_REG_01_I2SMODE_MASK	(0x1 << 4)
#define INNO_REG_01_I2SMODE_SLAVE	(0x0 << 4)
#define INNO_REG_01_I2SMODE_MASTER	(0x1 << 4)
#define INNO_REG_01_PINDIR_MASK		(0x1 << 5)
#define INNO_REG_01_PINDIR_IN_SLAVE	(0x0 << 5) /*direction of pin*/
#define INNO_REG_01_PINDIR_OUT_MASTER	(0x1 << 5)

#define INNO_REG_02_LRS_MASK		(0x3 << 2)
#define INNO_REG_02_LRS_NORMAL		(0x0 << 2) /*DAC Left Right Swap*/
#define INNO_REG_02_LRS_SWAP		(0x1 << 2)
#define INNO_REG_02_DACM_MASK		(0x3 << 3)
#define INNO_REG_02_DACM_PCM		(0x3 << 3) /*DAC Mode*/
#define INNO_REG_02_DACM_I2S		(0x2 << 3)
#define INNO_REG_02_DACM_LJM		(0x1 << 3)
#define INNO_REG_02_DACM_RJM		(0x0 << 3)
#define INNO_REG_02_VWL_MASK		(0x3 << 5)
#define INNO_REG_02_VWL_32BIT		(0x3 << 5) /*1/2Frame Valid Word Length*/
#define INNO_REG_02_VWL_24BIT		(0x2 << 5)
#define INNO_REG_02_VWL_20BIT		(0x1 << 5)
#define INNO_REG_02_VWL_16BIT		(0x0 << 5)
#define INNO_REG_02_LRCP_MASK		(0x1 << 7)
#define INNO_REG_02_LRCP_NORMAL		(0x0 << 7) /*Left Right Polarity*/
#define INNO_REG_02_LRCP_REVERSAL	(0x1 << 7)

#define INNO_REG_03_BCP_MASK		(0x1 << 0)
#define INNO_REG_03_BCP_NORMAL		(0x0 << 0) /*DAC bit clock polarity*/
#define INNO_REG_03_BCP_REVERSAL	(0x1 << 0)
#define INNO_REG_03_DACR_MASK		(0x1 << 1)
#define INNO_REG_03_DACR_RESET		(0x0 << 1) /*DAC Reset*/
#define INNO_REG_03_DACR_WORK		(0x1 << 1)
#define INNO_REG_03_FWL_MASK		(0x3 << 2)
#define INNO_REG_03_FWL_32BIT		(0x3 << 2) /*1/2Frame Word Length*/
#define INNO_REG_03_FWL_24BIT		(0x2 << 2)
#define INNO_REG_03_FWL_20BIT		(0x1 << 2)
#define INNO_REG_03_FWL_16BIT		(0x0 << 2)

#define INNO_REG_04_DACR_WORK		(0x1 << 0) /*DAC Right Module*/
#define INNO_REG_04_DACR_STOP		(0x0 << 0)
#define INNO_REG_04_DACL_WORK		(0x1 << 1) /*DAC Left Module*/
#define INNO_REG_04_DACL_STOP		(0x0 << 1)
#define INNO_REG_04_DACR_CLK_WORK	(0x1 << 2) /*DAC Right CLK*/
#define INNO_REG_04_DACR_CLK_STOP	(0x0 << 2)
#define INNO_REG_04_DACL_CLK_WORK	(0x1 << 3) /*DAC Left CLK*/
#define INNO_REG_04_DACL_CLK_STOP	(0x0 << 3)
#define INNO_REG_04_DACR_RV_WORK	(0x1 << 4) /*DAC Right Ref Voltage*/
#define INNO_REG_04_DACR_RV_STOP	(0x0 << 4)
#define INNO_REG_04_DACL_RV_WORK	(0x1 << 5) /*DAC Left Ref Voltage*/
#define INNO_REG_04_DACL_RV_STOP	(0x0 << 5)

#define INNO_REG_05_HPOUTR_EN_WORK	(0x1 << 0) /*HeadPhone Output Right*/
#define INNO_REG_05_HPOUTR_EN_STOP	(0x0 << 0)
#define INNO_REG_05_HPOUTL_EN_WORK	(0x1 << 1) /*HeadPhone Output Left*/
#define INNO_REG_05_HPOUTL_EN_STOP	(0x0 << 1)
#define INNO_REG_05_HPOUTR_WORK		(0x1 << 2) /*Init HP out right*/
#define INNO_REG_05_HPOUTR_INIT		(0x0 << 2)
#define INNO_REG_05_HPOUTL_WORK		(0x1 << 3) /*Init HP out left*/
#define INNO_REG_05_HPOUTL_INIT		(0x0 << 3)

#define INNO_REG_06_VOUTR_CZ_WORK	(0x1 << 0) /*Cross-Zero detection*/
#define INNO_REG_06_VOUTR_CZ_STOP	(0x0 << 0)
#define INNO_REG_06_VOUTL_CZ_WORK	(0x1 << 1) /*Cross-Zero detection*/
#define INNO_REG_06_VOUTL_CZ_STOP	(0x0 << 1)
#define INNO_REG_06_DACR_HL_RV_WORK	(0x1 << 2) /*High & Low Ref Voltage*/
#define INNO_REG_06_DACR_HL_RV_STOP	(0x0 << 2)
#define INNO_REG_06_DACL_HL_RV_WORK	(0x1 << 3) /*High & Low Ref Voltage*/
#define INNO_REG_06_DACL_HL_RV_STOP	(0x0 << 3)
#define INNO_REG_06_DAC_PRECHARGE	(0x0 << 4) /*PreCharge control for DAC*/
#define INNO_REG_06_DAC_DISCHARGE	(0x1 << 4)
#define INNO_REG_06_DAC_EN_WORK		(0x1 << 5) /*Enable Signal for DAC*/
#define INNO_REG_06_DAC_EN_STOP		(0x0 << 5)

/* Gain of output, 1.5db step: -39db(0x0) ~ 0db(0x1a) ~ 6db(0x1f) */
#define INNO_REG_07_HPOUTL_GAIN_0DB	0x1a
#define INNO_REG_07_HPOUTL_GAIN_N39DB	0x0
#define INNO_REG_08_HPOUTR_GAIN_0DB	0x1a
#define INNO_REG_08_HPOUTR_GAIN_N39DB	0x0


#define INNO_REG_09_HP_OUTR_POP_PRECHARGE	(0x1 << 0)
#define INNO_REG_09_HP_OUTR_POP_WORK		(0x2 << 0)
#define INNO_REG_09_HP_OUTL_POP_PRECHARGE	(0x1 << 2)
#define INNO_REG_09_HP_OUTL_POP_WORK		(0x2 << 2)
#define INNO_REG_09_HP_OUTR_MUTE_MASK	(0x1 << 4)
#define INNO_REG_09_HP_OUTR_MUTE_NO	(0x1 << 4)
#define INNO_REG_09_HP_OUTR_MUTE_YES	(0x0 << 4)
#define INNO_REG_09_HP_OUTL_MUTE_MASK	(0x1 << 5)
#define INNO_REG_09_HP_OUTL_MUTE_NO	(0x1 << 5)
#define INNO_REG_09_HP_OUTL_MUTE_YES	(0x0 << 5)
#define INNO_REG_09_DACR_INIT		(0x0 << 6)
#define INNO_REG_09_DACR_WORK		(0x1 << 6)
#define INNO_REG_09_DACL_INIT		(0x0 << 7)
#define INNO_REG_09_DACL_WORK		(0x1 << 7)

#define INNO_REG_09_POWERON (INNO_REG_09_HP_OUTR_POP_WORK | \
			      INNO_REG_09_HP_OUTL_POP_WORK | \
			      INNO_REG_09_HP_OUTR_MUTE_YES | \
			      INNO_REG_09_HP_OUTL_MUTE_YES | \
			      INNO_REG_09_DACR_WORK        | \
			      INNO_REG_09_DACL_WORK)

#define INNO_REG_10_CHARGE_SEL_CUR_400I_YES	(0x0 << 0)
#define INNO_REG_10_CHARGE_SEL_CUR_400I_NO	(0x1 << 0)
#define INNO_REG_10_CHARGE_SEL_CUR_260I_YES	(0x0 << 1)
#define INNO_REG_10_CHARGE_SEL_CUR_260I_NO	(0x1 << 1)
#define INNO_REG_10_CHARGE_SEL_CUR_130I_YES	(0x0 << 2)
#define INNO_REG_10_CHARGE_SEL_CUR_130I_NO	(0x1 << 2)
#define INNO_REG_10_CHARGE_SEL_CUR_100I_YES	(0x0 << 3)
#define INNO_REG_10_CHARGE_SEL_CUR_100I_NO	(0x1 << 3)
#define INNO_REG_10_CHARGE_SEL_CUR_050I_YES	(0x0 << 4)
#define INNO_REG_10_CHARGE_SEL_CUR_050I_NO	(0x1 << 4)
#define INNO_REG_10_CHARGE_SEL_CUR_027I_YES	(0x0 << 5)
#define INNO_REG_10_CHARGE_SEL_CUR_027I_NO	(0x1 << 5)

#define INNO_REG_10_MAX_CUR (INNO_REG_10_CHARGE_SEL_CUR_400I_YES | \
			     INNO_REG_10_CHARGE_SEL_CUR_260I_YES | \
			     INNO_REG_10_CHARGE_SEL_CUR_130I_YES | \
			     INNO_REG_10_CHARGE_SEL_CUR_100I_YES | \
			     INNO_REG_10_CHARGE_SEL_CUR_050I_YES | \
			     INNO_REG_10_CHARGE_SEL_CUR_027I_YES)



struct inno_reg_val {
	int reg;
	int val;
};

struct inno_reg_val inno_codec_open_path[] = {
	{ /* open current source */
		.reg = INNO_REG_06,
		.val = INNO_REG_06_DAC_EN_WORK     |
		       INNO_REG_06_DAC_PRECHARGE   |
		       INNO_REG_06_DACL_HL_RV_STOP |
		       INNO_REG_06_DACR_HL_RV_STOP |
		       INNO_REG_06_VOUTR_CZ_STOP   |
		       INNO_REG_06_VOUTL_CZ_STOP,
	},
	{ /* power on DAC path reference voltage */
		.reg = INNO_REG_04,
		.val = INNO_REG_04_DACR_RV_WORK    |
		       INNO_REG_04_DACL_RV_WORK    |
		       INNO_REG_04_DACL_CLK_STOP   |
		       INNO_REG_04_DACR_CLK_STOP   |
		       INNO_REG_04_DACL_STOP       |
		       INNO_REG_04_DACR_STOP,
	},
	{ /* PoP precharge work */
		.reg = INNO_REG_09,
		.val = INNO_REG_09_DACL_INIT       |
		       INNO_REG_09_DACR_INIT       |
		       INNO_REG_09_HP_OUTL_MUTE_YES |
		       INNO_REG_09_HP_OUTR_MUTE_YES |
		       INNO_REG_09_HP_OUTR_POP_WORK |
		       INNO_REG_09_HP_OUTL_POP_WORK,
	},
	{ /* start up HPOUTL HPOUTR */
		.reg = INNO_REG_05,
		.val = INNO_REG_05_HPOUTR_EN_WORK  |
		       INNO_REG_05_HPOUTL_EN_WORK  |
		       INNO_REG_05_HPOUTR_INIT     |
		       INNO_REG_05_HPOUTL_INIT,
	},
	{ /* now the HPOUTL HPOUTR init OK */
		.reg = INNO_REG_05,
		.val = INNO_REG_05_HPOUTR_EN_WORK  |
		       INNO_REG_05_HPOUTL_EN_WORK  |
		       INNO_REG_05_HPOUTR_WORK     |
		       INNO_REG_05_HPOUTL_WORK,
	},
	{ /* start up special reference voltage of DACL DACR */
		.reg = INNO_REG_06,
		.val = INNO_REG_06_DAC_EN_WORK     |
		       INNO_REG_06_DAC_PRECHARGE   |
		       INNO_REG_06_DACL_HL_RV_WORK |
		       INNO_REG_06_DACR_HL_RV_WORK |
		       INNO_REG_06_VOUTL_CZ_STOP   |
		       INNO_REG_06_VOUTR_CZ_STOP,
	},
	{ /* start up clock module of LR channel */
		.reg = INNO_REG_04,
		.val = INNO_REG_04_DACR_STOP       |
		       INNO_REG_04_DACL_STOP       |
		       INNO_REG_04_DACR_CLK_WORK   |
		       INNO_REG_04_DACL_CLK_WORK   |
		       INNO_REG_04_DACR_RV_WORK    |
		       INNO_REG_04_DACL_RV_WORK,
	},
	{ /* start up DACL DACR module */
		.reg = INNO_REG_04,
		.val = INNO_REG_04_DACR_WORK       |
		       INNO_REG_04_DACL_WORK       |
		       INNO_REG_04_DACR_CLK_WORK   |
		       INNO_REG_04_DACL_CLK_WORK   |
		       INNO_REG_04_DACR_RV_WORK    |
		       INNO_REG_04_DACL_RV_WORK,
	},
	{ /* end of the init state of DACL DACR */
		.reg = INNO_REG_09,
		.val = INNO_REG_09_POWERON,
	},
	{ /* set the gain of headphone output */
		.reg = INNO_REG_07,
		.val = INNO_REG_07_HPOUTL_GAIN_0DB,
	},
	{ /* set the gain of headphone output */
		.reg = INNO_REG_08,
		.val = INNO_REG_08_HPOUTR_GAIN_0DB,
	},
	{ /* end of the init state of DACL DACR */
		.reg = INNO_REG_09,
		.val = INNO_REG_09_DACL_WORK       |
		       INNO_REG_09_DACR_WORK       |
		       INNO_REG_09_HP_OUTL_MUTE_NO |
		       INNO_REG_09_HP_OUTR_MUTE_NO |
		       INNO_REG_09_HP_OUTR_POP_WORK |
		       INNO_REG_09_HP_OUTL_POP_WORK,
	},
};

struct inno_reg_val inno_codec_close_path[] = {
	{
		.reg = INNO_REG_06,
		.val = INNO_REG_06_DAC_EN_WORK     |
		       INNO_REG_06_DAC_PRECHARGE   |
		       INNO_REG_06_DACL_HL_RV_WORK |
		       INNO_REG_06_DACR_HL_RV_WORK |
		       INNO_REG_06_VOUTL_CZ_STOP   |
		       INNO_REG_06_VOUTR_CZ_STOP,
	},
	{
		.reg = INNO_REG_07,
		.val = INNO_REG_07_HPOUTL_GAIN_N39DB,
	},
	{
		.reg = INNO_REG_08,
		.val = INNO_REG_08_HPOUTR_GAIN_N39DB,
	},
	{
		.reg = INNO_REG_09,
		.val = INNO_REG_09_POWERON,
	},
	{
		.reg = INNO_REG_05,
		.val = INNO_REG_05_HPOUTR_EN_WORK  |
		       INNO_REG_05_HPOUTL_EN_WORK  |
		       INNO_REG_05_HPOUTR_INIT     |
		       INNO_REG_05_HPOUTL_INIT,
	},
	{
		.reg = INNO_REG_09,
		.val = INNO_REG_09_DACL_WORK       |
		       INNO_REG_09_DACR_WORK       |
		       INNO_REG_09_HP_OUTL_MUTE_YES |
		       INNO_REG_09_HP_OUTR_MUTE_YES |
		       INNO_REG_09_HP_OUTR_POP_PRECHARGE |
		       INNO_REG_09_HP_OUTL_POP_PRECHARGE,
	},
	{
		.reg = INNO_REG_04,
		.val = INNO_REG_04_DACR_RV_STOP    |
		       INNO_REG_04_DACL_RV_STOP    |
		       INNO_REG_04_DACL_CLK_STOP   |
		       INNO_REG_04_DACR_CLK_STOP   |
		       INNO_REG_04_DACL_STOP       |
		       INNO_REG_04_DACR_STOP,
	},
	{
		.reg = INNO_REG_06,
		.val = INNO_REG_06_DAC_DISCHARGE,
	},
	{
		.reg = INNO_REG_09,
		.val = INNO_REG_09_DACL_INIT       |
		       INNO_REG_09_DACR_INIT       |
		       INNO_REG_09_HP_OUTL_MUTE_YES |
		       INNO_REG_09_HP_OUTR_MUTE_YES |
		       INNO_REG_09_HP_OUTR_POP_PRECHARGE |
		       INNO_REG_09_HP_OUTL_POP_PRECHARGE,
	},
};

#endif
