/*
 * Copyright (c) 2015, Fuzhou Rockchip Electronics Co., Ltd
 *   Yakir Yang <ykk@rock-chips.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/hdmi.h>
#include <linux/mutex.h>
#include <linux/mfd/syscon.h>
#include <linux/of_device.h>

#include <drm/drm_of.h>
#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_edid.h>
#include <drm/drm_encoder_slave.h>

#include "rockchip_drm_drv.h"
#include "rockchip_drm_vop.h"

#include "inno_hdmi.h"

#define RGB			0
#define DDC_SEGMENT_ADDR        0x30

#define to_inno_hdmi(x)	container_of(x, struct inno_hdmi, x)

struct hdmi_vmode {
	bool mdvi;
	bool mhsyncpolarity;
	bool mvsyncpolarity;
	bool minterlaced;
	bool mdataenablepolarity;

	unsigned int mpixelclock;
	unsigned int mpixelrepetitioninput;
	unsigned int mpixelrepetitionoutput;
};

struct hdmi_data_info {
	unsigned int enc_in_format;
	unsigned int enc_out_format;
	unsigned int enc_color_depth;
	unsigned int colorimetry;
	unsigned int pix_repet_factor;
	unsigned int hdcp_enable;
	struct hdmi_vmode video_mode;
};

struct inno_hdmi_i2c {
	struct i2c_adapter	adap;

	struct mutex		lock;
	struct completion	cmp;

	u8                      ddc_addr;
	u8                      segment_addr;
};

struct inno_hdmi {
	struct drm_connector	connector;
	struct drm_encoder	encoder;

	struct device		*dev;
	struct drm_device	*drm_dev;
	struct clk		*pclk;

	int irq;
	int vic;
	void __iomem *regs;

	struct inno_hdmi_i2c	*i2c;
	struct i2c_adapter	*ddc;

	struct hdmi_data_info hdmi_data;
	struct drm_display_mode previous_mode;
};

static inline int hdmi_readb(struct inno_hdmi *hdmi, u16 offset, u8 *val)
{
        *val = readl_relaxed(hdmi->regs + (offset) * 0x04);

        return 0;
}

static inline int hdmi_writeb(struct inno_hdmi *hdmi, u16 offset, u32 val)
{
        writel_relaxed(val, hdmi->regs + (offset) * 0x04);

        return 0;
}

static inline int hdmi_modb(struct inno_hdmi *hdmi, u16 offset,
			    u32 msk, u32 val)
{
        u32 temp;

        temp = readl_relaxed(hdmi->regs +
                             (offset) * 0x04) & (0xFF - (msk));
        writel_relaxed(temp | ((val) & (msk)),
                       hdmi->regs + (offset) * 0x04);
        return 0;
}

static void inno_hdmi_sys_power(struct inno_hdmi *hdmi, bool enable)
{
	if (enable)
		hdmi_modb(hdmi, SYS_CTRL, m_POWER, v_PWR_ON);
	else
		hdmi_modb(hdmi, SYS_CTRL, m_POWER, v_PWR_OFF);
}

static void inno_hdmi_set_pwr_mode(struct inno_hdmi *hdmi, int mode)
{
	switch (mode) {
	case NORMAL:
		inno_hdmi_sys_power(hdmi, false);
		hdmi_writeb(hdmi, PHY_PRE_EMPHASIS, 0x6f);
		hdmi_writeb(hdmi, PHY_DRIVER, 0xbb);

		hdmi_writeb(hdmi, PHY_SYS_CTL, 0x15);
		hdmi_writeb(hdmi, PHY_SYS_CTL, 0x14);
		hdmi_writeb(hdmi, PHY_SYS_CTL, 0x10);
		hdmi_writeb(hdmi, PHY_CHG_PWR, 0x0f);
		hdmi_writeb(hdmi, 0xce, 0x00);
		hdmi_writeb(hdmi, 0xce, 0x01);
		inno_hdmi_sys_power(hdmi, true);
		break;
	case LOWER_PWR:
		inno_hdmi_sys_power(hdmi, false);
		hdmi_writeb(hdmi, PHY_DRIVER, 0x00);
		hdmi_writeb(hdmi, PHY_PRE_EMPHASIS, 0x00);
		hdmi_writeb(hdmi, PHY_CHG_PWR, 0x00);
		hdmi_writeb(hdmi, PHY_SYS_CTL, 0x17);
		break;
	default:
		dev_err(hdmi->dev, "unkown power mode %d\n", mode);
	}
}

static void inno_hdmi_reset(struct inno_hdmi *hdmi)
{
	u32 val;
	u32 msk;

	hdmi_modb(hdmi, SYS_CTRL, m_RST_DIGITAL, v_NOT_RST_DIGITAL);
	udelay(100);
	hdmi_modb(hdmi, SYS_CTRL, m_RST_ANALOG, v_NOT_RST_ANALOG);
	udelay(100);

	msk = m_REG_CLK_INV | m_REG_CLK_SOURCE | m_POWER | m_INT_POL;
	val = v_REG_CLK_INV | v_REG_CLK_SOURCE_SYS | v_PWR_ON | v_INT_POL_HIGH;
	hdmi_modb(hdmi, SYS_CTRL, msk, val);

	inno_hdmi_set_pwr_mode(hdmi, NORMAL);
}

static int inno_hdmi_setup(struct inno_hdmi *hdmi,
			   struct drm_display_mode *mode)
{
	int value;

	hdmi->vic = drm_match_cea_mode(mode);

	hdmi->hdmi_data.enc_in_format = RGB;
	hdmi->hdmi_data.enc_out_format = RGB;

	hdmi->hdmi_data.enc_color_depth = 8;
	hdmi->hdmi_data.pix_repet_factor = 0;

	/* Disable video and audio output */
	hdmi_modb(hdmi, AV_MUTE, m_AUDIO_MUTE | m_VIDEO_BLACK,
		  v_AUDIO_MUTE(1) | v_VIDEO_MUTE(1));

	/*
	 * Input video mode is SDR RGB24bit, data enable signal
	 * from external.
	 */
	hdmi_writeb(hdmi, VIDEO_CONTRL1, v_DE_EXTERNAL |
		    v_VIDEO_INPUT_FORMAT(VIDEO_INPUT_SDR_RGB444));

	value = v_VIDEO_INPUT_BITS(VIDEO_INPUT_8BITS) |
		v_VIDEO_OUTPUT_COLOR(0) | v_VIDEO_INPUT_CSP(0);
	hdmi_writeb(hdmi, VIDEO_CONTRL2, value);

	/* Set HDMI Mode */
	hdmi_writeb(hdmi, HDCP_CTRL, v_HDMI_DVI(1));

	/* Set ext video timing */
	value = v_EXTERANL_VIDEO(1) | v_HSYNC_POLARITY(1) |
		v_VSYNC_POLARITY(1);
	hdmi_writeb(hdmi, VIDEO_TIMING_CTL, value);

	value = mode->htotal;
	hdmi_writeb(hdmi, VIDEO_EXT_HTOTAL_L, value & 0xFF);
	hdmi_writeb(hdmi, VIDEO_EXT_HTOTAL_H, (value >> 8) & 0xFF);

	value = mode->htotal - mode->hdisplay;
	hdmi_writeb(hdmi, VIDEO_EXT_HBLANK_L, value & 0xFF);
	hdmi_writeb(hdmi, VIDEO_EXT_HBLANK_H, (value >> 8) & 0xFF);

	value = mode->hsync_start - mode->hdisplay;
	hdmi_writeb(hdmi, VIDEO_EXT_HDELAY_L, value & 0xFF);
	hdmi_writeb(hdmi, VIDEO_EXT_HDELAY_H, (value >> 8) & 0xFF);

	value = mode->hsync_end - mode->hsync_start;
	hdmi_writeb(hdmi, VIDEO_EXT_HDURATION_L, value & 0xFF);
	hdmi_writeb(hdmi, VIDEO_EXT_HDURATION_H, (value >> 8) & 0xFF);

	value = mode->vtotal;
	hdmi_writeb(hdmi, VIDEO_EXT_VTOTAL_L, value & 0xFF);
	hdmi_writeb(hdmi, VIDEO_EXT_VTOTAL_H, (value >> 8) & 0xFF);

	value = mode->vtotal - mode->vdisplay;
	hdmi_writeb(hdmi, VIDEO_EXT_VBLANK, value & 0xFF);

	value = mode->vsync_start - mode->vdisplay;
	hdmi_writeb(hdmi, VIDEO_EXT_VDELAY, value & 0xFF);

	value = mode->vsync_end - mode->vsync_start;
	hdmi_writeb(hdmi, VIDEO_EXT_VDURATION, value & 0xFF);

	hdmi_writeb(hdmi, PHY_PRE_DIV_RATIO, 0x1e);
	hdmi_writeb(hdmi, PHY_FEEDBACK_DIV_RATIO_LOW, 0x2c);
	hdmi_writeb(hdmi, PHY_FEEDBACK_DIV_RATIO_HIGH, 0x01);

	/* Disable video and audio output */
	hdmi_modb(hdmi, AV_MUTE, m_AUDIO_MUTE | m_VIDEO_BLACK,
		  v_AUDIO_MUTE(0) | v_VIDEO_MUTE(0));

	return 0;
}

static void inno_hdmi_encoder_mode_set(struct drm_encoder *encoder,
				       struct drm_display_mode *mode,
				       struct drm_display_mode *adj_mode)
{
	struct inno_hdmi *hdmi = to_inno_hdmi(encoder);

	inno_hdmi_setup(hdmi, mode);

	/* Store the display mode for plugin/DKMS poweron events */
	memcpy(&hdmi->previous_mode, mode, sizeof(hdmi->previous_mode));
}

static void inno_hdmi_encoder_disable(struct drm_encoder *encoder)
{
}

static void inno_hdmi_encoder_commit(struct drm_encoder *encoder)
{
}

static void inno_hdmi_encoder_prepare(struct drm_encoder *encoder)
{
	rockchip_drm_crtc_mode_config(encoder->crtc, DRM_MODE_CONNECTOR_HDMIA,
				      ROCKCHIP_OUT_MODE_P888);
}

static bool inno_hdmi_encoder_mode_fixup(struct drm_encoder *encoder,
					 const struct drm_display_mode *mode,
					 struct drm_display_mode *adj_mode)
{
	return true;
}

static struct drm_encoder_helper_funcs inno_hdmi_encoder_helper_funcs = {
	.mode_fixup = inno_hdmi_encoder_mode_fixup,
	.mode_set   = inno_hdmi_encoder_mode_set,
	.prepare    = inno_hdmi_encoder_prepare,
	.commit     = inno_hdmi_encoder_commit,
	.disable    = inno_hdmi_encoder_disable,
};

static struct drm_encoder_funcs inno_hdmi_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static enum drm_connector_status
inno_hdmi_connector_detect(struct drm_connector *connector, bool force)
{
	struct inno_hdmi *hdmi = to_inno_hdmi(connector);
	u8 value;

	hdmi_readb(hdmi, HDMI_STATUS, &value);

	return (value & m_HOTPLUG) ? connector_status_connected :
		connector_status_disconnected;
}

static int inno_hdmi_connector_get_modes(struct drm_connector *connector)
{
	struct inno_hdmi *hdmi = to_inno_hdmi(connector);
	struct edid *edid;
	int ret;

	if (!hdmi->ddc)
		return 0;

	edid = drm_get_edid(connector, hdmi->ddc);
	if (edid) {
		drm_mode_connector_update_edid_property(connector, edid);
		ret = drm_add_edid_modes(connector, edid);
	}

	return ret;
}

static enum drm_mode_status
inno_hdmi_connector_mode_valid(struct drm_connector *connector,
			       struct drm_display_mode *mode)
{
	return MODE_OK;
}

static struct drm_encoder *inno_hdmi_connector_best_encoder(struct drm_connector
							   *connector)
{
	struct inno_hdmi *hdmi = to_inno_hdmi(connector);

	return &hdmi->encoder;
}

static void inno_hdmi_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static struct drm_connector_funcs inno_hdmi_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.detect = inno_hdmi_connector_detect,
	.destroy = inno_hdmi_connector_destroy,
};

static struct drm_connector_helper_funcs inno_hdmi_connector_helper_funcs = {
	.get_modes = inno_hdmi_connector_get_modes,
	.mode_valid = inno_hdmi_connector_mode_valid,
	.best_encoder = inno_hdmi_connector_best_encoder,
};

static int inno_hdmi_register(struct drm_device *drm, struct inno_hdmi *hdmi)
{
	struct drm_encoder *encoder = &hdmi->encoder;

	encoder->possible_crtcs = drm_of_find_possible_crtcs(drm, hdmi->dev->of_node);
	/*
	 * If we failed to find the CRTC(s) which this encoder is
	 * supposed to be connected to, it's because the CRTC has
	 * not been registered yet.  Defer probing, and hope that
	 * the required CRTC is added later.
	 */
	if (encoder->possible_crtcs == 0)
		return -EPROBE_DEFER;

	drm_encoder_helper_add(encoder, &inno_hdmi_encoder_helper_funcs);
	drm_encoder_init(drm, encoder, &inno_hdmi_encoder_funcs,
			 DRM_MODE_ENCODER_TMDS);

	hdmi->connector.polled = DRM_CONNECTOR_POLL_HPD;

	drm_connector_helper_add(&hdmi->connector,
				 &inno_hdmi_connector_helper_funcs);
	drm_connector_init(drm, &hdmi->connector, &inno_hdmi_connector_funcs,
			   DRM_MODE_CONNECTOR_HDMIA);

	hdmi->connector.encoder = encoder;

	drm_mode_connector_attach_encoder(&hdmi->connector, encoder);

	return 0;
}

static irqreturn_t inno_hdmi_i2c_irq(struct inno_hdmi *hdmi)
{
	struct inno_hdmi_i2c *i2c = hdmi->i2c;
	u8 stat;

	hdmi_readb(hdmi, INTERRUPT_STATUS1, &stat);
	if (!(stat & m_INT_EDID_READY))
		return IRQ_NONE;

	/* clear EDID interrupt reg */
	hdmi_writeb(hdmi, INTERRUPT_STATUS1, m_INT_EDID_READY);

	complete(&i2c->cmp);

	return IRQ_HANDLED;
}

static irqreturn_t inno_hdmi_hardirq(int irq, void *dev_id)
{
	struct inno_hdmi *hdmi = dev_id;
	u8 interrupt;
	irqreturn_t ret = IRQ_NONE;

	if (hdmi->i2c)
		ret = inno_hdmi_i2c_irq(hdmi);

	hdmi_readb(hdmi, HDMI_STATUS, &interrupt);
	if (interrupt & m_INT_HOTPLUG) {
		ret = IRQ_WAKE_THREAD;
	}

	return ret;
}

static irqreturn_t inno_hdmi_irq(int irq, void *dev_id)
{
	struct inno_hdmi *hdmi = dev_id;
	u8 interrupt;

	hdmi_readb(hdmi, HDMI_STATUS, &interrupt);
	if (interrupt & m_INT_HOTPLUG) {
		hdmi_modb(hdmi, HDMI_STATUS, m_INT_HOTPLUG,
			  m_INT_HOTPLUG);
		drm_helper_hpd_irq_event(hdmi->connector.dev);
	}

	return IRQ_HANDLED;
}

static int inno_hdmi_i2c_wait(struct inno_hdmi *hdmi)
{
	struct inno_hdmi_i2c *i2c = hdmi->i2c;
	int stat;

	stat = wait_for_completion_timeout(&i2c->cmp, HZ / 10);
	if (!stat) {
		/* We tried to unwedge; give it another chance */
		stat = wait_for_completion_timeout(&i2c->cmp, HZ / 10);
		if (!stat)
			return -EAGAIN;
	}

	return 0;
}

static int inno_hdmi_i2c_read(struct inno_hdmi *hdmi, struct i2c_msg *msgs)
{
	int length = msgs->len;
	u8 *buf = msgs->buf;
	int ret;

	ret = inno_hdmi_i2c_wait(hdmi);
	if (ret)
		return ret;

	while (length--) {
		hdmi_readb(hdmi, EDID_FIFO_ADDR, buf++);
	}

	return 0;
}

static int inno_hdmi_i2c_write(struct inno_hdmi *hdmi, struct i2c_msg *msgs)
{
	struct inno_hdmi_i2c *i2c = hdmi->i2c;

	/*
	 * The DDC module only support read EDID message, so
	 * we assume that each word write to this i2c adapter
	 * should be the offset of EDID word address.
	 */
	if ((msgs->len != 1) ||
	    ((msgs->addr != DDC_ADDR) && (msgs->addr != DDC_SEGMENT_ADDR)))
		return -EINVAL;

	reinit_completion(&i2c->cmp);

	if (msgs->addr == DDC_SEGMENT_ADDR)
		hdmi->i2c->segment_addr = msgs->buf[0];
	if (msgs->addr == DDC_ADDR)
		hdmi->i2c->ddc_addr = msgs->buf[0];

	/* Set edid fifo first addr */
	hdmi_writeb(hdmi, EDID_FIFO_OFFSET, 0x00);

	/* Set edid word address 0x00/0x80 */
	hdmi_writeb(hdmi, EDID_WORD_ADDR, hdmi->i2c->ddc_addr);

	/* Set edid segment pointer */
	hdmi_writeb(hdmi, EDID_SEGMENT_POINTER, hdmi->i2c->segment_addr);

	return 0;
}

static int inno_hdmi_i2c_xfer(struct i2c_adapter *adap,
			      struct i2c_msg *msgs, int num)
{
	struct inno_hdmi *hdmi = i2c_get_adapdata(adap);
	struct inno_hdmi_i2c *i2c = hdmi->i2c;
	int i, ret = 0;

	mutex_lock(&i2c->lock);

	/* Clear and enable EDID interrupt */
	hdmi_writeb(hdmi, INTERRUPT_MASK1, m_INT_EDID_READY);
	hdmi_writeb(hdmi, INTERRUPT_STATUS1, m_INT_EDID_READY);

	for (i = 0; i < num; i++) {
		dev_dbg(hdmi->dev, "xfer: num: %d/%d, len: %d, flags: %#x\n",
			i + 1, num, msgs[i].len, msgs[i].flags);

		if (msgs[i].flags & I2C_M_RD)
			ret = inno_hdmi_i2c_read(hdmi, &msgs[i]);
		else
			ret = inno_hdmi_i2c_write(hdmi, &msgs[i]);

		if (ret < 0)
			break;
	}

	if (!ret)
		ret = num;

	/* Close edid irq */
	hdmi_writeb(hdmi, INTERRUPT_MASK1, 0);

	mutex_unlock(&i2c->lock);

	return ret;
}

static u32 inno_hdmi_i2c_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm inno_hdmi_algorithm = {
	.master_xfer	= inno_hdmi_i2c_xfer,
	.functionality	= inno_hdmi_i2c_func,
};

static void inno_hdmi_i2c_init(struct inno_hdmi *hdmi)
{
	int ddc_bus_freq;
	int hclk_rate;

	hclk_rate = clk_get_rate(hdmi->pclk);
	ddc_bus_freq = (hclk_rate >> 2) / HDMI_SCL_RATE;

	hdmi_writeb(hdmi, DDC_BUS_FREQ_L, ddc_bus_freq & 0xFF);
	hdmi_writeb(hdmi, DDC_BUS_FREQ_H, (ddc_bus_freq >> 8) & 0xFF);

	/* Close edid irq */
	hdmi_writeb(hdmi, INTERRUPT_MASK1, 0);

	/* clear EDID interrupt reg */
	hdmi_writeb(hdmi, INTERRUPT_STATUS1, m_INT_EDID_READY);
}

static struct i2c_adapter *inno_hdmi_i2c_adapter(struct inno_hdmi *hdmi)
{
	struct i2c_adapter *adap;
	struct inno_hdmi_i2c *i2c;
	int ret;

	i2c = devm_kzalloc(hdmi->dev, sizeof(*i2c), GFP_KERNEL);
	if (!i2c)
		return ERR_PTR(-ENOMEM);

	mutex_init(&i2c->lock);
	init_completion(&i2c->cmp);

	adap = &i2c->adap;
	adap->class = I2C_CLASS_DDC;
	adap->owner = THIS_MODULE;
	adap->dev.parent = hdmi->dev;
	adap->dev.of_node = hdmi->dev->of_node;
	adap->algo = &inno_hdmi_algorithm;
	strlcpy(adap->name, "DesignWare HDMI", sizeof(adap->name));
	i2c_set_adapdata(adap, hdmi);

	ret = i2c_add_adapter(adap);
	if (ret) {
		dev_warn(hdmi->dev, "cannot add %s I2C adapter\n", adap->name);
		devm_kfree(hdmi->dev, i2c);
		return ERR_PTR(ret);
	}

	hdmi->i2c = i2c;

	dev_info(hdmi->dev, "registered %s I2C bus driver\n", adap->name);

	return adap;
}

static int inno_hdmi_bind(struct device *dev, struct device *master,
				 void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct device_node *np = dev->of_node;
	struct drm_device *drm = data;
	struct device_node *ddc_node;
	struct inno_hdmi *hdmi;
	struct resource *iores;
	int irq;
	int ret;

	hdmi = devm_kzalloc(dev, sizeof(*hdmi), GFP_KERNEL);
	if (!hdmi)
		return -ENOMEM;

	hdmi->dev = dev;
	hdmi->drm_dev = drm;

	ddc_node = of_parse_phandle(np, "ddc-i2c-bus", 0);
	if (ddc_node) {
		hdmi->ddc = of_find_i2c_adapter_by_node(ddc_node);
		of_node_put(ddc_node);
		if (!hdmi->ddc) {
			dev_info(hdmi->dev, "failed to read ddc node\n");
			return -EPROBE_DEFER;
		}

	} else {
		dev_info(hdmi->dev, "no ddc property found\n");
	}

	iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iores)
		return -ENXIO;

	hdmi->regs = devm_ioremap_resource(dev, iores);
	if (IS_ERR(hdmi->regs))
		return PTR_ERR(hdmi->regs);

	hdmi->pclk = devm_clk_get(hdmi->dev, "pclk");
	if (IS_ERR(hdmi->pclk)) {
		dev_err(hdmi->dev, "Unable to get HDMI pclk clk\n");
		return PTR_ERR(hdmi->pclk);
	}

	ret = clk_prepare_enable(hdmi->pclk);
	if (ret) {
		dev_err(hdmi->dev, "Cannot enable HDMI pclk clock: %d\n", ret);
		return ret;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	inno_hdmi_reset(hdmi);

	hdmi->ddc = inno_hdmi_i2c_adapter(hdmi);
	if (IS_ERR(hdmi->ddc)) {
		hdmi->ddc = NULL;
		return PTR_ERR(hdmi->ddc);
	}

	inno_hdmi_i2c_init(hdmi);

	ret = inno_hdmi_register(drm, hdmi);
	if (ret)
		return ret;

	dev_set_drvdata(dev, hdmi);

	hdmi_modb(hdmi, HDMI_STATUS, m_MASK_INT_HOTPLUG, v_MASK_INT_HOTPLUG(1));

	ret = devm_request_threaded_irq(dev, irq, inno_hdmi_hardirq,
					inno_hdmi_irq, IRQF_SHARED,
					dev_name(dev), hdmi);

	return ret;
}

static void inno_hdmi_unbind(struct device *dev, struct device *master,
			     void *data)
{
	struct inno_hdmi *hdmi = dev_get_drvdata(dev);

	hdmi->connector.funcs->destroy(&hdmi->connector);
	hdmi->encoder.funcs->destroy(&hdmi->encoder);

	clk_disable_unprepare(hdmi->pclk);
	i2c_put_adapter(hdmi->ddc);
}

static const struct component_ops inno_hdmi_ops = {
	.bind	= inno_hdmi_bind,
	.unbind	= inno_hdmi_unbind,
};

static int inno_hdmi_probe(struct platform_device *pdev)
{
	return component_add(&pdev->dev, &inno_hdmi_ops);
}

static int inno_hdmi_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &inno_hdmi_ops);

	return 0;
}

static const struct of_device_id inno_hdmi_dt_ids[] = {
	{ .compatible = "rockchip,rk3036-inno-hdmi",
	},
	{},
};
MODULE_DEVICE_TABLE(of, inno_hdmi_dt_ids);

static struct platform_driver inno_hdmi_pltfm_driver = {
	.probe  = inno_hdmi_probe,
	.remove = inno_hdmi_remove,
	.driver = {
		.name = "innohdmi-rockchip",
		.of_match_table = inno_hdmi_dt_ids,
	},
};

module_platform_driver(inno_hdmi_pltfm_driver);

MODULE_AUTHOR("Yakir Yang <ykk@rock-chips.com>");
MODULE_DESCRIPTION("Rockchip Specific INNO-HDMI Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:innohdmi-rockchip");
