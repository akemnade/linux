/*
 * Copyright (C) 2013 ELAN Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef	ELAN_MULTI_TOUCH
#define	ELAN_MULTI_TOUCH

#define TWOFINGERQUEUEMAXCNT	4
#define BOUNDARY_WIDTH  	20
#define BOUNDARY_HEIGHT		20
#define TOUCH_AREA		20
#define MAXFINGERCOUNT		10
#define MAXDROPPACKAGECOUNT	1
#define DEFAULT_EDGE_WIDTH	120
 
/* EPSON 2013.05 */
#define ABS_AXIS_X_MAX	1024
#define ABS_AXIS_Y_MAX	640

//#define ELAN_TP_MODE
//#define ESD_PROVISION		// when ESD_PROVISION is valid, it must be valid too in linux/kxti9.h .

typedef struct _INPUT_PACKAGE
{
	long status;
	long number;
	long iX;
	long iY;
#ifdef ELAN_TP_MODE
	long RAWiX;
	long RAWiY;
#endif
	long MTiX;
	long MTiY;
}INPUT_PACKAGE, *PINPUT_PACKAGE;

typedef struct _MULTI_FINGERS
{
	long fCount;
	long Gesture;
	bool isfirstpackage;
	struct _INPUT_PACKAGE fingers[MAXFINGERCOUNT];
}MULTI_FINGERS, *PMULTI_FINGERS;


enum {
	GEST_UNKNOW = 0,
	GEST_ZOOM,
	GEST_FLICK_X,
	GEST_FLICK_Y,
	GEST_UNMOVE ,
	GEST_ONEMOVE,
	GEST_MOUSE
};

enum
{
	TAP_STATE_NONE = 0,
	TAP_STATE_WAIT_RISE_ONCE,
	TAP_STATE_WAIT_TWICE,
	TAP_STATE_WAIT_RISE_TWICE,
	TAP_STATE_FINISH,
	TAP_OFF,
	ONEFINGER_TAP_ON,
	ONEFINGER_HOLD_ON,
	TAP_STATE_NUM
};

enum {CURSOR_SLOW,CURSOR_NORMAL,CURSOR_FAST};
enum {ONEFINGER_REL,ONEFINGER_ABS};
enum {ROTATE_0,ROTATE_90,ROTATE_180,ROTATE_270};
enum {EDGE_NONE,EDGE_LEFT,EDGE_RIGHT,EDGE_TOP,EDGE_BOTTOM,CORNER_NONE,CORNER_LB,CORNER_RB,CORNER_LT,CORNER_RT};
enum {ARROW_RELEASE,ARROW_HOLD,ARROW_MOVE,ARROW_CLICK};

struct elan_tp_data
{
	struct input_dev *input;
	struct input_dev *input_touch;

	int  	width;
	int 	height;
	int  	rotate;
	int  	tmp_rotate;//EPSON 20130417 not change rotate while touching
	int  	edgewidth;
	int	 	onefingertype;
	int	 	mousespeed;
	int	 	firstpackage_status;
	int  	skippackage_count;
	/* EPSON 2013.05 */
	int  	shiftX;
	int  	shiftY;
	int  	rotation_enable;

	long  zoom_score;
	long  flick_score_x;
	long  flick_score_y;
	long  onemove_score;
	long  nDropPackageCount;
	long  TwoFingerQueueCount;
	long  TwoFigGestStatus;
	long  LastTwoFigGestStatus;
	/* EPSON 2013.05 */
	long  	cursorX;
	long  	cursorY;
	/* EPSON 2013.09 */
#ifdef ELAN_TP_MODE
	int		finger_flg;
#endif
	int		gesture_type;


	MULTI_FINGERS refPackage;
	MULTI_FINGERS currentPackage;
	MULTI_FINGERS firstPackage;
	MULTI_FINGERS lastPackage;
	MULTI_FINGERS GarbegePackage;
	MULTI_FINGERS TowFingerQueue[TWOFINGERQUEUEMAXCNT];


};

void elan_tp_setrotate_enable(int enable);
int elan_tp_getrotate_enable(void);
void elan_tp_setcursorX(int x);
int elan_tp_getcursorX(void);
void elan_tp_setcursorY(int y);
int elan_tp_getcursorY(void);

void elan_tp_setrotate(int rotate);
void elan_tp_setmousespeed(int speed);
void elan_tp_setedgewidth(int width);
void elan_tp_setonefingertype(int type);
int elan_tp_getrotate(void);
int elan_tp_getmousespeed(void);
int elan_tp_getedgewidth(void);
int elan_tp_getonefingertype(void);
#ifdef ELAN_TP_MODE
void elan_tp_setHomeMode_enable(int enable);
int elan_tp_getHomeMode_enable(void);
#endif
void elan_tp_initialize(int width,int height);
void elan_tp_do_touchpad(struct input_dev *input,struct input_dev *input_touch,MULTI_FINGERS* pInRawData);
void elan_tp_restore_rotate(int rotate); // this function must be called from elan_ekt1059_spi::probe()
#endif

