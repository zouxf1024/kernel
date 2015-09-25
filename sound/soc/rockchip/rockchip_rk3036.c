/*
 * Machine driver for rk3036 audio.
 *
 * Author: Rockchip Inc.
 * Author: Zheng ShunQian<zhengsq@rock-chips.com>
 */

#include <linux/device.h>
#include <linux/of.h>
#include <linux/module.h>

#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/soc-dapm.h>

static int rk3036_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned int dai_fmt = rtd->dai_link->dai_fmt;
	int mclk, ret;
	static int flag = 0;

	dev_info(rtd->dev, "codec machine: %s\n", __func__);

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, dai_fmt);
	if (ret < 0) {
		return ret;
	}

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, dai_fmt);
	if (ret < 0) {
		return ret;
	}

	switch (params_rate(params)) {
	case 8000:
	case 16000:
	case 24000:
	case 32000:
	case 48000:
	case 96000:
		mclk = 12288000;
		break;
	case 11025:
	case 22050:
	case 44100:
		mclk = 11289600;
		break;
	default:
		return -EINVAL;
	}

	/*Set the system clk for codec*/
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, mclk, SND_SOC_CLOCK_OUT);
	if (ret < 0) {
		dev_err(codec_dai->dev, "Can't set codec clock out %d\n", ret);
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(codec_dai, 0, mclk, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(codec_dai->dev, "Can't set codec clock in %d\n", ret);
		return ret;
	}

	return 0;
}

static struct snd_soc_ops rk3036_ops = {
	  .hw_params = rk3036_hw_params,
};

static int rk30_rk3036_codec_init(struct snd_soc_pcm_runtime *rtd)
{
	return 0;
}

static struct snd_soc_dai_link rk3036_dai = {
	.name = "RK3036",
	.stream_name = "RK3036 CODEC PCM",
	.codec_dai_name = "rk3036-codec-dai",
	.init = rk30_rk3036_codec_init,
	.ops = &rk3036_ops,
	/* set codec as slave */
	.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
		   SND_SOC_DAIFMT_CBS_CFS,
};

static struct snd_soc_card rockchip_rk3036_snd_card = {
	.name = "I2S-INNO-RK3036",
	.owner = THIS_MODULE,
	.dai_link = &rk3036_dai,
	.num_links = 1,
};

static int rockchip_rk3036_audio_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &rockchip_rk3036_snd_card;
	struct device_node *np = pdev->dev.of_node;

	dev_info(&pdev->dev, "codec machine: %s\n", __func__);
	platform_set_drvdata(pdev, card);
	card->dev = &pdev->dev;

	rk3036_dai.codec_of_node = of_parse_phandle(np,
			"rockchip,audio-codec", 0);
	if (!rk3036_dai.codec_of_node) {
		dev_err(&pdev->dev,
			"Property 'rockchip,audio-codec' missing or invalid\n");
		return -EINVAL;
	}

	rk3036_dai.cpu_of_node = of_parse_phandle(np,
			"rockchip,i2s-controller", 0);
	if (!rk3036_dai.cpu_of_node) {
		dev_err(&pdev->dev,
			"Property 'rockchip,i2s-controller' missing or invalid\n");
		return -EINVAL;
	}

	rk3036_dai.platform_of_node = rk3036_dai.cpu_of_node;

	return snd_soc_register_card(card);
}

static int rockchip_rk3036_audio_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	printk("codec machine: %s\n", __func__);
	snd_soc_unregister_card(card);

	return 0;
}

static const struct of_device_id rockchip_rk3036_of_match[] = {
		{ .compatible = "rockchip,rockchip-audio-rk3036", },
		{},
};
MODULE_DEVICE_TABLE(of, rockchip_rk3036_of_match);

static struct platform_driver rockchip_rk3036_audio_driver = {
	.driver = {
		.name   = "rk3036-audio",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(rockchip_rk3036_of_match),
	},
	.probe      = rockchip_rk3036_audio_probe,
	.remove     = rockchip_rk3036_audio_remove,
};
module_platform_driver(rockchip_rk3036_audio_driver);

MODULE_AUTHOR("Rockchip Inc.");
MODULE_DESCRIPTION("Rockchip rk3036 codec driver");
MODULE_LICENSE("GPL");
