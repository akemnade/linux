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
#include "inc/elan_tp_mouse.h"

void elan_tp_do_mouse(struct elan_tp_data* tpdata,MULTI_FINGERS lastPackage,MULTI_FINGERS currentPackage,long nStatus)
{
	long dx=0,dy=0;

	
	//first point must set as last point	
	if(currentPackage.isfirstpackage==false)
	{
		long oldx,oldy,x,y;
		oldx = lastPackage.fingers[0].iX;
		oldy = lastPackage.fingers[0].iY;
		x = currentPackage.fingers[0].iX;
		y = currentPackage.fingers[0].iY;
		
		dx = (x - oldx);
		dy = -(y - oldy);
	}

	switch(nStatus)
	{
	case ONEFINGER_TAP_ON:
		input_report_key(tpdata->input, BTN_LEFT,   1);
		input_report_key(tpdata->input, BTN_RIGHT,  0);
		input_report_key(tpdata->input, BTN_MIDDLE, 0);
		input_report_rel(tpdata->input, REL_WHEEL, 0);
		//input_report_rel(tpdata->input, REL_X, 1);
		//input_report_rel(tpdata->input, REL_Y, 1);	
		input_sync(tpdata->input);	

		input_report_key(tpdata->input, BTN_LEFT,   0);
		input_report_key(tpdata->input, BTN_RIGHT,  0);
		input_report_key(tpdata->input, BTN_MIDDLE, 0);	
		input_report_rel(tpdata->input, REL_WHEEL, 0);
		//input_report_rel(tpdata->input, REL_X, -1);
		//input_report_rel(tpdata->input, REL_Y, -1);
		input_sync(tpdata->input);
		//printk("mouse tapon\n");
		break;

	case ONEFINGER_HOLD_ON:
		input_report_key(tpdata->input, BTN_LEFT,   1);
		input_report_key(tpdata->input, BTN_RIGHT,  0);
		input_report_key(tpdata->input, BTN_MIDDLE, 0);
		input_report_rel(tpdata->input, REL_WHEEL, 0);
		input_report_rel(tpdata->input, REL_X, dx);
		input_report_rel(tpdata->input, REL_Y, dy);	
		input_sync(tpdata->input);
		//printk("mouse hold on\n");
		break;
	default:
		input_report_key(tpdata->input, BTN_LEFT,   0);
		input_report_key(tpdata->input, BTN_RIGHT,  0);
		input_report_key(tpdata->input, BTN_MIDDLE, 0);	
		input_report_rel(tpdata->input, REL_WHEEL,  0);
		if(currentPackage.fCount==1)
		{
			if(tpdata->mousespeed==CURSOR_FAST)
			{
				dx *= 2;
				dy *= 2;
			}
			else if(tpdata->mousespeed==CURSOR_SLOW)
			{
				dx /= 2;
				dy /= 2;
			}
			//printk("----mouse x=%d y=%d\n",dx,dy);				
			input_report_rel(tpdata->input, REL_X, dx);
			input_report_rel(tpdata->input, REL_Y, dy);
		}
		else
		{
			input_report_rel(tpdata->input, REL_X, 0);
			input_report_rel(tpdata->input, REL_Y, 0);
		}
		input_sync(tpdata->input);	
		//printk("mouse others\n");
	}
}
