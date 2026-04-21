// SPDX-License-Identifier: GPL-2.0
/*
 * aw99703.c   aw99703 backlight module
 *
 * Copyright (c) 2019 AWINIC Technology CO., LTD
 *
 *  Author: Joseph <zhangzetao@awinic.com.cn>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/types.h>
#include <linux/regulator/consumer.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/backlight.h>
#include <linux/regmap.h>
#include "leds_aw99703.h"

#define AW99703_NAME "aw99703"

static int aw99703_brightness_map(struct aw99703_data*priv, unsigned int level)
{
	if (priv == NULL)
		return -1;

	/* MAX_LEVEL_256 */
	if (priv->bl_map == 1) {
		if (level == 255)
			return 2047;
		return level * 8;
	}
	/* MAX_LEVEL_1024 */
	if (priv->bl_map == 2)
		return level * 2;
	/* MAX_LEVEL_2048 */
	if (priv->bl_map == 3)
		return level;

	return  level;
}

static int aw99703_bl_enable_channel(struct aw99703_data *drvdata)
{
	int ret = 0;

	if (drvdata->channel == 3) {
		pr_info("%s turn all channel on!\n", __func__);
		ret = regmap_update_bits(drvdata->regmap,
					 AW99703_REG_LEDCUR,
					 AW99703_LEDCUR_CHANNEL_MASK,
					 AW99703_LEDCUR_CH3_ENABLE |
					 AW99703_LEDCUR_CH2_ENABLE |
					 AW99703_LEDCUR_CH1_ENABLE);
	} else if (drvdata->channel == 2) {
		pr_info("%s turn two channel on!\n", __func__);
		ret = regmap_update_bits(drvdata->regmap,
					 AW99703_REG_LEDCUR,
					 AW99703_LEDCUR_CHANNEL_MASK,
					 AW99703_LEDCUR_CH2_ENABLE |
					 AW99703_LEDCUR_CH1_ENABLE);
	} else if (drvdata->channel == 1) {
		pr_info("%s turn one channel on!\n", __func__);
		ret = regmap_update_bits(drvdata->regmap,
					  AW99703_REG_LEDCUR,
					  AW99703_LEDCUR_CHANNEL_MASK,
					  AW99703_LEDCUR_CH1_ENABLE);
	} else {
		pr_info("%s all channels are going to be disabled\n", __func__);
		ret = regmap_update_bits(drvdata->regmap,
					 AW99703_REG_LEDCUR,
					 AW99703_LEDCUR_CHANNEL_MASK,
					 0x98);
	}

	return ret;
}

static void aw99703_pwm_mode_enable(struct aw99703_data *drvdata)
{
	if (drvdata->pwm_mode) {
		regmap_update_bits(drvdata->regmap,
					AW99703_REG_MODE,
					AW99703_MODE_PDIS_MASK,
					AW99703_MODE_PDIS_DISABLE);
		pr_info("%s pwm_mode is disable\n", __func__);
	} else {
		regmap_update_bits(drvdata->regmap,
					AW99703_REG_MODE,
					AW99703_MODE_PDIS_MASK,
					AW99703_MODE_PDIS_ENABLE);
		pr_info("%s pwm_mode is enable\n", __func__);
	}
}

static void aw99703_ramp_setting(struct aw99703_data *drvdata)
{
	regmap_update_bits(drvdata->regmap,
				AW99703_REG_TURNCFG,
				AW99703_TURNCFG_ON_TIM_MASK,
				drvdata->ramp_on_time << 4);
	pr_info("%s drvdata->ramp_on_time is 0x%x\n",
		__func__, drvdata->ramp_on_time);

	regmap_update_bits(drvdata->regmap,
				AW99703_REG_TURNCFG,
				AW99703_TURNCFG_OFF_TIM_MASK,
				drvdata->ramp_off_time);
	pr_info("%s drvdata->ramp_off_time is 0x%x\n",
		__func__, drvdata->ramp_off_time);

}

static void aw99703_transition_ramp(struct aw99703_data *drvdata)
{
	regmap_update_bits(drvdata->regmap,
				AW99703_REG_TRANCFG,
				AW99703_TRANCFG_PWM_TIM_MASK,
				drvdata->pwm_trans_dim);
	pr_info("%s drvdata->pwm_trans_dim is 0x%x\n", __func__,
		drvdata->pwm_trans_dim);

	regmap_update_bits(drvdata->regmap,
				AW99703_REG_TRANCFG,
				AW99703_TRANCFG_I2C_TIM_MASK,
				drvdata->i2c_trans_dim);
	pr_info("%s drvdata->i2c_trans_dim is 0x%x\n",
		__func__, drvdata->i2c_trans_dim);

}

static void aw99703_ovp_level_setting(struct aw99703_data *drvdata)
{
	int ovp_level;

	switch (drvdata->ovp_level) {
	case 0:
		ovp_level = AW99703_BSTCTR1_OVPSEL_17P5V;
		break;
	case 1:
		ovp_level = AW99703_BSTCTR1_OVPSEL_24V;
		break;
	case 2:
		ovp_level = AW99703_BSTCTR1_OVPSEL_31V;
		break;
	case 3:
		ovp_level = AW99703_BSTCTR1_OVPSEL_38V;
		break;
	default:
		ovp_level = AW99703_BSTCTR1_OVPSEL_41P5V;
		break;
	}
	regmap_update_bits(drvdata->regmap,
				AW99703_REG_BSTCTR1,
				AW99703_BSTCTR1_OVPSEL_MASK,
				ovp_level);
	pr_info("%s drvdata->ovp_level is 0x%x\n", __func__,
		drvdata->ovp_level);

}

static void aw99703_ocp_level_setting(struct aw99703_data *drvdata)
{
	regmap_update_bits(drvdata->regmap,
				AW99703_REG_BSTCTR1,
				AW99703_BSTCTR1_OCPSEL_MASK,
				drvdata->ocp_level);
	pr_info("%s drvdata->ocp_level is 0x%x\n", __func__,
		drvdata->ocp_level);
}

static void aw99703_switching_freq_setting(struct aw99703_data *drvdata)
{
	int frequency;

	if (drvdata->frequency)
		frequency = AW99703_BSTCTR1_SF_1000KHZ;
	else
		frequency = AW99703_BSTCTR1_SF_500KHZ;

	regmap_update_bits(drvdata->regmap,
				AW99703_REG_BSTCTR1,
				AW99703_BSTCTR1_SF_MASK,
				frequency);
	pr_info("%s drvdata->frequency is 0x%x\n", __func__,
		drvdata->frequency);
}

static void aw99703_bl_full_scale_setting(struct aw99703_data *drvdata)
{
	regmap_update_bits(drvdata->regmap,
				AW99703_REG_LEDCUR,
				AW99703_LEDCUR_BLFS_MASK,
				drvdata->full_scale_led << 3);
	pr_info("%s drvdata->full_scale_led is 0x%x\n", __func__,
		drvdata->full_scale_led);
}

static void aw99703_set_work_mode(struct aw99703_data *drvdata, int work_mode)
{
	int mode;

	switch (work_mode) {
	case 0:
		mode = AW99703_MODE_WORKMODE_STANDBY;
		break;
	case 1:
		mode = AW99703_MODE_WORKMODE_BACKLIGHT;
		break;
	default:
		mode = AW99703_MODE_WORKMODE_FLASH;
		break;
	}
	regmap_update_bits(drvdata->regmap,
				AW99703_REG_MODE,
				AW99703_MODE_WORKMODE_MASK,
				mode);
	pr_info("%s drvdata->work_mode is 0x%x\n", __func__,
		work_mode);
}

static void aw99703_map_type_setting(struct aw99703_data *drvdata)
{
	int map_type;

	if (drvdata->map_type)
		map_type = AW99703_MODE_MAP_LINEAR;
	else
		map_type = AW99703_MODE_MAP_EXPONENTIAL;

	regmap_update_bits(drvdata->regmap,
				AW99703_REG_MODE,
				AW99703_MODE_MAP_MASK,
				map_type);
	pr_info("%s drvdata->map_type is 0x%x\n", __func__,
		drvdata->map_type);
}

static void aw99703_flash_timeout_setting(struct aw99703_data *drvdata)
{
	regmap_update_bits(drvdata->regmap,
				AW99703_REG_FLASH,
				AW99703_FLASH_FLTO_TIM_MASK,
				drvdata->flash_timeout << 4);
	pr_info("%s drvdata->flash_timeout is 0x%x\n", __func__,
		drvdata->flash_timeout);
}

static void aw99703_flash_current_setting(struct aw99703_data *drvdata)
{
	regmap_update_bits(drvdata->regmap,
				AW99703_REG_FLASH,
				AW99703_FLASH_FL_S_MASK,
				drvdata->flash_current);
	pr_info("%s drvdata->flash_current is 0x%x\n", __func__,
		drvdata->flash_current);
}

static void aw99703_max_brightness_setting(struct aw99703_data *drvdata)
{
	regmap_write(drvdata->regmap,
				AW99703_REG_LEDLSB,
				MAX_BRIGHTNESS & 0x07);
	pr_info("%s led lsb is 0x%x\n", __func__,
		MAX_BRIGHTNESS & 0x07);
	regmap_write(drvdata->regmap,
				AW99703_REG_LEDMSB,
				(MAX_BRIGHTNESS >> 3) & 0xff);
	pr_info("%s led msb is 0x%x\n", __func__,
		(MAX_BRIGHTNESS >> 3) & 0xff);
}

static int aw99703_backlight_init(struct aw99703_data *drvdata)
{
	/* The startup process enters AUTO_FREQUENCE. */
	aw99703_switching_freq_setting(drvdata);
	aw99703_ovp_level_setting(drvdata);
	aw99703_ocp_level_setting(drvdata);

	regmap_write(drvdata->regmap, AW99703_REG_AFHIGH,
				drvdata->auto_freq_high);
	pr_info("%s drvdata->auto_freq_high is 0x%x\n", __func__,
		drvdata->auto_freq_high);
	regmap_write(drvdata->regmap, AW99703_REG_AFLOW,
				drvdata->auto_freq_low);
	pr_info("%s drvdata->auto_freq_low is 0x%x\n", __func__,
		drvdata->auto_freq_low);

	aw99703_bl_full_scale_setting(drvdata);
	aw99703_bl_enable_channel(drvdata);

	aw99703_ramp_setting(drvdata);
	aw99703_transition_ramp(drvdata);

	aw99703_pwm_mode_enable(drvdata);
	aw99703_set_work_mode(drvdata, drvdata->work_mode);
	aw99703_map_type_setting(drvdata);
	mdelay(6);

	aw99703_flash_timeout_setting(drvdata);
	aw99703_flash_current_setting(drvdata);

	aw99703_max_brightness_setting(drvdata);

	return 0;
}

static int aw99703_backlight_enable(struct aw99703_data *drvdata)
{
	regmap_update_bits(drvdata->regmap,
				AW99703_REG_MODE,
				AW99703_MODE_WORKMODE_MASK,
				AW99703_MODE_WORKMODE_BACKLIGHT);

	drvdata->enable = true;

	return 0;
}

static int aw99703_set_brightness(struct aw99703_data *drvdata, int brt_val)
{
	pr_info("%s brt_val is %d\n", __func__, brt_val);

	if ((drvdata->enable == false) && (brt_val > 0)) {
		aw99703_backlight_init(drvdata);
		aw99703_backlight_enable(drvdata);
	}

	brt_val = aw99703_brightness_map(drvdata, brt_val);

		/* enalbe bl mode */
		/* set backlight brt_val */
		regmap_write(drvdata->regmap,
				AW99703_REG_LEDLSB,
				brt_val&0x0007);
		regmap_write(drvdata->regmap,
				AW99703_REG_LEDMSB,
				(brt_val >> 3)&0xff);

	if (brt_val > 0)
		/* backlight enable */
		regmap_update_bits(drvdata->regmap,
					AW99703_REG_MODE,
					AW99703_MODE_WORKMODE_MASK,
					AW99703_MODE_WORKMODE_BACKLIGHT);
	else 
		/* standby mode */
		regmap_update_bits(drvdata->regmap,
					AW99703_REG_MODE,
					AW99703_MODE_WORKMODE_MASK,
					AW99703_MODE_WORKMODE_STANDBY);

	drvdata->brightness = brt_val;

	if (drvdata->brightness == 0)
		drvdata->enable = false;
	return 0;
}

static int aw99703_bl_get_brightness(struct backlight_device *bl_dev)
{
		return bl_dev->props.brightness;
}

static int aw99703_bl_update_status(struct backlight_device *bl_dev)
{
		struct aw99703_data *drvdata = bl_get_data(bl_dev);
		int brt;

		if (bl_dev->props.state & BL_CORE_SUSPENDED)
			bl_dev->props.brightness = 0;

		brt = bl_dev->props.brightness;
		/*
		 * Brightness register should always be written
		 * not only register based mode but also in PWM mode.
		 */
		return aw99703_set_brightness(drvdata, brt);
}

static const struct backlight_ops aw99703_bl_ops = {
		.update_status = aw99703_bl_update_status,
		.get_brightness = aw99703_bl_get_brightness,
};

static const struct regmap_config aw99703_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = AW99703_REG_MAX,
	//.writeable_reg = aw99703_writeable_reg,
	//.readable_reg = aw99703_readable_reg,
};

static int aw99703_read_chipid(struct aw99703_data *drvdata)
{
	int ret;
	unsigned int val;

	ret = regmap_read(drvdata->regmap, 0, &val);
	if (ret < 0)
		return ret;

	return val;
}

#if 0
int chargepump_set_backlight_level(struct aw99703_data *priv, unsigned int level)
{

	pr_info("%s aw99703_brt_level is %d\n", __func__, level);

	if (g_aw99703_data == NULL)
		return -1;

	if ((g_aw99703_data->enable == false) && (level > 0)) {
		aw99703_backlight_init(g_aw99703_data);
		aw99703_backlight_enable(g_aw99703_data);
	}

	level = aw99703_brightness_map(level);

	if (level > 0) {
		/* enalbe bl mode */
		/* set backlight level */
		aw99703_i2c_write(g_aw99703_data->client,
				AW99703_REG_LEDLSB,
				level & 0x0007);
		aw99703_i2c_write(g_aw99703_data->client,
				AW99703_REG_LEDMSB,
				(level >> 3) & 0xff);

		/* backlight enable */
		aw99703_i2c_write_bit(g_aw99703_data->client,
					AW99703_REG_MODE,
					AW99703_MODE_WORKMODE_MASK,
					AW99703_MODE_WORKMODE_BACKLIGHT);
	} else {
		/* standby mode */
		aw99703_i2c_write_bit(g_aw99703_data->client,
					AW99703_REG_MODE,
					AW99703_MODE_WORKMODE_MASK,
					AW99703_MODE_WORKMODE_STANDBY);
	}

	g_aw99703_data->brightness = level;

	if (g_aw99703_data->brightness == 0)
		g_aw99703_data->enable = false;

	return 0;
}
#endif

static void aw99703_get_dt_data(struct device *dev,
	struct aw99703_data *drvdata)
{
	int rc;
	struct device_node *np = dev->of_node;
	u32 bl_channel, temp;

	drvdata->hwen_gpio = devm_gpiod_get_optional(dev, "enable-gpios", GPIOD_OUT_HIGH);

	rc = of_property_read_u32(np, "aw99703,bl-map", &drvdata->bl_map);
	if (rc != 0)
		pr_err("%s bl_map not found!\n", __func__);
	else
		pr_info("%s bl_map=%d\n", __func__, drvdata->bl_map);

	drvdata->using_lsb = of_property_read_bool(np, "aw99703,using-lsb");
	pr_info("%s using_lsb --<%d>\n", __func__, drvdata->using_lsb);

	if (drvdata->using_lsb) {
		drvdata->default_brightness = 0x7ff;
		drvdata->max_brightness = 2047;
	} else {
		drvdata->default_brightness = 0xff;
		drvdata->max_brightness = 255;
	}

	rc = of_property_read_u32(np, "aw99703,ovp-level", &temp);
	if (rc) {
		pr_err("Invalid backlight over voltage protect level!\n");
	} else {
		drvdata->ovp_level = temp;
		pr_info("%s over voltage protect level --<%d >\n",
			__func__, drvdata->ovp_level);
	}

	rc = of_property_read_u32(np, "aw99703,ocp-level", &temp);
	if (rc) {
		pr_err("Invalid backlight over current protect level!\n");
	} else {
		drvdata->ocp_level = temp;
		pr_info("%s over current protect level --<%d >\n",
			__func__, drvdata->ocp_level);
	}

	rc = of_property_read_u32(np, "aw99703,switch-freq", &temp);
	if (rc) {
		pr_err("Invalid backlight switching frequency!\n");
	} else {
		drvdata->frequency = temp;
		pr_info("%s switching frequency  --<%d >\n",
			__func__, drvdata->frequency);
	}

	rc = of_property_read_u32(np, "aw99703,auto-freq-high", &temp);
	if (rc) {
		pr_err("Invalid backlight auto frequency high threshold!\n");
	} else {
		drvdata->auto_freq_high = temp;
		pr_info("%s auto frequency high threshold value  --<%d >\n",
			__func__, drvdata->auto_freq_high);
	}

	rc = of_property_read_u32(np, "aw99703,auto-freq-low", &temp);
	if (rc) {
		pr_err("Invalid backlight auto frequency low threshold!\n");
	} else {
		drvdata->auto_freq_low = temp;
		pr_info("%s auto frequency low threshold value  --<%d >\n",
			__func__, drvdata->auto_freq_low);
	}

	rc = of_property_read_u32(np, "aw99703,bl-fscal-led", &temp);
	if (rc) {
		pr_err("Invalid backlight full-scale led current!\n");
	} else {
		drvdata->full_scale_led = temp;
		pr_info("%s full-scale led current --<%d mA>\n",
			__func__, drvdata->full_scale_led);
	}

	rc = of_property_read_u32(np, "aw99703,bl-channel", &bl_channel);
	if (rc) {
		pr_err("Invalid channel setup!\n");
	} else {
		drvdata->channel = bl_channel;
		pr_info("%s bl-channel --<%x>\n", __func__, drvdata->channel);
	}

	rc = of_property_read_u32(np, "aw99703,turn-on-ramp", &temp);
	if (rc) {
		pr_err("Invalid ramp timing, turnon!\n");
	} else {
		drvdata->ramp_on_time = temp;
		pr_info("%s ramp on time --<%d ms>\n",
			__func__, drvdata->ramp_on_time);
	}

	rc = of_property_read_u32(np, "aw99703,turn-off-ramp", &temp);
	if (rc) {
		pr_err("Invalid ramp timing, turnoff!\n");
	} else {
		drvdata->ramp_off_time = temp;
		pr_info("%s ramp off time --<%d ms>\n",
			__func__, drvdata->ramp_off_time);
	}

	rc = of_property_read_u32(np, "aw99703,pwm-trans-dim", &temp);
	if (rc) {
		pr_err("Invalid pwm-tarns-dim value!\n");
	} else {
		drvdata->pwm_trans_dim = temp;
		pr_info("%s pwm trnasition dimming	--<%d ms>\n",
			__func__, drvdata->pwm_trans_dim);
	}

	rc = of_property_read_u32(np, "aw99703,i2c-trans-dim", &temp);
	if (rc) {
		pr_err("Invalid i2c-trans-dim value!\n");
	} else {
		drvdata->i2c_trans_dim = temp;
		pr_info("%s i2c transition dimming --<%d ms>\n",
			__func__, drvdata->i2c_trans_dim);
	}

	rc = of_property_read_u32(np, "aw99703,pwm-mode", &drvdata->pwm_mode);
	if (rc != 0)
		pr_err("%s pwm-mode not found!\n", __func__);
	else
		pr_info("%s pwm_mode=%d\n", __func__, drvdata->pwm_mode);

	rc = of_property_read_u32(np, "aw99703,map-type", &temp);
	if (rc) {
		pr_err("Invalid map type!\n");
	} else {
		drvdata->map_type = temp;
		pr_info("%s map type  --<%x>\n", __func__, drvdata->map_type);
	}

	rc = of_property_read_u32(np, "aw99703,work-mode", &temp);
	if (rc) {
		pr_err("Invalid work mode!\n");
	} else {
		drvdata->work_mode = temp;
		pr_info("%s work mode  --<%x>\n", __func__, drvdata->work_mode);
	}

	rc = of_property_read_u32(np, "aw99703,flash-timeout-time", &temp);
	if (rc) {
		pr_err("Invalid flash timeout time!\n");
	} else {
		drvdata->flash_timeout = temp;
		pr_info("%s flash timeout time --<%x>\n", __func__,
			drvdata->flash_timeout);
	}

	rc = of_property_read_u32(np, "aw99703,flash-current", &temp);
	if (rc) {
		pr_err("Invalid flash current!\n");
	} else {
		drvdata->flash_current = temp;
		pr_info("%s  flash current --<%x>\n", __func__,
			drvdata->flash_current);
	}
}

static void aw99703_disable(void *data)
{
	struct aw99703_data *drvdata = data;
	gpiod_set_value(drvdata->hwen_gpio, 0);

	regulator_disable(drvdata->vin);
}

static int aw99703_probe(struct i2c_client *client)
{
	struct aw99703_data *drvdata;
	struct backlight_device *bl_dev;
	struct backlight_properties props = {} ;
	int ret = 0;

	drvdata = devm_kzalloc(&client->dev, sizeof(struct aw99703_data),
			       GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->client = client;
	drvdata->brightness = 0;
	drvdata->enable = false;
	mutex_init(&drvdata->lock);
	aw99703_get_dt_data(&client->dev, drvdata);
	i2c_set_clientdata(client, drvdata);

	drvdata->regmap = devm_regmap_init_i2c(client, &aw99703_regmap_config);
	if (IS_ERR(drvdata->regmap))
		return dev_err_probe(&client->dev, PTR_ERR(drvdata->regmap),
				    "Failed to init regmap\n");

	drvdata->vin = devm_regulator_get(&client->dev, "vin");
	if (IS_ERR(drvdata->vin))
		return dev_err_probe(&client->dev,
				     PTR_ERR(drvdata->vin),
				     "Failed to get regulator\n");

	drvdata->hwen_gpio = devm_gpiod_get_optional(&client->dev, "enable",
						     GPIOD_OUT_LOW);
	if (IS_ERR(drvdata->hwen_gpio))
		return dev_err_probe(&client->dev,
				     PTR_ERR(drvdata->vin),
				     "Failed to get gpio\n");

	ret = regulator_enable(drvdata->vin);
	if (ret < 0)
		return ret;

	ret = devm_add_action_or_reset(&client->dev, aw99703_disable, drvdata);
	if (ret)
		return ret;

	gpiod_direction_output(drvdata->hwen_gpio, 1);
	ret = aw99703_read_chipid(drvdata);
	if (ret != AW99703_CHIP_ID)
		return dev_err_probe(&client->dev, -ENODEV,
				     "Unknown chip id %02x\n", ret);

	props.type = BACKLIGHT_RAW;
	props.brightness = MAX_BRIGHTNESS;
	props.max_brightness = MAX_BRIGHTNESS;
	bl_dev = devm_backlight_device_register(&client->dev,
		       				dev_name(&client->dev),
						&client->dev, drvdata,
						&aw99703_bl_ops, &props);
	if (IS_ERR(bl_dev))
		return dev_err_probe(&client->dev, PTR_ERR(bl_dev),
				     "Failed to register backlight!\n");

	return 0;
}

static int aw99703_suspend(struct device *dev)
{
	int ret;
	struct aw99703_data *drvdata = dev_get_drvdata(dev);

	aw99703_set_work_mode(drvdata, 0);
	gpiod_set_value(drvdata->hwen_gpio, 0);
	ret = regulator_disable(drvdata->vin);
	if (ret < 0) {
		gpiod_set_value(drvdata->hwen_gpio, 1);
		return ret;
	}

	return 0;
}

static int aw99703_resume(struct device *dev)
{
	struct aw99703_data *drvdata = dev_get_drvdata(dev);
	int ret;

	ret = regulator_enable(drvdata->vin);
	if (ret < 0)
		return ret;
	
	gpiod_set_value(drvdata->hwen_gpio, 1);
	aw99703_backlight_init(drvdata);
	return 0;
}

static const struct dev_pm_ops aw99703_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(aw99703_suspend, aw99703_resume)
};

static const struct i2c_device_id aw99703_id[] = {
	{ "aw99703", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, aw99703_id);

static const struct of_device_id match_table[] = {
	{.compatible = "awinic,aw99703"},
	{ }
};
MODULE_DEVICE_TABLE(of, match_table);

static struct i2c_driver aw99703_i2c_driver = {
	.probe = aw99703_probe,
	.id_table = aw99703_id,
	.driver = {
		.name = AW99703_NAME,
		.owner = THIS_MODULE,
		.of_match_table = match_table,
		.pm = &aw99703_pm_ops,
	},
};

module_i2c_driver(aw99703_i2c_driver);
MODULE_DESCRIPTION("Backlight driver for aw99703");
MODULE_LICENSE("GPL v2");

