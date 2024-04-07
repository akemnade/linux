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
#include "inc/elan_tp_arrowkey.h"

#define KEYHOLD_GAP	2
#define KEYMOVE_GAP	15	
// #define KEYMOVE_GAP	60	
#define KEYHOLD_CNT	20
/* 20130409 EPSON for adjusting Key Scroll Speed.
 * This value depends on KEYHOLD_CNT,
 * please set up in the range 0 < KEYSCROLL_SPEED < KEYHOLD_CNT.
 */
#define KEYSCROLL_SPEED 10

struct _key_state 
{
	int start_x;
	int start_y;
	int distance_x;
	int distance_y;
	int key;
	int keystatus;
	int lastkeystatus;
	int key_generate;
	int destroy_hold;
	int hold_count;
};

struct _key_state KeyState;

void elan_tp_key_initial(void)
{
	KeyState.start_x=0;
	KeyState.start_y=0;
	KeyState.distance_x=0;
	KeyState.distance_y=0;
	KeyState.key=0;
	KeyState.keystatus=0;
	KeyState.lastkeystatus=0;
	KeyState.key_generate=0;
	KeyState.destroy_hold=0;
	KeyState.hold_count=0;
}

int elan_tp_detect_edge_for_arrowkey(int rotate,int edge_percent,int tp_width,int tp_height,int x,int y)
{
	int nRet = EDGE_NONE;
	int edge_width = edge_percent; 
	int edge_height = edge_percent;
	int temp;
	if(rotate==ROTATE_90 || rotate==ROTATE_270)
	{
		temp = tp_width;
		tp_width = tp_height;
		tp_height = temp;	
	}	

	//detect edge
	if(x<= edge_width)
		nRet = EDGE_LEFT;
	else if(x>= (tp_width-edge_width))
		nRet = EDGE_RIGHT;
	else if(y>= (tp_height-edge_height))
		nRet = EDGE_TOP;
	else if(y<= edge_height)
		nRet = EDGE_BOTTOM;

	return nRet;
}

void elan_tp_release_arrowkey(struct elan_tp_data* tpdata)
{
	//if(KeyState.keystatus==ARROW_HOLD)
	//{
		input_report_key(tpdata->input, KEY_DOWN, 0);
		input_sync(tpdata->input);
		input_report_key(tpdata->input, KEY_UP, 0);
		input_sync(tpdata->input);
		input_report_key(tpdata->input, KEY_LEFT, 0);
		input_sync(tpdata->input);
		input_report_key(tpdata->input, KEY_RIGHT, 0);
		input_sync(tpdata->input);
	//}
	elan_tp_key_initial();			
}



int elan_tp_do_arrowkey(struct elan_tp_data* tpdata,int tap_status)
{

	int edge_status = tpdata->firstpackage_status;

	if(tpdata->currentPackage.fCount==0)
		KeyState.keystatus = ARROW_RELEASE;
	else
		KeyState.keystatus  = ARROW_MOVE;

	//first point must set as last point
	if(tpdata->currentPackage.isfirstpackage==true)
	{
		elan_tp_key_initial();
		KeyState.start_x = tpdata->currentPackage.fingers[0].iX;
		KeyState.start_y = tpdata->currentPackage.fingers[0].iY;
	}
	else
	{
		if(KeyState.keystatus==ARROW_MOVE)	
		{
			if(KeyState.key_generate==0)
			{
				KeyState.distance_x = (tpdata->currentPackage.fingers[0].iX - KeyState.start_x);
				KeyState.distance_y = (tpdata->currentPackage.fingers[0].iY - KeyState.start_y);
			}
			else
			{
				int dx = abs(tpdata->currentPackage.fingers[0].iX - tpdata->lastPackage.fingers[0].iX);
				int dy = abs(tpdata->currentPackage.fingers[0].iY - tpdata->lastPackage.fingers[0].iY);
				if(dx<=KEYHOLD_GAP && dy<=KEYHOLD_GAP)
					KeyState.keystatus = ARROW_HOLD;

				if(KeyState.lastkeystatus==ARROW_HOLD && KeyState.keystatus==ARROW_MOVE)
					KeyState.destroy_hold=1;
			}
		}
	}


	switch(edge_status)
	{
	case CORNER_LB:
	case CORNER_RB:
	case CORNER_LT:
	case CORNER_RT:
		if(tap_status==ONEFINGER_TAP_ON && KeyState.keystatus==ARROW_RELEASE)
		{
			KeyState.keystatus = ARROW_CLICK;
			KeyState.key = KEY_ENTER;
			KeyState.key_generate=1;
			//printk("click corner %d\n",edge_status);
		} else {
			long x, y;
			int rotate = tpdata->rotate;
			int nRet = EDGE_NONE;
			
			x = tpdata->currentPackage.fingers[0].iX;
			y = tpdata->currentPackage.fingers[0].iY;
			
			nRet = elan_tp_detect_edge_for_arrowkey(rotate,
				DEFAULT_EDGE_WIDTH,
				tpdata->width, tpdata->height,
				x, y);
			
			switch (nRet) {
			case EDGE_LEFT:
			case EDGE_RIGHT:
				if(abs(KeyState.distance_y)>KEYMOVE_GAP)
				{
					if(KeyState.distance_y<0)
						KeyState.key = KEY_DOWN;
					else
						KeyState.key = KEY_UP;
					if((KeyState.keystatus==ARROW_RELEASE && KeyState.lastkeystatus==ARROW_MOVE) ||
						(KeyState.keystatus==ARROW_RELEASE && KeyState.lastkeystatus==ARROW_HOLD))
						KeyState.keystatus = ARROW_CLICK;
					KeyState.key_generate=1;
					//printk("edge_status=%d key=%d\n",edge_status,KeyState.key);
				}
				break;
			case EDGE_TOP:
			case EDGE_BOTTOM:
				if(abs(KeyState.distance_x)>KEYMOVE_GAP)
				{
					if(KeyState.distance_x<0)				
						KeyState.key = KEY_LEFT;
					else
						KeyState.key = KEY_RIGHT;
					if((KeyState.keystatus==ARROW_RELEASE && KeyState.lastkeystatus==ARROW_MOVE) ||
						(KeyState.keystatus==ARROW_RELEASE && KeyState.lastkeystatus==ARROW_HOLD))
						KeyState.keystatus = ARROW_CLICK;
					KeyState.key_generate=1;
					//printk("edge_status=%d key=%d\n",edge_status,KeyState.key);
				}
				break;
			}
		}
		break;
	case EDGE_LEFT:
	case EDGE_RIGHT:
		if(abs(KeyState.distance_y)>KEYMOVE_GAP)
		{
			if(KeyState.distance_y<0)				
				KeyState.key = KEY_DOWN;
			else
				KeyState.key = KEY_UP;
			if((KeyState.keystatus==ARROW_RELEASE && KeyState.lastkeystatus==ARROW_MOVE) ||
				(KeyState.keystatus==ARROW_RELEASE && KeyState.lastkeystatus==ARROW_HOLD))
				KeyState.keystatus = ARROW_CLICK;
			KeyState.key_generate=1;
			//printk("edge_status=%d key=%d\n",edge_status,KeyState.key);
		}
		break;
	case EDGE_TOP:
	case EDGE_BOTTOM:
		if(abs(KeyState.distance_x)>KEYMOVE_GAP)
		{
			if(KeyState.distance_x<0)				
				KeyState.key = KEY_LEFT;
			else
				KeyState.key = KEY_RIGHT;
			if((KeyState.keystatus==ARROW_RELEASE && KeyState.lastkeystatus==ARROW_MOVE) ||
				(KeyState.keystatus==ARROW_RELEASE && KeyState.lastkeystatus==ARROW_HOLD))
				KeyState.keystatus = ARROW_CLICK;
			KeyState.key_generate=1;
			//printk("edge_status=%d key=%d\n",edge_status,KeyState.key);
		}
		break;
	}
	KeyState.lastkeystatus=KeyState.keystatus;
	

	if(KeyState.key_generate==1)
	{
		switch(KeyState.keystatus)
		{
		case ARROW_RELEASE:
			//printk("key release %d\n",KeyState.key);
			input_report_key(tpdata->input, KeyState.key, 0);
			input_sync(tpdata->input);
			elan_tp_key_initial();	
			break;	
		case ARROW_HOLD:
			if(KeyState.destroy_hold==0)
			{
				//printk("key hold %d\n",KeyState.key);
				if( KeyState.hold_count==KEYHOLD_CNT )
				{
					input_report_key(tpdata->input, KeyState.key, 1);
					input_sync(tpdata->input);
					KeyState.hold_count = KEYHOLD_CNT+1;
				}
				/* 20130409 EPSON for adjusting Key Scroll Speed*/
				if(KeyState.hold_count<KEYHOLD_CNT) {
					KeyState.hold_count++;
				} else {
					if ((KEYSCROLL_SPEED) > 0) {
						if (((KEYHOLD_CNT) - KEYSCROLL_SPEED) > 0) {
							input_report_key(tpdata->input, KeyState.key, 0);
							input_sync(tpdata->input);
							KeyState.hold_count = KEYHOLD_CNT - KEYSCROLL_SPEED;
						}
					}
				}
			}
			else
			{
				input_report_key(tpdata->input, KeyState.key, 0);
				input_sync(tpdata->input);
			}
			break;
		case ARROW_CLICK:
			//printk("key click %d\n",KeyState.key);
			input_report_key(tpdata->input, KeyState.key, 1);
			input_report_key(tpdata->input, KeyState.key, 0);
			input_sync(tpdata->input);
			elan_tp_key_initial();		
			break;
		}
	}
	return 0;
} 
