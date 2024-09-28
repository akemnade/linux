/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/mfd/jd9930.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include <linux/input.h>


#define JD9930_V3P3_ENABLE		1

struct jd9930_data {
	int num_regulators;
	struct jd9930 *jd9930;
	struct regulator_dev **rdev;
	struct regulator *reg_vdd;
};


static struct jd9930_data *gptJD9930_DATA;

static int jd9930_vcom = { 1250000 };

static int jd9930_is_power_good(struct jd9930 *jd9930);

static int _jd9930_rals_onoff(struct jd9930 *jd9930,int iIsON);

/*
static uint32_t getDiffuS(struct timeval *tvEnd,struct timeval *tvBegin) 
{
	uint32_t dwDiffus = (tvEnd->tv_sec-tvBegin->tv_sec)*1000000;
	dwDiffus += (tvEnd->tv_usec);
	dwDiffus -= (tvBegin->tv_usec);
  return (uint32_t) (dwDiffus); // ms
}
*/

static unsigned int jd9930_to_reg_val(unsigned int cur_reg, unsigned int fld_mask,
							   unsigned int fld_val)
{
	return (cur_reg & (~fld_mask)) | fld_val;
}

static ssize_t jd9930_powerup_read(struct device *dev, struct device_attribute *attr,char *buf)
{
	struct jd9930 *jd9930 = dev_get_drvdata(dev);
	sprintf(buf,"%d\n",(int)gpiod_get_value(jd9930->gpio_pmic_powerup));
	return strlen(buf);
}
static ssize_t jd9930_powerup_write(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t count)
{
	struct jd9930 *jd9930 = dev_get_drvdata(dev);
	//struct jd9930_platform_data *pdata = jd9930->pdata;
	int iVal=-1;

	sscanf(buf,"%d\n",&iVal);
	if(1==iVal) {
		_jd9930_rals_onoff(jd9930,1);
	}
	else if(0==iVal) {
		_jd9930_rals_onoff(jd9930,0);
	}
	else {
		printk(KERN_ERR"%s() invalid parameter !!it must be [1|0] \n",__FUNCTION__);
	}
	return count;
}

static ssize_t jd9930_regs_read(struct device *dev, struct device_attribute *attr,char *buf)
{
	struct jd9930 *jd9930 = dev_get_drvdata(dev);
	int i;
	unsigned int cur_reg_val; /* current register value to modify */

	for (i=0; i<JD9930_REG_NUM; i++) {
		//struct jd9930_platform_data *pdata = jd9930->pdata;
		jd9930_reg_read(jd9930, i ,&cur_reg_val);
		printk ("[%s-%d] reg %d vlaue is %X\n",__func__,__LINE__, i, cur_reg_val);
		sprintf (&buf[i*5],"0x%02X,",(int)cur_reg_val);
	}
	sprintf (&buf[i*16],"\n");
	return strlen(buf);
}
static DEVICE_ATTR (powerup,0644,jd9930_powerup_read,jd9930_powerup_write);
static DEVICE_ATTR (regs,0444,jd9930_regs_read,NULL);


#if 0
/*
 * Regulator operations
 */
/* Convert uV to the VCOM register bitfield setting */

static int vcom2_rs_to_uV(unsigned int reg_setting)
{
	if (reg_setting <= JD9930_VCOM_MIN_SET)
		return JD9930_VCOM_MIN_uV;
	if (reg_setting >= JD9930_VCOM_MAX_SET)
		return JD9930_VCOM_MAX_uV;
	return -(reg_setting * JD9930_VCOM_STEP_uV);
}

static int vcom2_uV_to_rs(int uV)
{
	if (uV <= JD9930_VCOM_MIN_uV)
		return JD9930_VCOM_MIN_SET;
	if (uV >= JD9930_VCOM_MAX_uV)
		return JD9930_VCOM_MAX_SET;
	return (-uV) / JD9930_VCOM_STEP_uV;
}
#endif


static int _jd9930_vcom_set_voltage(struct jd9930 *jd9930,
					int minuV, int uV, unsigned *selector)
{
	//unsigned int cur_reg_val; /* current register value to modify */
	//unsigned int new_reg_val; /* new register value to write */
	int retval=0;
        unsigned char regVCOM;
	
        regVCOM = (unsigned char)(((uV)*1000)/JD9930_VCOM_STEP_uV);

	retval = JD9930_REG_WRITE_EX(jd9930,VCOM_SET,regVCOM);

	if(retval>=0) {
		jd9930->vcom_uV = uV;
	}

	return retval;

}

static int jd9930_vcom_set_voltage(struct regulator_dev *reg,
					int minuV, int uV, unsigned *selector)
{
	struct jd9930 *jd9930 = rdev_get_drvdata(reg);
	return _jd9930_vcom_set_voltage(jd9930,minuV,uV,selector);
}


static int _jd9930_vcom_get_voltage(struct jd9930 *jd9930,
				    struct regulator_dev *reg)
{
	int vcomValue;
	int iVCOMmV;
	int iChk;

        if(jd9930->need_reinit) {
		return jd9930->vcom_uV;
		/* REVISIT, do we need to rewrite it? Whoever will call it */
	}

        iChk = JD9930_REG_READ(jd9930,VCOM_SET);
	if (iChk < 0)
		return iChk;

        return iChk*JD9930_VCOM_STEP_uV;
}
static int jd9930_vcom_get_voltage(struct regulator_dev *reg)
{
	struct jd9930 *jd9930 = rdev_get_drvdata(reg);
	return _jd9930_vcom_get_voltage(jd9930,reg);
}

static int jd9930_vcom_enable(struct regulator_dev *reg)
{
	struct jd9930 *jd9930 = rdev_get_drvdata(reg);
	return 0;
}

static int jd9930_vcom_disable(struct regulator_dev *reg)
{
	struct jd9930 *jd9930 = rdev_get_drvdata(reg);
	return 0;
}



#define VPOS_NEG_MIN_UV	7040000
#define VPOS_NEG_MAX_UV	15060000
static const int giJD9930_vpos_neg_uVA[] = {
	VPOS_NEG_MIN_UV, // <=04H ,
	7260000, // 05H ,
	7490000, // 06H ,
	7710000, // 07H ,
	7930000, // 08H ,
	8150000, // 09H ,
	8380000, // 0AH ,
	8600000, // 0BH ,
	8820000, // 0CH ,
	9040000, // 0DH ,
	9270000, // 0EH ,
	9490000, // 1FH ,
	9710000, // 10H ,
	9940000, // 11H ,
	10160000, // 12H ,
	10380000, // 13H ,
	10600000, // 14H ,
	10830000, // 15H ,
	11050000, // 16H , 
	11270000, // 17H ,
	11490000, // 18H ,
	11720000, // 19H ,
	11940000, // 1AH ,
	12160000, // 1BH ,
	12380000, // 1CH ,
	12610000, // 1DH ,
	12830000, // 1EH ,
	13050000, // 1FH ,
	13280000, // 20H ,
	13500000, // 21H ,
	13720000, // 22H ,
	13940000, // 23H ,
	14170000, // 24H ,
	14390000, // 25H ,
	14610000, // 26H ,
	14830000, // 27H ,
	VPOS_NEG_MAX_UV, // 28H ,
};
#define JD9930_VPOSNEG_REG_TO_uV(reg) \
({\
	int iuV;\
	if(reg<=0x04) {\
		iuV = VPOS_NEG_MIN_UV;\
	}\
	else if(reg>=0x28) {\
		iuV = VPOS_NEG_MAX_UV;\
	}\
	else {\
		iuV = giJD9930_vpos_neg_uVA[(reg-4)];\
	}\
	iuV;\
})	

#define JD9930_VPOSNEG_uV_TO_REG(_uV) \
({\
	int _iRegIdx,_iFoundRegIdx=4;\
	for(_iRegIdx=0;\
	 _iRegIdx<sizeof(giJD9930_vpos_neg_uVA)/sizeof(giJD9930_vpos_neg_uVA[0]);\
	 _iRegIdx++) \
	 {\
		if(giJD9930_vpos_neg_uVA[_iRegIdx]>(_uV)) {\
			break;\
		}\
		else {\
			_iFoundRegIdx = _iRegIdx+4;\
		}\
	}\
	_iFoundRegIdx;\
})


static int _jd9930_vposneg_set_voltage(struct jd9930 *jd9930,
					int minuV, int uV, unsigned *selector)
{
	//unsigned int cur_reg_val; /* current register value to modify */
	//unsigned int new_reg_val; /* new register value to write */
	int retval=0;
	int iReg;

	//dbgENTER();
	iReg = JD9930_VPOSNEG_uV_TO_REG(uV);
	JD9930_REG_WRITE_EX(jd9930,VPOS_VNEG_SET,iReg);
	dev_dbg(jd9930->dev, "%s() : iReg=%d,minuV=%d,uV=%d\n",__func__,iReg,minuV,uV);

	//dbgLEAVE();
	return retval;

}


static int jd9930_vposneg_set_voltage(struct regulator_dev *reg,
		int minuV, int uV, unsigned *selector)
{
	struct jd9930 *jd9930 = rdev_get_drvdata(reg);
	return _jd9930_vposneg_set_voltage(jd9930,minuV,uV,selector);
}

static int _jd9930_vposneg_get_voltage(struct jd9930 *jd9930,
		struct regulator_dev *reg)
{
	int iuV;
	int iRegVPosNeg;
	//dbgENTER();
	iRegVPosNeg = JD9930_REG_READ_EX(jd9930,VPOS_VNEG_SET,1);
	if(iRegVPosNeg<0) {
		// register reading error . 
		iRegVPosNeg = JD9930_REG_GET(jd9930,VPOS_VNEG_SET);
	}
	iuV = JD9930_VPOSNEG_REG_TO_uV(iRegVPosNeg);
	dev_dbg(jd9930->dev,"%s() : vpos/vneg =%d uV\n",__func__,iuV);

	//dbgLEAVE();
	return iuV;
}
static int jd9930_vposneg_get_voltage(struct regulator_dev *reg)
{
	struct jd9930 *jd9930 = rdev_get_drvdata(reg);
	return _jd9930_vposneg_get_voltage(jd9930,reg);
}




static int jd9930_is_power_good(struct jd9930 *jd9930)
{
	/*
	 * XOR of polarity (starting value) and current
	 * value yields whether power is good.
	 */
	return gpiod_get_value(jd9930->gpio_pmic_pwrgood) ^
		jd9930->pwrgood_polarity;
}

int jd9930_get_power_status(struct jd9930 *jd9930,int iIsGetCached,int *O_piPG,unsigned char *O_pbFaults)
{
	//int iPG;

	if(O_piPG) {
		*O_piPG = jd9930_is_power_good(jd9930);
	}
	return 0;
}


static void jd9930_int_func(struct work_struct *work)
{
	//unsigned int dwReg;
	struct jd9930 *jd9930 = container_of(work, struct jd9930, int_work);
	unsigned char bFaults=0;
	int iChk ,iPG ;

	dev_dbg(jd9930->dev, "%s()\n",__FUNCTION__);

	iChk = jd9930_get_power_status(jd9930,0,&iPG,&bFaults);
	if(iChk<0) {
		dev_err(jd9930->dev, "%s(),get power status failed !\n",__FUNCTION__);
		return ;
	}

	if(bFaults) {
		printk(KERN_ERR"JD9930 faults occured !!,faults=0x%x\n",bFaults);
	//	jd9930->int_state = MSC_RAW_EPD_UNKOWN_ERROR;
		jd9930->int_state = bFaults;
#ifdef JD9930_VDROP_PROC_IN_KERNEL//[
#else //][!JD9930_VDROP_PROC_IN_KERNEL
		ntx_report_event(EV_MSC,MSC_RAW,jd9930->int_state);
#endif //]JD9930_VDROP_PROC_IN_KERNEL

		if(jd9930->pfnINTCB) {
			jd9930->pfnINTCB(jd9930->int_state);
		}
	}
	else {
		if(jd9930->pfnINTCB) {
			if(iPG) {
				jd9930->int_state = 0; //MSC_RAW_EPD_POWERGOOD_YES;
			}
			else {
				jd9930->int_state = 1; //MSC_RAW_EPD_POWERGOOD_NO;
			}
			jd9930->pfnINTCB(jd9930->int_state);
		}
	}

}

// get latest interrupt state of jd9930 .
int jd9930_int_state_get(struct jd9930 *jd9930)
{
	if(jd9930->int_state>0) {
		return jd9930->int_state;
	}
	return -1;
}

// clear latest interrupt state of jd9930 .
int jd9930_int_state_clear(struct jd9930 *jd9930)
{
	jd9930->int_state=0;
	return 0;
}

int jd9930_int_callback_setup(struct jd9930 *jd9930,JD9930_INTEVT_CB fnCB)
{

	if(jd9930) {
		jd9930->pfnINTCB=fnCB;
	}
	else {
		pr_err("%s() jd9930 not exist !\n",__func__);
	}

	return 0;
}


static int jd9930_wait_power_good(struct jd9930 *jd9930,int iWaitON)
{
	int i;
	//int iChk;
	int iPG;
	int iRet = -ETIMEDOUT;

	dev_dbg(jd9930->dev, "%s():waiton=%d\n",__FUNCTION__,iWaitON);

	for (i = 0; i < 20; i++)
	{
		msleep(10);

		//printk("%s() : sleep %d us\n",__FUNCTION__,diffus);	

		iPG = jd9930_is_power_good(jd9930);

		if(iWaitON) {
			if (iPG) {
				iRet = 0;
				dev_dbg(jd9930->dev,"%s():cnt=%d,PG=%d,%d\n",
						__FUNCTION__,i,iPG,jd9930_is_power_good(jd9930));
				break;
			}

		}
		else {
			if (!iPG) {
				dev_dbg(jd9930->dev,"%s():cnt=%d,PG=%d\n",__FUNCTION__,i,iPG);
				iRet = 0;
				break;
			}
		}
	}


	if(iRet<0) {
		printk(KERN_ERR"%s():waiting(%d) for PG=%d (%d) timeout\n",
			__FUNCTION__,i,iWaitON,iPG);
	}

	return iRet;
}

static int _jd9930_rals_onoff(struct jd9930 *jd9930,int iIsON)
{

	if(!jd9930) {
		pr_err("%s() : error object !\n",__FUNCTION__);
		return -1;
	}

	if(iIsON) {
		gpiod_set_value(jd9930->gpio_pmic_powerup,1);	
	}
	else {
		gpiod_set_value(jd9930->gpio_pmic_powerup,0);	
	}
	dev_dbg(jd9930->dev, "%s(%d) ON=%d \n",__func__,__LINE__,iIsON);

	return 0;
}



static int jd9930_display_enable(struct regulator_dev *reg)
{
	struct jd9930 *jd9930 = rdev_get_drvdata(reg);
	int iRet;
	int iTryCnt=0;

#ifdef TURNOFF_PWR_EPD_UPDATED //[
	{
		if(0==gpiod_get_value(jd9930->gpio_pmic_pwrall)) {
			jd9930_chip_power(jd9930,1);
		}
	}
#endif //] TURNOFF_PWR_EPD_UPDATED

	do {
		++iTryCnt;


		_jd9930_rals_onoff(jd9930,1);

		iRet = jd9930_wait_power_good(jd9930,1);
		if(iRet<0) {
			printk("%s power on failed %d,retry ...\n",__FUNCTION__,iTryCnt);
#if 1
			printk("reseting JD9930 chip ...\n");
			msleep(JD9930_SLEEP0_MS);
			jd9930_chip_power(jd9930,0);
			jd9930_chip_power(jd9930,1);
#endif
		}
		else {
			break;
		}
	} while (iTryCnt<=5);
	return iRet;
}


static int jd9930_display_disable(struct regulator_dev *reg)
{
#if 0
	printk(KERN_ERR"%s():skipped !!\n",__FUNCTION__);
	return 0;
#else
	struct jd9930 *jd9930 = rdev_get_drvdata(reg);
	int iRet;

	//dbgENTER();
#if 0
	if(jd9930->dwPwrOnOffFlags&EPD_PMIC_FLAGS_NIGHTMODE) {
		jd9930_switch_nightmode(jd9930,1);
	}
	else {
		jd9930_switch_nightmode(jd9930,0);
	}
#endif

	_jd9930_rals_onoff(jd9930,0);
	iRet = jd9930_wait_power_good(jd9930,0);

#ifdef TURNOFF_PWR_EPD_UPDATED //[
	jd9930_chip_power(jd9930,0);
#endif //] TURNOFF_PWR_EPD_UPDATED

	//dbgLEAVE();

	return iRet;
#endif
}

static int jd9930_display_is_enabled(struct regulator_dev *reg)
{
	struct jd9930 *jd9930 = rdev_get_drvdata(reg);
	int iChk;
	int iPG;
	struct gpio_desc *gpiod_pwrall;

	if(0==gpiod_get_value(jd9930->gpio_pmic_pwrall)) {
		printk("%s(%d) PG=0 when chip power off\n",__func__,__LINE__);
		return 0;
	}
	
	iChk = jd9930_get_power_status(jd9930,0,&iPG,0);
	if(iChk>=0) {
		return iPG;
	}
	else {
		printk(KERN_ERR"%s(%d) get power status failed !\n",__func__,__LINE__);
		return 0;
	}
}

static int jd9930_display_setflags(struct regulator_dev *reg,unsigned long dwFlags)
{
	struct jd9930 *jd9930 = rdev_get_drvdata(reg);
	int iRet = 0;

	//printk("%s(0x%x)\n",__func__,dwFlags);
	if(!jd9930) {
		return -1;
	}

	jd9930->dwPwrOnOffFlags = dwFlags;

	return iRet;
}



#ifdef JD9930_V3P3_ENABLE//[


static int jd9930_v3p3_enable(struct regulator_dev *reg)
{
	//struct jd9930 *jd9930 = rdev_get_drvdata(reg);
	return 0;
}

static int jd9930_v3p3_disable(struct regulator_dev *reg)
{
	//struct jd9930 *jd9930 = rdev_get_drvdata(reg);
	return 0;

}

#endif //] JD9930_V3P3_ENABLE

static struct regulator_ops jd9930_display_ops = {
	.enable = jd9930_display_enable,
//	.setflags = jd9930_display_setflags,
	.disable = jd9930_display_disable,
	.is_enabled = jd9930_display_is_enabled,
};

static struct regulator_ops jd9930_vcom_ops = {
//	.enable = jd9930_vcom_enable,
//	.disable = jd9930_vcom_disable,
	.get_voltage = jd9930_vcom_get_voltage,
	.set_voltage = jd9930_vcom_set_voltage,
};
static struct regulator_ops jd9930_vposneg_ops = {
	.get_voltage = jd9930_vposneg_get_voltage,
	.set_voltage = jd9930_vposneg_set_voltage,
};
static struct regulator_ops jd9930_v3p3_ops = {
	.enable = jd9930_v3p3_enable,
	.disable = jd9930_v3p3_disable,
};

/*
 * Regulator descriptors
 */
static struct regulator_desc jd9930_reg[] = {
{
	.name = "DISPLAY",
	.id = JD9930_DISPLAY,
	.ops = &jd9930_display_ops,
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
},
{
	.name = "VCOM",
	.id = JD9930_VCOM,
	.ops = &jd9930_vcom_ops,
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
},
{
	.name = "VPOSNEG",
	.id = JD9930_VPOSNEG,
	.ops = &jd9930_vposneg_ops,
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
},
#ifdef JD9930_V3P3_ENABLE//[
{
	.name = "V3P3",
	.id = JD9930_VP3V3,
	.ops = &jd9930_v3p3_ops,
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
},
#endif //]JD9930_V3P3_ENABLE
};

int jd9930_switch_nightmode(struct jd9930 *jd9930,int iIsNightMode)
{
	int iRet = JD9930_RET_SUCCESS;
	int iChk;
	unsigned int cur_reg,new_reg_val;
	unsigned int fld_mask = 0;	  /* register mask for bitfield to modify */
	unsigned int fld_val = 0;	  /* new bitfield value to write */
	unsigned char bIsNM_Old,bIsNM_New;

	if(!jd9930) {
		return JD9930_RET_OBJ_ERR;
	}


	iChk = JD9930_REG_GET(jd9930,CTRL_REG1);
	if(iChk>=0) {
		cur_reg = iChk;
	}
	else {
		// read register error .
	}
	bIsNM_Old = JD9930_BITFEXT(cur_reg,NM);
	bIsNM_New = iIsNightMode?1:0;

	if(bIsNM_Old!=bIsNM_New) {
		fld_mask |= JD9930_BITFMASK(NM);
		fld_val |= JD9930_BITFVAL(NM,bIsNM_New);
		new_reg_val = jd9930_to_reg_val(cur_reg, fld_mask, fld_val);
		//DBG_MSG("%s(),set CTRL_REG1 =0x%x\n",__FUNCTION__,new_reg_val);
		//JD9930_REG_WRITE_EX(jd9930,PWRON_DLY,new_reg_val);
		JD9930_REG_SET(jd9930,CTRL_REG1,(unsigned char)new_reg_val);
		JD9930_REG_WRITE(jd9930,CTRL_REG1);
	}
	
#ifdef GDEBUG //[
	printk(KERN_DEBUG"%s(%d) NM=%d\n",__func__,__LINE__,iIsNightMode);
#endif //] GDEBUG

	return iRet;
}
static void jd9930_setup_timings(struct jd9930 *jd9930)
{

	unsigned char bOnDly1, bOnDly2, bOnDly3, bOnDly4;
	unsigned int cur_reg,new_reg_val;
	unsigned int fld_mask = 0;	  /* register mask for bitfield to modify */
	unsigned int fld_val = 0;	  /* new bitfield value to write */
	int iChk;

	do {
		if( 0xff==jd9930->on_delay1 &&
				0xff==jd9930->on_delay2 &&
				0xff==jd9930->on_delay3 &&
				0xff==jd9930->on_delay4)
		{
			// nothing to do .
			dev_dbg(jd9930->dev, "%s(),skipped on_delay settings !\n",__FUNCTION__);
			break ;
		}

		iChk = JD9930_REG_GET(jd9930,PWRON_DELAY);
		if(iChk>=0) {
			cur_reg = iChk;
		}

		bOnDly1 = JD9930_BITFEXT(cur_reg,TDLY1);
		bOnDly2 = JD9930_BITFEXT(cur_reg,TDLY2);
		bOnDly3 = JD9930_BITFEXT(cur_reg,TDLY3);
		bOnDly4 = JD9930_BITFEXT(cur_reg,TDLY4);
		
		if(0xff!=jd9930->on_delay1 && bOnDly1!=jd9930->on_delay1) {
			fld_mask |= JD9930_BITFMASK(TDLY1);
			fld_val |= JD9930_BITFVAL(TDLY1, jd9930->on_delay1);
		}
		if(0xff!=jd9930->on_delay2 && bOnDly1!=jd9930->on_delay2) {
			fld_mask |= JD9930_BITFMASK(TDLY2);
			fld_val |= JD9930_BITFVAL(TDLY2, jd9930->on_delay2);
		}
		if(0xff!=jd9930->on_delay3 && bOnDly1!=jd9930->on_delay3) {
			fld_mask |= JD9930_BITFMASK(TDLY3);
			fld_val |= JD9930_BITFVAL(TDLY3, jd9930->on_delay3);
		}
		if(0xff!=jd9930->on_delay4 && bOnDly1!=jd9930->on_delay4) {
			fld_mask |= JD9930_BITFMASK(TDLY4);
			fld_val |= JD9930_BITFVAL(TDLY4, jd9930->on_delay4);
		}
		new_reg_val = jd9930_to_reg_val(cur_reg, fld_mask, fld_val);
		if(cur_reg!=new_reg_val) {
			dev_dbg(jd9930->dev, "%s(),set poweron delay=0x%x\n",__FUNCTION__,new_reg_val);
			//JD9930_REG_WRITE_EX(jd9930,PWRON_DLY,new_reg_val);
			JD9930_REG_SET(jd9930,PWRON_DELAY,(unsigned char)new_reg_val);
		}
	}while(0);

	JD9930_REG_SET(jd9930,VGH_EXT,jd9930->vgh_ext);
	JD9930_REG_SET(jd9930,VGL_EXT,jd9930->vgl_ext);
	JD9930_REG_SET(jd9930,VGHNM_EXT,jd9930->vghnm_ext);
	JD9930_REG_SET(jd9930,VGHNM,jd9930->vghnm);
	JD9930_REG_SET(jd9930,DISA_DELAY,jd9930->disa_delay);
	JD9930_REG_SET(jd9930,XON_DELAY,jd9930->xon_delay);
	JD9930_REG_SET(jd9930,XON_LEN,jd9930->xon_len);
}

#define CHECK_PROPERTY_ERROR_KFREE(prop) \
do { \
	int ret = of_property_read_u32(jd9930->dev->of_node, \
					#prop, &jd9930->prop); \
	if (ret < 0) { \
		return ret;	\
	}	\
} while (0);

#ifdef CONFIG_OF
static int jd9930_pmic_dt_parse_pdata(struct platform_device *pdev,
					struct jd9930_platform_data *pdata)
{
	struct jd9930 *jd9930 = dev_get_drvdata(pdev->dev.parent);
	struct device_node *pmic_np, *regulators_np, *reg_np;//*VLDO_np, 

	struct jd9930_regulator_data *rdata;
	int ret,i;
	u32 dwTemp;

	pmic_np = of_node_get(jd9930->dev->of_node);
	if (!pmic_np) {
		dev_err(&pdev->dev, "could not find pmic sub-node\n");
		return -ENODEV;
	}


	regulators_np = of_find_node_by_name(pmic_np, "regulators");
	if (!regulators_np) {
		dev_err(&pdev->dev, "could not find regulators sub-node\n");
		return -EINVAL;
	}

	dev_dbg(&pdev->dev, "%s() regulators_np@%p\n",__func__,regulators_np);

#if 0
	printk("skip regulators ---\n");
#else
	pdata->num_regulators = of_get_child_count(regulators_np);
	dev_info(&pdev->dev, "num_regulators %d\n", pdata->num_regulators);


	//DBG_MSG("%s(%d):skipped !!\n",__FILE__,__LINE__);return -1;

	rdata = devm_kzalloc(&pdev->dev, sizeof(*rdata) *
				pdata->num_regulators, GFP_KERNEL);
	if (!rdata) {
		of_node_put(regulators_np);
		dev_err(&pdev->dev, "could not allocate memory for"
			"regulator data\n");
		return -ENOMEM;
	}

	//DBG_MSG("%s(%d):test ok !!\n",__FILE__,__LINE__);return -1;
	pdata->regulators = rdata;
	for_each_child_of_node(regulators_np, reg_np) {
		dev_dbg(&pdev->dev, "%s():regulator name=\"%s\"\n",__FUNCTION__,reg_np->name);

		for (i = 0; i < ARRAY_SIZE(jd9930_reg); i++)
			if (!of_node_cmp(reg_np->name, jd9930_reg[i].name))
				break;

		if (i == ARRAY_SIZE(jd9930_reg)) {
			dev_warn(&pdev->dev, "don't know how to configure "
				"regulator %s\n", reg_np->name);
			pdata->num_regulators--;
			continue;
		}

		rdata->id = i;
		rdata->initdata = of_get_regulator_init_data(&pdev->dev,
							     reg_np, &jd9930_reg[i]);
		rdata->reg_node = reg_np;
		rdata++;
	}

#endif

	of_node_put(regulators_np);


	if(of_property_read_u32(pmic_np,"on_delay1",&dwTemp)) {
		jd9930->on_delay1=0xff;
		dev_info(&pdev->dev, "failed to get on_delay1 property,default will be 0x0\n");
	}
	else {
		jd9930->on_delay1=(unsigned char)dwTemp;
		dev_info(&pdev->dev, "on_delay1=0x%x\n",jd9930->on_delay1);
	}

	if(of_property_read_u32(pmic_np,"on_delay2",&dwTemp)) {
		jd9930->on_delay2=0xff;
		dev_info(&pdev->dev, "failed to get on_delay2 property,default will be 0x0\n");
	}
	else {
		jd9930->on_delay2=(unsigned char)dwTemp;
		dev_info(&pdev->dev, "on_delay2=0x%x\n",jd9930->on_delay2);
	}

	if(of_property_read_u32(pmic_np,"on_delay3",&dwTemp)) {
		jd9930->on_delay3=0xff;
		dev_info(&pdev->dev, "failed to get on_delay3 property,default will be 0x0\n");
	}
	else {
		jd9930->on_delay3=(unsigned char)dwTemp;
		dev_info(&pdev->dev, "on_delay3=0x%x\n",jd9930->on_delay3);
	}

	if(of_property_read_u32(pmic_np,"on_delay4",&dwTemp)) {
		jd9930->on_delay4=0xff;
		dev_info(&pdev->dev, "failed to get on_delay4 property,default will be 0x0\n");
	}
	else {
		jd9930->on_delay4=(unsigned char)dwTemp;
		dev_info(&pdev->dev, "on_delay4=0x%x\n",jd9930->on_delay4);
	}


	if(of_property_read_u32(pmic_np,"vgh_ext",&dwTemp)) {
		dev_info(&pdev->dev, "failed to get vgh_ext property\n");
	}
	else {
		jd9930->vgh_ext=(unsigned char)dwTemp;
		dev_info(&pdev->dev, "vgh_ext=0x%x\n",jd9930->vgh_ext);
	}

	if(of_property_read_u32(pmic_np,"vgl_ext",&dwTemp)) {
		dev_info(&pdev->dev, "failed to get vgl_ext property\n");
	}
	else {
		jd9930->vgl_ext=(unsigned char)dwTemp;
		dev_info(&pdev->dev, "vgl_ext=0x%x\n",jd9930->vgl_ext);
	}

	if(of_property_read_u32(pmic_np,"vghnm_ext",&dwTemp)) {
		dev_info(&pdev->dev, "failed to get vghnm_ext property\n");
	}
	else {
		jd9930->vghnm_ext=(unsigned char)dwTemp;
		dev_info(&pdev->dev, "vghnm_ext=0x%x\n",jd9930->vghnm_ext);
	}

	if(of_property_read_u32(pmic_np,"vghnm",&dwTemp)) {
		dev_info(&pdev->dev, "failed to get vghnm property\n");
	}
	else {
		jd9930->vghnm=(unsigned char)dwTemp;
		dev_info(&pdev->dev, "vghnm=0x%x\n",jd9930->vghnm);
	}

	if(of_property_read_u32(pmic_np,"disa_delay",&dwTemp)) {
		dev_info(&pdev->dev, "failed to get disa_delay property\n");
	}
	else {
		jd9930->disa_delay=(unsigned char)dwTemp;
		dev_info(&pdev->dev, "disa_delay=0x%x\n",jd9930->disa_delay);
	}

	if(of_property_read_u32(pmic_np,"xon_delay",&dwTemp)) {
		dev_info(&pdev->dev, "failed to get xon_delay property\n");
	}
	else {
		jd9930->xon_delay=(unsigned char)dwTemp;
		dev_info(&pdev->dev, "xon_delay=0x%x\n",jd9930->xon_delay);
	}

	if(of_property_read_u32(pmic_np,"xon_len",&dwTemp)) {
		dev_info(&pdev->dev, "failed to get xon_len property\n");
	}
	else {
		jd9930->xon_len=(unsigned char)dwTemp;
		dev_info(&pdev->dev, "xon_len=0x%x\n",jd9930->xon_len);
	}




	if (of_property_read_u32(pmic_np, "turnoff_delay_ep3v3", &jd9930->turnoff_delay_ep3v3))
	{
		jd9930->turnoff_delay_ep3v3=0;
		dev_info(&pdev->dev, "failed to get turnoff_delay_ep3v3 property,default will be 0\n");
	}
	else {
		dev_info(&pdev->dev, "turnoff_delay_ep3v3=%d\n", jd9930->turnoff_delay_ep3v3);
	}


	//jd9930->max_wait = (6 + 6 + 6 + 6);


	if (ret < 0) {
		dev_err(&pdev->dev, "request powerup gpio failed (%d)!\n",ret);
		//goto err;
	}
	else {
		//jd9930->jiffies_chip_wake = jiffies+msecs_to_jiffies(JD9930_WAKEUP_MS);
		//mdelay(JD9930_WAKEUP0_MS);
	}

err:
	return 0;

}
#else
static int jd9930_pmic_dt_parse_pdata(struct platform_device *pdev,
					struct jd9930 *jd9930)
{
	return 0;
}
#endif	/* !CONFIG_OF */



/*
 * Regulator init/probing/exit functions
 */
static int jd9930_regulator_probe(struct platform_device *pdev)
{
	struct jd9930 *jd9930 = dev_get_drvdata(pdev->dev.parent);
	struct jd9930_platform_data *pdata = jd9930->pdata;
	struct jd9930_data *priv;
	struct regulator_dev **rdev;
	struct regulator_config config = { };
	int size, i, ret = 0;

	dev_dbg(&pdev->dev, "%s starting , of_node=%p\n",__FUNCTION__,jd9930->dev->of_node);


	//jd9930->pwrgood_polarity = 1;
	if (jd9930->dev->of_node) {
		
		ret = jd9930_pmic_dt_parse_pdata(pdev, pdata);
		if (ret)
			return ret;
	}

	dev_dbg(&pdev->dev, "%s(%d) pdev=%p,pdata->num_regulators=%d\n",__FUNCTION__,__LINE__,
			pdev,pdata->num_regulators);

	priv = devm_kzalloc(&pdev->dev, sizeof(struct jd9930_data),
			       GFP_KERNEL);
	if (!priv)
		return -ENOMEM;


	if(pdata->num_regulators>0) {
		size = sizeof(struct regulator_dev *) * pdata->num_regulators;
		priv->rdev = devm_kzalloc(&pdev->dev, size, GFP_KERNEL);
		if (!priv->rdev)
			return -ENOMEM;
	}


	printk("%s(%d): JD9930@%p,PG=%d\n",__FUNCTION__,__LINE__,
		jd9930,jd9930_is_power_good(jd9930));

	rdev = priv->rdev;
	priv->num_regulators = pdata->num_regulators;
	priv->jd9930 = jd9930;
	platform_set_drvdata(pdev, priv);
	

	jd9930->need_reinit = 0;
	jd9930->vcom_uV = jd9930_vcom;

	// reset default regiters value . 
	JD9930_REG_SET(jd9930,VCOM_SET,0x10);
#if 0
	JD9930_REG_SET(jd9930,VPOS_VNEG_SET,0);
	JD9930_REG_SET(jd9930,PWRON_DELAY,0);
	JD9930_REG_SET(jd9930,VGH_EXT,0);
	JD9930_REG_SET(jd9930,VGL_EXT,0);
	JD9930_REG_SET(jd9930,VGHNM_EXT,0);
	JD9930_REG_SET(jd9930,VGHNM,0);
	JD9930_REG_SET(jd9930,DISA_DELAY,0);
	JD9930_REG_SET(jd9930,XON_DELAY,0);
	JD9930_REG_SET(jd9930,XON_LEN,0);
#endif

	JD9930_REG_SET(jd9930,CTRL_REG1,0x2);// V3P3_EN .
	JD9930_REG_SET(jd9930,CTRL_REG2,0x0f); // VP 5A limit, VN 5A limit . 




	jd9930_setup_timings(jd9930);// power on delay timings setup . 

	jd9930_regs_init(jd9930);

//	_jd9930_vcom_set_voltage(jd9930, jd9930->vcom_uV, jd9930->vcom_uV, NULL);
//	jd9930_set_vcom(jd9930,jd9930->vcom_uV/1000,0);
	printk("%s():initial vcom=%duV\n ",__FUNCTION__,jd9930->vcom_uV);
	
	priv->reg_vdd = regulator_get(jd9930->dev, "epd");
	if(!priv->reg_vdd) {
		dev_warn(&pdev->dev, "regulator vdd get failed\n");
	}

	for (i = 0; i < pdata->num_regulators; i++) {
		int id = pdata->regulators[i].id;

		config.dev = jd9930->dev;
		config.init_data = pdata->regulators[i].initdata;
		config.driver_data = jd9930;
		config.of_node = pdata->regulators[i].reg_node;

		rdev[i] = devm_regulator_register(&pdev->dev, &jd9930_reg[id], &config);
		if (IS_ERR(rdev[i])) {
			ret = PTR_ERR(rdev[i]);
			dev_err(&pdev->dev, "regulator init failed for %d\n",id);
			rdev[i] = NULL;
			goto err;
		}
		else {
			dev_info(&pdev->dev, "rdev[%d] %s @ %p\n",i,rdev[i]->desc->name, rdev[i]);
		}
	}


#if 1
	ret = device_create_file(jd9930->dev, &dev_attr_powerup);
	if (ret < 0) {
		dev_err(jd9930->dev, "fail : jd9930 powerup create.\n");
		return ret;
	}
	ret = device_create_file(jd9930->dev, &dev_attr_regs);
	if (ret < 0) {
		dev_err(jd9930->dev, "fail : jd9930 regs create.\n");
		return ret;
	}
#endif

	INIT_WORK(&jd9930->int_work, jd9930_int_func);
	jd9930->int_workqueue = create_singlethread_workqueue("JD9930_INT");
	if(!jd9930->int_workqueue) {
		dev_err(jd9930->dev, "jd9930 int workqueue creating failed !\n");
	}

	gptJD9930_DATA = priv;
    	dev_dbg(jd9930->dev, "%s success\n",__FUNCTION__);
	return 0;
err:
	return ret;
}

static const struct platform_device_id jd9930_pmic_id[] = {
	{ "jd9930-pmic", 0},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(platform, jd9930_pmic_id);

static struct platform_driver jd9930_regulator_driver = {
	.probe = jd9930_regulator_probe,
	.id_table = jd9930_pmic_id,
	.driver = {
		.name = "jd9930-pmic",
	},
};

static int __init jd9930_regulator_init(void)
{
	return platform_driver_register(&jd9930_regulator_driver);
}
subsys_initcall_sync(jd9930_regulator_init);

static void __exit jd9930_regulator_exit(void)
{
	platform_driver_unregister(&jd9930_regulator_driver);
}
module_exit(jd9930_regulator_exit);

#if 0
/*
 * Parse user specified options (`jd9930:')
 * example:
 *   jd9930:pass=2,vcom=-1250000
 */
static int __init jd9930_setup(char *options)
{
	int ret;
	char *opt;
	unsigned long ulResult;
	while ((opt = strsep(&options, ",")) != NULL) {
		if (!*opt)
			continue;
		if (!strncmp(opt, "vcom=", 5)) {
			int offs = 5;
			if (opt[5] == '-')
				offs = 6;
			ret = kstrtoul((const char *)(opt + offs), 0, &ulResult);
			jd9930_vcom = (int) ulResult;
			if (ret < 0)
				return ret;
			jd9930_vcom = -jd9930_vcom;
		}
	}

	return 1;
}

__setup("jd9930:", jd9930_setup);
#endif

struct regulator * jd9930_get_regulator_by_id(int iRegId)
{
	struct regulator *ptRegulator=0;
	int i;
	
	if(!gptJD9930_DATA) {
		printk("%s jd9930 regulator driver not ready !\n",__func__);
		return 0;
	}

	printk("%s(%d) find regid=%d\n",__func__,__LINE__,iRegId);
	for(i=0;i<gptJD9930_DATA->num_regulators;i++) {
		printk(" jd9930 reg[%d]\n",i);
		printk(" jd9930 reg[%d] rdev@%p\n",i,gptJD9930_DATA->rdev[i]);
		printk(" jd9930 reg[%d] rdev desc%p\n",i,gptJD9930_DATA->rdev[i]->desc);
		printk(" jd9930 reg[%d] rdev name=\"%s\"\n",i,gptJD9930_DATA->rdev[i]->desc->name);
		if(gptJD9930_DATA->rdev[i]->desc->id == iRegId) {

			printk(" jd9930 reg[%d] name=\"%s\",jd9930@%p\n",
				i,gptJD9930_DATA->rdev[i]->desc->name,gptJD9930_DATA->jd9930);

			ptRegulator = devm_regulator_get(gptJD9930_DATA->jd9930->dev,gptJD9930_DATA->rdev[i]->desc->name);

			printk(" jd9930 reg[%d] name=\"%s\" @ %p\n",
				i,gptJD9930_DATA->rdev[i]->desc->name,ptRegulator);
			break;
		}
	}
	
	return ptRegulator ;
}
struct regulator * jd9930_get_display_regulator(void)
{
	return jd9930_get_regulator_by_id(JD9930_DISPLAY) ;
}
struct regulator * jd9930_get_vcom_regulator(void)
{
	return jd9930_get_regulator_by_id(JD9930_VCOM) ;
}
struct regulator * jd9930_get_vdd_regulator(void)
{
	if(gptJD9930_DATA) {
		return gptJD9930_DATA->reg_vdd;
	}
	else {
		return 0;
	}
}



/* Module information */
MODULE_DESCRIPTION("JD9930 regulator driver");
MODULE_LICENSE("GPL");

