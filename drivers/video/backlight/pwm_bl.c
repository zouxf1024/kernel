/*
 * linux/drivers/video/backlight/pwm_bl.c
 *
 * simple PWM based backlight control, board code has to setup
 * 1) pin configuration so PWM waveforms can output
 * 2) platform_data being correctly configured
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/pwm_backlight.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/rockchip/grf.h>
#include <linux/kthread.h>


struct pwm_bl_data {
	struct pwm_device	*pwm;
	struct device		*dev;
	unsigned int		period;
	unsigned int		lth_brightness;
	unsigned int		*levels;
	bool			enabled;
	struct regmap		*regmap_base;
	struct regulator	*power_supply;
	struct gpio_desc	*enable_gpio;
	struct gpio_desc	*ext_gpio;
	struct gpio_desc	*out_gpio;
	unsigned int		scale;
	bool			legacy;
	struct task_struct	*ph_thread;
	int			(*notify)(struct device *,
					  int brightness);
	void			(*notify_after)(struct device *,
					int brightness);
	int			(*check_fb)(struct device *, struct fb_info *);
	void			(*exit)(struct device *);
};

static void pwm_backlight_power_on(struct pwm_bl_data *pb, int brightness)
{
	int err;

	if (pb->enabled)
		return;

	err = regulator_enable(pb->power_supply);
	if (err < 0)
		dev_err(pb->dev, "failed to enable power supply\n");

	if (pb->enable_gpio)
		gpiod_set_value(pb->enable_gpio, 1);

	pwm_enable(pb->pwm);
	pb->enabled = true;
}

static void pwm_backlight_power_off(struct pwm_bl_data *pb)
{
	if (!pb->enabled)
		return;

	pwm_config(pb->pwm, 0, pb->period);
	pwm_disable(pb->pwm);

	if (pb->enable_gpio)
		gpiod_set_value(pb->enable_gpio, 0);

	regulator_disable(pb->power_supply);
	pb->enabled = false;
}

static int compute_duty_cycle(struct pwm_bl_data *pb, int brightness)
{
	unsigned int lth = pb->lth_brightness;
	u64 duty_cycle;

	if (pb->levels)
		duty_cycle = pb->levels[brightness];
	else
		duty_cycle = brightness;

	duty_cycle *= pb->period - lth;
	do_div(duty_cycle, pb->scale);

	return duty_cycle + lth;
}

static int pwm_backlight_update_status(struct backlight_device *bl)
{
	struct pwm_bl_data *pb = bl_get_data(bl);
	int brightness = bl->props.brightness;
	int duty_cycle;
	static u32 gpio_iomux_val;

	if (bl->props.ctl_state == 0) {
		printk("cannot control the backlight \n");
		bl->props.power = FB_BLANK_POWERDOWN;
		return 0;
	}

	if (bl->props.power != FB_BLANK_UNBLANK ||
	    bl->props.fb_blank != FB_BLANK_UNBLANK ||
	    bl->props.state & BL_CORE_FBBLANK)
		brightness = 0;

	if (pb->notify)
		brightness = pb->notify(pb->dev, brightness);

	// [ 20200520 Modified by jyyang to control the backlight for suspend mode
	if (brightness > 0) 
	{
		duty_cycle = compute_duty_cycle(pb, brightness);
		pwm_config(pb->pwm, duty_cycle, pb->period);
		printk("backlight on : %d \n", brightness);
		bl->props.power = FB_BLANK_UNBLANK;
		pwm_backlight_power_on(pb, brightness);
		
		if (bl->props.ctl_state && pb->ph_thread) {
			printk("Disable suspend mode control(backlight ON) \n");
			// stop thread 
			kthread_stop(pb->ph_thread);
			pb->ph_thread = NULL;

			// Set gpio output
			gpiod_direction_output_raw(pb->out_gpio, 0);
			// Set output low
			gpiod_set_value(pb->out_gpio, 0);
			msleep(20);
			//gpiod_set_value(pb->out_gpio, 1);

			// Get io mux value for uart3 tx
			regmap_read(pb->regmap_base, RK3288_GRF_GPIO7B_IOMUX, &gpio_iomux_val);
			// Set alternate function for uart3 tx
			if (!(gpio_iomux_val & BIT(0)))
				regmap_write(pb->regmap_base, RK3288_GRF_GPIO7B_IOMUX, 
						gpio_iomux_val | (BIT(17) | BIT(16) | BIT(0)));	
		}
	} else {
		printk("backlight off \n");
		pwm_backlight_power_off(pb);
		bl->props.power = FB_BLANK_POWERDOWN;
		
		if (bl->props.ctl_state && pb->ph_thread) {
			printk("Disable suspend mode control(backlight OFF) \n");
			// stop thread 
			kthread_stop(pb->ph_thread);
			pb->ph_thread = NULL;

			// Set gpio output
			gpiod_direction_output_raw(pb->out_gpio, 0);
			// Set output low
			gpiod_set_value(pb->out_gpio, 0);
			msleep(20);
			//gpiod_set_value(pb->out_gpio, 1);

			// Get io mux value for uart3 tx
			regmap_read(pb->regmap_base, RK3288_GRF_GPIO7B_IOMUX, &gpio_iomux_val);
			// Set alternate function for uart3 tx
			if (!(gpio_iomux_val & BIT(0)))
				regmap_write(pb->regmap_base, RK3288_GRF_GPIO7B_IOMUX,
						gpio_iomux_val | (BIT(17) | BIT(16) | BIT(0)));
		}
	}
	// ] Modified by jyyang to control the backlight for suspend mode

	if (pb->notify_after)
		pb->notify_after(pb->dev, brightness);

	return 0;
}

static int pwm_backlight_check_fb(struct backlight_device *bl,
				  struct fb_info *info)
{
	struct pwm_bl_data *pb = bl_get_data(bl);

	return !pb->check_fb || pb->check_fb(pb->dev, info);
}

static const struct backlight_ops pwm_backlight_ops = {
	.update_status	= pwm_backlight_update_status,
	.check_fb	= pwm_backlight_check_fb,
};

#ifdef CONFIG_OF
static int pwm_backlight_parse_dt(struct device *dev,
				  struct platform_pwm_backlight_data *data)
{
	struct device_node *node = dev->of_node;
	struct property *prop;
	int length;
	u32 value;
	int ret;

	if (!node)
		return -ENODEV;

	memset(data, 0, sizeof(*data));

	/* determine the number of brightness levels */
	prop = of_find_property(node, "brightness-levels", &length);
	if (!prop)
		return -EINVAL;

	data->max_brightness = length / sizeof(u32);

	/* read brightness levels from DT property */
	if (data->max_brightness > 0) {
		size_t size = sizeof(*data->levels) * data->max_brightness;

		data->levels = devm_kzalloc(dev, size, GFP_KERNEL);
		if (!data->levels)
			return -ENOMEM;

		ret = of_property_read_u32_array(node, "brightness-levels",
						 data->levels,
						 data->max_brightness);
		if (ret < 0)
			return ret;

		ret = of_property_read_u32(node, "default-brightness-level",
					   &value);
		if (ret < 0)
			return ret;

		data->dft_brightness = value;
		data->max_brightness--;
	}

	data->enable_gpio = -EINVAL;

	// 20200520 Added by jyyang to get the gpio number
	data->ext_gpio = of_get_named_gpio(node, "ext-gpios", 0);
	return 0;
}

static struct of_device_id pwm_backlight_of_match[] = {
	{ .compatible = "pwm-backlight" },
	{ }
};

MODULE_DEVICE_TABLE(of, pwm_backlight_of_match);
#else
static int pwm_backlight_parse_dt(struct device *dev,
				  struct platform_pwm_backlight_data *data)
{
	return -ENODEV;
}
#endif

// [ 20200520 Added by jyyang to check the suspend mode
static int pwm_monitor_thread(void *arg)
{
	struct backlight_device *bl = arg;
	struct pwm_bl_data *pb = bl_get_data(bl);
	int state, cur_state;

	state = gpiod_get_value(pb->ext_gpio);

	printk("start pwm_monitor_thread \n");

	while(!kthread_should_stop())
	{
		cur_state = gpiod_get_value(pb->ext_gpio);	
		if (cur_state && state != cur_state) {
			state = cur_state;
			bl->props.ctl_state = 1;
			bl->props.power = FB_BLANK_UNBLANK;
			pwm_backlight_update_status(bl);
		}
		/*
		else {
			bl->props.power = FB_BLANK_POWERDOWN;
			pwm_backlight_power_off(pb);
		}
		*/

		ssleep(1);
	}

	printk("end pwm_monitor_thread \n");

	return 0;
}
// ] 20200520 Added by jyyang to check the suspend mode

static int pwm_backlight_probe(struct platform_device *pdev)
{
	struct platform_pwm_backlight_data *data = dev_get_platdata(&pdev->dev);
	struct platform_pwm_backlight_data defdata;
	struct backlight_properties props;
	struct backlight_device *bl;
	struct device_node *node = pdev->dev.of_node, *np;
	struct pwm_bl_data *pb;
	int initial_blank = FB_BLANK_UNBLANK;
	struct pwm_args pargs;
	int ret;

	if (!data) {
		ret = pwm_backlight_parse_dt(&pdev->dev, &defdata);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to find platform data\n");
			return ret;
		}

		data = &defdata;
	}

	if (data->init) {
		ret = data->init(&pdev->dev);
		if (ret < 0)
			return ret;
	}

	pb = devm_kzalloc(&pdev->dev, sizeof(*pb), GFP_KERNEL);
	if (!pb) {
		ret = -ENOMEM;
		goto err_alloc;
	}

	if (data->levels) {
		unsigned int i;

		for (i = 0; i <= data->max_brightness; i++)
			if (data->levels[i] > pb->scale)
				pb->scale = data->levels[i];

		pb->levels = data->levels;
	} else
		pb->scale = data->max_brightness;

	pb->notify = data->notify;
	pb->notify_after = data->notify_after;
	pb->check_fb = data->check_fb;
	pb->exit = data->exit;
	pb->dev = &pdev->dev;
	pb->enabled = false;

	pb->enable_gpio = devm_gpiod_get_optional(&pdev->dev, "enable",
						  GPIOD_ASIS);
	if (IS_ERR(pb->enable_gpio)) {
		ret = PTR_ERR(pb->enable_gpio);
		goto err_alloc;
	}

	/*
	 * Compatibility fallback for drivers still using the integer GPIO
	 * platform data. Must go away soon.
	 */
	if (!pb->enable_gpio && gpio_is_valid(data->enable_gpio)) {
		ret = devm_gpio_request_one(&pdev->dev, data->enable_gpio,
					    GPIOF_OUT_INIT_HIGH, "enable");
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to request GPIO#%d: %d\n",
				data->enable_gpio, ret);
			goto err_alloc;
		}

		pb->enable_gpio = gpio_to_desc(data->enable_gpio);
	}

	if (pb->enable_gpio) {
		/*
		 * If the driver is probed from the device tree and there is a
		 * phandle link pointing to the backlight node, it is safe to
		 * assume that another driver will enable the backlight at the
		 * appropriate time. Therefore, if it is disabled, keep it so.
		 */
		if (node && node->phandle &&
		    gpiod_get_direction(pb->enable_gpio) == GPIOF_DIR_OUT &&
		    gpiod_get_value(pb->enable_gpio) == 0)
			;//initial_blank = FB_BLANK_POWERDOWN;	// 20200520 Deleted by jyyang
		else
			gpiod_direction_output(pb->enable_gpio, 1);
	}

	// [ 20200518 Added by jyyang to check the suspend mode
	pb->ext_gpio = devm_gpiod_get_optional(&pdev->dev, "ext",
		       				GPIOD_ASIS);

	if (IS_ERR(pb->ext_gpio)) {
		printk("## error ext_gpio \n");
	}

	if (!pb->ext_gpio && gpio_is_valid(data->ext_gpio)) {
		ret = devm_gpio_request_one(&pdev->dev, data->ext_gpio,
						GPIOF_IN, "ext-gpios");
		if (ret < 0) {
			printk("## failed to request GPIO#%d: %d \n",
					data->ext_gpio, ret);
		}

		pb->ext_gpio = gpio_to_desc(data->ext_gpio);
	}

	if (pb->ext_gpio) {
		printk("## set direction input for ext_gpio \n");
		gpiod_direction_input(pb->ext_gpio);
	}

	pb->out_gpio = devm_gpiod_get_optional(&pdev->dev, "out",
						GPIOD_ASIS);

	if (IS_ERR(pb->out_gpio)) {
		printk("## error out_gpio \n");
	}

	if(!pb->out_gpio && gpio_is_valid(data->out_gpio)) {
		ret = devm_gpio_request_one(&pdev->dev, data->ext_gpio,
						GPIOF_IN, "out-gpios");
		if (ret < 0) {
			printk("## failed to request GPIO#%d: %d \n", 
					data->out_gpio, ret);
		}
	}
	// ] 20200518 Added by jyyang to check the suspend mode

	pb->power_supply = devm_regulator_get(&pdev->dev, "power");
	if (IS_ERR(pb->power_supply)) {
		ret = PTR_ERR(pb->power_supply);
		goto err_alloc;
	}

	if (node && node->phandle && !regulator_is_enabled(pb->power_supply))
		initial_blank = FB_BLANK_POWERDOWN;

	pb->pwm = devm_pwm_get(&pdev->dev, NULL);
	if (IS_ERR(pb->pwm) && PTR_ERR(pb->pwm) != -EPROBE_DEFER && !node) {
		dev_err(&pdev->dev, "unable to request PWM, trying legacy API\n");
		pb->legacy = true;
		pb->pwm = pwm_request(data->pwm_id, "pwm-backlight");
	}

	if (IS_ERR(pb->pwm)) {
		ret = PTR_ERR(pb->pwm);
		if (ret != -EPROBE_DEFER)
			dev_err(&pdev->dev, "unable to request PWM\n");
		goto err_alloc;
	}

	dev_dbg(&pdev->dev, "got pwm for backlight\n");

	pwm_adjust_config(pb->pwm);
	/*
	 * The DT case will set the pwm_period_ns field to 0 and store the
	 * period, parsed from the DT, in the PWM device. For the non-DT case,
	 * set the period from platform data if it has not already been set
	 * via the PWM lookup table.
	 */
	pwm_get_args(pb->pwm, &pargs);
	pb->period = pargs.period;
	if (!pb->period && (data->pwm_period_ns > 0))
		pb->period = data->pwm_period_ns;

	pb->lth_brightness = data->lth_brightness * (pb->period / pb->scale);

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = data->max_brightness;
	bl = backlight_device_register(dev_name(&pdev->dev), &pdev->dev, pb,
				       &pwm_backlight_ops, &props);
	if (IS_ERR(bl)) {
		dev_err(&pdev->dev, "failed to register backlight\n");
		ret = PTR_ERR(bl);
		if (pb->legacy)
			pwm_free(pb->pwm);
		goto err_alloc;
	}

	if (data->dft_brightness > data->max_brightness) {
		dev_warn(&pdev->dev,
			 "invalid default brightness level: %u, using %u\n",
			 data->dft_brightness, data->max_brightness);
		data->dft_brightness = data->max_brightness;
	}

	bl->props.brightness = data->dft_brightness;
	bl->props.power = initial_blank;

	// [ 20200520 Added by jyyang to check the suspend mode
	if (pb->ext_gpio) {
		if (gpiod_get_value(pb->ext_gpio)) {
			bl->props.ctl_state = 1;
			backlight_update_status(bl);
		}
		else {
			bl->props.power = 0;
			bl->props.power = FB_BLANK_POWERDOWN;
			pwm_backlight_power_off(pb);
		}
	}
	else {
		bl->props.ctl_state = 1;
		backlight_update_status(bl);
	}

	/* request the GPIOS */
	if (bl->props.power == FB_BLANK_POWERDOWN)
	{
		pb->ph_thread = kthread_run(pwm_monitor_thread, bl, "pwm_monitor_thread");
		if (pb->ph_thread == NULL)
			printk("### ERROR create pwm thread\n");
	}

	np = of_parse_phandle(node, "rockchip,grf", 0);
	if (np)
	{
		pb->regmap_base = syscon_node_to_regmap(np);
		if (IS_ERR(pb->regmap_base))
			printk("### ERROOR regmap base address \n");
	}
	// ] 20200520 Added by jyyang to check the suspend mode

	platform_set_drvdata(pdev, bl);

	return 0;

err_alloc:
	if (data->exit)
		data->exit(&pdev->dev);
	return ret;
}

static int pwm_backlight_remove(struct platform_device *pdev)
{
	struct backlight_device *bl = platform_get_drvdata(pdev);
	struct pwm_bl_data *pb = bl_get_data(bl);

	backlight_device_unregister(bl);
	pwm_backlight_power_off(pb);

	// [ 20200528 Added by jyyang to remove thread
	if (pb->ph_thread) {
		kthread_stop(pb->ph_thread);
		pb->ph_thread = NULL;
	}
	// ] 20200528 Added by jyyang to remove thread 

	if (pb->exit)
		pb->exit(&pdev->dev);
	if (pb->legacy)
		pwm_free(pb->pwm);

	return 0;
}

static void pwm_backlight_shutdown(struct platform_device *pdev)
{
	struct backlight_device *bl = platform_get_drvdata(pdev);
	struct pwm_bl_data *pb = bl_get_data(bl);

	pwm_backlight_power_off(pb);
}

#ifdef CONFIG_PM_SLEEP
static int pwm_backlight_suspend(struct device *dev)
{
	struct backlight_device *bl = dev_get_drvdata(dev);
	struct pwm_bl_data *pb = bl_get_data(bl);

	if (pb->notify)
		pb->notify(pb->dev, 0);

	pwm_backlight_power_off(pb);

	if (pb->notify_after)
		pb->notify_after(pb->dev, 0);

	return 0;
}

static int pwm_backlight_resume(struct device *dev)
{
	struct backlight_device *bl = dev_get_drvdata(dev);

	backlight_update_status(bl);

	return 0;
}
#endif

static const struct dev_pm_ops pwm_backlight_pm_ops = {
#ifdef CONFIG_PM_SLEEP
	.suspend = pwm_backlight_suspend,
	.resume = pwm_backlight_resume,
	.poweroff = pwm_backlight_suspend,
	.restore = pwm_backlight_resume,
#endif
};

static struct platform_driver pwm_backlight_driver = {
	.driver		= {
		.name		= "pwm-backlight",
		.pm		= &pwm_backlight_pm_ops,
		.of_match_table	= of_match_ptr(pwm_backlight_of_match),
	},
	.probe		= pwm_backlight_probe,
	.remove		= pwm_backlight_remove,
	.shutdown	= pwm_backlight_shutdown,
};

module_platform_driver(pwm_backlight_driver);

MODULE_DESCRIPTION("PWM based Backlight Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pwm-backlight");
