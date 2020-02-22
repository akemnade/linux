/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

/*!
 * @file pmic/core/tps6518x.c
 * @brief This file contains TPS6518x specific PMIC code. This implementaion
 * may differ for each PMIC chip.
 *
 * @ingroup PMIC_CORE
 */

/*
 * Includes
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
// #include <linux/pmic_status.h>
/****** start include/linux/pmic_status.h *******/

/*
 * Copyright 2004-2015 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU Lesser General
 * Public License.  You may obtain a copy of the GNU Lesser General
 * Public License Version 2.1 or later at the following locations:
 *
 * http://www.opensource.org/licenses/lgpl-license.html
 * http://www.gnu.org/copyleft/lgpl.html
 */
#ifndef __ASM_ARCH_MXC_PMIC_STATUS_H__
#define __ASM_ARCH_MXC_PMIC_STATUS_H__
#include <asm-generic/errno-base.h>
#ifdef __KERNEL__
#include <asm/uaccess.h>	/* copy_{from,to}_user() */
#endif
/*!
 * @file arch-mxc/pmic_status.h
 * @brief PMIC APIs return code definition.
 *
 * @ingroup PMIC_CORE
 */

/*!
 * @enum PMIC_STATUS
 * @brief Define return values for all PMIC APIs.
 *
 * These return values are used by all of the PMIC APIs.
 *
 * @ingroup PMIC
 */
typedef enum {
	PMIC_SUCCESS = 0,	/*!< The requested operation was successfully
				   completed.                                     */
	PMIC_ERROR = -1,	/*!< The requested operation could not be completed
				   due to an error.                               */
	PMIC_PARAMETER_ERROR = -2,	/*!< The requested operation failed because
					   one or more of the parameters was
					   invalid.                             */
	PMIC_NOT_SUPPORTED = -3,	/*!< The requested operation could not be
					   completed because the PMIC hardware
					   does not support it. */
	PMIC_SYSTEM_ERROR_EINTR = -EINTR,

	PMIC_MALLOC_ERROR = -5,	/*!< Error in malloc function             */
	PMIC_UNSUBSCRIBE_ERROR = -6,	/*!< Error in un-subscribe event          */
	PMIC_EVENT_NOT_SUBSCRIBED = -7,	/*!< Event occur and not subscribed       */
	PMIC_EVENT_CALL_BACK = -8,	/*!< Error - bad call back                */
	PMIC_CLIENT_NBOVERFLOW = -9,	/*!< The requested operation could not be
					   completed because there are too many
					   PMIC client requests */
} PMIC_STATUS;

/*
 * Bitfield macros that use rely on bitfield width/shift information.
 */
#define BITFMASK(field) (((1U << (field ## _WID)) - 1) << (field ## _LSH))
#define BITFVAL(field, val) ((val) << (field ## _LSH))
#define BITFEXT(var, bit) ((var & BITFMASK(bit)) >> (bit ## _LSH))

/*
 * Macros implementing error handling
 */
#define CHECK_ERROR(a)			\
do {					\
		int ret = (a); 			\
		if (ret != PMIC_SUCCESS)	\
	return ret; 			\
} while (0)

#define CHECK_ERROR_KFREE(func, freeptrs) \
do { \
	int ret = (func); \
	if (ret != PMIC_SUCCESS) { \
		freeptrs;	\
		return ret;	\
	}	\
} while (0);

#endif				/* __ASM_ARCH_MXC_PMIC_STATUS_H__ */

/****** end include/linux/pmic_status.h *******/
#include <linux/mfd/core.h>
#include <linux/mfd/tps6518x.h>
#include <asm/mach-types.h>




/*
 * EPDC PMIC I2C address
 */
#define EPDC_PMIC_I2C_ADDR 0x48

//unsigned long gdwSafeTick_To_TurnON_RailPower;


static int tps6518x_detect(struct i2c_client *client, struct i2c_board_info *info);
static struct regulator *gpio_regulator;

static struct mfd_cell tps6518x_devs[] = {
	{ .name = "tps6518x-pmic", },
	{ .name = "tps6518x-sns", },
};

static const unsigned short normal_i2c[] = {EPDC_PMIC_I2C_ADDR, I2C_CLIENT_END};




#define PULLUP_WAKEPIN_IF_LOW	1


int tps6518x_reg_read(struct tps6518x *tps6518x,int reg_num, unsigned int *reg_val)
{
	int result;
	struct i2c_client *tps6518x_client = tps6518x->i2c_client;


	if (tps6518x_client == NULL) {
		dev_err(&tps6518x_client->dev,
			"tps6518x I2C adaptor not ready !\n");
		return PMIC_ERROR;
	}

#ifdef PULLUP_WAKEPIN_IF_LOW//[
	if(0==tps6518x->gpio_pmic_wakeup) {
		dev_warn(&tps6518x_client->dev,
			"tps6518x wakeup gpio not avalible !!\n");
	}
	else {
		if(0==gpio_get_value(tps6518x->gpio_pmic_wakeup)) 
		{

			dev_warn(&tps6518x_client->dev,
				"%s(%d, ) with wakeup=0 !!!\n",__FUNCTION__,reg_num);
			tps6518x_chip_power(tps6518x,1,1,-1);
			printk("%s(%d): 6518x wakeup=%d\n",__FUNCTION__,__LINE__,
					gpio_get_value(tps6518x->gpio_pmic_wakeup));
		}
	}
#endif //]PULLUP_WAKEPIN_IF_LOW	
	

	if(tps6518x->dwSafeTickToCommunication && time_before(jiffies,tps6518x->dwSafeTickToCommunication)) {
		unsigned long dwTicks = tps6518x->dwSafeTickToCommunication-jiffies;	
		msleep(jiffies_to_msecs(dwTicks));
		dev_info(&tps6518x_client->dev,"msleep %ld ticks for resume times\n",dwTicks);
	}
	result = i2c_smbus_read_byte_data(tps6518x_client, reg_num);
	if (result < 0) {
		dev_err(&tps6518x_client->dev,
			"Unable to read tps6518x register%d via I2C\n",reg_num);
		return PMIC_ERROR;
	}

	*reg_val = result;
	return PMIC_SUCCESS;
}

EXPORT_SYMBOL(tps6518x_reg_read);

int tps6518x_reg_write(struct tps6518x *tps6518x,int reg_num, const unsigned int reg_val)
{
	int result;
	struct i2c_client *tps6518x_client=tps6518x->i2c_client;

	if (tps6518x_client == NULL) {
		dev_err(&tps6518x_client->dev,
			"tps6518x I2C adaptor not ready !\n");
		return PMIC_ERROR;
	}

#ifdef PULLUP_WAKEPIN_IF_LOW//[
	if(0==tps6518x->gpio_pmic_wakeup) {
		dev_warn(&tps6518x_client->dev,
			"tps6518x wakeup gpio not avalible !!\n");
	}
	else {
		if(0==gpio_get_value(tps6518x->gpio_pmic_wakeup)) 
		{
			dev_warn(&tps6518x_client->dev,
				"%s(%d,0x%x): with wakeup=0 !!!\n",
				__FUNCTION__,reg_num,reg_val);

			tps6518x_chip_power(tps6518x,1,1,-1);

			printk("%s(%d): 6518x wakeup=%d\n",__FUNCTION__,__LINE__,
					gpio_get_value(tps6518x->gpio_pmic_wakeup));
		}
	}
#endif //]PULLUP_WAKEPIN_IF_LOW

	if(tps6518x->dwSafeTickToCommunication && time_before(jiffies,tps6518x->dwSafeTickToCommunication)) {
		unsigned long dwTicks = tps6518x->dwSafeTickToCommunication-jiffies;	
		msleep(jiffies_to_msecs(dwTicks));
		dev_info(&tps6518x_client->dev,"msleep %ld ticks for resume times\n",dwTicks);
	}
	result = i2c_smbus_write_byte_data(tps6518x_client, reg_num, reg_val);
	if (result < 0) {
		dev_err(&tps6518x_client->dev,
			"Unable to write TPS6518x register%d via I2C\n",reg_num);
		return PMIC_ERROR;
	}

	return PMIC_SUCCESS;
}

EXPORT_SYMBOL(tps6518x_reg_write);

int tps6518x_chip_power(struct tps6518x *tps6518x,int iIsON,int iIsWakeup,int iIsRailsON)
{
	int iPwrallCurrentStat=-1;
	int iWakeupCurrentStat=-1;
	int iRailsCurrentStat=-1;
	int iRet = 0;
	unsigned int dwDummy;
	int iChk,iRetryCnt;
	const int iRetryCntMax = 10;
	//unsigned int cur_reg; /* current register value to modify */
	//unsigned int new_reg_val; /* new register value to write */
	//unsigned int fld_mask;	  /* register mask for bitfield to modify */
	//unsigned int fld_val;	  /* new bitfield value to write */

	//printk("%s(%d,%d,%d)\n",__FUNCTION__,iIsON,iIsWakeup,iIsRailsON);

	if(!tps6518x) {
		printk(KERN_ERR"%s(): object error !! \n",__FUNCTION__);
		return -1;
	}

	if (!gpio_is_valid(tps6518x->gpio_pmic_pwrall)) {
		printk(KERN_ERR"%s(): no epdc pmic pwrall pin available\n",__FUNCTION__);
		return -2;
	}


	iPwrallCurrentStat = gpio_get_value(tps6518x->gpio_pmic_pwrall);
	iWakeupCurrentStat = gpio_get_value(tps6518x->gpio_pmic_wakeup);
	iRailsCurrentStat = gpio_get_value(tps6518x->gpio_pmic_powerup);

	if(1==iIsON) {
		gpio_set_value(tps6518x->gpio_pmic_pwrall,1);
		if(iPwrallCurrentStat!=1) {
			msleep(4);
		}

		if(1==iIsWakeup) {
			if(iWakeupCurrentStat!=1) {

				//printk("%s(%d),wakeup set 1\n",__FUNCTION__,__LINE__);
		//		i2c_lock_adapter (tps6518x->i2c_client->adapter);
				gpio_set_value(tps6518x->gpio_pmic_wakeup,1);
		//		i2c_unlock_adapter (tps6518x->i2c_client->adapter);
				//msleep(4);

				tps6518x->dwSafeTickToCommunication = jiffies+TPS6518X_WAKEUP_WAIT_TICKS;
				/*
				tps6518x_reg_read(tps6518x,REG_TPS65180_ENABLE,&cur_reg);
				fld_mask = BITFMASK(STANDBY);
				fld_val = BITFVAL(STANDBY,false);
				new_reg_val = to_reg_val(cur_reg, fld_mask, fld_val);
				tps6518x_reg_write(tps6518x,REG_TPS65180_ENABLE, new_reg_val);
				*/

				// dummy INT registers .
				iRetryCnt = 0;
				for (iRetryCnt=0;iRetryCnt<=iRetryCntMax;iRetryCnt++) {
					iChk = tps6518x_reg_read(tps6518x,REG_TPS65180_INT1,&dwDummy);
					if(PMIC_ERROR == iChk) {
						printk(KERN_WARNING"%s(): i2c communication error !retry %d/%d\n",
							__FUNCTION__,iRetryCnt,iRetryCntMax);
						msleep(2);
					}
					else {
						break;
					}
				}
				tps6518x_reg_read(tps6518x,REG_TPS65180_INT2,&dwDummy);
#if 1
				// restore registers here ....
// FIXME: this is a dependency of core from tps6518x-regulator
				tps6518x_setup_timings(tps6518x);
				tps6518x->timing_need_restore = 0;
#endif

				iRet = 2;
			}
			else {
				//tps6518x_reg_read(tps6518x,REG_TPS65180_ENABLE,&cur_reg);
				iRet = 3;
			}


			if(1==iIsRailsON) {
				/*
				fld_mask = BITFMASK(ACTIVE);
				fld_val = BITFVAL(ACTIVE,true);
				new_reg_val = to_reg_val(cur_reg, fld_mask, fld_val);
				tps6518x_reg_write(tps6518x,REG_TPS65180_ENABLE, new_reg_val);
				*/

				gpio_set_value(tps6518x->gpio_pmic_powerup,1);
			}
			else if(0==iIsRailsON){
				if(1==iRailsCurrentStat) {

					/*
					fld_mask = BITFMASK(ACTIVE);
					fld_val = BITFVAL(ACTIVE,false);
					new_reg_val = to_reg_val(cur_reg, fld_mask, fld_val);
					tps6518x_reg_write(tps6518x,REG_TPS65180_ENABLE, new_reg_val);
					*/

					gpio_set_value(tps6518x->gpio_pmic_powerup,0);
					tps6518x_reg_read(tps6518x,REG_TPS65180_INT1,&dwDummy);
					tps6518x_reg_read(tps6518x,REG_TPS65180_INT2,&dwDummy);
				}
			}
			//printk("%s(%d),%d->%d\n",__FUNCTION__,__LINE__,iWakeupCurrentStat,gpio_get_value(tps6518x->gpio_pmic_wakeup));

		}
		else {
			/*
			if(1==iRailsCurrentStat) {
				tps6518x_reg_read(tps6518x,REG_TPS65180_ENABLE,&cur_reg);

				fld_mask = BITFMASK(ACTIVE);
				fld_val = BITFVAL(ACTIVE,false);
				new_reg_val = to_reg_val(cur_reg, fld_mask, fld_val);
				tps6518x_reg_write(tps6518x,REG_TPS65180_ENABLE, new_reg_val);
			}
			*/
			gpio_set_value(tps6518x->gpio_pmic_powerup,0);


			/*
			if(1==iWakeupCurrentStat) {
				tps6518x_reg_read(tps6518x,REG_TPS65180_INT1,&dwDummy);
				tps6518x_reg_read(tps6518x,REG_TPS65180_INT2,&dwDummy);


				fld_mask = BITFMASK(STANDBY);
				fld_val = BITFVAL(STANDBY,true);
				new_reg_val = to_reg_val(cur_reg, fld_mask, fld_val);
				tps6518x_reg_write(tps6518x,REG_TPS65180_ENABLE, new_reg_val);
			}
			*/

			gpio_set_value(tps6518x->gpio_pmic_wakeup,0);

			tps6518x->timing_need_restore = 1;//need restore regs .
			tps6518x->vcom_setup = 0;//need setup vcom again .
			tps6518x->iRegEnable = 0;
			iRet = 1;
		}
	}
	else {
		/*
		if(1==iWakeupCurrentStat) {
			tps6518x_reg_read(tps6518x,REG_TPS65180_ENABLE,&cur_reg);

			fld_mask = BITFMASK(ACTIVE);
			fld_val = BITFVAL(ACTIVE,false);
			new_reg_val = to_reg_val(cur_reg, fld_mask, fld_val);
			tps6518x_reg_write(tps6518x,REG_TPS65180_ENABLE, new_reg_val);
		}
		*/
		gpio_set_value(tps6518x->gpio_pmic_powerup,0);

		
		if(1==iWakeupCurrentStat) {
			tps6518x_reg_read(tps6518x,REG_TPS65180_INT1,&dwDummy);
			tps6518x_reg_read(tps6518x,REG_TPS65180_INT2,&dwDummy);


			/*
			fld_mask = BITFMASK(STANDBY);
			fld_val = BITFVAL(STANDBY,true);
			new_reg_val = to_reg_val(cur_reg, fld_mask, fld_val);
			tps6518x_reg_write(tps6518x,REG_TPS65180_ENABLE, new_reg_val);
			*/
		}
		gpio_set_value(tps6518x->gpio_pmic_wakeup,0);
		tps6518x->timing_need_restore = 1;//need restore regs .
		tps6518x->vcom_setup = 0;//need setup vcom again .
		tps6518x->iRegEnable = 0;

		gpio_set_value(tps6518x->gpio_pmic_pwrall,0);

		iRet = 0;
	}

	return iRet;
}

EXPORT_SYMBOL(tps6518x_chip_power);

#ifdef CONFIG_OF
static struct tps6518x_platform_data *tps6518x_i2c_parse_dt_pdata(
					struct device *dev)
{
	struct tps6518x_platform_data *pdata;
	struct device_node *pmic_np;
	struct tps6518x *tps6518x = dev_get_drvdata(dev);
	int ret;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(dev, "could not allocate memory for pdata\n");
		return ERR_PTR(-ENOMEM);
	}

	pmic_np = of_node_get(dev->of_node);
	if (!pmic_np) {
		dev_err(dev, "could not find pmic sub-node\n");
		return ERR_PTR(-ENODEV);
	}

	tps6518x->gpio_pmic_pwrall = of_get_named_gpio(pmic_np,
					"gpio_pmic_pwrall", 0);
	if (!gpio_is_valid(tps6518x->gpio_pmic_pwrall)) {
		dev_err(dev, "no epdc pmic pwrall pin available\n");
		return ERR_PTR(-ENODEV);
	}
	else {
		printk("%s():gpio_pmic_pwrall=%d\n",__FUNCTION__,tps6518x->gpio_pmic_pwrall);
	}
	ret = devm_gpio_request_one(dev, tps6518x->gpio_pmic_pwrall,
				GPIOF_OUT_INIT_HIGH, "epdc-pmic-pwrall");
	if (ret < 0) {
		dev_err(dev, "request pwrall gpio failed (%d)!\n",ret);
		//goto err;
	}
	else {
		printk("%s():gpio_pmic_pwrall init set 1\n",__FUNCTION__);
	}

	

	tps6518x->gpio_pmic_wakeup = of_get_named_gpio(pmic_np,
					"gpio_pmic_wakeup", 0);
	if (!gpio_is_valid(tps6518x->gpio_pmic_wakeup)) {
		dev_err(dev, "no epdc pmic wakeup pin available\n");
		return ERR_PTR(-ENODEV);
	}
	else {
		printk("%s():gpio_pmic_wakeup=%d\n",__FUNCTION__,tps6518x->gpio_pmic_wakeup);
	}
	ret = devm_gpio_request_one(dev, tps6518x->gpio_pmic_wakeup,
				GPIOF_OUT_INIT_HIGH, "epdc-pmic-wake");
	if (ret < 0) {
		dev_err(dev, "request wakeup gpio failed (%d)!\n",ret);
		//goto err;
	}
	else {
		printk("%s():gpio_pmic_wakeup init set 1\n",__FUNCTION__);
		tps6518x->dwSafeTickToCommunication = jiffies+TPS6518X_WAKEUP_WAIT_TICKS;
	}

	tps6518x->gpio_pmic_vcom_ctrl = of_get_named_gpio(pmic_np,
					"gpio_pmic_vcom_ctrl", 0);
	if (!gpio_is_valid(tps6518x->gpio_pmic_vcom_ctrl)) {
		dev_err(dev, "no epdc pmic vcom_ctrl pin available\n");
	}
	ret = devm_gpio_request_one(dev, tps6518x->gpio_pmic_vcom_ctrl,
				GPIOF_OUT_INIT_LOW, "epdc-vcom");
	if (ret < 0) {
		dev_err(dev, "request vcom gpio failed (%d)!\n",ret);
	}

	tps6518x->gpio_pmic_powerup = of_get_named_gpio(pmic_np,
					"gpio_pmic_powerup", 0);
	if (!gpio_is_valid(tps6518x->gpio_pmic_powerup)) {
		dev_err(dev, "no epdc pmic powerup pin available\n");
	}
	else {
		printk("%s():gpio_pmic_powerup=%d\n",__FUNCTION__,tps6518x->gpio_pmic_powerup);
	}
	ret = devm_gpio_request_one(dev, tps6518x->gpio_pmic_powerup,
				GPIOF_OUT_INIT_LOW, "epdc-powerup");
	if (ret < 0) {
		dev_err(dev, "request powerup gpio failed (%d)!\n",ret);
	}
	else {
	}


	return pdata;
}
#else
static struct tps6518x_platform_data *tps6518x_i2c_parse_dt_pdata(
					struct device *dev)
{
	return NULL;
}
#endif	/* !CONFIG_OF */

static int tps6518x_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
	struct tps6518x *tps6518x;
	struct tps6518x_platform_data *pdata = client->dev.platform_data;
	struct device_node *np = client->dev.of_node;
	int ret = 0;

	printk("tps6518x_probe calling\n");

	if (!np)
		return -ENODEV;


/*	
	gpio_regulator = devm_regulator_get(&client->dev, "SENSOR");
	if (!IS_ERR(gpio_regulator)) {
		ret = regulator_enable(gpio_regulator);
		if (ret) {
			dev_err(&client->dev, "PMIC power on failed !\n");
			return ret;
		}
	}
*/


	/* Create the PMIC data structure */
	tps6518x = kzalloc(sizeof(struct tps6518x), GFP_KERNEL);
	if (tps6518x == NULL) {
		kfree(client);
		return -ENOMEM;
	}

	tps6518x->vcom_setup = 0;// need setup vcom again .
	tps6518x->timing_need_restore = 0;//  .

	/* Initialize the PMIC data structure */
	i2c_set_clientdata(client, tps6518x);
	tps6518x->dev = &client->dev;
	tps6518x->i2c_client = client;


	if (tps6518x->dev->of_node) {
		pdata = tps6518x_i2c_parse_dt_pdata(tps6518x->dev);
		if (IS_ERR(pdata)) {
			ret = PTR_ERR(pdata);
			goto err1;
		}

	}


	if (!gpio_is_valid(tps6518x->gpio_pmic_pwrall)) {
		dev_err(&client->dev, "pwrall gpio not available !!\n");
		goto err1;
	}

	if(1!=gpio_get_value(tps6518x->gpio_pmic_pwrall)) {
		dev_info(&client->dev, "turning on the PMIC chip power ...\n");
		gpio_set_value(tps6518x->gpio_pmic_pwrall,1);
		mdelay(2);
	}

	if (!gpio_is_valid(tps6518x->gpio_pmic_wakeup)) {
		dev_err(&client->dev, "wakeup gpio not available !!\n");
		goto err1;
	}

	if (!gpio_get_value(tps6518x->gpio_pmic_wakeup)) 
	{
		dev_info(&client->dev, "wake on PMIC ...\n");
		gpio_set_value(tps6518x->gpio_pmic_wakeup,1);
		tps6518x->dwSafeTickToCommunication = jiffies+TPS6518X_WAKEUP_WAIT_TICKS;
		//mdelay(TPS6518X_WAKEUP_MS);
	}

	ret = tps6518x_detect(client, NULL);
	if (ret)
		goto err1;

	mfd_add_devices(tps6518x->dev, -1, tps6518x_devs,
			ARRAY_SIZE(tps6518x_devs),
			NULL, 0, NULL);

	tps6518x->pdata = pdata;

	dev_info(&client->dev, "PMIC TPS6518x for eInk display\n");

	printk("tps6518x_probe success\n");

	return ret;

err2:
	mfd_remove_devices(tps6518x->dev);
err1:
	kfree(tps6518x);

	return ret;
}


static int tps6518x_remove(struct i2c_client *i2c)
{
	struct tps6518x *tps6518x = i2c_get_clientdata(i2c);

	mfd_remove_devices(tps6518x->dev);

	return 0;
}

static int gSleep_Mode_Suspend = 0;

static int tps6518x_suspend(struct device *dev)
{
	struct i2c_client *client = i2c_verify_client(dev);    
	struct tps6518x *tps6518x = i2c_get_clientdata(client);

	//printk("%s()\n",__FUNCTION__);

	if(gpio_get_value(tps6518x->gpio_pmic_powerup))
	{
		dev_warn(dev,"suspend skipped ! rails power must be powered off !!\n");
		return -1;
	}

	gpio_set_value(tps6518x->gpio_pmic_vcom_ctrl,0);


	if (gSleep_Mode_Suspend) {
		tps6518x_chip_power(tps6518x,0,0,0);
		
		if (gpio_is_valid(tps6518x->gpio_pmic_v3p3)) {
			gpio_set_value(tps6518x->gpio_pmic_v3p3,0);
		}

	}
	else {
		tps6518x_chip_power(tps6518x,1,1,0);
	}

	return 0;
}


static int tps6518x_resume(struct device *dev)
{
	struct i2c_client *client = i2c_verify_client(dev);    
	struct tps6518x *tps6518x = i2c_get_clientdata(client);
	int iChk;
	
	//printk("%s()\n",__FUNCTION__);

	if (gpio_is_valid(tps6518x->gpio_pmic_v3p3)) {
		gpio_set_value(tps6518x->gpio_pmic_v3p3, 1);
	}

	iChk = tps6518x_chip_power(tps6518x,1,1,0);


#if 0
	// connot use I2C communications at this moment , the I2C pads not resumed .
	if(1==iChk) {
		int iRetryMax=10;
		int i;
		for (i=0;i<iRetryMax;i++) {
			if(tps6518x_restore_vcom(tps6518x)>=0) {
				break;
			}
			else {
				printk("vcom restore retry #%d/%d\n",i+1,iRetryMax);
				mdelay(10);
			}
		}
	}
#endif

#if 0
	gpio_set_value(tps6518x->gpio_pmic_powerup,1);
#endif

	return 0;
}


#ifdef LOWLEVEL_SUSPEND //[
static int tps6518x_suspend_late(struct device *dev)
{
	return 0;
}
static int tps6518x_resume_early(struct device *dev)
{
	return 0;
}
#endif //]LOWLEVEL_SUSPEND


/* Return 0 if detection is successful, -ENODEV otherwise */
static int tps6518x_detect(struct i2c_client *client,
			  struct i2c_board_info *info)
{
	//struct tps6518x_platform_data *pdata = client->dev.platform_data;
	struct i2c_adapter *adapter = client->adapter;
	struct tps6518x *tps6518x = i2c_get_clientdata(client);
	u8 revId;

	const int iMaxRetryCnt = 10;
	int iRetryN;
	int iIsDeviceReady;

	int iStatus ;

	printk("tps6518x_detect calling\n");

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&adapter->dev,"I2C adapter error! \n");
		return -ENODEV;
	}

	for (iIsDeviceReady=0,iRetryN=1;iRetryN<=iMaxRetryCnt;iRetryN++)
	{
		/* identification */
		iStatus = i2c_smbus_read_byte_data(client,REG_TPS6518x_REVID);
		if(iStatus>=0) {
			iIsDeviceReady = 1;
			tps6518x->revID = revId = (u8)iStatus;
			break;
		}
		else {
			msleep(2);
			dev_err(&adapter->dev,
					"Device probe no ACK , retry %d/%d ... \n",iRetryN,iMaxRetryCnt);
		}
	}

	if(!iIsDeviceReady) {
		dev_err(&adapter->dev,
		    "Device no ACK and retry %d times failed \n",iMaxRetryCnt);
		return -ENODEV;
	}

	printk("%s():revId=0x%x\n",__FUNCTION__,tps6518x->revID);

	

	/*
	 * Known rev-ids
	 * tps165180 pass 1 = 0x50, tps65180 pass2 = 0x60, tps65181 pass1 = 0x51, tps65181 pass2 = 0x61, 
	 * tps65182, 
	 * tps65185 pass0 = 0x45, tps65186 pass0 0x46, tps65185 pass1 = 0x55, tps65186 pass1 0x56, tps65185 pass2 = 0x65, tps65186 pass2 0x66
	 */
	if (!((revId == TPS65180_PASS1) ||
		 (revId == TPS65181_PASS1) ||
		 (revId == TPS65180_PASS2) ||
		 (revId == TPS65181_PASS2) ||
		 (revId == TPS65185_PASS0) ||
		 (revId == TPS65186_PASS0) ||
		 (revId == TPS65185_PASS1) ||
		 (revId == TPS65186_PASS1) ||
		 (revId == TPS65185_PASS2) ||
		 (revId == TPS65186_PASS2)))
	{
		dev_info(&adapter->dev,
		    "Unsupported chip (Revision ID=0x%02X).\n",  revId);
		return -ENODEV;
	}

	if (info) {
		strlcpy(info->type, "tps6518x_sensor", I2C_NAME_SIZE);
	}

	printk("tps6518x_detect success\n");
	return 0;
}

static const struct i2c_device_id tps6518x_id[] = {
	{ "tps6518x", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tps6518x_id);

static const struct of_device_id tps6518x_dt_ids[] = {
	{
		.compatible = "ti,tps6518x",
		.data = (void *) &tps6518x_id[0],
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, tps6518x_dt_ids);

//#define LOWLEVEL_SUSPEND		1
static const struct dev_pm_ops tps6518x_dev_pm= {
	.suspend = tps6518x_suspend,
	.resume = tps6518x_resume,
#ifdef LOWLEVEL_SUSPEND //[
	.suspend_late = tps6518x_suspend_late,
	.resume_early = tps6518x_resume_early,
#endif //]LOWLEVEL_SUSPEND
};


static struct i2c_driver tps6518x_driver = {
	.driver = {
		   .name = "tps6518x",
		   .owner = THIS_MODULE,
		   .of_match_table = tps6518x_dt_ids,
		   .pm = (&tps6518x_dev_pm),
	},
	.probe = tps6518x_probe,
	.remove = tps6518x_remove,
	.id_table = tps6518x_id,
	.detect = tps6518x_detect,
	.address_list = &normal_i2c[0],
};

static int __init tps6518x_init(void)
{
	return i2c_add_driver(&tps6518x_driver);
}

static void __exit tps6518x_exit(void)
{
	i2c_del_driver(&tps6518x_driver);
}



/*
 * Module entry points
 */
subsys_initcall(tps6518x_init);
module_exit(tps6518x_exit);

MODULE_AUTHOR("???");
MODULE_DESCRIPTION("TPS6518xx PMIC for Electronic Paper Display driver");
MODULE_LICENSE("GPL");
