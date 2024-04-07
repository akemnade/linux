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
#ifndef	_ELAN_TP_DOUBLE_TAP_H
#define	_ELAN_TP_DOUBLE_TAP_H
long elan_tp_detect_tap(long fingercnt,long x,long y,int edgestate);
void elan_tp_tap_init(void);
void elan_tp_enable_tap(bool bEnable);
#endif
