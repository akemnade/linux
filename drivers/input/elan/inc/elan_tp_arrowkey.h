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
#ifndef	ELAN_TP_ARROWKEY_H
#define	ELAN_TP_ARROWKEY_H
int elan_tp_do_arrowkey(struct elan_tp_data* tpdata,int tap_status);
void elan_tp_release_arrowkey(struct elan_tp_data* tpdata);
#endif
