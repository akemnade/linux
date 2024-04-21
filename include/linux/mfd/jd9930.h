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
#ifndef __LINUX_REGULATOR_JD9930_H_ //[
#define __LINUX_REGULATOR_JD9930_H_

#define JD9930_VDROP_PROC_IN_KERNEL		1

/*
 * PMIC Register Addresses
 */
enum {
    REG_JD9930_TMST_VALUE = 0x0,
    REG_JD9930_VCOM_SET,
    REG_JD9930_VPOS_VNEG_SET,
    REG_JD9930_PWRON_DELAY,
    REG_JD9930_VGH_EXT,
    REG_JD9930_VGL_EXT,
    REG_JD9930_VGHNM_EXT,
    REG_JD9930_VGHNM,
    REG_JD9930_DISA_DELAY,
    REG_JD9930_XON_DELAY,
    REG_JD9930_XON_LEN,
    REG_JD9930_CTRL_REG1,
    REG_JD9930_CTRL_REG2,
    JD9930_REG_NUM,
};

//#define JD9930_MAX_REGISTER   0xFF

/*
 * Bitfield macros that use rely on bitfield width/shift information.
 */
#define JD9930_BITFMASK(field) (((1U << (JD9930_##field ## _WID)) - 1) << (JD9930_##field ## _LSH))
#define JD9930_BITFVAL(field, val) ((val) << (JD9930_##field ## _LSH))
#define JD9930_BITFEXT(var, bit) ((var & JD9930_BITFMASK(bit)) >> (JD9930_##bit ## _LSH))


////////////////////////////////
/* Power On Delay Time (03h)*/
#define JD9930_TDLY4_LSH 6
#define JD9930_TDLY4_WID 2
#define JD9930_TDLY3_LSH 4
#define JD9930_TDLY3_WID 2
#define JD9930_TDLY2_LSH 2
#define JD9930_TDLY2_WID 2
#define JD9930_TDLY1_LSH 0
#define JD9930_TDLY1_WID 2

/////////////////////////////////
// CONTROL REG1 (0Bh)
#define JD9930_SS_TIME_LSH	6
#define JD9930_SS_TIME_WID	2
// V3P3_EN 
#define JD9930_V3P3_EN_LSH	1
#define JD9930_V3P3_EN_WID	1
// night mode/day mode selector .
#define JD9930_NM_LSH	0
#define JD9930_NM_WID	1 

/////////////////////////////////
// CONTROL REG2 (0Ch)
// VP boost converter current limit setting .
#define JD9930_VP_CL_LSH	2
#define JD9930_VP_CL_WID	2
// VN buck-boost converter current limit setting .
#define JD9930_VN_CL_LSH	0
#define JD9930_VN_CL_WID	2


/*
 * VCOM Definitions
 *
 * The register fields accept voltages in the range 0V to -2.75V, but the
 * VCOM parametric performance is only guaranteed from -0.3V to -2.5V.
 */
#define JD9930_VCOM_MIN_uV   -5000000
#define JD9930_VCOM_MAX_uV          0
//#define JD9930_VCOM_MIN_SET         0
//#define JD9930_VCOM_MAX_SET		255
//#define JD9930_VCOM_BASE_uV     19607
#define JD9930_VCOM_STEP_uV     19607



//#define JD9930_VCOM_MIN_VAL         0
//#define JD9930_VCOM_MAX_VAL       255

#define JD9930_WAKEUP0_MS	4
#define JD9930_WAKEUP_MS	20
#define JD9930_SLEEP0_MS	4

//#define TURNOFF_PWR_EPD_UPDATED	1
struct regulator_init_data;

typedef void (JD9930_INTEVT_CB)(int iEVENT);

struct jd9930 {
	/* chip revision */
	struct device *dev;
	struct jd9930_platform_data *pdata;

	/* Platform connection */
	struct i2c_client *i2c_client;

	/* Timings */
	unsigned char on_delay1;
	unsigned char on_delay2;
	unsigned char on_delay3;
	unsigned char on_delay4;

	unsigned char vgh_ext;
	unsigned char vgl_ext;
	unsigned char vghnm_ext;
	unsigned char vghnm;
	unsigned char disa_delay;
	unsigned char xon_delay;
	unsigned char xon_len;


	//unsigned char bFaultFlags,bOPMode;

	/* GPIOs */
	struct gpio_desc *gpio_pmic_pwrgood;
	struct gpio_desc *gpio_pmic_xon_ctrl;
	struct gpio_desc *gpio_pmic_ts_en;
	struct gpio_desc *gpio_pmic_powerup;
	//int gpio_pmic_powerup_stat;
	struct gpio_desc *gpio_pmic_pwrall;
	

	/* JD9930 part variables */
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

	//int iNeedReloadVCOM; // VCOM must be loaded again .
	//unsigned regVCOM1,regVCOM2;
	
	int int_state; // interrupt state .
	JD9930_INTEVT_CB *pfnINTCB;

	struct work_struct int_work;
	struct workqueue_struct *int_workqueue;

	// registers cache . 
	unsigned char regTMST_VALUE;
	unsigned char regVCOM_SET;
	unsigned char regVPOS_VNEG_SET;
	unsigned char regPWRON_DELAY;
	unsigned char regVGH_EXT;
	unsigned char regVGL_EXT;
	unsigned char regVGHNM_EXT;
	unsigned char regVGHNM;
	unsigned char regDISA_DELAY;
	unsigned char regXON_DELAY;
	unsigned char regXON_LEN;
	unsigned char regCTRL_REG1;
	unsigned char regCTRL_REG2;


	unsigned int turnoff_delay_ep3v3; // EP_VMAX off the delay few time than turn off EP_3V3
	unsigned int dwPwrOnOffFlags;// flags about power on/off . 

};

enum {
    /* In alphabetical order */
    JD9930_DISPLAY, /* virtual master enable */
    JD9930_VCOM,
    JD9930_VPOSNEG,
    JD9930_TMST,
	JD9930_VP3V3,
    JD9930_NUM_REGULATORS,
};

/*
 * Declarations
 */
struct regulator_init_data;
struct jd9930_regulator_data;

struct jd9930_platform_data {

	int gpio_pmic_pwrgood;
	int gpio_pmic_xon_ctrl;
	int gpio_pmic_ts_en;
	int gpio_pmic_powerup;
	int gpio_pmic_powerup_stat;
	int gpio_pmic_pwrall;
	int vcom_uV;

	/* PMIC */
	struct jd9930_regulator_data *regulators;
	int num_regulators;
};

struct jd9930_regulator_data {
	int id;
	struct regulator_init_data *initdata;
	struct device_node *reg_node;
};

int jd9930_chip_power(struct jd9930 *jd9930,int iIsON);
int jd9930_reg_write(struct jd9930 *jd9930,int reg_num, const unsigned int reg_val);
int jd9930_reg_read(struct jd9930 *jd9930,int reg_num, unsigned int *reg_val);

int jd9930_get_temperature(struct jd9930 *jd9930,int *O_piTemperature);
int jd9930_get_vcom(struct jd9930 *jd9930,int *O_piVCOMmV);
int jd9930_set_vcom(struct jd9930 *jd9930,int iVCOMmV,int iIsWriteToFlash);
//int jd9930_restore_vcom(struct jd9930 *jd9930);

int jd9930_int_state_clear(struct jd9930 *jd9930);
int jd9930_int_state_get(struct jd9930 *jd9930);

int jd9930_int_callback_setup(struct jd9930 *jd9930,JD9930_INTEVT_CB fnCB);

int jd9930_get_power_status(struct jd9930 *jd9930,int iIsGetCached,int *O_PG,unsigned char *O_pbFaults);

int jd9930_switch_nightmode(struct jd9930 *jd9930,int iIsNightMode);
int jd9930_regs_init(struct jd9930 *jd9930);
struct jd9930 *jd9930_get(void);

/*
 * jd9930_to_reg_val(): Creates a register value with new data
 *
 * Creates a new register value for a particular field.  The data
 * outside of the new field is not modified.
 *
 * @cur_reg: current value in register
 * @reg_mask: mask of field bits to be modified
 * @fld_val: new value for register field.
 */
#if 0
static unsigned int jd9930_to_reg_val(unsigned int cur_reg, unsigned int fld_mask,
							   unsigned int fld_val)
{
	return (cur_reg & (~fld_mask)) | fld_val;
}
#endif 

#define JD9930_RET_SUCCESS							0
#define JD9930_RET_GPIOINITFAIL					(-1)
#define JD9930_RET_MEMNOTENOUGH					(-2)
#define JD9930_RET_NOTINITEDSTATE				(-3)
#define JD9930_RET_PWRDWNWORKPENDING		(-4)
#define JD9930_RET_I2CCHN_NOTFOUND			(-5)
#define JD9930_RET_I2CTRANS_ERR					(-6)
#define JD9930_RET_OBJ_ERR					(-7)


#define JD9930_REG_FLD_SET(ptJD9930,_regName,_bFieldName,_bSetVal)		\
({\
	unsigned int fld_mask,fld_val;\
	unsigned int cur_reg,new_reg_val;\
	\
	fld_mask=(((1U << (_bFieldName##_WID)) - 1) << (_bFieldName##_LSH));\
	fld_val=((_bSetVal) << (_bFieldName##_LSH));\
	cur_reg=ptJD9930->reg##_regName;\
	new_reg_val = jd9930_to_reg_val(cur_reg,fld_mask,fld_val);\
	ptJD9930->reg##_regName=(unsigned char)new_reg_val;\
	new_reg_val;\
})

#define JD9930_REG_FLD_WRITE(ptJD9930,_regName,_bFieldName,_bSetVal)		\
({\
	int _iRet=JD9930_RET_SUCCESS;\
	int _iChk;\
	unsigned int fld_mask,fld_val;\
	unsigned int cur_reg,new_reg_val;\
	\
	fld_mask=(((1U << (_bFieldName##_WID)) - 1) << (_bFieldName##_LSH));\
	fld_val=((_bSetVal) << (_bFieldName##_LSH));\
	cur_reg=ptJD9930->reg##_regName;\
	new_reg_val = jd9930_to_reg_val(cur_reg,fld_mask,fld_val);\
	\
	if(cur_reg!=new_reg_val) {\
		DBG_MSG("%s():reg %s 0x%x=>0x%x\n",__FUNCTION__,#_regName,cur_reg,new_reg_val);\
		_iChk = jd9930_reg_write(ptJD9930,REG_JD9930_##_regName,new_reg_val);\
		if(PMIC_SUCCESS!=_iChk) {\
 			_iRet=JD9930_RET_I2CTRANS_ERR;\
		}\
 		else {\
			ptJD9930->reg##_regName=(unsigned char)new_reg_val;\
		}\
	}\
	else {\
	}\
	_iRet;\
})

#define JD9930_REG_WRITE_EX(ptJD9930,_regName,_regSetVal)		\
({\
	int _iRet=JD9930_RET_SUCCESS;\
	int _iChk;\
	_iChk = jd9930_reg_write(ptJD9930,REG_JD9930_##_regName,_regSetVal);\
	if(0!=_iChk) {\
 		_iRet=JD9930_RET_I2CTRANS_ERR;\
	}\
 	else {\
		ptJD9930->reg##_regName=(unsigned char)_regSetVal;\
	}\
 	_iRet;\
})

#define JD9930_REG_WRITE(ptJD9930,_regName)		\
({\
	int _iRet=JD9930_RET_SUCCESS;\
	_iRet = JD9930_REG_WRITE_EX(ptJD9930,_regName,ptJD9930->reg##_regName);\
 	_iRet;\
})

#define JD9930_REG_READ_EX(ptJD9930,_regName,_into_cache)	\
({\
	int _iRet=JD9930_RET_SUCCESS;\
	int _iChk;\
	unsigned int cur_reg;\
	\
	_iChk = jd9930_reg_read(ptJD9930,REG_JD9930_##_regName,&cur_reg);\
	if(0!=_iChk) {\
 		_iRet=JD9930_RET_I2CTRANS_ERR;\
	}\
	else {\
 		if(_into_cache) {\
			(ptJD9930)->reg##_regName=(unsigned char)cur_reg;\
 		}\
		_iRet=cur_reg;\
	}\
	_iRet;\
})

#define JD9930_REG_READ(ptJD9930,_regName)	\
	JD9930_REG_READ_EX(ptJD9930,_regName,0)

#define JD9930_REG_FLD_GET(ptJD9930,_regName,_fldName)	\
({\
	int _iRet;\
	{\
		unsigned char fld_mask,reg;\
		fld_mask=(((1U << (_fldName##_WID)) - 1) << (_fldName##_LSH));\
		reg=((ptJD9930)->reg##_regName&fld_mask)>>_fldName##_LSH;\
		_iRet = (int)(reg) ;\
	}\
	_iRet;\
})

#define JD9930_REG_GET(ptJD9930,_regName)		\
	(ptJD9930)->reg##_regName

#define JD9930_REG_SET(ptJD9930,_regName,val)		\
	(ptJD9930)->reg##_regName=val


#endif //] __LINUX_REGULATOR_JD9930_H_

