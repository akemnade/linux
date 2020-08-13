// SPDX-License-Identifier: GPL-2.0+
/*
 * Fuel gauge driver for the RICOH RN5T618 power management chip family
 *
 * Copyright (C) 2020 Andreas Kemnade
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/mfd/rn5t618.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/slab.h>

#define CHG_STATE_CHG_OFF	0
#define CHG_STATE_CHG_READY_VADP	1
#define CHG_STATE_CHG_TRICKLE	2
#define CHG_STATE_CHG_RAPID	3
#define CHG_STATE_CHG_COMPLETE	4
#define CHG_STATE_SUSPEND	5
#define CHG_STATE_VCHG_OVER_VOL	6
#define CHG_STATE_BAT_ERROR	7
#define CHG_STATE_NO_BAT	8
#define CHG_STATE_BAT_OVER_VOL	9
#define CHG_STATE_BAT_TEMP_ERR	10
#define CHG_STATE_DIE_ERR	11
#define CHG_STATE_DIE_SHUTDOWN	12
#define CHG_STATE_NO_BAT2	13
#define CHG_STATE_CHG_READY_VUSB	14

struct rn5t618_charger_info {
	struct rn5t618 *rn5t618;
	struct platform_device *pdev;
	struct power_supply *gauge;
	struct power_supply *usb;
	struct power_supply *adp;
	int irq;
};

static enum power_supply_property rn5t618_usb_props[] = {
	/* input current limit is not very accurate */
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property rn5t618_adp_props[] = {
	/* input current limit is not very accurate */
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
};


static enum power_supply_property rn5t618_gauge_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_NOW,
};

static int rn5t618_gauge_read_doublereg(struct rn5t618_charger_info *info,
					u8 reg, u16 *result)
{
	int ret;
	u8 data[2];

	ret = regmap_bulk_read(info->rn5t618->regmap, reg, data, sizeof(data));
	if (ret)
		return ret;

	*result = data[0] << 8;
	*result |= data[1];

	return 0;
}

static int rn5t618_decode_status(unsigned int status)
{
	switch(status & 0x1f) {
	case CHG_STATE_CHG_OFF:
	case CHG_STATE_SUSPEND:
	case CHG_STATE_VCHG_OVER_VOL:
	case CHG_STATE_DIE_SHUTDOWN:
		return POWER_SUPPLY_STATUS_DISCHARGING;

	case CHG_STATE_CHG_TRICKLE:
	case CHG_STATE_CHG_RAPID:
		return POWER_SUPPLY_STATUS_CHARGING;

	case CHG_STATE_CHG_COMPLETE:
		return POWER_SUPPLY_STATUS_FULL;

	default:
		return POWER_SUPPLY_STATUS_NOT_CHARGING;
	}
}

static int rn5t618_gauge_status(struct rn5t618_charger_info *info,
				union power_supply_propval *val)
{
	unsigned int v;
	int ret;

	ret = regmap_read(info->rn5t618->regmap, RN5T618_CHGSTATE, &v);
	if (ret)
		return ret;

	val->intval = POWER_SUPPLY_STATUS_UNKNOWN;

	if (v & 0xc0) { /* USB or ADP plugged */	
		val->intval = rn5t618_decode_status(v);
	} else
		val->intval = POWER_SUPPLY_STATUS_DISCHARGING;

	return ret;
}

static int rn5t618_gauge_voltage_now(struct rn5t618_charger_info *info,
				     union power_supply_propval *val)
{
	u16 res;
	int ret;

	ret = rn5t618_gauge_read_doublereg(info, RN5T618_VOLTAGE_1, &res);
	if (ret)
		return ret;

	val->intval = res * 2 * 2500 / 4095 * 1000;

	return 0;
}

static int rn5t618_gauge_current_now(struct rn5t618_charger_info *info,
				union power_supply_propval *val)
{
	u16 res;
	int ret;

	ret = rn5t618_gauge_read_doublereg(info, RN5T618_CC_AVEREG1, &res);
	if (ret)
		return ret;

	val->intval = res;
	if (val->intval & (1 << 13))
		val->intval = val->intval - (1 << 14);

	val->intval *= 1000;

	return 0;
}

static int rn5t618_gauge_capacity(struct rn5t618_charger_info *info,
				  union power_supply_propval *val)
{
	unsigned int v;
	int ret;

	ret = regmap_read(info->rn5t618->regmap, RN5T618_SOC, &v);
	if (ret)
		return ret;

	val->intval = v;

	return 0;
}

static int rn5t618_gauge_temp(struct rn5t618_charger_info *info,
			      union power_supply_propval *val)
{
	u16 res;
	int ret;

	ret = rn5t618_gauge_read_doublereg(info, RN5T618_TEMP_1, &res);
	if (ret)
		return ret;

	val->intval = res;
	if (val->intval & (1 << 11))
		val->intval = val->intval - (1 << 12);

	val->intval /= 16;

	return 0;
}

static int rn5t618_gauge_tte(struct rn5t618_charger_info *info,
			     union power_supply_propval *val)
{
	u16 res;
	int ret;

	ret = rn5t618_gauge_read_doublereg(info, RN5T618_TT_EMPTY_H, &res);
	if (ret)
		return ret;

	if (res == 65535)
		return -ENODATA;

	val->intval = res * 60;

	return 0;
}

static int rn5t618_gauge_ttf(struct rn5t618_charger_info *info,
			     union power_supply_propval *val)
{
	u16 res;
	int ret;

	ret = rn5t618_gauge_read_doublereg(info, RN5T618_TT_FULL_H, &res);
	if (ret)
		return ret;

	if (res == 65535)
		return -ENODATA;

	val->intval = res * 60;

	return 0;
}

static int rn5t618_gauge_charge_full(struct rn5t618_charger_info *info,
				     union power_supply_propval *val)
{
	u16 res;
	int ret;

	ret = rn5t618_gauge_read_doublereg(info, RN5T618_FA_CAP_H, &res);
	if (ret)
		return ret;

	val->intval = res * 1000;

	return 0;
}

static int rn5t618_gauge_charge_now(struct rn5t618_charger_info *info,
				    union power_supply_propval *val)
{
	u16 res;
	int ret;

	ret = rn5t618_gauge_read_doublereg(info, RN5T618_RE_CAP_H, &res);
	if (ret)
		return ret;

	val->intval = res * 1000;

	return 0;
}

static int rn5t618_gauge_get_property(struct power_supply *psy,
				      enum power_supply_property psp,
				      union power_supply_propval *val)
{
	int ret = 0;
        struct rn5t618_charger_info *info = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		ret = rn5t618_gauge_status(info, val);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = rn5t618_gauge_voltage_now(info, val);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		ret = rn5t618_gauge_current_now(info, val);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		ret = rn5t618_gauge_capacity(info, val);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		ret = rn5t618_gauge_temp(info, val);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		ret = rn5t618_gauge_tte(info, val);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		ret = rn5t618_gauge_ttf(info, val);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		ret = rn5t618_gauge_charge_full(info, val);
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		ret = rn5t618_gauge_charge_now(info, val);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static void rn5t618_gauge_external_power_changed(struct power_supply *psy)
{
}

static int rn5t618_adp_get_property(struct power_supply *psy,
				      enum power_supply_property psp,
				      union power_supply_propval *val)
{
        struct rn5t618_charger_info *info = power_supply_get_drvdata(psy);
	unsigned int chgstate;
	unsigned int regval;
	bool online;
	int ret;

	ret = regmap_read(info->rn5t618->regmap, RN5T618_CHGSTATE, &chgstate);
	if (ret)
		return ret;

	online = !! (chgstate & 0x40);

	switch(psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = online;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		if (!online) {
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
			break;
		}
		val->intval = rn5t618_decode_status(chgstate);
		if (val->intval == POWER_SUPPLY_STATUS_DISCHARGING)
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;

		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = regmap_read(info->rn5t618->regmap, RN5T618_REGISET1, &regval);
		if (ret < 0)
			return ret;

		val->intval = 1000 * 100 * (1 + (regval & 0x1f));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int rn5t618_adp_set_property(struct power_supply *psy,
				     enum power_supply_property psp,
				     const union power_supply_propval *val)
{
        struct rn5t618_charger_info *info = power_supply_get_drvdata(psy);
	int ret;

        switch (psp) {
        case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		if ((val->intval < 100000) || (val->intval > 1500000))
			return -EINVAL;

		/* input limit */
		ret = regmap_write(info->rn5t618->regmap, RN5T618_REGISET1,
				   0x00 | ((val->intval - 1)/ 100000));
		if (ret < 0)
			return ret;

		/* charge limit */
		ret = regmap_update_bits(info->rn5t618->regmap, RN5T618_CHGISET,
					 0x1F, ((val->intval - 1)/ 100000));
		if (ret < 0)
			return ret;

                break;
        default:
                return -EINVAL;
        }

        return 0;
}

static int rn5t618_usb_get_property(struct power_supply *psy,
				      enum power_supply_property psp,
				      union power_supply_propval *val)
{
        struct rn5t618_charger_info *info = power_supply_get_drvdata(psy);
	unsigned int chgstate;
	unsigned int regval;
	bool online;
	int ret;

	ret = regmap_read(info->rn5t618->regmap, RN5T618_CHGSTATE, &chgstate);
	if (ret)
		return ret;

	online = !! (chgstate & 0x80);

	switch(psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = online;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		if (!online) {
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
			break;
		}
		val->intval = rn5t618_decode_status(chgstate);
		if (val->intval == POWER_SUPPLY_STATUS_DISCHARGING)
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;

		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = regmap_read(info->rn5t618->regmap, RN5T618_REGISET2, &regval);
		if (ret < 0)
			return ret;

		val->intval = 1000 * 100 * (1 + (regval & 0x1f));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int rn5t618_usb_set_property(struct power_supply *psy,
				     enum power_supply_property psp,
				     const union power_supply_propval *val)
{
        struct rn5t618_charger_info *info = power_supply_get_drvdata(psy);
	int ret;

        switch (psp) {
        case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		if ((val->intval < 100000) || (val->intval > 1500000))
			return -EINVAL;

		/* input limit */
		ret = regmap_write(info->rn5t618->regmap, RN5T618_REGISET2,
				   0xE0 | ((val->intval - 1)/ 100000));
		if (ret < 0)
			return ret;

		/* charge limit */
		ret = regmap_update_bits(info->rn5t618->regmap, RN5T618_CHGISET,
					 0x1F, ((val->intval - 1)/ 100000));
		if (ret < 0)
			return ret;

                break;
        default:
                return -EINVAL;
        }

        return 0;
}

static int rn5t618_usb_property_is_writeable(struct power_supply *psy,
					     enum power_supply_property psp)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		return true;
	default:
		return false;
	}
}

static const struct power_supply_desc rn5t618_gauge_desc = {
	.name                   = "rn5t618-gauge",
        .type                   = POWER_SUPPLY_TYPE_BATTERY,
        .properties             = rn5t618_gauge_props,
        .num_properties         = ARRAY_SIZE(rn5t618_gauge_props),
        .get_property           = rn5t618_gauge_get_property,
        .external_power_changed = rn5t618_gauge_external_power_changed,

};

static const struct power_supply_desc rn5t618_adp_desc = {
	.name                   = "rn5t618-adp",
        .type                   = POWER_SUPPLY_TYPE_MAINS,
        .properties             = rn5t618_usb_props,
        .num_properties         = ARRAY_SIZE(rn5t618_adp_props),
        .get_property           = rn5t618_adp_get_property,
        .set_property           = rn5t618_adp_set_property,
        .property_is_writeable  = rn5t618_usb_property_is_writeable,
};

static const struct power_supply_desc rn5t618_usb_desc = {
	.name                   = "rn5t618-usb",
        .type                   = POWER_SUPPLY_TYPE_USB,
        .properties             = rn5t618_usb_props,
        .num_properties         = ARRAY_SIZE(rn5t618_usb_props),
        .get_property           = rn5t618_usb_get_property,
        .set_property           = rn5t618_usb_set_property,
        .property_is_writeable  = rn5t618_usb_property_is_writeable,
};

static irqreturn_t rn5t618_charger_irq(int irq, void *data)
{
	struct device *dev = data;
	struct rn5t618_charger_info *info = dev_get_drvdata(dev);
	
	unsigned int ctrl, stat1, stat2, err;

	regmap_read(info->rn5t618->regmap, RN5T618_CHGERR_IRR, &err);
	regmap_read(info->rn5t618->regmap, RN5T618_CHGCTRL_IRR, &ctrl);
	regmap_read(info->rn5t618->regmap, RN5T618_CHGSTAT_IRR1, &stat1);
	regmap_read(info->rn5t618->regmap, RN5T618_CHGSTAT_IRR2, &stat2);

	regmap_write(info->rn5t618->regmap, RN5T618_CHGERR_IRR, 0);
	regmap_write(info->rn5t618->regmap, RN5T618_CHGCTRL_IRR, 0);
	regmap_write(info->rn5t618->regmap, RN5T618_CHGSTAT_IRR1, 0);
	regmap_write(info->rn5t618->regmap, RN5T618_CHGSTAT_IRR2, 0);

	dev_info(dev, "chgerr: %x chgctrl: %x chgstat: %x chgstat2: %x\n",
		err, ctrl, stat1, stat2);

	power_supply_changed(info->usb);
	power_supply_changed(info->gauge);

	return IRQ_HANDLED;
}

static int rn5t618_gauge_probe(struct platform_device *pdev)
{
	int ret = 0;
	unsigned int v;
	struct power_supply_config psy_cfg = {};
	struct rn5t618_charger_info *info;

        info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->pdev = pdev;
	info->rn5t618 = dev_get_drvdata(pdev->dev.parent);
	info->irq = -1;

        platform_set_drvdata(pdev, info);

	ret = regmap_read(info->rn5t618->regmap, RN5T618_CONTROL, &v);
	if (ret)
		return ret;

	if (! (v & 1)) {
		dev_info(&pdev->dev, "Fuel gauge not enabled, enabling now\n");
		dev_info(&pdev->dev, "Expect unprecise results\n");
		regmap_update_bits(info->rn5t618->regmap, RN5T618_CONTROL,
				   1, 1);
	}

	psy_cfg.drv_data = info;
	info->gauge = devm_power_supply_register(&pdev->dev, &rn5t618_gauge_desc, &psy_cfg);
	if (IS_ERR(info->gauge)) {
		ret = PTR_ERR(info->gauge);
		dev_err(&pdev->dev, "failed to register gauge: %d\n", ret);
		return ret;
	}

	info->adp = devm_power_supply_register(&pdev->dev, &rn5t618_adp_desc, &psy_cfg);
	if (IS_ERR(info->adp)) {
		ret = PTR_ERR(info->adp);
		dev_err(&pdev->dev, "failed to register adp: %d\n", ret);
		return ret;
	}

	info->usb = devm_power_supply_register(&pdev->dev, &rn5t618_usb_desc, &psy_cfg);
	if (IS_ERR(info->gauge)) {
		ret = PTR_ERR(info->gauge);
		dev_err(&pdev->dev, "failed to register usb: %d\n", ret);
		return ret;
	}

	if (info->rn5t618->irq_data)
		info->irq = regmap_irq_get_virq(info->rn5t618->irq_data,
						RN5T618_IRQ_CHG);

	if (info->irq < 0)
		info->irq = -1;
	else {
		ret = devm_request_threaded_irq(&pdev->dev, info->irq, NULL,
						rn5t618_charger_irq,
						IRQF_ONESHOT,
                                                "rn5t618_charger",
                                                &pdev->dev);

		if (ret < 0) {
			dev_err(&pdev->dev, "request IRQ:%d fail\n", info->irq);
                        info->irq = -1;
		}
	}

	return 0;
}

static struct platform_driver rn5t618_gauge_driver = {
        .driver = {
                .name   = "rn5t618-gauge",
        },
        .probe = rn5t618_gauge_probe,
};

module_platform_driver(rn5t618_gauge_driver);
MODULE_ALIAS("platform:rn5t618-gauge");
MODULE_DESCRIPTION("RICOH RN5T618 Fuel gauge driver");
MODULE_LICENSE("GPL");

