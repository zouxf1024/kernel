
#include <linux/clk.h>
#include <linux/crc32.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/netdevice.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/wakelock.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include <linux/completion.h>

#include <linux/mfd/syscon.h>
#include <linux/regmap.h>

#include "rk29_vmac.h"

#define RK3036_GRF_SOC_CON0 0x00140

static struct regmap *vmac_grf;
struct device_node *vmac_node;

static struct regmap *rockchip_vmac_get_grf(void)
{
	// printk("%s -- line = %d, vmac_node = 0x%x, IS_ERR = %d\n",
	// 	__func__, __LINE__, (unsigned int)vmac_node, IS_ERR(vmac_grf));

	if (IS_ERR_OR_NULL(vmac_grf))
		vmac_grf = syscon_regmap_lookup_by_phandle(vmac_node, "rockchip,grf");
	return vmac_grf;
}

static int rk30_vmac_register_set(void)
{
	struct regmap *grf = rockchip_vmac_get_grf();
	u32 val;

	// printk("%s -- line = %d IS_ERR = %d\n", __func__, __LINE__, IS_ERR(grf));

	//config rk30 vmac as rmii
	regmap_write(grf, RK3036_GRF_SOC_CON0, (0<<8) | ((1<<8)<<16));
	//newrev_en
	regmap_write(grf, RK3036_GRF_SOC_CON0, (1<<15) | ((1<<15)<<16));

	regmap_read(grf, RK3036_GRF_SOC_CON0, &val);
	// printk("%s -- line = %d val = 0x%x\n", __func__, __LINE__, val);

	return 0;
}

static int rk30_rmii_io_init(void)
{
	printk("enter %s \n",__func__);

	//rk3188 gpio3 and sdio drive strength , 
	//grf_writel((0x0f<<16)|0x0f, RK3188_GRF_IO_CON3);

	return 0;
}

static int rk30_rmii_io_deinit(void)
{
	//phy power down
	printk("enter %s \n",__func__);
	return 0;
}

static int rk30_rmii_power_control(int enable)
{
	return 0;
}

static int rk29_vmac_speed_switch(int speed)
{
	struct regmap *grf = rockchip_vmac_get_grf();

	// printk("%s -- line = %d speed = %d, IS_ERR = %d\n",
	// 	__func__, __LINE__, speed, IS_ERR(grf));

	if (10 == speed) {
		regmap_write(grf, RK3036_GRF_SOC_CON0, (0<<9) | ((1<<9)<<16));
	} else {
		regmap_write(grf, RK3036_GRF_SOC_CON0, (1<<9) | ((1<<9)<<16));
	}

	// regmap_read(grf, RK3036_GRF_SOC_CON0, &val);
	// printk("%s -- line = %d val = 0x%x\n", __func__, __LINE__, val);

	return 0;
}

struct rk29_vmac_platform_data board_vmac_data = {
	.vmac_register_set = rk30_vmac_register_set,
	.rmii_io_init = rk30_rmii_io_init,
	.rmii_io_deinit = rk30_rmii_io_deinit,
	.rmii_power_control = rk30_rmii_power_control,
	.rmii_speed_switch = rk29_vmac_speed_switch,
};
