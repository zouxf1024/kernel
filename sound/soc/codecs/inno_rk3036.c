/*
 * Driver of Inno codec for rk3036 by Rockchip Inc.
 *
 * Author: Rockchip Inc.
 * Author: Zheng ShunQian<zhengsq@rock-chips.com>
 */

#define DEBUG

#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/io.h>

#include "inno_rk3036.h"

struct rk3036_codec_priv {
	void __iomem *base;
	struct clk *pclk;
	struct regmap *regmap;
};


static int rk3036_codec_dai_set_sysclk(struct snd_soc_dai *dai, int clk_id,
				   unsigned int fre, int dir)
{
	struct snd_soc_codec *codec = dai->codec;

	dev_dbg(codec->dev, "rk3036_codec dai set sysclk: fre %d, dir %d\n",
		fre, dir);
	/*TODO */
	return 0;
}

static int rk3036_codec_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;
	unsigned int reg01_val = 0,  reg02_val = 0, reg03_val = 0;

	dev_dbg(codec->dev, "rk3036_codec dai set fmt : %08x\n", fmt);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		reg01_val |= INNO_REG_01_PINDIR_IN_SLAVE |
			     INNO_REG_01_I2SMODE_SLAVE;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		reg01_val |= INNO_REG_01_PINDIR_OUT_MASTER |
			     INNO_REG_01_I2SMODE_MASTER;
		break;
	default:
		dev_err(codec->dev, "invalid fmt\n");
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_A:
		reg02_val |= INNO_REG_02_DACM_PCM;
		break;
	case SND_SOC_DAIFMT_I2S:
		reg02_val |= INNO_REG_02_DACM_I2S;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		reg02_val |= INNO_REG_02_DACM_RJM;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		reg02_val |= INNO_REG_02_DACM_LJM;
		break;
	default:
		dev_err(codec->dev, "set dai format failed\n");
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		reg02_val |= INNO_REG_02_LRCP_NORMAL;
		reg03_val |= INNO_REG_03_BCP_NORMAL;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		reg02_val |= INNO_REG_02_LRCP_REVERSAL;
		reg03_val |= INNO_REG_03_BCP_REVERSAL;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		reg02_val |= INNO_REG_02_LRCP_REVERSAL;
		reg03_val |= INNO_REG_03_BCP_NORMAL;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		reg02_val |= INNO_REG_02_LRCP_NORMAL;
		reg03_val |= INNO_REG_03_BCP_REVERSAL;
		break;
	default:
		dev_err(codec->dev, "set dai format failed\n");
		return -EINVAL;
	}

	snd_soc_update_bits(codec, INNO_REG_01, INNO_REG_01_I2SMODE_MASK |
			    INNO_REG_01_PINDIR_MASK, reg01_val);
	snd_soc_update_bits(codec, INNO_REG_02, INNO_REG_02_LRCP_MASK |
			    INNO_REG_02_DACM_MASK, reg02_val);
	snd_soc_update_bits(codec, INNO_REG_03, INNO_REG_03_BCP_MASK,
			    reg03_val);
	return 0;
}

static int rk3036_codec_dai_digital_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	unsigned int val = 0;

	dev_dbg(codec->dev, "rk3036_codec dai mute : %d\n", mute);
	if (mute)
		val |= INNO_REG_09_HP_OUTR_MUTE_YES |
		       INNO_REG_09_HP_OUTL_MUTE_YES;
	else
		val |= INNO_REG_09_HP_OUTR_MUTE_NO |
		       INNO_REG_09_HP_OUTL_MUTE_NO;

	snd_soc_update_bits(codec, INNO_REG_09, INNO_REG_09_HP_OUTR_MUTE_MASK |
			    INNO_REG_09_HP_OUTL_MUTE_MASK, val);
	return 0;
}

static int rk3036_codec_dai_hw_params(struct snd_pcm_substream *substream,
				  struct snd_pcm_hw_params *hw_params,
				  struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	unsigned int reg02_val = 0, reg03_val = 0;

	dev_dbg(codec->dev, "rk3036_codec dai hw_params");

	switch (params_format(hw_params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		reg02_val |= INNO_REG_02_VWL_16BIT;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		reg02_val |= INNO_REG_02_VWL_20BIT;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		reg02_val |= INNO_REG_02_VWL_24BIT;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		reg02_val |= INNO_REG_02_VWL_32BIT;
		break;
	default:
		return -EINVAL;
	}

	reg02_val |= INNO_REG_02_LRCP_NORMAL;
	reg03_val |= INNO_REG_03_FWL_32BIT | INNO_REG_03_DACR_WORK;

	snd_soc_update_bits(codec, INNO_REG_02, INNO_REG_02_LSR_MASK |
			    INNO_REG_02_VWL_MASK, reg02_val);
	snd_soc_update_bits(codec, INNO_REG_03, INNO_REG_03_DACR_MASK |
			    INNO_REG_03_FWL_MASK, reg03_val);
	return 0;
}

#define RK3036_CODEC_RATES (SNDRV_PCM_RATE_8000  | \
			    SNDRV_PCM_RATE_16000 | \
			    SNDRV_PCM_RATE_32000 | \
			    SNDRV_PCM_RATE_44100 | \
			    SNDRV_PCM_RATE_48000 | \
			    SNDRV_PCM_RATE_96000)

/*TODO: the detail of the format bits, LE/BE, U20/S20 ?*/
#define RK3036_CODEC_FMTS (SNDRV_PCM_FMTBIT_S16_LE  | \
			   SNDRV_PCM_FMTBIT_S20_3LE | \
			   SNDRV_PCM_FMTBIT_S24_LE  | \
			   SNDRV_PCM_FMTBIT_S32_LE)

struct snd_soc_dai_ops rk3036_codec_dai_ops = {
	.set_sysclk	= rk3036_codec_dai_set_sysclk,
	.set_fmt	= rk3036_codec_dai_set_fmt,
	.digital_mute	= rk3036_codec_dai_digital_mute,
	.hw_params	= rk3036_codec_dai_hw_params,
};

struct snd_soc_dai_driver rk3036_codec_dai_driver[] = {
	{
		.name = "rk3036-codec-dai",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RK3036_CODEC_RATES,
			.formats = RK3036_CODEC_FMTS,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RK3036_CODEC_RATES,
			.formats = RK3036_CODEC_FMTS,
		},
		.ops = &rk3036_codec_dai_ops,
		.symmetric_rates = 1,
	},
};

static void rk3036_codec_reset(struct snd_soc_codec *codec)
{
	dev_dbg(codec->dev, "rk3036_codec reset\n");
	snd_soc_write(codec, INNO_REG_00,
		      INNO_REG_00_CSR_RESET | INNO_REG_00_CDCR_RESET);
	mdelay(10);
	snd_soc_write(codec, INNO_REG_00,
		      INNO_REG_00_CSR_WORK | INNO_REG_00_CDCR_WORK);
	mdelay(10);
}

static void rk3036_codec_power_off(struct snd_soc_codec * codec)
{
	struct inno_reg_val *reg_val;
	int i;

	dev_dbg(codec->dev, "rk3036_codec power off\n");
	/* set a big current for capacitor charging. */
	snd_soc_write(codec, INNO_REG_10, INNO_REG_10_MAX_CUR);
	/* start discharge. */
	snd_soc_write(codec, INNO_REG_06, INNO_REG_06_DAC_DISCHARGE);

	for (i = 0; i < ARRAY_SIZE(inno_codec_close_path); i++) {
		reg_val = &inno_codec_open_path[i];
		snd_soc_write(codec, reg_val->reg, reg_val->val);
		mdelay(5);
	}
}

static void rk3036_codec_power_on(struct snd_soc_codec * codec)
{
	struct inno_reg_val *reg_val;
	int i;

	dev_dbg(codec->dev, "rk3036_codec power on\n");
	/* set a big current for capacitor discharging. */
	snd_soc_write(codec, INNO_REG_10, INNO_REG_10_MAX_CUR);
	mdelay(10);
	/* start precharge */
	snd_soc_write(codec, INNO_REG_06, INNO_REG_06_DAC_PRECHARGE);
	mdelay(100);

	for (i = 0; i < ARRAY_SIZE(inno_codec_open_path); i++) {
		reg_val = &inno_codec_open_path[i];
		snd_soc_write(codec, reg_val->reg, reg_val->val);
		mdelay(5);
	}
}

static int rk3036_codec_probe(struct snd_soc_codec *codec)
{
	dev_dbg(codec->dev, "rk3036_codec probe\n");

	rk3036_codec_reset(codec);

	rk3036_codec_power_off(codec);
	/* wait for capacitor discharge finished. */
	mdelay(10);

	rk3036_codec_power_on(codec);

	return 0;
}

static int rk3036_codec_remove(struct snd_soc_codec *codec)
{
	dev_dbg(codec->dev, "rk3036_codec remove\n");
	rk3036_codec_power_off(codec);

	return 0;
}

static int rk3036_codec_set_bias_level(struct snd_soc_codec *codec,
				       enum snd_soc_bias_level level)
{
	dev_dbg(codec->dev, "rk3036_codec set bias\n");
	/*TODO */
	switch (level) {
	case SND_SOC_BIAS_ON:
		break;

	case SND_SOC_BIAS_PREPARE:
		break;

	case SND_SOC_BIAS_STANDBY:
		break;

	case SND_SOC_BIAS_OFF:
		break;
	}

	return 0;
}

static struct regmap *rk3036_codec_get_regmap(struct device *dev)
{
	struct rk3036_codec_priv *priv = dev_get_drvdata(dev);
	dev_dbg(dev, "rk3036_codec get regmap\n");
	return priv->regmap;
}

static struct snd_soc_codec_driver rk3036_codec_driver = {
	.probe		= rk3036_codec_probe,
	.remove		= rk3036_codec_remove,
	.set_bias_level	= rk3036_codec_set_bias_level,
	.get_regmap	= rk3036_codec_get_regmap,
};

static int codec_reg_read(void *context, unsigned int reg, unsigned int *val)
{
	struct rk3036_codec_priv *priv = context;
	void __iomem *base = priv->base;

	*val = readl(base + reg);
	return 0;
}

static int codec_reg_write(void *context, unsigned int reg, unsigned int val)
{
	struct rk3036_codec_priv *priv = context;
	void __iomem *base = priv->base;
	
	writel(val, base + reg);
	return 0;
}

static struct regmap_bus codec_regmap_bus = {
	.reg_read = codec_reg_read,
	.reg_write = codec_reg_write,
	.reg_format_endian_default = REGMAP_ENDIAN_NATIVE,
	.val_format_endian_default = REGMAP_ENDIAN_NATIVE,
};

static struct regmap_config rk3036_codec_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 32,
	.val_bits = 8,
};

static int rk3036_codec_platform_probe(struct platform_device *pdev)
{
	struct rk3036_codec_priv *priv;
	struct device_node *of_node = pdev->dev.of_node;
	struct resource *res;
	void __iomem *base;
	struct regmap *grf;
	int ret;

	dev_dbg(&pdev->dev, "rk3036_codec platform probe\n");
	priv = devm_kzalloc(&pdev->dev, sizeof(struct rk3036_codec_priv),
			    GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	priv->base = base;
	priv->regmap = devm_regmap_init(&pdev->dev, &codec_regmap_bus, priv,
					&rk3036_codec_regmap_config);
	if (IS_ERR(priv->regmap)) {
		dev_err(&pdev->dev, "init regmap failed\n");
		return PTR_ERR(priv->regmap);
	}

	grf = syscon_regmap_lookup_by_phandle(of_node, "rockchip,grf");
	if (IS_ERR(grf)) {
		dev_err(&pdev->dev, "needs 'rockchip,grf' property\n");
		return PTR_ERR(grf);
	}
	ret = regmap_write(grf, 0x00140, BIT(16 + 10) | BIT(10));
	if (ret != 0) {
		dev_err(&pdev->dev, "Could not write to GRF: %d\n", ret);
		return ret;
	}

	priv->pclk = devm_clk_get(&pdev->dev, "acodec_pclk");
	if (IS_ERR(priv->pclk))
		return PTR_ERR(priv->pclk);

	ret = clk_prepare_enable(priv->pclk);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to enable clk\n");
		return ret;
	}

	dev_set_drvdata(&pdev->dev, priv);

	ret = snd_soc_register_codec(&pdev->dev, &rk3036_codec_driver,
				      rk3036_codec_dai_driver,
				      ARRAY_SIZE(rk3036_codec_dai_driver));
	if (!ret) {
		clk_disable_unprepare(priv->pclk);
		dev_set_drvdata(&pdev->dev, NULL);
	}

	return ret;
}

static int rk3036_codec_platform_remove(struct platform_device *pdev)
{
	struct rk3036_codec_priv *priv = dev_get_drvdata(&pdev->dev);

	dev_dbg(&pdev->dev, "rk3036_codec platform remove\n");
	snd_soc_unregister_codec(&pdev->dev);
	clk_disable_unprepare(priv->pclk);

	return 0;
}

static void rk3036_codec_platform_shutdown(struct platform_device *pdev)
{
	/*TODO:*/
}

static const struct of_device_id rk3036_codec_of_match[] = {
	{ .compatible = "rk3036-codec", },
	{}
};
MODULE_DEVICE_TABLE(of, rk3036_codec_of_match);

/*TODO: suspend*/
static struct platform_driver rk3036_codec_platform_driver = {
	.driver = {
		.name = "rk3036-codec-platform",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(rk3036_codec_of_match),
	},
	.probe = rk3036_codec_platform_probe,
	.remove = rk3036_codec_platform_remove,
	.shutdown = rk3036_codec_platform_shutdown,
};

module_platform_driver(rk3036_codec_platform_driver);

MODULE_AUTHOR("Rockchip Inc.");
MODULE_DESCRIPTION("Rockchip rk3036 codec driver");
MODULE_LICENSE("GPL");
