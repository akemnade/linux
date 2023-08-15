// SPDX-License-Identifier: GPL-2.0
/*
 * Clock driver for twl device.
 *
 * inspired by the driver for the Palmas device
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/mfd/twl.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define VREG_STATE              2
#define TWL6030_CFG_STATE_OFF   0x00
#define TWL6030_CFG_STATE_ON    0x01
#define TWL6030_CFG_STATE_MASK  0x03

struct twl_clk32k_desc {
	const char *clk_name;
	u8 base;
};

struct twl_clock_info {
	struct device *dev;
	struct clk_hw hw;
	const struct twl_clk32k_desc *clk_desc;
};

static inline int
twlclk_read(struct twl_clock_info *info, unsigned slave_subgp, unsigned offset)
{
        u8 value;
        int status;

        status = twl_i2c_read_u8(slave_subgp,
                        &value, info->clk_desc->base + offset);
        return (status < 0) ? status : value;
}

static inline int
twlclk_write(struct twl_clock_info *info, unsigned slave_subgp, unsigned offset,
                                                 u8 value)
{
        return twl_i2c_write_u8(slave_subgp,
                        value, info->clk_desc->base + offset);
}

static inline struct twl_clock_info *to_twl_clks_info(struct clk_hw *hw)
{
	return container_of(hw, struct twl_clock_info, hw);
}

static unsigned long twl_clks_recalc_rate(struct clk_hw *hw,
					     unsigned long parent_rate)
{
	return 32768;
}

static int twl6032_clks_prepare(struct clk_hw *hw)
{
	struct twl_clock_info *cinfo = to_twl_clks_info(hw);
	int ret;

	ret = twlclk_write(cinfo, TWL_MODULE_PM_RECEIVER, VREG_STATE,
			   TWL6030_CFG_STATE_ON);
	if (ret < 0)
		dev_err(cinfo->dev, "clk prepare failed\n");

	return ret;
}

static void twl6032_clks_unprepare(struct clk_hw *hw)
{
	struct twl_clock_info *cinfo = to_twl_clks_info(hw);
	int ret;

	ret = twlclk_write(cinfo, TWL_MODULE_PM_RECEIVER, VREG_STATE,
			   TWL6030_CFG_STATE_OFF);
	if (ret < 0)
		dev_err(cinfo->dev, "clk unprepare failed\n");
}

static int twl6032_clks_is_prepared(struct clk_hw *hw)
{
	struct twl_clock_info *cinfo = to_twl_clks_info(hw);
	int val;

	val = twlclk_read(cinfo, TWL_MODULE_PM_RECEIVER, VREG_STATE);
	if (val < 0) {
		dev_err(cinfo->dev, "clk read failed\n");
		return val;
	}

	val &= TWL6030_CFG_STATE_MASK;

	return val == TWL6030_CFG_STATE_ON;
}

static const struct clk_ops twl6032_clks_ops = {
	.prepare	= twl6032_clks_prepare,
	.unprepare	= twl6032_clks_unprepare,
	.is_prepared	= twl6032_clks_is_prepared,
	.recalc_rate	= twl_clks_recalc_rate,
};

struct twl_clks_of_match_data {
	struct clk_init_data init;
	const struct twl_clk32k_desc desc;
};

static const struct twl_clks_of_match_data twl6032_of_clk32kg = {
	.init = {
		.name = "clk32kg",
		.ops = &twl6032_clks_ops,
		.flags = CLK_IGNORE_UNUSED,
	},
	.desc = {
		.clk_name = "clk32kg",
		.base = 0x8C,
	},
};

static const struct twl_clks_of_match_data twl6032_of_clk32kaudio = {
	.init = {
		.name = "clk32kaudio",
		.ops = &twl6032_clks_ops,
		.flags = CLK_IGNORE_UNUSED,
	},
	.desc = {
		.clk_name = "clk32kaudio",
		.base = 0x8F,
	},
};

static const struct of_device_id twl_clks_of_match[] = {
	{
		.compatible = "ti,twl6032-clk32kg",
		.data = &twl6032_of_clk32kg,
	},
	{
		.compatible = "ti,twl6032-clk32kaudio",
		.data = &twl6032_of_clk32kaudio,
	},
	{ },
};
MODULE_DEVICE_TABLE(of, twl_clks_of_match);

static int twl_clks_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	const struct twl_clks_of_match_data *match_data;
	struct twl_clock_info *cinfo;
	int ret;

	match_data = of_device_get_match_data(&pdev->dev);
	if (!match_data)
		return 1;

	cinfo = devm_kzalloc(&pdev->dev, sizeof(*cinfo), GFP_KERNEL);
	if (!cinfo)
		return -ENOMEM;

	platform_set_drvdata(pdev, cinfo);

	cinfo->dev = &pdev->dev;

	cinfo->clk_desc = &match_data->desc;
	cinfo->hw.init = &match_data->init;
	ret = devm_clk_hw_register(&pdev->dev, &cinfo->hw);
	if (ret) {
		dev_err(&pdev->dev, "Fail to register clock %s, %d\n",
			match_data->desc.clk_name, ret);
		return ret;
	}

	ret = of_clk_add_hw_provider(node, of_clk_hw_simple_get, &cinfo->hw);
	if (ret < 0)
		dev_err(&pdev->dev, "Fail to add clock driver, %d\n", ret);
	return ret;
}

static void twl_clks_remove(struct platform_device *pdev)
{
	of_clk_del_provider(pdev->dev.of_node);
}

static struct platform_driver twl_clks_driver = {
	.driver = {
		.name = "twl-clk",
		.of_match_table = twl_clks_of_match,
	},
	.probe = twl_clks_probe,
	.remove_new = twl_clks_remove,
};

module_platform_driver(twl_clks_driver);

MODULE_DESCRIPTION("Clock driver for TWL Series Devices");
MODULE_ALIAS("platform:twl-clk");
MODULE_LICENSE("GPL v2");
