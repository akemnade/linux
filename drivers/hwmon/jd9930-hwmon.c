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
 * jd9930-hwmon.c
 *
 * Copyright (C) 2003-2004 Alexey Fisher <fishor@mail.ru>
 *                         Jean Delvare <khali@linux-fr.org>
 *
 * The JD9930 is a sensor chip made by Silergy .
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
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/mfd/jd9930.h>

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
static int jd9930_sensor_probe(struct platform_device *pdev);
static int jd9930_sensor_remove(struct platform_device *pdev);

static const struct platform_device_id jd9930_sns_id[] = {
	{ "jd9930-sns", 0},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(platform, jd9930_sns_id);

/*
 * Driver data (common to all clients)
 */
static struct platform_driver jd9930_sensor_driver = {
	.probe = jd9930_sensor_probe,
	.remove = jd9930_sensor_remove,
	.id_table = jd9930_sns_id,
	.driver = {
		.name = "jd9930_sensor",
	},
};


/*
 * Client data (each client gets its own)
 */
struct jd9930_data {
	struct device *hwmon_dev;
	struct jd9930 *jd9930;
};



int jd9930_get_temperature(struct jd9930 *jd9930,int *O_piTemperature)
{
	int iTemp = 0;
	unsigned int reg_val;
	int iChk;
	struct gpio_desc *gpiod_ts_en;
	int iOldTS_En_stat;


	if(!jd9930) {
		printk(KERN_ERR"%s(),JD9930 object error !!\n",__FUNCTION__);
		return -1;
	}


	iOldTS_En_stat= gpiod_get_value(jd9930->gpio_pmic_ts_en);
	if(0==iOldTS_En_stat) {
		gpiod_set_value(jd9930->gpio_pmic_ts_en,1);
	}

	iChk = JD9930_REG_READ(jd9930,TMST_VALUE);
	if(gpiod_ts_en) {
		if(0==iOldTS_En_stat) {
			gpiod_set_value(jd9930->gpio_pmic_ts_en,0);
		}
	}

	if(iChk>=0) {
		reg_val = iChk;
	}
	else {
		printk(KERN_ERR"%s(),JD9930 temperature read error !!\n",__FUNCTION__);
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
	struct jd9930_data *data = dev_get_drvdata(dev);
	int iTemp ;

	if(0==jd9930_get_temperature(data->jd9930,&iTemp)) {
		return snprintf(buf, PAGE_SIZE, "%d\n", iTemp);
	}
	else {
		return snprintf(buf, PAGE_SIZE, "?\n");
	}
}



int jd9930_set_vcom(struct jd9930 *jd9930,int iVCOMmV,int iIsWriteToFlash)
{
	//long vcom_reg_val ;
	int iRet = 0;
	int iChk;
	unsigned char regVCOM;
	//printk("%s(%d);\n",__FUNCTION__,__LINE__);

	if(!jd9930) {
		printk(KERN_ERR"%s(),JD9930 object error !!\n",__FUNCTION__);
		return -1;
	}

	if(iVCOMmV>0) {
		printk(KERN_ERR"%s(),VCOMmV cannot >0 !!\n",__FUNCTION__);
		return -2;
	}


	regVCOM = (unsigned char)(((-iVCOMmV)*1000)/JD9930_VCOM_STEP_uV);
	dev_dbg(jd9930->dev, "vcom=>%dmV,VCOM reg=0x%x\n",
			iVCOMmV,regVCOM);

	/*
	 * get the interrupt status register value
	 */
	do
	{


		iChk = JD9930_REG_WRITE_EX(jd9930,VCOM_SET,regVCOM);
		if(iChk<0) {
			dev_err(jd9930->dev, "write regVCOM_SET=0x%x failed\n",regVCOM);
			iRet = -5;
		}


		if(iRet>=0) {
			jd9930->vcom_uV = iVCOMmV*1000;
		}


	}while(0);

	//printk("%s(%d);\n",__FUNCTION__,__LINE__);
	return iRet;
}

int jd9930_get_vcom(struct jd9930 *jd9930,int *O_piVCOMmV)
{
	unsigned int reg_val;
	unsigned int vcom_reg_val;
	unsigned short wTemp;
	int iVCOMmV;
	int iChk;

	//printk("%s(%d),jd9930=%p;\n",__FUNCTION__,__LINE__,jd9930);

	if(!jd9930) {
		return -1;
	}

	if(jd9930->need_reinit) {
		iVCOMmV = jd9930->vcom_uV/1000;
		if(O_piVCOMmV) {
			*O_piVCOMmV = iVCOMmV;
			printk("return cached VCOM=%dmV\n",*O_piVCOMmV);
		}
		else {
			printk(KERN_ERR"%s():parameter error !!\n",__FUNCTION__);
		}
		jd9930_set_vcom(jd9930,iVCOMmV,0);
		return 0;
	}

	/*
	 * get the vcom registers
	 */

	iChk = JD9930_REG_READ(jd9930,VCOM_SET);
	if(iChk>=0) {
		vcom_reg_val = iChk;
	}
	else {
		return -1;
	}
	iVCOMmV = ((vcom_reg_val*JD9930_VCOM_STEP_uV)/1000);

	if(O_piVCOMmV) {
		*O_piVCOMmV = iVCOMmV;
	}
	
	printk(KERN_DEBUG"%s(%d) VCOMmV=%d\n",__FUNCTION__,__LINE__,iVCOMmV);
	return 0;
}

static ssize_t show_vcom(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct jd9930_data *data = dev_get_drvdata(dev);
	int iVCOM_mV;

	jd9930_get_vcom(data->jd9930,&iVCOM_mV);
	return snprintf(buf, PAGE_SIZE, "%dmV\n",iVCOM_mV);
}

static ssize_t set_vcom(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	//unsigned int reg_val;
	long vcom_reg_val = simple_strtol(buf,NULL,10);
	struct jd9930_data *data = dev_get_drvdata(dev);

	jd9930_set_vcom(data->jd9930,vcom_reg_val,0);
	return count;
}


static DEVICE_ATTR(temp_input, S_IRUGO, show_temp_input, NULL);
static DEVICE_ATTR(vcom_value, S_IWUSR | S_IRUGO, show_vcom, set_vcom);

static struct attribute *jd9930_attributes[] = {
	&dev_attr_temp_input.attr,
//	&dev_attr_intr_input.attr,
	&dev_attr_vcom_value.attr,
	NULL
};

static const struct attribute_group jd9930_group = {
	.attrs = jd9930_attributes,
};

/*
 * Real code
 */
static int jd9930_sensor_probe(struct platform_device *pdev)
{
	struct jd9930_data *data;
	int err;

	printk("jd9930_sensor_probe starting\n");




	data = kzalloc(sizeof(struct jd9930_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}
	data->jd9930 = dev_get_drvdata(pdev->dev.parent);
	/* Register sysfs hooks */
	err = sysfs_create_group(&pdev->dev.kobj, &jd9930_group);
	if (err)
		goto exit_free;

	data->hwmon_dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		goto exit_remove_files;
	}

	platform_set_drvdata(pdev, data);

	printk("jd9930_sensor_probe success\n");
	return 0;

exit_remove_files:
	sysfs_remove_group(&pdev->dev.kobj, &jd9930_group);
exit_free:
	kfree(data);
exit:
	return err;
}

static int jd9930_sensor_remove(struct platform_device *pdev)
{
	struct jd9930_data *data = platform_get_drvdata(pdev);

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&pdev->dev.kobj, &jd9930_group);

	kfree(data);
	return 0;
}

module_platform_driver(jd9930_sensor_driver);

MODULE_DESCRIPTION("JD9930 sensor driver");
MODULE_LICENSE("GPL");

