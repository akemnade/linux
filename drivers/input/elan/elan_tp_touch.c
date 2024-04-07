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
#include <linux/input/mt.h>

#include "inc/elan_tp_define.h"
#include "inc/elan_tp_util.h"
#include "inc/elan_tp_mouse.h"


#define NONMOVE_TH		7	
#define FLICK_TH			10
#define ZOOM_TH			15000

#define SHIFT_DIV_RATE	1


void elan_tp_translateRelativeMT(struct elan_tp_data* tpdata)
{
	long x0,x1,y0,y1;
#define MOVE_CENTER
#ifdef MOVE_CENTER
	long mx,my;
#endif
	
	if (tpdata->currentPackage.fCount == 2) {
		x0 = tpdata->currentPackage.fingers[0].iX;
		x1 = tpdata->currentPackage.fingers[1].iX;
		y0 = tpdata->currentPackage.fingers[0].iY;
		y1 = tpdata->currentPackage.fingers[1].iY;
		
		if (tpdata->lastPackage.fCount != 2) {
			tpdata->shiftX = tpdata->cursorX - x0;
			tpdata->shiftY = (ABS_AXIS_Y_MAX - tpdata->cursorY) - y0;
#ifdef MOVE_CENTER
			x0 += tpdata->shiftX;
			x1 += tpdata->shiftX;
			y0 += tpdata->shiftY;
			y1 += tpdata->shiftY;
			mx = (x0 + x1)>>1;
			my = (y0 + y1)>>1;
			tpdata->shiftX += tpdata->cursorX - mx;
			tpdata->shiftY += (ABS_AXIS_Y_MAX - tpdata->cursorY) - my;
#endif
		}
		
		tpdata->currentPackage.fingers[0].MTiX = tpdata->currentPackage.fingers[0].iX + tpdata->shiftX;
		tpdata->currentPackage.fingers[1].MTiX = tpdata->currentPackage.fingers[1].iX + tpdata->shiftX;
		tpdata->currentPackage.fingers[0].MTiY = tpdata->currentPackage.fingers[0].iY + tpdata->shiftY;
		tpdata->currentPackage.fingers[1].MTiY = tpdata->currentPackage.fingers[1].iY + tpdata->shiftY;
	}
}

long elan_tp_GestureFingerCount(long nGesture)
{
	if(nGesture==GEST_ZOOM)
		return 2;
	return 1;
}

long elan_tp_RealDoWhichGesture(struct elan_tp_data* tpdata)
{
	long nScore = 0;
	long nGesture = GEST_UNMOVE;
	
	if ((tpdata->gesture_type == GEST_FLICK_X)||(tpdata->gesture_type == GEST_FLICK_Y)) {
		return tpdata->gesture_type;
	}

	if(tpdata->onemove_score>nScore)
	{
		nScore = tpdata->onemove_score;
		nGesture = GEST_ONEMOVE;
	}

	if(tpdata->zoom_score>nScore)
	{
		nScore = tpdata->zoom_score;
		nGesture = GEST_ZOOM;
	}

	if(tpdata->flick_score_x>nScore)
	{
		nScore = tpdata->flick_score_x;
		nGesture = GEST_FLICK_X;
		tpdata->gesture_type = GEST_FLICK_X;
	}
	if(tpdata->flick_score_y>nScore)
	{
		nScore = tpdata->flick_score_y;
		nGesture = GEST_FLICK_Y;
		tpdata->gesture_type = GEST_FLICK_Y;
	}
	return nGesture;
}

void elan_tp_initial_twofinger_status(struct elan_tp_data* tpdata)
{
	//printk("<<<<<<First Two Finger Down>>>>>>\n");
	tpdata->zoom_score=0;
	tpdata->flick_score_x=0;
	tpdata->flick_score_y=0;
	tpdata->onemove_score=0;
	tpdata->TwoFingerQueueCount = 0;
	tpdata->currentPackage.Gesture = GEST_UNMOVE;
	tpdata->refPackage = tpdata->currentPackage;
	tpdata->TwoFigGestStatus = GEST_UNMOVE;
	tpdata->LastTwoFigGestStatus = GEST_UNMOVE;
}

long elan_tp_GetTwoFingerGesture(struct elan_tp_data* tpdata)
{
	long	dX,dY ,current_dist,last_dist,delta_dist,dX0,dY0,dX1,dY1; 
	bool ismove = false;
	bool isXdirectSame = false;
	bool isYdirectSame = false;

  	//if first two finger down, than copy two finger data to reference package 
 	if(tpdata->lastPackage.fCount!=2 && tpdata->currentPackage.fCount==2)
		elan_tp_initial_twofinger_status(tpdata);
    
	//get dx0,dy0,dx1,dy1 with last two finger position
	dX0 = (tpdata->currentPackage.fingers[0].iX - tpdata->refPackage.fingers[0].iX) ; 
	dY0 = (tpdata->currentPackage.fingers[0].iY - tpdata->refPackage.fingers[0].iY) ; 
	dX1 = (tpdata->currentPackage.fingers[1].iX - tpdata->refPackage.fingers[1].iX) ; 
	dY1 = (tpdata->currentPackage.fingers[1].iY - tpdata->refPackage.fingers[1].iY) ;
		
	//check two finger move or not
	if(abs(dX0)>=NONMOVE_TH && abs(dX1)>=NONMOVE_TH)
		ismove = true;
	if(abs(dY0)>=NONMOVE_TH && abs(dY1)>=NONMOVE_TH)
		ismove = true;

	if(ismove)
	{
	   	//get current two finger distance
	   	dX = (tpdata->currentPackage.fingers[1].iX - tpdata->currentPackage.fingers[0].iX) ; 
		dY = (tpdata->currentPackage.fingers[1].iY - tpdata->currentPackage.fingers[0].iY) ;
		current_dist = dX*dX + dY*dY;
		//printk("current dx=%ld dy=%ld dist=%ld\n",dX,dY,current_dist);
		 		

		//get reference two finger distance
	   	dX = (tpdata->refPackage.fingers[1].iX - tpdata->refPackage.fingers[0].iX) ; 
		dY = (tpdata->refPackage.fingers[1].iY - tpdata->refPackage.fingers[0].iY) ;
		last_dist = dX*dX + dY*dY; 
		//printk("last dx=%ld dy=%ld dist=%ld\n",dX,dY,last_dist);
	
		delta_dist = abs(current_dist - last_dist);
		//printk("=====delta dist=%ld\n",delta_dist);
	


		//printk("dx0=%ld dy0=%ld dx1=%ld dy1=%ld current_dist=%ld last_dist=%ld delta_dist=%ld\n",dX0,dY0,dX1,dY1,current_dist,last_dist,delta_dist);
		if(delta_dist>=ZOOM_TH && ( (dX0*dX1<0) || (dY0*dY1<0) ) )
		{
			tpdata->zoom_score++; 
			tpdata->onemove_score = 0;
			tpdata->currentPackage.Gesture = GEST_ZOOM;
			//printk("zoom in/out\n ") ;
		}
		else
		{
			//get dx0,dy0,dx1,dy1 with current and  first finger position
			dX0 = (tpdata->currentPackage.fingers[0].iX - tpdata->firstPackage.fingers[0].iX) ; 
			dY0 = (tpdata->currentPackage.fingers[0].iY - tpdata->firstPackage.fingers[0].iY) ; 
			dX1 = (tpdata->currentPackage.fingers[1].iX - tpdata->firstPackage.fingers[1].iX) ; 
			dY1 = (tpdata->currentPackage.fingers[1].iY - tpdata->firstPackage.fingers[1].iY) ;

			if(dX0*dX1 < 0)
				isXdirectSame = false;
			else 
				isXdirectSame = true;

			if(dY0*dY1 < 0)
				isYdirectSame = false;
			else
				isYdirectSame = true;

			//finger 1 & 2 are not the same direction, we call zoom
			if(isYdirectSame==false && isXdirectSame==false)
			{
				tpdata->zoom_score++; 
				tpdata->onemove_score = 0;
				tpdata->currentPackage.Gesture = GEST_ZOOM;
			}
			else if(isXdirectSame==true && (abs(dX0)>abs(dY0)) )
			{
				//over flick threshold, we define this is flick gesture
				if(abs(dX0)>=FLICK_TH || abs(dX1)>=FLICK_TH)
				{
					//printk("FLICK X ") ;
					tpdata->flick_score_x+=2;	
					tpdata->onemove_score = 0;
					tpdata->currentPackage.Gesture = GEST_FLICK_X ;
				}
				//otherwise, we define this is move gesture
				else
				{
					//printk("One Move  ") ;
					tpdata->onemove_score++; 
					tpdata->currentPackage.Gesture = GEST_ONEMOVE;
				}			
			}
			else if(isYdirectSame==true && (abs(dY0)>abs(dX0)) )
			{
				//over flick threshold, we define this is flick gesture
				if(abs(dY0)>=FLICK_TH || abs(dY1)>=FLICK_TH)
				{
					//printk("FLICK Y ") ;
					tpdata->flick_score_y+=2;	
					tpdata->onemove_score = 0;
					tpdata->currentPackage.Gesture = GEST_FLICK_Y ;
				}
				//otherwise, we define this is move gesture
				else
				{
					//printk("One Move  ") ;
					tpdata->onemove_score++; 
					tpdata->currentPackage.Gesture = GEST_ONEMOVE;
				}
			}
		}
		tpdata->refPackage = tpdata->currentPackage;
	}
	else
	{
		//printk("UnMove  ") ;
		tpdata->currentPackage.Gesture = GEST_UNMOVE;	
	}
	return elan_tp_RealDoWhichGesture(tpdata);
}

void elan_tp_ReleaseTouchFinger(struct elan_tp_data* tpdata)
{
	int i,j;
	int aIndex[20];
	int nCount=0;
	int bFind = 0;

	if(tpdata->lastPackage.fCount<=tpdata->currentPackage.fCount)
		return;


	for(i=0;i<tpdata->lastPackage.fCount;i++)
	{
		bFind = 0;
		//printk("lastpackage [%d] number=%ld\n",i,tpdata->lastPackage.fingers[i].number);
		for(j=0;j<tpdata->currentPackage.fCount;j++)
		{
			//printk("currPackage [%d] number=%ld\n",j,tpdata->currentPackage.fingers[j].number);
			if(tpdata->lastPackage.fingers[i].number==tpdata->currentPackage.fingers[j].number)
			{
				bFind = 1;
				break;
			}			
		}
		if(bFind == 0)
		{	
			//printk("add to remove ID = %ld\n",tpdata->lastPackage.fingers[i].number);	
			aIndex[nCount] = tpdata->lastPackage.fingers[i].number;
			nCount++;
		}
	}

	if(nCount>0)
	{
		for(i=0;i<nCount;i++)
		{	
			input_mt_slot(tpdata->input_touch, aIndex[i]);
			input_mt_report_slot_state(tpdata->input_touch, MT_TOOL_FINGER, 0);
		}
		input_sync(tpdata->input_touch);	
	}	
}




void elan_tp_ReleaseAllTouchFinger(struct elan_tp_data* tpdata)
{
	int i=0;
	tpdata->TwoFingerQueueCount = 0;	

	if(tpdata->lastPackage.fCount>0)
	{
		input_report_key(tpdata->input_touch, BTN_TOUCH, 0);
		for(i=0;i<tpdata->lastPackage.fCount;i++)
		{	
			input_mt_slot(tpdata->input_touch, tpdata->lastPackage.fingers[i].number);
			input_mt_report_slot_state(tpdata->input_touch, MT_TOOL_FINGER, 0);
		}
		input_sync(tpdata->input_touch);
	}
	
}

void elan_tp_xy_rotate(int width,int height,int rotate,PMULTI_FINGERS package)
{
	int i=0;
	int count = package->fCount;

	switch(rotate)
	{
	case ROTATE_0:
	case ROTATE_180:
		for(i=0;i<count;i++)
			package->fingers[i].iY = height - package->fingers[i].iY;
		break;
	case ROTATE_90:
	case ROTATE_270:
		for(i=0;i<count;i++)
		{
			package->fingers[i].iY = width - package->fingers[i].iY;
			package->fingers[i].iX = (package->fingers[i].iX*width)/height;
			package->fingers[i].iY = (package->fingers[i].iY*height)/width;
		}
		break;
	}
	

}

void elan_tp_touch_abs_report(struct elan_tp_data* tpdata,MULTI_FINGERS lastpackage,MULTI_FINGERS currentpackage)
{
	int Count = currentpackage.fCount;
	int i;
	
	if (Count == 1) {
		if (currentpackage.fingers[0].number != 0) {
			input_report_key(tpdata->input_touch, BTN_TOUCH, 1);
			input_mt_slot(tpdata->input_touch, currentpackage.fingers[1].number);
			input_mt_report_slot_state(tpdata->input_touch, MT_TOOL_FINGER, 1);
			input_report_abs(tpdata->input_touch, ABS_MT_TRACKING_ID, currentpackage.fingers[1].number);
			input_report_abs(tpdata->input_touch, ABS_MT_TOUCH_MAJOR, TOUCH_AREA);
			input_report_abs(tpdata->input_touch, ABS_MT_TOUCH_MINOR, TOUCH_AREA);
			input_report_abs(tpdata->input_touch, ABS_MT_WIDTH_MAJOR, TOUCH_AREA);
			input_report_abs(tpdata->input_touch, ABS_MT_POSITION_X, currentpackage.fingers[1].MTiX);
			input_report_abs(tpdata->input_touch, ABS_MT_POSITION_Y, ABS_AXIS_Y_MAX - currentpackage.fingers[1].MTiY);
		} else {
			input_report_key(tpdata->input_touch, BTN_TOUCH, 1);
			input_mt_slot(tpdata->input_touch, currentpackage.fingers[0].number);
			input_mt_report_slot_state(tpdata->input_touch, MT_TOOL_FINGER, 1);
			input_report_abs(tpdata->input_touch, ABS_MT_TRACKING_ID, currentpackage.fingers[0].number);
			input_report_abs(tpdata->input_touch, ABS_MT_TOUCH_MAJOR, TOUCH_AREA);
			input_report_abs(tpdata->input_touch, ABS_MT_TOUCH_MINOR, TOUCH_AREA);
			input_report_abs(tpdata->input_touch, ABS_MT_WIDTH_MAJOR, TOUCH_AREA);
			input_report_abs(tpdata->input_touch, ABS_MT_POSITION_X, currentpackage.fingers[0].MTiX);
			input_report_abs(tpdata->input_touch, ABS_MT_POSITION_Y, ABS_AXIS_Y_MAX - currentpackage.fingers[0].MTiY);
		}
	} else {
		//elan_tp_xy_rotate(tpdata->width,tpdata->height,tpdata->rotate,&currentpackage);
		for(i=0;i<Count;i++)
		{
			input_report_key(tpdata->input_touch, BTN_TOUCH, 1);
			input_mt_slot(tpdata->input_touch, currentpackage.fingers[i].number);
			input_mt_report_slot_state(tpdata->input_touch, MT_TOOL_FINGER, 1);
			input_report_abs(tpdata->input_touch, ABS_MT_TRACKING_ID, currentpackage.fingers[i].number);
			input_report_abs(tpdata->input_touch, ABS_MT_TOUCH_MAJOR, TOUCH_AREA);
			input_report_abs(tpdata->input_touch, ABS_MT_TOUCH_MINOR, TOUCH_AREA);
			input_report_abs(tpdata->input_touch, ABS_MT_WIDTH_MAJOR, TOUCH_AREA);
			input_report_abs(tpdata->input_touch, ABS_MT_POSITION_X, currentpackage.fingers[i].MTiX);
			input_report_abs(tpdata->input_touch, ABS_MT_POSITION_Y, ABS_AXIS_Y_MAX - currentpackage.fingers[i].MTiY);
		}
	}
	if(Count>0)
		input_sync(tpdata->input_touch);
	/*if(tpdata->TwoFigGestStatus==GEST_ZOOM )
	{
		int Count = currentpackage.fCount;
		int i;
		//printk("gesture=%ld , abs report\n",tpdata->TwoFigGestStatus);
		elan_tp_xy_rotate(tpdata->width,tpdata->height,tpdata->rotate,&currentpackage);	
		for(i=0;i<Count;i++)
		{
			input_report_key(tpdata->input_touch, BTN_TOUCH, 1);
			input_mt_slot(tpdata->input_touch, currentpackage.fingers[i].number);
			input_mt_report_slot_state(tpdata->input_touch, MT_TOOL_FINGER, 1);
			input_report_abs(tpdata->input_touch, ABS_MT_TRACKING_ID, currentpackage.fingers[i].number);
			input_report_abs(tpdata->input_touch, ABS_MT_TOUCH_MAJOR, TOUCH_AREA);
			input_report_abs(tpdata->input_touch, ABS_MT_TOUCH_MINOR, TOUCH_AREA);
			input_report_abs(tpdata->input_touch, ABS_MT_WIDTH_MAJOR, TOUCH_AREA);
			input_report_abs(tpdata->input_touch, ABS_MT_POSITION_X, currentpackage.fingers[i].iX);
			input_report_abs(tpdata->input_touch, ABS_MT_POSITION_Y, currentpackage.fingers[i].iY);
		}
		if(Count>0)
			input_sync(tpdata->input_touch);
	}
	else
	{
		//printk("gesture=%ld , rel report\n",tpdata->TwoFigGestStatus);
		elan_tp_get_midposition(&currentpackage);
		elan_tp_get_midposition(&lastpackage);
		currentpackage.fCount = 1;
		lastpackage.fCount = 1;
		if(tpdata->LastTwoFigGestStatus==GEST_ZOOM )
			elan_tp_ReleaseAllTouchFinger(tpdata);
		elan_tp_do_mouse(tpdata,lastpackage,currentpackage,TAP_OFF);
	}*/
}

void elan_tp_do_touch(struct elan_tp_data* tpdata)
{
	MULTI_FINGERS currentPackage,lastPackage;
	long i;
	long nCurrentFingerCount = tpdata->currentPackage.fCount;
	currentPackage = tpdata->currentPackage;
	lastPackage = tpdata->lastPackage;
		
	if(nCurrentFingerCount==2)
	{
		tpdata->TwoFigGestStatus = elan_tp_GetTwoFingerGesture(tpdata);
		nCurrentFingerCount = elan_tp_GestureFingerCount(tpdata->TwoFigGestStatus);

		if(tpdata->TwoFingerQueueCount<TWOFINGERQUEUEMAXCNT)
		{
			tpdata->TowFingerQueue[tpdata->TwoFingerQueueCount] = tpdata->currentPackage;
			tpdata->TwoFingerQueueCount++;
			return;
		}
		else if(tpdata->TwoFingerQueueCount==TWOFINGERQUEUEMAXCNT)
		{			
			if(tpdata->TwoFigGestStatus!=GEST_UNMOVE)
			{			
				for(i=0;i<TWOFINGERQUEUEMAXCNT;i++)
				{					
					currentPackage = tpdata->TowFingerQueue[i];
					currentPackage.fCount = nCurrentFingerCount;

					if(i==0)
						lastPackage = currentPackage;
					else
						lastPackage = tpdata->TowFingerQueue[i-1];

					elan_tp_touch_abs_report(tpdata,lastPackage,currentPackage);
				}
			}
		}
		tpdata->TwoFingerQueueCount++;
		currentPackage = tpdata->currentPackage;
		currentPackage.fCount = nCurrentFingerCount;
	}

	elan_tp_touch_abs_report(tpdata,lastPackage,currentPackage);
	tpdata->LastTwoFigGestStatus = tpdata->TwoFigGestStatus;  
}

#ifdef ELAN_TP_MODE
void elan_tp_do_touch_HomeMode(struct elan_tp_data* tpdata)
{
	input_report_key(tpdata->input_touch, BTN_TOUCH, 1);
	input_mt_slot(tpdata->input_touch, tpdata->currentPackage.fingers[0].number);
	input_mt_report_slot_state(tpdata->input_touch, MT_TOOL_FINGER, 1);
	input_report_abs(tpdata->input_touch, ABS_MT_TRACKING_ID, tpdata->currentPackage.fingers[0].number);
	input_report_abs(tpdata->input_touch, ABS_MT_TOUCH_MAJOR, TOUCH_AREA);
	input_report_abs(tpdata->input_touch, ABS_MT_TOUCH_MINOR, TOUCH_AREA);
	input_report_abs(tpdata->input_touch, ABS_MT_WIDTH_MAJOR, TOUCH_AREA);
	switch(tpdata->rotate) {
	case ROTATE_0:
		input_report_abs(tpdata->input_touch, ABS_MT_POSITION_X, tpdata->currentPackage.fingers[0].RAWiX);
		input_report_abs(tpdata->input_touch, ABS_MT_POSITION_Y, ABS_AXIS_Y_MAX - tpdata->currentPackage.fingers[0].RAWiY);
		break;
	case ROTATE_180:
		input_report_abs(tpdata->input_touch, ABS_MT_POSITION_X, ABS_AXIS_X_MAX - tpdata->currentPackage.fingers[0].RAWiX);
		input_report_abs(tpdata->input_touch, ABS_MT_POSITION_Y, tpdata->currentPackage.fingers[0].RAWiY);
		break;
	case ROTATE_90:
		input_report_abs(tpdata->input_touch, ABS_MT_POSITION_X, ABS_AXIS_X_MAX - (tpdata->currentPackage.fingers[0].RAWiY*ABS_AXIS_X_MAX/ABS_AXIS_Y_MAX));
		input_report_abs(tpdata->input_touch, ABS_MT_POSITION_Y, ABS_AXIS_Y_MAX - (tpdata->currentPackage.fingers[0].RAWiX*ABS_AXIS_Y_MAX/ABS_AXIS_X_MAX));
		break;
	case ROTATE_270:
		input_report_abs(tpdata->input_touch, ABS_MT_POSITION_X, tpdata->currentPackage.fingers[0].RAWiY*ABS_AXIS_X_MAX/ABS_AXIS_Y_MAX);
		input_report_abs(tpdata->input_touch, ABS_MT_POSITION_Y, tpdata->currentPackage.fingers[0].RAWiX*ABS_AXIS_Y_MAX/ABS_AXIS_X_MAX);
		break;
	default:
		break;
	}
	input_sync(tpdata->input_touch);
}
#endif
