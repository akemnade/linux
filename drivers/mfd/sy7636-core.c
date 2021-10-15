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
 * @file pmic/core/sy7636.c
 * @brief This file contains SY7636 specific PMIC code. This implementaion
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
#include <linux/mfd/core.h>
#include <linux/mfd/sy7636.h>
#include <asm/mach-types.h>

/*
 * EPDC PMIC I2C address
 */
#define EPDC_PMIC_I2C_ADDR 0x66


#define SY7636_PWRON_READYUS	2000

static int sy7636_detect(struct i2c_client *client, struct i2c_board_info *info);
static struct regulator *gpio_regulator;

static struct mfd_cell sy7636_devs[] = {
	{ .name = "sy7636-pmic", },
	{ .name = "sy7636-sns", },
};

static const unsigned short normal_i2c[] = {EPDC_PMIC_I2C_ADDR, I2C_CLIENT_END};


int sy7636_reg_read(struct sy7636 *sy7636,int reg_num, unsigned int *reg_val)
{
	int result;
	struct i2c_client *sy7636_client = sy7636->i2c_client;

	sy7636_EN(sy7636,1);

	if (sy7636_client == NULL) {
		dev_err(&sy7636_client->dev,
			"sy7636 I2C adaptor not ready !\n");
		return -EINVAL;
	}


	do {
		result = i2c_smbus_read_byte_data(sy7636_client, reg_num);
		if (result < 0) {
			if(!time_is_before_jiffies(sy7636->jiffies_chip_wake)) {
				dev_err(&sy7636_client->dev,
					"Unable to read sy7636 reg%d via I2C after waked , retry...\n",reg_num);
				continue;
			}
			dev_err(&sy7636_client->dev,
				"Unable to read sy7636 register%d via I2C\n",reg_num);
			return -EINVAL;
		}
		else {
			dev_dbg(sy7636->dev, "%s():reg%d=0x%x\n",__FUNCTION__,reg_num,result);
		}
		break;
		mdelay(1);
	}while(0);

	*reg_val = result;


	return PMIC_SUCCESS;
}
EXPORT_SYMBOL(sy7636_reg_read);

int sy7636_reg_write(struct sy7636 *sy7636,int reg_num, const unsigned int reg_val)
{
	int result;
	struct i2c_client *sy7636_client=sy7636->i2c_client;

	sy7636_EN(sy7636,1);

	if (sy7636_client == NULL) {
		dev_err(&sy7636_client->dev,
			"sy7636 I2C adaptor not ready !\n");
		return -ENODEV;
	}

	do {
		result = i2c_smbus_write_byte_data(sy7636_client, reg_num, reg_val);
		if (result < 0) {
			if(!time_is_before_jiffies(sy7636->jiffies_chip_wake)) {
				dev_err(&sy7636_client->dev,
					"Unable to write SY7636 reg%d via I2C after waked! retry ...\n",reg_num);
				continue;
			}
			dev_err(&sy7636_client->dev,
				"Unable to write SY7636 register%d via I2C\n",reg_num);
			return -EIO;
		}
		else {
			dev_dbg(sy7636->dev, "%s():reg%d<=0x%x\n",__FUNCTION__,reg_num,reg_val);
		}
		break;
		mdelay(1);
	}while(1);


	return PMIC_SUCCESS;
}
EXPORT_SYMBOL(sy7636_reg_write);

static const char *gszSY7636_fault_strA[] = 
{
	"no faults", //0
	"UVP at VB rail", //1
	"UVP at VN rail", //2
	"UVP at VPOS rail", //3
	"UVP at VNEG rail", //4
	"UVP at VDDH rail", //5
	"UVP at VEE rail", //6
	"SCP at VB rail", //7
	"SCP at VN rail", //8
	"SCP at VPOS rail", //9
	"SCP at VNEG rail", //A
	"SCP at VDDH rail", //B
	"SCP at VEE rail", //C
	"SCP at VCOM rail", //D
	"UVLO", //E
	"Thermal shutdown", //F
};
const char *sy7636_get_fault_string(unsigned char bFault)
{
	if(bFault>0xf) {
		return "fault code over range !!";
	}
	else {
		return gszSY7636_fault_strA[bFault];
	}
}
EXPORT_SYMBOL(sy7636_get_fault_string);


int sy7636_EN(struct sy7636 *sy7636,int iEN)
{
	int iEN_Old_State;

	if (!gpio_is_valid(sy7636->gpio_pmic_powerup)) {
		dev_err(sy7636->dev, "%s():epdc pmic powerup pin available\n",__FUNCTION__);
		return -1;
	}

	//iEN_Old_State = gpio_get_value(sy7636->gpio_pmic_powerup);
	iEN_Old_State = sy7636->gpio_pmic_powerup_stat;
	if(iEN==iEN_Old_State) {
		return iEN_Old_State;
	}

	if(iEN) {
		gpio_set_value(sy7636->gpio_pmic_powerup,1);sy7636->gpio_pmic_powerup_stat = 1;
		sy7636->jiffies_chip_wake = jiffies+msecs_to_jiffies(SY7636_WAKEUP_MS);
		if(0==iEN_Old_State) {
			dev_dbg(sy7636->dev, "%s():EN 0->1 .\n",__FUNCTION__);
			mdelay(SY7636_WAKEUP0_MS);
			sy7636_regs_init(sy7636);	
			sy7636->need_reinit = 0;
		}
	}
	else {
		sy7636->need_reinit = 1;//need setup vcom/regs again .
		gpio_set_value(sy7636->gpio_pmic_powerup,0);sy7636->gpio_pmic_powerup_stat = 0;
		if(1==iEN_Old_State) {
			dev_dbg(sy7636->dev, "%s():EM 1->0 .\n",__FUNCTION__);
			udelay(SY7636_PWRON_READYUS/2);
		}
	}

	return iEN_Old_State;
}
EXPORT_SYMBOL(sy7636_EN);

int sy7636_chip_power(struct sy7636 *sy7636,int iIsON)
{
	int iPwrallCurrentStat=-1;
	int iRet = 0;
	
	unsigned long waitDelayTime;

	if(!sy7636) {
		printk(KERN_ERR"%s(): object error !! \n",__FUNCTION__);
		return -1;
	}

	if (!gpio_is_valid(sy7636->gpio_pmic_pwrall)) {
		printk(KERN_ERR"%s(): no epdc pmic pwrall pin available\n",__FUNCTION__);
		return -2;
	}

	//iPwrallCurrentStat = sy7636->gpio_pmic_pwrall_stat;
	iPwrallCurrentStat = gpio_get_value(sy7636->gpio_pmic_pwrall);

	if(iIsON) {
		gpio_set_value(sy7636->gpio_pmic_pwrall,1);
		sy7636->jiffies_chip_on = jiffies;
		if(0==iPwrallCurrentStat) {
			dev_dbg(sy7636->dev, "%s():chip power 0->1 .\n",__FUNCTION__);
			udelay(SY7636_PWRON_READYUS);
		}
		sy7636_EN(sy7636,1);
	}
	else {
		sy7636_EN(sy7636,0);

		gpio_set_value(sy7636->gpio_pmic_pwrall,0);
		
		waitDelayTime = jiffies + sy7636->turnoff_delay_ep3v3;
		
		while(time_before(jiffies, waitDelayTime)) {
			//printk("wait delay time for turn off EP_3V3 ...\n");
		}
	}


	if(iPwrallCurrentStat!=iIsON) {
		if(iIsON) {
			// state change and turn on .
			iRet = 1;
		}
		else {
			// state change and turn off .
			iRet = 2;
		}
	}

	return iRet;
}


int sy7636_get_FaultFlags(struct sy7636 *sy7636,int iIsGetCached)
{
	unsigned int dwReg;
	int iRet ;
	int iChk;

	if(!sy7636) {
		pr_err("%s():error object !\n",__FUNCTION__);
		return -2;
	}

	if(iIsGetCached) {
		iRet = (int)SY7636_REG_GET(sy7636,FAULTFLAGS);
	}
	else {
		iChk = SY7636_REG_READ(sy7636,FAULTFLAGS);
		if(iChk>=0) {
			iRet = iChk;
		}
		else {
			iRet = -1;
			dev_err(sy7636->dev, "%s():reading fault flags failed !\n",__FUNCTION__);
		}
	}
	return iRet;
}

int sy7636_get_power_status(struct sy7636 *sy7636,int iIsGetCached,int *O_piPG,unsigned char *O_pbFaults)
{
	int iRet;
	unsigned char bFaultFlags;
	unsigned char bFaults;
	int iPG;

	iRet = sy7636_get_FaultFlags(sy7636,iIsGetCached);

	if(iRet<0) {
		bFaults = 0;
		bFaultFlags = 0;
		iRet = -1;
		return -1;
	}
	else {
		bFaultFlags = (unsigned char)iRet;
		bFaults = BITFEXT(bFaultFlags,FAULTS);
		iPG = BITFEXT(bFaultFlags,PG);
		if(bFaults) {
			iRet = 1;
		}
		else {
			iRet = 0;
		}
	}

	if(O_piPG) {
		*O_piPG = iPG;
	}

	if(O_pbFaults) {
		*O_pbFaults = bFaults;
	}

	return iRet;
}
EXPORT_SYMBOL(sy7636_get_power_status);

#ifdef CONFIG_OF
static struct sy7636_platform_data *sy7636_i2c_parse_dt_pdata(
					struct device *dev)
{
	struct sy7636_platform_data *pdata;
	struct device_node *pmic_np;
	struct sy7636 *sy7636 = dev_get_drvdata(dev);
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

	sy7636->gpio_pmic_pwrall = of_get_named_gpio(pmic_np,
					"gpio_pmic_pwrall", 0);
	if (!gpio_is_valid(sy7636->gpio_pmic_pwrall)) {
		dev_err(dev, "no epdc pmic pwrall pin available\n");
		return ERR_PTR(-ENODEV);
	}
	else {
		printk("%s():gpio_pmic_pwrall=%d\n",__FUNCTION__,sy7636->gpio_pmic_pwrall);
	}
	ret = devm_gpio_request_one(dev, sy7636->gpio_pmic_pwrall,
				GPIOF_OUT_INIT_HIGH, "epdc-pmic-pwrall");
	if (ret < 0) {
		dev_err(dev, "request pwrall gpio failed (%d)!\n",ret);
		//goto err;
	}
	else {
		printk("%s():gpio_pmic_pwrall init set 1\n",__FUNCTION__);
		//sy7636->gpio_pmic_pwrall_stat = 1;
		sy7636->jiffies_chip_on = jiffies;
		udelay(SY7636_PWRON_READYUS);
	}

	

	sy7636->gpio_pmic_vcom_ctrl = of_get_named_gpio(pmic_np,
					"gpio_pmic_vcom_ctrl", 0);
	if (!gpio_is_valid(sy7636->gpio_pmic_vcom_ctrl)) {
		dev_err(dev, "no epdc pmic vcom_ctrl pin available\n");
	}
	ret = devm_gpio_request_one(dev, sy7636->gpio_pmic_vcom_ctrl,
				GPIOF_OUT_INIT_LOW, "epdc-vcom");
	if (ret < 0) {
		dev_err(dev, "request vcom gpio failed (%d)!\n",ret);
	}

	sy7636->gpio_pmic_powerup = of_get_named_gpio(pmic_np,
					"gpio_pmic_powerup", 0);
	if (!gpio_is_valid(sy7636->gpio_pmic_powerup)) {
		dev_err(dev, "no epdc pmic powerup pin available\n");
	}
	else {
		printk("%s():gpio_pmic_powerup=%d\n",__FUNCTION__,sy7636->gpio_pmic_powerup);
	}
	ret = devm_gpio_request_one(dev, sy7636->gpio_pmic_powerup,
				GPIOF_OUT_INIT_LOW, "epdc-powerup");
	sy7636->gpio_pmic_powerup_stat = -1;
	if (ret < 0) {
		dev_err(dev, "request powerup gpio failed (%d)!\n",ret);
	}
	else {
		sy7636->gpio_pmic_powerup_stat = 0;
	}


	return pdata;
}
#else
static struct sy7636_platform_data *sy7636_i2c_parse_dt_pdata(
					struct device *dev)
{
	return NULL;
}
#endif	/* !CONFIG_OF */

static int sy7636_probe(struct i2c_client *client)
{
	struct sy7636 *sy7636;
	struct sy7636_platform_data *pdata = client->dev.platform_data;
	struct device_node *np = client->dev.of_node;
	int ret = 0;


	printk("sy7636_probe calling\n");

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
	sy7636 = kzalloc(sizeof(struct sy7636), GFP_KERNEL);
	if (sy7636 == NULL) {
		kfree(client);
		return -ENOMEM;
	}

	//sy7636->gpio_pmic_powerup_stat = -1;
	//sy7636->gpio_pmic_pwrall_stat = -1;
	sy7636->fake_vp3v3_stat = 0;
	sy7636->need_reinit = 1;// need setup vcom again .
	sy7636->jiffies_chip_on = jiffies;// .
	sy7636->jiffies_chip_wake = jiffies;// .

	sy7636->on_delay1 = 0xff;
	sy7636->on_delay2 = 0xff;
	sy7636->on_delay3 = 0xff;
	sy7636->on_delay4 = 0xff;

	sy7636->VLDO = (unsigned int)-1;

	/* Initialize the PMIC data structure */
	i2c_set_clientdata(client, sy7636);
	sy7636->dev = &client->dev;
	sy7636->i2c_client = client;


	if (sy7636->dev->of_node) {
		pdata = sy7636_i2c_parse_dt_pdata(sy7636->dev);
		if (IS_ERR(pdata)) {
			ret = PTR_ERR(pdata);
			goto err1;
		}

	}


	if (!gpio_is_valid(sy7636->gpio_pmic_pwrall)) {
		dev_err(&client->dev, "pwrall gpio not available !!\n");
		goto err1;
	}

	//if(1!=sy7636->gpio_pmic_pwrall_stat) 
	if(1!=gpio_get_value(sy7636->gpio_pmic_pwrall)) 
	{
		dev_info(&client->dev, "PMIC chip off, turning on the PMIC chip power ...\n");
		gpio_set_value(sy7636->gpio_pmic_pwrall,1);
		sy7636->jiffies_chip_on = jiffies+1;
		//sy7636->gpio_pmic_pwrall_stat = 1;
		udelay(SY7636_PWRON_READYUS);
	}

	ret = sy7636_detect(client, NULL);
	if (ret)
		goto err1;

	mfd_add_devices(sy7636->dev, -1, sy7636_devs,
			ARRAY_SIZE(sy7636_devs),
			NULL, 0, NULL);

	sy7636->pdata = pdata;

	dev_info(&client->dev, "PMIC SY7636 for eInk display\n");

	printk("sy7636_probe success\n");

	return ret;

err2:
	mfd_remove_devices(sy7636->dev);
err1:
	kfree(sy7636);

	return ret;
}


static void sy7636_remove(struct i2c_client *i2c)
{
	struct sy7636 *sy7636 = i2c_get_clientdata(i2c);

	mfd_remove_devices(sy7636->dev);
}

int gSleep_Mode_Suspend = 0;

int sy7636_regs_init(struct sy7636 *sy7636)
{
	int iRet;

	dev_dbg(sy7636->dev, "%s(%d)\n",__FUNCTION__,__LINE__);
	// write registers . 	

	SY7636_REG_WRITE(sy7636,VPDD_LEN);
	SY7636_REG_WRITE(sy7636,VEE_VP_EXT);


	SY7636_REG_WRITE(sy7636,VCOM_ADJ1);
	SY7636_REG_WRITE(sy7636,VCOM_ADJ2);

	SY7636_REG_WRITE(sy7636,VLDO_ADJ);
	SY7636_REG_WRITE(sy7636,PWRON_DLY);


	dev_dbg(sy7636->dev, "%s(%d)\n",__FUNCTION__,__LINE__);

	return iRet;
}
EXPORT_SYMBOL(sy7636_regs_init);

static int sy7636_suspend_late(struct device *dev)
{
	struct i2c_client *client = i2c_verify_client(dev);    
	struct sy7636 *sy7636 = i2c_get_clientdata(client);

	if (gSleep_Mode_Suspend) {
		sy7636_chip_power(sy7636,0);
	}
	return 0;
}


static int sy7636_resume_early(struct device *dev)
{
	struct i2c_client *client = i2c_verify_client(dev);    
	struct sy7636 *sy7636 = i2c_get_clientdata(client);


	return 0;
}

static int sy7636_suspend(struct device *dev)
{
	struct i2c_client *client = i2c_verify_client(dev);    
	struct sy7636 *sy7636 = i2c_get_clientdata(client);

	//gpio_set_value(sy7636->gpio_pmic_vcom_ctrl,0);

	
	return 0;
}

static int sy7636_resume(struct device *dev)
{
	struct i2c_client *client = i2c_verify_client(dev);    
	struct sy7636 *sy7636 = i2c_get_clientdata(client);
	int iChk;
	unsigned int dwReg;
	

	//gpio_set_value(sy7636->gpio_pmic_vcom_ctrl,0);

	if (gSleep_Mode_Suspend) {
		sy7636_chip_power(sy7636,1);
	}
	iChk = SY7636_REG_READ(sy7636,OPMODE);
	if(iChk>=0) {
		dwReg = (unsigned int)iChk;
		dev_dbg(sy7636->dev, "%s() : OP=0x%x,RailsEN=%d,RailsDisable=0x%x\n",__FUNCTION__,
			dwReg,(int)BITFEXT(dwReg,RAILS_ON),BITFEXT(dwReg,RAILS_DISABLE));
	}
	else {
		return -1;
	}

	if(sy7636->need_reinit) 
	{
		sy7636_regs_init(sy7636);	
		sy7636->need_reinit = 0;
	}

	return 0;
}

/* Return 0 if detection is successful, -ENODEV otherwise */
static int sy7636_detect(struct i2c_client *client,
			  struct i2c_board_info *info)
{
	//struct sy7636_platform_data *pdata = client->dev.platform_data;
	struct i2c_adapter *adapter = client->adapter;
	struct sy7636 *sy7636 = i2c_get_clientdata(client);

	const int iMaxRetryCnt = 100;
	int iRetryN;
	int iIsDeviceReady;

	int iStatus ;

	printk("sy7636_detect calling\n");

	sy7636_EN(sy7636,1);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&adapter->dev,"I2C adapter error! \n");
		return -ENODEV;
	}

	for (iIsDeviceReady=0,iRetryN=1;iRetryN<=iMaxRetryCnt;iRetryN++)
	{
		/* identification */

		iStatus = i2c_smbus_read_byte_data(client,REG_SY7636_OPMODE);
		if(iStatus>=0) {
			iIsDeviceReady = 1;
			//sy7636->bOPMode = (unsigned char)iStatus;
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

	printk("%s():opmode=0x%x\n",__FUNCTION__,(unsigned char)iStatus);


	if (info) {
		strlcpy(info->type, "sy7636_sensor", I2C_NAME_SIZE);
	}

	printk("sy7636_detect success\n");
	return 0;
}

static const struct of_device_id sy7636_dt_ids[] = {
	{
		.compatible = "silergy,sy7636",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, sy7636_dt_ids);

static const struct dev_pm_ops sy7636_dev_pm= {
	.suspend_late = sy7636_suspend_late,
	.resume_early = sy7636_resume_early,
	.suspend = sy7636_suspend,
	.resume = sy7636_resume,
};


static struct i2c_driver sy7636_i2c_driver = {
	.driver = {
		   .name = "sy7636",
		   .owner = THIS_MODULE,
		   .of_match_table = sy7636_dt_ids,
		   .pm = (&sy7636_dev_pm),
	},
	.probe = sy7636_probe,
	.remove = sy7636_remove,
	.detect = sy7636_detect,
};

module_i2c_driver(sy7636_i2c_driver);
MODULE_LICENSE("GPL v2");
