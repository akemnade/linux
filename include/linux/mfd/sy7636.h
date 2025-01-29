/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
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
#ifndef __LINUX_REGULATOR_SY7636_H_ //[
#define __LINUX_REGULATOR_SY7636_H_

#define EN_ONOFF_WITH_RAILS	1
#define SY7636_VCOM_EXTERNAL	1
#define SY7636_LIGHTNESS_ENABLE	1
#define SY7636_READY_MS		1

#define SY7636_VDROP_PROC_IN_KERNEL		1

/* to be removed again */
#define PMIC_SUCCESS 0
/*
 * PMIC Register Addresses
 */
enum {
    REG_SY7636_OPMODE = 0x0,
    REG_SY7636_VCOM_ADJ1,
    REG_SY7636_VCOM_ADJ2,
    REG_SY7636_VLDO_ADJ,
    REG_SY7636_VPDD_LEN,
    REG_SY7636_VEE_VP_EXT,
    REG_SY7636_PWRON_DLY,
    REG_SY7636_FAULTFLAGS,
    REG_SY7636_THERM,
    SY7636_REG_NUM,
};

#define SY7636_MAX_REGISTER   0xFF

/*
 * Bitfield macros that use rely on bitfield width/shift information.
 */
#define BITFMASK(field) (((1U << (field ## _WID)) - 1) << (field ## _LSH))
#define BITFVAL(field, val) ((val) << (field ## _LSH))
#define BITFEXT(var, bit) ((var & BITFMASK(bit)) >> (bit ## _LSH))


#define ALL_LSH 0
#define ALL_WID 8

/* OP MODE (0h )*/
#define RAILS_ON_LSH 7
#define RAILS_ON_WID 1
#define VCOM_MANUAL_LSH 6
#define VCOM_MANUAL_WID 1
#define LIGHTNESS_LSH 5
#define LIGHTNESS_WID 1
#define VDDH_DISABLE_LSH 4
#define VDDH_DISABLE_WID 1
#define VEE_DISABLE_LSH 3
#define VEE_DISABLE_WID 1
#define VPOS_DISABLE_LSH 2
#define VPOS_DISABLE_WID 1
#define VNEG_DISABLE_LSH 1
#define VNEG_DISABLE_WID 1
#define VCOM_DISABLE_LSH 0
#define VCOM_DISABLE_WID 1
// all rails bits .
#define RAILS_DISABLE_LSH 0
#define RAILS_DISABLE_WID 5


//VCOM_ADJ2 (02h) : VCOM2 and VDDH_EXT 
#define VCOM2_B8_LSH	7
#define VCOM2_B8_WID	1
#define VDDH_EXT_LSH	0
#define VDDH_EXT_WID	5



/* VLDO voltage adjustment control (03h)*/
#define VLDO_ADJ_LSH	5 // VPOS & VNEG voltage adjustment . 
#define VLDO_ADJ_WID	3
#define VPDD_ADJ_LSH	0 // VPDD voltage setting at lightness mode . 
#define VPDD_ADJ_WID	5 


/* VPDD LEN Control(04h) */
#define VPDD_LEN_LSH	0
#define VPDD_LEN_WID	5

/* VEE_EXT & VP_EXT Control (05h) */
#define VP_EXT_LSH		5
#define VP_EXT_WID		2
#define VEE_EXT_LSH		0
#define VEE_EXT_WID		5

/* Power On Delay Time (06h)*/
#define TDLY4_LSH 6
#define TDLY4_WID 2
#define TDLY3_LSH 4
#define TDLY3_WID 2
#define TDLY2_LSH 2
#define TDLY2_WID 2
#define TDLY1_LSH 0
#define TDLY1_WID 2

/* FAULT_FLAGS (07h) */
#define FAULTS_LSH 1
#define FAULTS_WID 4
#define PG_LSH 0
#define PG_WID 1

/*
 * VCOM Definitions
 *
 * The register fields accept voltages in the range 0V to -2.75V, but the
 * VCOM parametric performance is only guaranteed from -0.3V to -2.5V.
 */
#define SY7636_VCOM_MAX_uV   5110000
#define SY7636_VCOM_MIN_uV          0
#define SY7636_VCOM_MIN_SET         0
#define SY7636_VCOM_MAX_SET       511
#define SY7636_VCOM_BASE_uV     10000
#define SY7636_VCOM_STEP_uV     10000



#define SY7636_VCOM_MIN_VAL         0
#define SY7636_VCOM_MAX_VAL       255

#define SY7636_WAKEUP0_MS	4
#define SY7636_WAKEUP_MS	20
#define SY7636_SLEEP0_MS	4


struct regulator_init_data;

typedef void *(SY7636_INTEVT_CB)(int iEVENT);

struct sy7636 {
	/* chip revision */
	struct device *dev;
	struct sy7636_platform_data *pdata;

	/* Platform connection */
	struct i2c_client *i2c_client;

	/* Timings */
	u32 on_delay1;
	u32 on_delay2;
	u32 on_delay3;
	u32 on_delay4;

	unsigned int VLDO;

	//unsigned char bFaultFlags,bOPMode;

	/* GPIOs */
	int gpio_pmic_pwrgood;
	int gpio_pmic_vcom_ctrl;
	int gpio_pmic_powerup;
	int gpio_pmic_powerup_stat;
	int gpio_pmic_pwrall;
	

	/* SY7636 part variables */
	int vcom_uV;

	bool need_reinit;// need reinit registers . 

	unsigned long jiffies_chip_on;
	unsigned long jiffies_chip_wake;

	/* powerup/powerdown wait time */
	//int max_wait;

	/* Dynamically determined polarity for PWRGOOD */
	int pwrgood_polarity;

	//int gpio_pmic_powerup_stat;
	//int gpio_pmic_pwrall_stat;
	int fake_vp3v3_stat;

	//int iNeedReloadVCOM; // VCOM must be loaded again .
	//unsigned regVCOM1,regVCOM2;
	
	int int_state; // interrupt state .
	SY7636_INTEVT_CB *pfnINTCB;

	struct work_struct int_work;
	struct workqueue_struct *int_workqueue;

	unsigned char regOPMODE;
	unsigned char regVCOM_ADJ1;
	unsigned char regVCOM_ADJ2;
	unsigned char regVLDO_ADJ;
	unsigned char regVPDD_LEN;
	unsigned char regVEE_VP_EXT;
	unsigned char regPWRON_DLY;
	unsigned char regFAULTFLAGS;
	unsigned char regTHERM;

	unsigned int turnoff_delay_ep3v3; // EP_VMAX off the delay few time than turn off EP_3V3
};

enum {
    /* In alphabetical order */
    SY7636_DISPLAY, /* virtual master enable */
    SY7636_VCOM,
    SY7636_TMST,
		SY7636_VP3V3,
    SY7636_NUM_REGULATORS,
};

/*
 * Declarations
 */
struct regulator_init_data;
struct sy7636_regulator_data;

struct sy7636_platform_data {
	unsigned int on_delay1;
	unsigned int on_delay2;
	unsigned int on_delay3;
	unsigned int on_delay4;

	unsigned int VLDO;

	int gpio_pmic_pwrgood;
	int gpio_pmic_vcom_ctrl;
	int gpio_pmic_powerup;
	int gpio_pmic_powerup_stat;
	int gpio_pmic_pwrall;
	int vcom_uV;

	/* PMIC */
	struct sy7636_regulator_data *regulators;
	int num_regulators;
};

struct sy7636_regulator_data {
	int id;
	struct regulator_init_data *initdata;
	struct device_node *reg_node;
};

int sy7636_reg_write(struct sy7636 *sy7636,int reg_num, const unsigned int reg_val);
int sy7636_reg_read(struct sy7636 *sy7636,int reg_num, unsigned int *reg_val);


int sy7636_EN(struct sy7636 *sy7636,int iEN);

int sy7636_get_temperature(struct sy7636 *sy7636,int *O_piTemperature);
int sy7636_get_vcom(struct sy7636 *sy7636,int *O_piVCOMmV);
int sy7636_set_vcom(struct sy7636 *sy7636,int iVCOMmV,int iIsWriteToFlash);
//int sy7636_restore_vcom(struct sy7636 *sy7636);

int sy7636_int_state_clear(struct sy7636 *sy7636);
int sy7636_int_state_get(struct sy7636 *sy7636);

int sy7636_int_callback_setup(struct sy7636 *sy7636,SY7636_INTEVT_CB fnCB);

int sy7636_get_FaultFlags(struct sy7636 *sy7636,int iIsGetCached);
const char *sy7636_get_fault_string(unsigned char bFault);
int sy7636_get_power_status(struct sy7636 *sy7636,int iIsGetCached,int *O_PG,unsigned char *O_pbFaults);

int sy7636_regs_init(struct sy7636 *sy7636);


/*
 * sy7636_to_reg_val(): Creates a register value with new data
 *
 * Creates a new register value for a particular field.  The data
 * outside of the new field is not modified.
 *
 * @cur_reg: current value in register
 * @reg_mask: mask of field bits to be modified
 * @fld_val: new value for register field.
 */
static unsigned int sy7636_to_reg_val(unsigned int cur_reg, unsigned int fld_mask,
							   unsigned int fld_val)
{
	return (cur_reg & (~fld_mask)) | fld_val;
}

#define SY7636_RET_SUCCESS							0
#define SY7636_RET_GPIOINITFAIL					(-1)
#define SY7636_RET_MEMNOTENOUGH					(-2)
#define SY7636_RET_NOTINITEDSTATE				(-3)
#define SY7636_RET_PWRDWNWORKPENDING		(-4)
#define SY7636_RET_I2CCHN_NOTFOUND			(-5)
#define SY7636_RET_I2CTRANS_ERR					(-6)


#define SY7636_REG_FLD_SET(ptSY7636,_regName,_bFieldName,_bSetVal)		\
({\
	unsigned int fld_mask,fld_val;\
	unsigned int cur_reg,new_reg_val;\
	\
	fld_mask=(((1U << (_bFieldName##_WID)) - 1) << (_bFieldName##_LSH));\
	fld_val=((_bSetVal) << (_bFieldName##_LSH));\
	cur_reg=ptSY7636->reg##_regName;\
	new_reg_val = sy7636_to_reg_val(cur_reg,fld_mask,fld_val);\
	ptSY7636->reg##_regName=(unsigned char)new_reg_val;\
	new_reg_val;\
})

#define SY7636_REG_FLD_WRITE(ptSY7636,_regName,_bFieldName,_bSetVal)		\
({\
	int _iRet=SY7636_RET_SUCCESS;\
	int _iChk;\
	unsigned int fld_mask,fld_val;\
	unsigned int cur_reg,new_reg_val;\
	\
	fld_mask=(((1U << (_bFieldName##_WID)) - 1) << (_bFieldName##_LSH));\
	fld_val=((_bSetVal) << (_bFieldName##_LSH));\
	cur_reg=ptSY7636->reg##_regName;\
	new_reg_val = sy7636_to_reg_val(cur_reg,fld_mask,fld_val);\
	\
	if(cur_reg!=new_reg_val) {\
		DBG_MSG("%s():reg %s 0x%x=>0x%x\n",__FUNCTION__,#_regName,cur_reg,new_reg_val);\
		_iChk = sy7636_reg_write(ptSY7636,REG_SY7636_##_regName,new_reg_val);\
		if(PMIC_SUCCESS!=_iChk) {\
 			_iRet=SY7636_RET_I2CTRANS_ERR;\
		}\
 		else {\
			ptSY7636->reg##_regName=(unsigned char)new_reg_val;\
		}\
	}\
	else {\
	}\
	_iRet;\
})

#define SY7636_REG_WRITE_EX(ptSY7636,_regName,_regSetVal)		\
({\
	int _iRet=SY7636_RET_SUCCESS;\
	int _iChk;\
	_iChk = sy7636_reg_write(ptSY7636,REG_SY7636_##_regName,_regSetVal);\
	if(PMIC_SUCCESS!=_iChk) {\
 		_iRet=SY7636_RET_I2CTRANS_ERR;\
	}\
 	else {\
		ptSY7636->reg##_regName=(unsigned char)_regSetVal;\
	}\
 	_iRet;\
})

#define SY7636_REG_WRITE(ptSY7636,_regName)		\
({\
	int _iRet=SY7636_RET_SUCCESS;\
	_iRet = SY7636_REG_WRITE_EX(ptSY7636,_regName,ptSY7636->reg##_regName);\
 	_iRet;\
})

#define SY7636_REG_READ_EX(ptSY7636,_regName,_into_cache)	\
({\
	int _iRet=SY7636_RET_SUCCESS;\
	int _iChk;\
	unsigned int cur_reg;\
	\
	_iChk = sy7636_reg_read(ptSY7636,REG_SY7636_##_regName,&cur_reg);\
	if(PMIC_SUCCESS!=_iChk) {\
 		_iRet=SY7636_RET_I2CTRANS_ERR;\
	}\
	else {\
 		if(_into_cache) {\
			(ptSY7636)->reg##_regName=(unsigned char)cur_reg;\
 		}\
		_iRet=cur_reg;\
	}\
	_iRet;\
})

#define SY7636_REG_READ(ptSY7636,_regName)	\
	SY7636_REG_READ_EX(ptSY7636,_regName,0)

#define SY7636_REG_FLD_GET(ptSY7636,_regName,_fldName)	\
({\
	int _iRet;\
	{\
		unsigned char fld_mask,reg;\
		fld_mask=(((1U << (_fldName##_WID)) - 1) << (_fldName##_LSH));\
		reg=((ptSY7636)->reg##_regName&fld_mask)>>_fldName##_LSH;\
		_iRet = (int)(reg) ;\
	}\
	_iRet;\
})

#define SY7636_REG_GET(ptSY7636,_regName)		\
	(ptSY7636)->reg##_regName

#define SY7636_REG_SET(ptSY7636,_regName,val)		\
	(ptSY7636)->reg##_regName=val

#define SY7636_SET_VCOM_MV(ptSY7636,iVCOMmV)	\
	{\
		unsigned short wVCOM_val;\
		wVCOM_val = (unsigned short)((-iVCOMmV)/10);\
		(ptSY7636)->regVCOM_ADJ1 = (unsigned char)(wVCOM_val&0xff);\
		if(wVCOM_val&0x100) {\
			(ptSY7636)->regVCOM_ADJ2 |= 0x80;\
		}\
		else {\
			(ptSY7636)->regVCOM_ADJ2 &= ~0x80;\
		}\
	}


#endif //] __LINUX_REGULATOR_SY7636_H_

