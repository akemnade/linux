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
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/usb/input.h>
#include <linux/hid.h>
#include <linux/spinlock.h>
#include "inc/elan_tp_define.h"
#include "inc/elan_tp_tap.h"
#include "inc/elan_tp_util.h"
#include "inc/elan_tp_mouse.h"
#include "inc/elan_tp_touch.h"
#include "inc/elan_tp_arrowkey.h"

struct elan_tp_data tpdata;

#ifdef ELAN_TP_MODE
/* EPSON 2013.09 */
void elan_tp_setHomeMode_enable(int enable)
{
	if (enable == 0 || enable == 1) {
		tpdata.finger_flg = enable;
	}
}
int elan_tp_getHomeMode_enable(void)
{
	return tpdata.finger_flg;
}
#endif

/* EPSON 2013.05 */
void elan_tp_setrotate_enable(int enable)
{
	tpdata.rotation_enable = enable;
}
int elan_tp_getrotate_enable(void)
{
	return tpdata.rotation_enable;
}
void elan_tp_setcursorX(int x)
{
	unsigned long temp = (x*546)>>9; // x * 1.066
	tpdata.cursorX = temp;
}
int elan_tp_getcursorX(void)
{
	return tpdata.cursorX;
}
void elan_tp_setcursorY(int y)
{
	unsigned long temp = (y*607)>>9;	// y * 1.185
	tpdata.cursorY = temp;
}
int elan_tp_getcursorY(void)
{
	return tpdata.cursorY;
}

void elan_tp_skip_packages(int nCount)
{
	tpdata.skippackage_count = nCount;
}

void elan_tp_setrotate(int rotate)
{
	/* EPSON 2013.05 */
	if (tpdata.rotation_enable != 1) {
		return;
	}
	if(rotate!=tpdata.tmp_rotate)
	{
		/*EPSON 20130417 Not change rotate while touching.
		 */	
		//tpdata.rotate = rotate;
		tpdata.tmp_rotate = rotate;
		elan_tp_skip_packages(0);
	}
}

void elan_tp_setmousespeed(int speed)
{
	tpdata.mousespeed = speed;
}

void elan_tp_setedgewidth(int width)
{
	tpdata.edgewidth = width;
}

void elan_tp_setonefingertype(int type)
{
	tpdata.onefingertype = type;
}

int elan_tp_getrotate(void)
{
	return tpdata.rotate;
}

int elan_tp_getmousespeed(void)
{
	return tpdata.mousespeed;
}

int elan_tp_getedgewidth(void)
{
	return tpdata.edgewidth;
}

int elan_tp_getonefingertype(void)
{
	return tpdata.onefingertype;
}

// this function must be called from elan_ekt1059_spi::probe()
void elan_tp_restore_rotate(int rotate)
{
	tpdata.rotate = rotate;
	tpdata.tmp_rotate = rotate;
}

void elan_tp_initialize(int width,int height)
{
	tpdata.width = width;
	tpdata.height = height;
	tpdata.edgewidth = DEFAULT_EDGE_WIDTH;
	tpdata.onefingertype = ONEFINGER_REL;
	tpdata.mousespeed = CURSOR_NORMAL;
	//tpdata.mousespeed = CURSOR_FAST;
/* 20130122 EPSON change default ROTATE angle.
	tpdata.rotate = ROTATE_0;
*/
	tpdata.rotate = ROTATE_270;
/* 20130122 EPSON add temporary rotate.*/
	tpdata.tmp_rotate = ROTATE_270;
	tpdata.firstpackage_status = EDGE_NONE;
	tpdata.skippackage_count = 0;
	tpdata.cursorX = 0;
	tpdata.cursorY = 0;
	tpdata.shiftX = 0;
	tpdata.shiftY = 0;
	tpdata.rotation_enable = 0;
#ifdef ELAN_TP_MODE
	/* EPSON 2013.09 */
	tpdata.finger_flg = 0;
#endif
	tpdata.gesture_type = GEST_UNMOVE;
}



bool elan_tp_dobounce(long lastfinger,long currentfinger)
{
	if(lastfinger != currentfinger)
		tpdata.nDropPackageCount=0;

	if(currentfinger!=0)
	{
		if(tpdata.nDropPackageCount < MAXDROPPACKAGECOUNT)
		{
			tpdata.GarbegePackage = tpdata.currentPackage;	
			tpdata.nDropPackageCount++;
			return false;
		}
	}
	return true;
}

void elan_tp_do_touchpad(struct input_dev *input,struct input_dev *input_touch,MULTI_FINGERS* pInRawData)
{
	long lastfingercnt=0;
	long status=0;
	long currentfingercnt = 0;
	int x0,y0,edgestate=EDGE_NONE;

	if(input==NULL || input_touch==NULL || pInRawData==NULL)
	{
		printk("can't find touch pad input device\n");
		return;
	}
	tpdata.input = input;
	tpdata.input_touch = input_touch;
	tpdata.currentPackage.fCount = pInRawData->fCount;
	if(tpdata.currentPackage.fCount>=3)
		return;	

	currentfingercnt = tpdata.currentPackage.fCount;
	lastfingercnt = tpdata.GarbegePackage.fCount;
	/*EPSON 20130417 Not change rotate while touching.*/
	if (tpdata.lastPackage.fCount == 0) {
	  tpdata.rotate = tpdata.tmp_rotate;
	}
	if(elan_tp_dobounce(lastfingercnt,currentfingercnt))
	{
		elan_tp_translateRawdata(tpdata.width,tpdata.height,tpdata.rotate,&(tpdata.currentPackage),pInRawData);
		elan_tp_translateRelativeMT(&tpdata);

		//check is first package or not
		lastfingercnt = tpdata.lastPackage.fCount;
		if(lastfingercnt!=currentfingercnt && currentfingercnt!=0)
		{
			tpdata.currentPackage.isfirstpackage = true;
			tpdata.firstPackage = tpdata.currentPackage;
			elan_tp_enable_tap(true);
			tpdata.skippackage_count = 0;
		}
		else
			tpdata.currentPackage.isfirstpackage = false;
		
		if(currentfingercnt!=0)
		{
			elan_tp_ObjectTracking(&(tpdata.lastPackage),&(tpdata.currentPackage));
	 
			if(tpdata.skippackage_count)
			{
				tpdata.skippackage_count--;
				goto backup_package;
			}
		}

		x0 = tpdata.currentPackage.fingers[0].iX;
		y0 = tpdata.currentPackage.fingers[0].iY;

		if(currentfingercnt!=2)
		{
			edgestate = elan_tp_detect_edge(tpdata.rotate,tpdata.edgewidth,tpdata.width,tpdata.height,x0,y0);
			status = elan_tp_detect_tap(currentfingercnt, x0, y0, edgestate);
			//printk("edge = %d x0=%d y0=%d status=%ld\n",edgestate,x0,y0,status);
		}

		if(tpdata.currentPackage.isfirstpackage==true)
			tpdata.firstpackage_status = edgestate;	

		switch(currentfingercnt)
		{
		case 0:
			tpdata.gesture_type = GEST_UNMOVE;
#ifdef ELAN_TP_MODE		
			if(lastfingercnt==1 && tpdata.onefingertype==ONEFINGER_REL && tpdata.finger_flg == 0)
#else
			if(lastfingercnt==1 && tpdata.onefingertype==ONEFINGER_REL)
#endif
			{
				if(tpdata.firstpackage_status == EDGE_NONE)
					elan_tp_do_mouse(&tpdata,tpdata.lastPackage,tpdata.currentPackage,status);
				else
					elan_tp_do_arrowkey(&tpdata,status);
			}
			else
				elan_tp_ReleaseAllTouchFinger(&tpdata);
			elan_tp_initial_twofinger_status(&tpdata);
			break;
		
		case 1:
#ifdef ELAN_TP_MODE
			if (tpdata.finger_flg == 1) {
				//elan_tp_ReleaseTouchFinger(&tpdata);
				elan_tp_do_touch_HomeMode(&tpdata);
			} else {
#endif
				if(lastfingercnt==2)
					elan_tp_ReleaseAllTouchFinger(&tpdata);	

				if(tpdata.onefingertype==ONEFINGER_REL)
				{		

					if(tpdata.firstpackage_status==EDGE_NONE)
						elan_tp_do_mouse(&tpdata,tpdata.lastPackage,tpdata.currentPackage,status);
					else
					{
						if(edgestate == EDGE_NONE)				
						{
							tpdata.firstpackage_status = EDGE_NONE;
							elan_tp_release_arrowkey(&tpdata);
							elan_tp_enable_tap(false);
							elan_tp_do_mouse(&tpdata,tpdata.lastPackage,tpdata.currentPackage,TAP_OFF);
						}
						else
							elan_tp_do_arrowkey(&tpdata,status);
					}
				}
				else
				{
					elan_tp_do_touch(&tpdata);
				}
#ifdef ELAN_TP_MODE
			}
#endif
			break;
		case 2:
			elan_tp_ReleaseTouchFinger(&tpdata);
			elan_tp_do_touch(&tpdata);
			break;
		}
backup_package:
		tpdata.lastPackage = tpdata.currentPackage;
	}	
}



