/*
 *-------------------------------------------------------------------------------
 *
 *  File Name   : elan_tp_timer.c
 *  Function    : provide timer function to prevent TP hang-up for BT Project.
 *
 *  History     : 2014.02.13    Created by SEIKO EPSON BT Project Team.
 *
 *-------------------------------------------------------------------------------
 *  Copyright(C) SEIKO EPSON CORPORATION 2014. All rights reserved.
 *-------------------------------------------------------------------------------
 */

#ifndef	_ELAN_TP_TIMER_H
#define	_ELAN_TP_TIMER_H

int init_elan_tp_timer(int gpio, int irq);
void remove_elan_tp_timer(void);
void elan_tp_irq_OK(void);

#endif
