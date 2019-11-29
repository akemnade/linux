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
 * tps65185.c
 *
 * Based on the MAX1619 driver.
 * Copyright (C) 2003-2004 Alexey Fisher <fishor@mail.ru>
 *                         Jean Delvare <khali@linux-fr.org>
 *
 * The TPS65185 is a sensor chip made by Texass Instruments.
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
#include <linux/mfd/tps6518x.h>

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
static int tps6518x_sensor_probe(struct platform_device *pdev);
static int tps6518x_sensor_remove(struct platform_device *pdev);

static const struct platform_device_id tps6518x_sns_id[] = {
	{ "tps6518x-sns", 0},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(platform, tps6518x_sns_id);

/*
 * Driver data (common to all clients)
 */
static struct platform_driver tps6518x_sensor_driver = {
	.probe = tps6518x_sensor_probe,
	.remove = tps6518x_sensor_remove,
	.id_table = tps6518x_sns_id,
	.driver = {
		.name = "tps6518x_sensor",
	},
};


/*
 * Client data (each client gets its own)
 */
struct tps6518x_data {
	struct device *hwmon_dev;
	struct tps6518x *tps6518x;
};



int tps6518x_get_temperature(struct tps6518x *tps6518x,int *O_piTemperature)
{
	int iTemp;
	unsigned int reg_val;

	if(!tps6518x) {
		printk(KERN_ERR"%s(),65185 object error !!\n",__FUNCTION__);
		return -1;
	}

	/*
	 * begin Temperature conversion
	 */
	switch (tps6518x->revID & 0xff)
	{
	   case TPS65180_PASS1 :
	   case TPS65180_PASS2 :
	   case TPS65181_PASS1 :
	   case TPS65181_PASS2 :
		    reg_val = 0x80;
		    tps6518x_reg_write(tps6518x,REG_TPS65180_TMST_CONFIG, reg_val);
		    // wait for completion completed
		    while ((0x20 & reg_val) == 0)
		    {
		        msleep(1);
		        tps6518x_reg_read(tps6518x,REG_TPS65180_TMST_CONFIG, &reg_val);
		    }
	        break;
	   case TPS65185_PASS0 :
	   case TPS65186_PASS0 :
	   case TPS65185_PASS1 :
	   case TPS65186_PASS1 :
	   case TPS65185_PASS2 :
	   case TPS65186_PASS2 :
		    reg_val = 0x80;
		    tps6518x_reg_write(tps6518x,REG_TPS65185_TMST1, reg_val);
		    // wait for completion completed
		    while ((0x20 & reg_val) == 0)
		    {
		        msleep(1);
		        tps6518x_reg_read(tps6518x,REG_TPS65185_TMST1, &reg_val);
		    }
	        break;
	   default:
		break;	

	}


	tps6518x_reg_read(tps6518x,REG_TPS6518x_TMST_VAL, &reg_val);
	iTemp = temp_from_reg(reg_val);

	if(O_piTemperature) {
		printk("%s():temperature = %d\n",__FUNCTION__,iTemp);
		*O_piTemperature = iTemp;
	}
	
	return 0;
}

EXPORT_SYMBOL(tps6518x_get_temperature);

/*
 * Sysfs stuff
 */
static ssize_t show_temp_input(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tps6518x_data *data = dev_get_drvdata(dev);
	int iTemp ;

	if(0==tps6518x_get_temperature(data->tps6518x,&iTemp)) {
		return snprintf(buf, PAGE_SIZE, "%d\n", iTemp);
	}
	else {
		return snprintf(buf, PAGE_SIZE, "?\n");
	}
}

static ssize_t show_intr_regs(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	unsigned int reg_val;
	unsigned int intr_reg_val;
	struct tps6518x_data *data = dev_get_drvdata(dev);
	/*
	 * get the interrupt status register value
	 */
	switch (data->tps6518x->revID)
	{
	   case TPS65180_PASS1 :
	   case TPS65180_PASS2 :
	   case TPS65181_PASS1 :
	   case TPS65181_PASS2 :
		    tps6518x_reg_read(data->tps6518x,REG_TPS65180_INT1, &intr_reg_val);
		    tps6518x_reg_read(data->tps6518x,REG_TPS65180_INT2, &reg_val);
		    intr_reg_val |= reg_val<<8;
	        break;
	   case TPS65185_PASS0 :
	   case TPS65186_PASS0 :
	   case TPS65185_PASS1 :
	   case TPS65186_PASS1 :
	   case TPS65185_PASS2 :
	   case TPS65186_PASS2 :
		    tps6518x_reg_read(data->tps6518x,REG_TPS65185_INT1, &intr_reg_val);
		    tps6518x_reg_read(data->tps6518x,REG_TPS65185_INT2, &reg_val);
		    intr_reg_val |= reg_val<<8;
	        break;
	   default:
		break;	

	}

	return snprintf(buf, PAGE_SIZE, "0x%04X\n", intr_reg_val);
}



int tps6518x_set_vcom(struct tps6518x *tps6518x,int iVCOMmV,int iIsWriteToFlash)
{
	//long vcom_reg_val ;
	int iRet = 0;
	unsigned int regINT_EN1=0,regVCOM1=0, regVCOM2=0, regINT1=0;
	unsigned short wVCOM_val ;

	if(!tps6518x) {
		printk(KERN_ERR"%s(),65185 object error !!\n",__FUNCTION__);
		return -1;
	}

	if(iVCOMmV>0) {
		printk(KERN_ERR"%s(),VCOMmV cannot <=0 !!\n",__FUNCTION__);
		return -2;
	}


	wVCOM_val = (unsigned short)((-iVCOMmV)/10);
	dev_dbg(tps6518x->dev, "vcom=>%dmV,wVCOM_val=0x%x,6518x regID=0x%x\n",
			iVCOMmV,wVCOM_val,tps6518x->revID);

	/*
	 * get the interrupt status register value
	 */
	switch (tps6518x->revID)
	{
	   case TPS65180_PASS1 :
	   case TPS65180_PASS2 :
	   case TPS65181_PASS1 :
	   case TPS65181_PASS2 :
		    tps6518x_reg_write(tps6518x,REG_TPS65180_VCOM_ADJUST, wVCOM_val&0xff);
	        break;
	   case TPS65185_PASS0 :
	   case TPS65186_PASS0 :
	   case TPS65185_PASS1 :
	   case TPS65186_PASS1 :
	   case TPS65185_PASS2 :
	   case TPS65186_PASS2 :

				if(tps6518x_reg_read(tps6518x,REG_TPS65185_VCOM2,&regVCOM2)<0) {
					printk(KERN_ERR"%s():TPS6518x VCOM2 reading error !\n",__FUNCTION__);
					iRet = -4;
					break;
				}

#if 0
				// make sure set the VCOM at standby mode .
				if(1==tps6518x->gpio_pmic_powerup_stat) {
					
					printk(KERN_ERR"%s(),WARNING : VCOM setting with powerup=1,force set to standby .\n",
							__FUNCTION__);

					gpio_set_value(tps6518x->gpio_pmic_powerup,0);
					tps6518x->gpio_pmic_powerup_stat = 0;
				}
#endif


				regVCOM1 = (unsigned char)(wVCOM_val&0xff);
				if(tps6518x_reg_write(tps6518x,REG_TPS65185_VCOM1, regVCOM1)<0) {
					dev_err(tps6518x->dev, "write regVCOM1=0x%x failed\n",regVCOM1);
					iRet = -5;
				}

				if(wVCOM_val&0x100) {
					regVCOM2 |= 1;
				}
				else {
					regVCOM2 &= ~1;
				}

		    if(tps6518x_reg_write(tps6518x,REG_TPS65185_VCOM2, regVCOM2)<0) {
					dev_err(tps6518x->dev, "write regVCOM2=0x%x failed\n",regVCOM2);
					iRet = -5;
				}

				dev_dbg(tps6518x->dev, "write regVCOM1=0x%x,regVCOM2=0x%x\n",regVCOM1,regVCOM2);

				if(iRet>=0) {
					tps6518x->vcom_uV = iVCOMmV*1000;
					tps6518x->vcom_setup = 1;
				}

				if(iIsWriteToFlash) {
					volatile unsigned int tryCnt=0;
#define TPS6518X_WRITE_TO_FLASH_WAITUS_INTERVAL	500
#define TPS6518X_WRITE_TO_FLASH_WAITUS_MAX			1000000

					if(tps6518x_reg_read(tps6518x,REG_TPS65185_INT_EN1,&regINT_EN1)<0) {
						printk(KERN_ERR"%s():TPS6518x INT_EN1 reading error !\n",__FUNCTION__);
						iRet = -3;
						break;
					}

					regINT_EN1 |= 0x01; //PRGC_EN of INT_EN1
		    	tps6518x_reg_write(tps6518x,REG_TPS65185_INT_EN1,regINT_EN1);
					regVCOM2 |= 0x40; //PROG of VCOM2
		    	tps6518x_reg_write(tps6518x,REG_TPS65185_VCOM2,regVCOM2);
					do {
						if(tps6518x_reg_read(tps6518x,REG_TPS65185_INT1,&regINT1)<0) {
							printk(KERN_ERR"%s():TPS6518x INT1 reading error !\n",__FUNCTION__);
							iRet = -5;
							break;
						}
						udelay(TPS6518X_WRITE_TO_FLASH_WAITUS_INTERVAL);
						if(++tryCnt>=(TPS6518X_WRITE_TO_FLASH_WAITUS_MAX/TPS6518X_WRITE_TO_FLASH_WAITUS_INTERVAL)) {
							break;
						}
					} while( !(regINT1&0x01) /* PRGC of INT1 */ );

					if( 0==iRet && !(regINT1&0x01) ) {
						printk(KERN_ERR"%s():wait for PRGC of INT1 timeout !\n",__FUNCTION__);
						iRet = -6;
					}

#if 0


					{
						unsigned char regVCOM1_chk,regVCOM2_chk;

						printk("checking the wrote vcom \n");

						tps6518x_reg_write(tps6518x,REG_TPS65185_VCOM1, 0);
						regVCOM2 &= ~1;
						tps6518x_reg_write(tps6518x,REG_TPS65185_VCOM2,regVCOM2);

						gpio_set_value(tps6518x->gpio_pmic_wakeup,0); // enter sleep mode .
						tps6518x->gpio_pmic_wakeup_stat = 0;
						mdelay(2);
						gpio_set_value(tps6518x->gpio_pmic_wakeup,1); // enter standby mode .
						tps6518x->gpio_pmic_wakeup_stat = 1;
						mdelay(TPS6518X_WAKEUP_MS);

						
						if(tps6518x_reg_read(tps6518x,REG_TPS65185_VCOM2,&regVCOM2_chk)<0) {
							printk(KERN_ERR"%s():TPS6518x VCOM2 reading error !\n",__FUNCTION__);
							iRet = -4;
						}
						else {
							if(regVCOM2!=regVCOM2_chk) {
								printk(KERN_ERR"%s():VCOM2 checking error !\n",__FUNCTION__);
							}
						}

						if(tps6518x_reg_read(tps6518x,REG_TPS65185_VCOM1,&regVCOM1_chk)<0) {
							printk(KERN_ERR"%s():TPS6518x VCOM1 reading error !\n",__FUNCTION__);
							iRet = -4;
						}
						else {
							if(regVCOM1!=regVCOM1_chk) {
								printk(KERN_ERR"%s():VCOM1 checking error !\n",__FUNCTION__);
							}
						}

					}


#endif

				}
	      break;
	   default:
		break;	

	}

	return iRet;
}

EXPORT_SYMBOL(tps6518x_set_vcom);

int tps6518x_get_vcom(struct tps6518x *tps6518x,int *O_piVCOMmV)
{
	unsigned int reg_val;
	unsigned int vcom_reg_val;

	if(!tps6518x) {
		return -1;
	}

	if(!tps6518x->vcom_setup) {
		int iVCOMmV = tps6518x->vcom_uV/1000;
		if(O_piVCOMmV) {
			*O_piVCOMmV = iVCOMmV;
			printk("return cached VCOM=%dmV\n",*O_piVCOMmV);
		}
		else {
			printk(KERN_ERR"%s():parameter error !!\n",__FUNCTION__);
		}
		tps6518x_set_vcom(tps6518x,iVCOMmV,0);
		return 0;
	}

	/*
	 * get the vcom registers
	 */
	switch (tps6518x->revID)
	{
	   case TPS65180_PASS1 :
	   case TPS65180_PASS2 :
	   case TPS65181_PASS1 :
	   case TPS65181_PASS2 :
		    tps6518x_reg_read(tps6518x,REG_TPS65180_VCOM_ADJUST, &vcom_reg_val);
	        break;
	   case TPS65185_PASS0 :
	   case TPS65186_PASS0 :
	   case TPS65185_PASS1 :
	   case TPS65186_PASS1 :
	   case TPS65185_PASS2 :
	   case TPS65186_PASS2 :
		    tps6518x_reg_read(tps6518x,REG_TPS65185_VCOM1, &vcom_reg_val);
		    tps6518x_reg_read(tps6518x,REG_TPS65185_VCOM2, &reg_val);
		    vcom_reg_val |= reg_val<<8;
	        break;
	   default:
		break;	

	}

	if(O_piVCOMmV) {
		*O_piVCOMmV = -vcom_reg_val*10;
	}
	
	return 0;
}

EXPORT_SYMBOL(tps6518x_get_vcom);

static ssize_t show_vcom(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tps6518x_data *data = dev_get_drvdata(dev);
	int iVCOM_mV;

	tps6518x_get_vcom(data->tps6518x,&iVCOM_mV);
	return snprintf(buf, PAGE_SIZE, "%dmV\n",iVCOM_mV);
}

static ssize_t set_vcom(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int reg_val;
	long vcom_reg_val = simple_strtol(buf,NULL,10);
	struct tps6518x_data *data = dev_get_drvdata(dev);

	tps6518x_set_vcom(data->tps6518x,-vcom_reg_val,0);
	return count;
}


static DEVICE_ATTR(temp_input, S_IRUGO, show_temp_input, NULL);
static DEVICE_ATTR(intr_input, S_IRUGO, show_intr_regs, NULL);
static DEVICE_ATTR(vcom_value, S_IWUSR | S_IRUGO, show_vcom, set_vcom);

static struct attribute *tps6518x_attributes[] = {
	&dev_attr_temp_input.attr,
	&dev_attr_intr_input.attr,
	&dev_attr_vcom_value.attr,
	NULL
};

static const struct attribute_group tps6518x_group = {
	.attrs = tps6518x_attributes,
};

/*
 * Real code
 */
static int tps6518x_sensor_probe(struct platform_device *pdev)
{
	struct tps6518x_data *data;
	int err;
    printk("tps6518x_sensor_probe starting\n");

	data = kzalloc(sizeof(struct tps6518x_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}
	data->tps6518x = dev_get_drvdata(pdev->dev.parent);
	/* Register sysfs hooks */
	err = sysfs_create_group(&pdev->dev.kobj, &tps6518x_group);
	if (err)
		goto exit_free;

	data->hwmon_dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		goto exit_remove_files;
	}

	platform_set_drvdata(pdev, data);

    printk("tps6518x_sensor_probe success\n");
	return 0;

exit_remove_files:
	sysfs_remove_group(&pdev->dev.kobj, &tps6518x_group);
exit_free:
	kfree(data);
exit:
	return err;
}

static int tps6518x_sensor_remove(struct platform_device *pdev)
{
	struct tps6518x_data *data = platform_get_drvdata(pdev);

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&pdev->dev.kobj, &tps6518x_group);

	kfree(data);
	return 0;
}

static int __init sensors_tps6518x_init(void)
{
	return platform_driver_register(&tps6518x_sensor_driver);
}
module_init(sensors_tps6518x_init);

static void __exit sensors_tps6518x_exit(void)
{
	platform_driver_unregister(&tps6518x_sensor_driver);
}
module_exit(sensors_tps6518x_exit);

MODULE_DESCRIPTION("TPS6518x sensor driver");
MODULE_LICENSE("GPL");
