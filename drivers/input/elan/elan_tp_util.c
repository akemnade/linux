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

#include "inc/elan_tp_util.h"



void elan_tp_translateRawdata(long width,long height,int rotate,PMULTI_FINGERS current_package,PMULTI_FINGERS pInRawData)
{

	int temp;
	int count = current_package->fCount;
	int i;

	for(i=0;i<count;i++)
	{
		long x = (long)(pInRawData->fingers[i].iX/4);
		long y = (long)(pInRawData->fingers[i].iY/4);
		
		/*
		if (i==1) {
			if ((pInRawData->fingers[0].iX) > (pInRawData->fingers[1].iX)) {
				x += 30;
			} else if ((pInRawData->fingers[0].iX) < (pInRawData->fingers[1].iX)) {
				x -= 30;
			} else {
				;
			}
			
			if ((pInRawData->fingers[0].iY) > (pInRawData->fingers[1].iY)) {
				y += 50;
			} else if ((pInRawData->fingers[0].iY) < (pInRawData->fingers[1].iY)) {
				y -= 50;
			} else {
				;
			}
		}
		*/
#ifdef ELAN_TP_MODE
		current_package->fingers[i].RAWiX = x;
		current_package->fingers[i].RAWiY = y;
#endif

		if(x<0) x=0;
		if(y<0) y=0;
		if(x>width)  x=width;
		if(y>height) y=height;
	

		switch(rotate)
		{
		case ROTATE_0:
			break;
		case ROTATE_90:
			temp = x;
			x = height - y;
			y = temp;
			break;
		case ROTATE_180:
			x = width  - x;
			y = height - y;
			break;
		case ROTATE_270:
			temp = x;
			x = y;
			y = width - temp;
			break;
		}	
		current_package->fingers[i].iX = x;
		current_package->fingers[i].iY = y;
		//printk("finger %d x=%ld y=%ld\n\n",i,x,y);
	}

}

void elan_tp_filter(PMULTI_FINGERS LastProcessedPackages,PMULTI_FINGERS current_package)
{
	long lastfingercnt = LastProcessedPackages->fCount;
	long currentfingercnt = current_package->fCount;
	long x,y,i,j;

	//filter
	if(lastfingercnt==currentfingercnt)
	{
		if(currentfingercnt==1)
		{
			x = (long)((LastProcessedPackages->fingers[0].iX*3 + current_package->fingers[0].iX*1)/4);
			y = (long)((LastProcessedPackages->fingers[0].iY*3 + current_package->fingers[0].iY*1)/4);

			current_package->fingers[0].iX = x;
			current_package->fingers[0].iY = y;	
			
		}
		else			
		{
			for(i=0;i<currentfingercnt;i++)
			{
				for(j=0;j<currentfingercnt;j++)
				{
					if(LastProcessedPackages->fingers[i].number==current_package->fingers[j].number)
					{
						x = (long)((LastProcessedPackages->fingers[i].iX*2 + current_package->fingers[j].iX*2)/4);
						y = (long)((LastProcessedPackages->fingers[i].iY*3 + current_package->fingers[j].iY*1)/4);

						current_package->fingers[j].iX = x;
						current_package->fingers[j].iY = y;	
					}
				}
			}		
		}
	}
	

	if(lastfingercnt!=0 && currentfingercnt==0)
	{
		for(i=0;i<lastfingercnt;i++)
		{
			current_package->fingers[i].iX = LastProcessedPackages->fingers[i].iX;
			current_package->fingers[i].iY = LastProcessedPackages->fingers[i].iY;
		}		
	}
}

void elan_tp_ObjectTracking(PMULTI_FINGERS LastProcessedPackages,PMULTI_FINGERS mf)
{
	
	long number;
	long i;
	long oldx1, oldy1, oldx2, oldy2;
	long newx1, newy1, newx2, newy2;
	long d1, d2;
	long d11, d12, d21, d22;
	long lastfingercnt = LastProcessedPackages->fCount;
	long currentfingercnt = mf->fCount;
	

	if( lastfingercnt==0)
	{
		//No previous points to be referenced.
		//Re-number this package.
		number = 0;
		for( i=0; i< mf->fCount; i++)
		{
			mf->fingers[i].number = number;
			number++;
		}
	}
	else
	{	
		if( lastfingercnt == 0 || lastfingercnt == 3)
		{
			//Previous points is an all-finger-up package or 3-fingers package, 
			//Re-number current package.
			number = 0;
			for( i=0; i< mf->fCount; i++)
			{
				mf->fingers[i].number = number;
				number++;
			}
			
		}
		else if( lastfingercnt == 1)
		{
			if( currentfingercnt == 9)
			{
				//Current Package is a all-finger-up package,
				//it's not necessary to number it.

				//dont process finger up message here...
			}
			else if( currentfingercnt == 1)
			{
				//Last package and current package are all 1-finger package,
				//copy number from previous points
				mf->fingers[0].number = LastProcessedPackages->fingers[0].number;
			}
			else if( currentfingercnt == 2)
			{
				//Last package is 1-finger package, current package is 2-finger package.
				//New finger coming, try to find out how to number new finger...
				//do something here...
				oldx1 = LastProcessedPackages->fingers[0].iX;
				oldy1 = LastProcessedPackages->fingers[0].iY;
				newx1 = mf->fingers[0].iX;
				newy1 = mf->fingers[0].iY;
				newx2 = mf->fingers[1].iX;
				newy2 = mf->fingers[1].iY;
				d1 = (newx1-oldx1)*(newx1-oldx1);
				d1 = d1 + (newy1-oldy1)*(newy1-oldy1);
				d2 = (newx2-oldx1)*(newx2-oldx1);
				d2 = d2 + (newy2-oldy1)*(newy2-oldy1);
				/*
				if( d1 > d2)
				{
					//New finger of current package is the same with finger of last package.
					mf->fingers[1].number = LastProcessedPackages->fingers[0].number;
					if( LastProcessedPackages->fingers[0].number == 1)
						mf->fingers[0].number = 1;
					else
						mf->fingers[0].number = 0;
				}
				else if( d2 > d1)
				{
					//Old finger of current package is the same with finger of last package.
					mf->fingers[0].number = LastProcessedPackages->fingers[0].number;
					if( LastProcessedPackages->fingers[0].number == 1)
						mf->fingers[1].number = 1;
					else
						mf->fingers[1].number = 0;
				}
				else
				{
					//It's impossible to reach here,
					//But if it happens, re-number all finger of current package.
					mf->fingers[0].number = 0;
					mf->fingers[1].number = 1;
				}
				*/
				if( d1 > d2)
				{
					mf->fingers[0].number = 0;
					mf->fingers[1].number = 1;
				}
				else if( d2 > d1)
				{
					mf->fingers[0].number = 1;
					mf->fingers[1].number = 0;
				}
				else
				{
					//It's impossible to reach here,
					//But if it happens, re-number all finger of current package.
					mf->fingers[0].number = 0;
					mf->fingers[1].number = 1;
				}
			}
		}
		else if( lastfingercnt == 2)
		{
			if( currentfingercnt == 0)
			{
				//Current Package is a all-finger-up package,
				//it's not necessary to number it.

				//dont process finger up message here...
			}
			else if( currentfingercnt == 1)
			{
				//Last package is 2-finger package, current package is 1-finger package.
				//One finger up, trace which one is up..
				
				oldx1 = LastProcessedPackages->fingers[0].iX;
				oldy1 = LastProcessedPackages->fingers[0].iY;
				oldx2 = LastProcessedPackages->fingers[1].iX;
				oldy2 = LastProcessedPackages->fingers[1].iY;

				newx1 = mf->fingers[0].iX;
				newy1 = mf->fingers[0].iY;
				
				d1 = (newx1-oldx1)*(newx1-oldx1);
				d1 = d1 + (newy1-oldy1)*(newy1-oldy1);
				d2 = (newx1-oldx2)*(newx1-oldx2);
				d2 = d2 + (newy1-oldy2)*(newy1-oldy2);

				if( d1 > d2)
				{
					// 1st-finger of last package is up.
					// Assign number of 2nd-finger of last package to current package.
					mf->fingers[0].number = LastProcessedPackages->fingers[1].number;
				}
				else if( d2 > d1)
				{
					// 2nd-finger of last package is up.
					// Assign number of 1st-finger of last package to current package.
					mf->fingers[0].number = LastProcessedPackages->fingers[0].number;
				}
				else
				{
					//It's impossible to reach here.
					mf->fingers[0].number = LastProcessedPackages->fingers[0].number;
				}
			}
			else if( currentfingercnt == 2)
			{
				//All last package and current package are 2-fingers
				//Try to find out how to number the fingers.
				
				oldx1 = LastProcessedPackages->fingers[0].iX;
				oldy1 = LastProcessedPackages->fingers[0].iY;
				oldx2 = LastProcessedPackages->fingers[1].iX;
				oldy2 = LastProcessedPackages->fingers[1].iY;

				newx1 = mf->fingers[0].iX;
				newy1 = mf->fingers[0].iY;
				newx2 = mf->fingers[1].iX;
				newy2 = mf->fingers[1].iY;
					
				d11 = (newx1-oldx1)*(newx1-oldx1);
				d11 = d11+(newy1-oldy1)*(newy1-oldy1);
				d12 = (newx1-oldx2)*(newx1-oldx2);
				d12 = d12+(newy1-oldy2)*(newy1-oldy2);
				
				d21 = (newx2-oldx1)*(newx2-oldx1);
				d21 = d21+(newy2-oldy1)*(newy2-oldy1);
				d22 = (newx2-oldx2)*(newx2-oldx2);
				d22 = d22+(newy2-oldy2)*(newy2-oldy2);
				
				//Do lazily judge
				//Short distance have great connection.
				
				if( d11 <= d21 && d12 >= d22)
				{
					//new p1 is near old p1
					//new p2 is near old p2
					mf->fingers[0].number = LastProcessedPackages->fingers[0].number;
					mf->fingers[1].number = LastProcessedPackages->fingers[1].number;
				}
				else if( d11 >= d21 && d12 <= d22)
				{
					//new p1 is near old p2
					//new p2 is near old p1
					mf->fingers[0].number = LastProcessedPackages->fingers[1].number;
					mf->fingers[1].number = LastProcessedPackages->fingers[0].number;					
				}
				else if( d11<=d21 && d12<=d22)
				{
					//new p1 is near old p1
					//new p1 is new old p2
					
					mf->fingers[0].number = LastProcessedPackages->fingers[0].number;
					mf->fingers[1].number = LastProcessedPackages->fingers[1].number;
				}
				else if( d11>=d21 && d12>=d22)
				{
					//new p2 is near old p1
					//new p2 is new old p2
					
					mf->fingers[0].number = LastProcessedPackages->fingers[0].number;
					mf->fingers[1].number = LastProcessedPackages->fingers[1].number;
				}
				else
				{
					//It's impossible to reach here.
					mf->fingers[0].number = LastProcessedPackages->fingers[0].number;
					mf->fingers[1].number = LastProcessedPackages->fingers[1].number;				
				}
			}
		}

		//filter
		elan_tp_filter(LastProcessedPackages,mf);
	}
}



bool elan_tp_isoverboundary(long width,long height,PMULTI_FINGERS package)
{
	long left = BOUNDARY_WIDTH;
	long right = width - BOUNDARY_WIDTH;
	long top = BOUNDARY_HEIGHT;
	long bottom = height - BOUNDARY_HEIGHT;	
	long fingercount = package->fCount;
	int i=0;

	//if finger up , don't check boundary
	if(fingercount==0)
		return false;

	for(i=0;i<fingercount;i++)
	{
		long x = package->fingers[i].iX;
		long y = package->fingers[i].iY;


		if(x<left || x>right)
			return true;
		if(y<top || y>bottom)
			return true;
	}
	return false;
}

int elan_tp_detect_corner(int rotate, int edge_percent,int tp_width,int tp_height,int x,int y)
{
	int nRet = CORNER_NONE;
	int edge_width = edge_percent;
	int edge_height = edge_percent;
	int temp;
	if(rotate==ROTATE_90 || rotate==ROTATE_270)
	{
		temp = tp_width;
		tp_width = tp_height;
		tp_height = temp;	
	}


	if(x<=edge_width && y<=edge_height)
		nRet = CORNER_LB;
	if(x<=edge_width && y>=(tp_height-edge_height))
		nRet = CORNER_LT;
	if(x>=(tp_width-edge_width) && y<=edge_height)
		nRet = CORNER_RB;
	if(x>=(tp_width-edge_width) && y>=(tp_height-edge_height) )
		nRet = CORNER_RT;
	return nRet;
}

int elan_tp_detect_edge(int rotate,int edge_percent,int tp_width,int tp_height,int x,int y)
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

	//detect corner
	if(nRet!=EDGE_NONE)
	{
		if(x<=edge_width && y<=edge_height)
			nRet = CORNER_LB;
		if(x<=edge_width && y>=(tp_height-edge_height))
			nRet = CORNER_LT;
		if(x>=(tp_width-edge_width) && y<=edge_height)
			nRet = CORNER_RB;
		if(x>=(tp_width-edge_width) && y>=(tp_height-edge_height) )
			nRet = CORNER_RT;
	}
	return nRet;
}

void elan_tp_get_midposition(PMULTI_FINGERS package)
{
	int mid_x = (package->fingers[0].iX + package->fingers[1].iX)/2;
	int mid_y = (package->fingers[0].iY + package->fingers[1].iY)/2;
	package->fingers[0].iX = mid_x;
	package->fingers[0].iY = mid_y;
}









