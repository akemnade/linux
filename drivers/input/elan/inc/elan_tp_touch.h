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
#ifndef _ELAN_TP_TOUCH_H
#define _ELAN_TP_TOUCH_H

#include "elan_tp_define.h"

void elan_tp_do_touch(struct elan_tp_data* tpdata);
#ifdef ELAN_TP_MODE
void elan_tp_do_touch_HomeMode(struct elan_tp_data* tpdata);
#endif
void elan_tp_DoTwoFingerUp(struct elan_tp_data* tpdata,long lastfinger,long currentfinger);
void elan_tp_ReleaseAllTouchFinger(struct elan_tp_data* tpdata);
void elan_tp_ReleaseTouchFinger(struct elan_tp_data* tpdata);
void elan_tp_initial_twofinger_status(struct elan_tp_data* tpdata);
void elan_tp_translateRelativeMT(struct elan_tp_data* tpdata);
#endif
