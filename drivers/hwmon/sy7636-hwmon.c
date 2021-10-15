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
/*
 * sy7636-hwmon.c
 *
 * Copyright (C) 2003-2004 Alexey Fisher <fishor@mail.ru>
 *                         Jean Delvare <khali@linux-fr.org>
 *
 * The SY7636 is a sensor chip made by Silergy .
 * It reports up to two temperatures (its own plus up to
 * one external one).
 */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/mfd/sy7636.h>

#include <linux/gpio.h>

/*
 * Conversions
 */
static int temp_from_reg(int val)
{
	return val;
}

/*
 * Functions declaration
 */
static int sy7636_sensor_probe(struct platform_device *pdev);
static int sy7636_sensor_remove(struct platform_device *pdev);

static const struct platform_device_id sy7636_sns_id[] = {
	{ "sy7636-sns", 0},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(platform, sy7636_sns_id);

/*
 * Driver data (common to all clients)
 */
static struct platform_driver sy7636_sensor_driver = {
	.probe = sy7636_sensor_probe,
	.remove = sy7636_sensor_remove,
	.id_table = sy7636_sns_id,
	.driver = {
		.name = "sy7636_sensor",
	},
};


/*
 * Client data (each client gets its own)
 */
struct sy7636_data {
	struct device *hwmon_dev;
	struct sy7636 *sy7636;
};



int sy7636_get_temperature(struct sy7636 *sy7636,int *O_piTemperature)
{
	int iTemp = 0;
	unsigned int reg_val;
	int iChk;

	if(!sy7636) {
		printk(KERN_ERR"%s(),SY7636 object error !!\n",__FUNCTION__);
		return -1;
	}

	iChk = SY7636_REG_READ(sy7636,THERM);
	if(iChk>=0) {
		reg_val = iChk;
	}
	else {
		printk(KERN_ERR"%s(),SY7636 temperature read error !!\n",__FUNCTION__);
		return -1;
	}
	iTemp = temp_from_reg(reg_val);

	if(O_piTemperature) {
		printk("%s():temperature = %d,reg=0x%x\n",__FUNCTION__,iTemp,reg_val);
		*O_piTemperature = iTemp;
	}
	
	return 0;
}


/*
 * Sysfs stuff
 */
static ssize_t show_temp_input(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sy7636_data *data = dev_get_drvdata(dev);
	int iTemp ;

	if(0==sy7636_get_temperature(data->sy7636,&iTemp)) {
		return snprintf(buf, PAGE_SIZE, "%d\n", iTemp);
	}
	else {
		return snprintf(buf, PAGE_SIZE, "?\n");
	}
}



int sy7636_set_vcom(struct sy7636 *sy7636,int iVCOMmV,int iIsWriteToFlash)
{
	//long vcom_reg_val ;
	int iRet = 0;
	unsigned int regINT_EN1=0,regVCOM1=0, regVCOM2=0, regINT1=0;
	unsigned short wVCOM_val ;
	int iChk;
	//printk("%s(%d);\n",__FUNCTION__,__LINE__);

	if(!sy7636) {
		printk(KERN_ERR"%s(),SY7636 object error !!\n",__FUNCTION__);
		return -1;
	}

	if(iVCOMmV>0) {
		printk(KERN_ERR"%s(),VCOMmV cannot <=0 !!\n",__FUNCTION__);
		return -2;
	}


	wVCOM_val = (unsigned short)((-iVCOMmV)/10);
	dev_dbg(sy7636->dev, "vcom=>%dmV,wVCOM_val=0x%x\n",
			iVCOMmV,wVCOM_val);

	/*
	 * get the interrupt status register value
	 */
	do
	{

		iChk = SY7636_REG_READ(sy7636,VCOM_ADJ2);
		if(iChk>=0) {
			regVCOM2 = iChk;
		}
		else {
			dev_err(sy7636->dev,"SY7636 VCOM2 reading error !\n");
			iRet = -4;
			break;
		}

		regVCOM1 = (unsigned char)(wVCOM_val&0xff);
		iChk = SY7636_REG_WRITE_EX(sy7636,VCOM_ADJ1,regVCOM1);
		if(iChk<0) {
			dev_err(sy7636->dev, "write regVCOM1=0x%x failed\n",regVCOM1);
			iRet = -5;
		}

		if(wVCOM_val&0x100) {
			regVCOM2 |= 0x80;
		}
		else {
			regVCOM2 &= ~0x80;
		}

		iChk = SY7636_REG_WRITE_EX(sy7636,VCOM_ADJ2,regVCOM2);
		if(iChk<0) {
			dev_err(sy7636->dev, "write regVCOM2=0x%x failed\n",regVCOM2);
			iRet = -5;
		}

		dev_dbg(sy7636->dev, "write regVCOM1=0x%x,regVCOM2=0x%x\n",regVCOM1,regVCOM2);

		if(iRet>=0) {
			sy7636->vcom_uV = iVCOMmV*1000;
		}


	}while(0);

	//printk("%s(%d);\n",__FUNCTION__,__LINE__);
	return iRet;
}

int sy7636_get_vcom(struct sy7636 *sy7636,int *O_piVCOMmV)
{
	unsigned int reg_val;
	unsigned int vcom_reg_val;
	unsigned short wTemp;
	int iVCOMmV;
	int iChk;

	//printk("%s(%d),sy7636=%p;\n",__FUNCTION__,__LINE__,sy7636);

	if(!sy7636) {
		return -1;
	}

	if(sy7636->need_reinit) {
		iVCOMmV = sy7636->vcom_uV/1000;
		if(O_piVCOMmV) {
			*O_piVCOMmV = iVCOMmV;
			printk("return cached VCOM=%dmV\n",*O_piVCOMmV);
		}
		else {
			printk(KERN_ERR"%s():parameter error !!\n",__FUNCTION__);
		}
		sy7636_set_vcom(sy7636,iVCOMmV,0);
		return 0;
	}

	/*
	 * get the vcom registers
	 */

	iChk = SY7636_REG_READ(sy7636,VCOM_ADJ1);
	if(iChk>=0) {
		vcom_reg_val = iChk;
	}
	else {
		return -1;
	}
	iChk = SY7636_REG_READ(sy7636,VCOM_ADJ2);
	if(iChk>=0) {
		reg_val = iChk;
	}
	else {
		return -1;
	}
	wTemp = vcom_reg_val;
	if(reg_val&0x80) {
		wTemp |= 0x100;
	}
	else {
		wTemp &= ~0x100;
	}
	iVCOMmV = -(wTemp*10);

	if(O_piVCOMmV) {
		*O_piVCOMmV = iVCOMmV;
	}
	
	//printk("%s(%d);\n",__FUNCTION__,__LINE__);
	return 0;
}

static ssize_t show_vcom(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sy7636_data *data = dev_get_drvdata(dev);
	int iVCOM_mV;

	sy7636_get_vcom(data->sy7636,&iVCOM_mV);
	return snprintf(buf, PAGE_SIZE, "%dmV\n",iVCOM_mV);
}

static ssize_t set_vcom(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int reg_val;
	long vcom_reg_val = simple_strtol(buf,NULL,10);
	struct sy7636_data *data = dev_get_drvdata(dev);

	sy7636_set_vcom(data->sy7636,-vcom_reg_val,0);
	return count;
}


static DEVICE_ATTR(temp_input, S_IRUGO, show_temp_input, NULL);
static DEVICE_ATTR(vcom_value, S_IWUSR | S_IRUGO, show_vcom, set_vcom);

static struct attribute *sy7636_attributes[] = {
	&dev_attr_temp_input.attr,
//	&dev_attr_intr_input.attr,
	&dev_attr_vcom_value.attr,
	NULL
};

static const struct attribute_group sy7636_group = {
	.attrs = sy7636_attributes,
};

/*
 * Real code
 */
static int sy7636_sensor_probe(struct platform_device *pdev)
{
	struct sy7636_data *data;
	int err;

	printk("sy7636_sensor_probe starting\n");




	data = kzalloc(sizeof(struct sy7636_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}
	data->sy7636 = dev_get_drvdata(pdev->dev.parent);
	/* Register sysfs hooks */
	err = sysfs_create_group(&pdev->dev.kobj, &sy7636_group);
	if (err)
		goto exit_free;

	data->hwmon_dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		goto exit_remove_files;
	}

	platform_set_drvdata(pdev, data);

	printk("sy7636_sensor_probe success\n");
	return 0;

exit_remove_files:
	sysfs_remove_group(&pdev->dev.kobj, &sy7636_group);
exit_free:
	kfree(data);
exit:
	return err;
}

static int sy7636_sensor_remove(struct platform_device *pdev)
{
	struct sy7636_data *data = platform_get_drvdata(pdev);

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&pdev->dev.kobj, &sy7636_group);

	kfree(data);
	return 0;
}

static int __init sensors_sy7636_init(void)
{
	return platform_driver_register(&sy7636_sensor_driver);
}
module_init(sensors_sy7636_init);

static void __exit sensors_sy7636_exit(void)
{
	platform_driver_unregister(&sy7636_sensor_driver);
}
module_exit(sensors_sy7636_exit);

MODULE_DESCRIPTION("SY7636 sensor driver");
MODULE_LICENSE("GPL");

