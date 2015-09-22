#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/gpio.h>

struct dwc_otg_control_usb {
	struct gpio *host_gpios;
	struct gpio *otg_gpios;
};

static struct dwc_otg_control_usb *control_usb;

static int rk_usb_control_probe(struct platform_device *pdev)
{
	int gpio, err;
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;

	control_usb =
	    devm_kzalloc(&pdev->dev, sizeof(*control_usb), GFP_KERNEL);
	if (!control_usb) {
		dev_err(&pdev->dev, "unable to alloc memory for control usb\n");
		ret = -ENOMEM;
		goto out;
	}

	control_usb->host_gpios =
	    devm_kzalloc(&pdev->dev, sizeof(struct gpio), GFP_KERNEL);
	if (!control_usb->host_gpios) {
		dev_err(&pdev->dev, "unable to alloc memory for host_gpios\n");
		ret = -ENOMEM;
		goto out;
	}

	gpio = of_get_named_gpio(np, "host_drv_gpio", 0);
	control_usb->host_gpios->gpio = gpio;
	if (!gpio_is_valid(gpio)) {
		dev_err(&pdev->dev, "invalid host gpio%d\n", gpio);
	} else {
		err = devm_gpio_request(&pdev->dev, gpio, "host_drv_gpio");
		if (err) {
			dev_err(&pdev->dev,
				"failed to request GPIO%d for host_drv\n",
				gpio);
			ret = err;
			goto out;
		}

		gpio_direction_output(control_usb->host_gpios->gpio, 1);
	}

	control_usb->otg_gpios =
	    devm_kzalloc(&pdev->dev, sizeof(struct gpio), GFP_KERNEL);
	if (!control_usb->otg_gpios) {
		dev_err(&pdev->dev, "unable to alloc memory for otg_gpios\n");
		ret = -ENOMEM;
		goto out;
	}

	gpio = of_get_named_gpio(np, "otg_drv_gpio", 0);
	control_usb->otg_gpios->gpio = gpio;

	if (!gpio_is_valid(gpio)) {
		dev_err(&pdev->dev, "invalid otg gpio%d\n", gpio);
	} else {
		err = devm_gpio_request(&pdev->dev, gpio, "otg_drv_gpio");
		if (err) {
			dev_err(&pdev->dev,
				"failed to request GPIO%d for otg_drv\n", gpio);
			ret = err;
			goto out;
		}

		gpio_direction_output(control_usb->otg_gpios->gpio, 1);
	}

out:
	return ret;
}

static int rk_usb_control_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id rk_usb_control_id_table[] = {
	{ .compatible = "rockchip,rk3036-usb-control" },
	{}
};

static struct platform_driver rk_usb_control_driver = {
	.probe = rk_usb_control_probe,
	.remove = rk_usb_control_remove,
	.driver = {
		.name = "rk3036-usb-control",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(rk_usb_control_id_table),
	},
};

static int __init dwc_otg_control_usb_init(void)
{
	return platform_driver_register(&rk_usb_control_driver);
}
subsys_initcall(dwc_otg_control_usb_init);

static void __exit dwc_otg_control_usb_exit(void)
{
	platform_driver_unregister(&rk_usb_control_driver);
}
module_exit(dwc_otg_control_usb_exit);

MODULE_ALIAS("platform: dwc_control_usb");
MODULE_AUTHOR("RockChip Inc.");
MODULE_DESCRIPTION("RockChip Control Module USB Driver");
MODULE_LICENSE("GPL v2");
