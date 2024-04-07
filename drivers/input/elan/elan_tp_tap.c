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


#define SINGLE_TAP_WAIT		250
#define ONEFINGER_HOLD_WAIT	150
#define ONE_ADJ_DIST_TH		50


struct _tap_state 
{
	unsigned long tap_begin;
	long state;
	long FirstTapFinger;
	long bClear;
	long TapPosX;
	long TapPosY;
	long bFingerMove;
	bool bEnable;
};

struct _tap_state TapState;

void elan_tp_tap_init(void)
{
	TapState.state = TAP_STATE_NONE;
	TapState.tap_begin = 0;
	TapState.bClear = 0;
	TapState.bFingerMove = 0;
	TapState.FirstTapFinger = 0;
	TapState.bEnable = true;
	//printk("initial signal tap data!\n");
	
}

void elan_tp_enable_tap(bool bEnable)
{
	TapState.bEnable = bEnable;
	if(bEnable==true)
		elan_tp_tap_init();
}

long elan_tp_CheckSingleTapTimeOverflow(void)
{
	unsigned long diff = jiffies - TapState.tap_begin;
	if(diff > msecs_to_jiffies(SINGLE_TAP_WAIT))
		return 1;
	return 0;
} 

long elan_tp_CheckSingleHoldTimeOverflow(void)
{
	unsigned long diff = jiffies - TapState.tap_begin;
	if(diff > msecs_to_jiffies(ONEFINGER_HOLD_WAIT))
		return 1;
	return 0;
} 

long elan_tp_detect_tap(long fingercnt,long x,long y,int edgestate)
{
	long ret = TAP_OFF;

	if(TapState.bClear==1)
		elan_tp_tap_init();
	if(fingercnt==2)
	{
		elan_tp_tap_init();		
		return ret;
	}

	if(TapState.bEnable==false)
		return ret;

	switch(TapState.state)
	{
	case TAP_STATE_NONE:
		if( fingercnt != 0 )
		{	
			TapState.tap_begin = jiffies;			
			TapState.state = TAP_STATE_WAIT_RISE_ONCE;	
			TapState.FirstTapFinger = fingercnt;	
			TapState.TapPosX = x;
			TapState.TapPosY = y;
			//printk("signal tap entry!\n");
		}
		else
			elan_tp_tap_init();
		break;

	case TAP_STATE_WAIT_RISE_ONCE:
		if( fingercnt == 0  )
		{
			if(elan_tp_CheckSingleTapTimeOverflow()==0 && TapState.bFingerMove==0)
			{	
				TapState.state = TAP_STATE_FINISH;
				ret =  ONEFINGER_TAP_ON;
				//printk("signal tap success\n");				
			}
			elan_tp_tap_init();
		}
		else
		{
			if(elan_tp_CheckSingleHoldTimeOverflow()==0)
			{
				if(TapState.bFingerMove==0 && edgestate==EDGE_NONE)
				{
					long dx = x - TapState.TapPosX;
					long dy = y - TapState.TapPosY;
					long dist = dx*dx + dy*dy;
					if(dist>ONE_ADJ_DIST_TH)
					{
						TapState.bFingerMove = 1;
						//printk("signal tap finger move!\n");
					}
				}
			}
			else
			{
				if(TapState.bFingerMove==0)
					ret = ONEFINGER_HOLD_ON;
			}
		}	
		break;
	}

	return ret;

}



