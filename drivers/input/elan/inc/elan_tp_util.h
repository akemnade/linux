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
#ifndef _ELAN_TP_UTIL_H
#define _ELAN_TP_UTIL_H

#include "elan_tp_define.h"

bool elan_tp_isoverboundary(long width,long height,PMULTI_FINGERS package);
void elan_tp_ObjectTracking(PMULTI_FINGERS LastProcessedPackages,PMULTI_FINGERS mf);
void elan_tp_translateRawdata(long width,long height,int rotate,PMULTI_FINGERS current_package,PMULTI_FINGERS pInRawpackage);
int elan_tp_detect_corner(int rotate,int edge_percent,int tp_width,int tp_height,int x,int y);
int elan_tp_detect_edge(int rotate,int edge_percent,int tp_width,int tp_height,int x,int y);
void elan_tp_get_midposition(PMULTI_FINGERS package);
#endif
