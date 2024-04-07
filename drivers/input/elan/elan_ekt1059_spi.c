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

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------
 * Version 
 * 1.00.01 : SPI Read test
 * 1.00.02 : support one finger (relative mode) 
 * 1.00.03 : add two finger (abs mode) and one finger (abs mode)
 * 1.00.04 : update elan_tp_alg.c file
 * 1.00.05 : add rotation, mouse speed, edge scrolling functions 
 * 1.00.06 : Modify edge scroll gesture 
 * 1.00.07 : (1) optimize edge scroll gesture (when finger leaf edge scroll area)
   	       (2) modify function elan_tp_mouse() and elan_tp_touch_abs_report() prototype
		 (3) Add function elan_tp_get_midposition() and elan_tp_initial_twofinger_status()
 * 1.00.08 : (1) fix bug "select area" can't work. 
   	       (2) modify detect signal tap function (elan_tp_tap.c)
		 (3) change detect edge function before detect signal tap (elan_tp_alg.c)
		 (4) Add function elan_tp_enable_tap() and elan_tp_CheckSingleHoldTimeOverflow() in elan_tp_tap.c
 * 1.00.09 : (1) fix bug "When finger put on touchpad and rotate occured, cursor will skipping".
			=> We add function elan_tp_skip_packages() in elan_tp_alg.c to record how many packages must be rejected when rotate is changed.
  	       (2) fix bug "when touchpad rotate to ROTATE_90 or ROTATE_270, right top and bottom "select area" and left edge scrll can't work"
			=> Modify function elan_tp_detect_edge() in elan_tp_util.c. We disable calling function elan_tp_detect_corner(), but copy some 
			   source codes about elan_tp_detect_corner() to elan_tp_detect_edge().
		 (3) fix bug "when edge scrll continues input occured and then move finger out of edge scroll area, the arrow key function can't stop."
			=> Modify function elan_tp_do_arrowkey() in elan_tp_arrowkey.c. When "ARROW_HOLD" event occured, we just remove source code 
			   "KeyState.hold_count=0" and only report arrow key down once to Android. 
 *------------------------------------------------------------------------------------------------------------------------------------------------------------
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/spi/spi.h>
#include <linux/input.h>
#include <linux/input/mt.h>
//#include <mach/gpio.h>
#include <linux/kthread.h>
#include <linux/io.h>
#include "inc/elan_tp_define.h"
#include "inc/elan_tp_timer.h"	//EPSON 20140213 for prevent TP hang-up

#define HELLO_PACKET		0x55
#define TOUCHPAD_WIDTH		1024	
#define TOUCHPAD_HEIGHT		640
#define VERSION 			1.00.08

#define ENABLE_DELAY_START

struct elan_tp_spi 
{
	struct work_struct work;
#if defined(ENABLE_DELAY_START)
	struct delayed_work init_work;
	struct workqueue_struct *init_workq;
#endif
	struct workqueue_struct *workq;
	struct input_dev *input_touch;
	struct input_dev *input_mouse;
	struct spi_device * spi;
	struct spi_transfer *t;

	int irq;
	int gpio;
	bool binitialize;
};

/* 20130122 EPSON for Power-OFF Mode */
extern int power_off_mode;
static int data_reset = 0;
static struct mutex buf_lock;

#ifdef ELAN_LIVE_WALL
/* for livewallpaper */
static int tmp_X = -1;
static int tmp_Y = -1;
#endif

#ifdef ELAN_TP_MODE
/* EPSON 2013.09 */
static ssize_t elan_tp_HomeMode_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int val = elan_tp_getHomeMode_enable();
	printk(KERN_INFO "elan_tp_HomeMode_enable_show=%d\n", val);
	sprintf(buf, "%d\n", val);
	return 4;
}

static ssize_t elan_tp_HomeMode_enable_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int val;
	val = *buf;
#if 0
	if (val == '0') {
		val = 0;
	} else {
		val = 1;
	}
#endif
	elan_tp_setHomeMode_enable(val);
	printk(KERN_INFO "elan_tp_HomeMode_enable_store=%d\n", val);
	return 4;
}
#endif

#ifdef ELAN_LIVE_WALL
/* for livewallpaper */
static ssize_t elan_tp_get_coord_x_show(struct device *dev, struct device_attribute *attr, char *buf)
{
  buf[0] = tmp_X&0xFF;
  buf[1] = (tmp_X>>8)&0xFF;
  buf[2] = (tmp_X>>16)&0xFF;
  buf[3] = (tmp_X>>24)&0xFF;
  return 4; // return value '4' following other sysfs methods.
}
static ssize_t elan_tp_get_coord_y_show(struct device *dev, struct device_attribute *attr, char *buf)
{
  buf[0] = tmp_Y&0xFF;
  buf[1] = (tmp_Y>>8)&0xFF;
  buf[2] = (tmp_Y>>16)&0xFF;
  buf[3] = (tmp_Y>>24)&0xFF;
  return 4; // return value '4' following other sysfs methods.
}
#endif

/* EPSON 2013.05 */
static ssize_t elan_tp_rotate_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int val = elan_tp_getrotate_enable();
	printk(KERN_INFO "elan_tp_rotate_enable_show=%d\n", val);
	return 4;
}

static ssize_t elan_tp_rotate_enable_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int val;
	val = *buf;

	elan_tp_setrotate_enable(val);
	//printk(KERN_INFO "elan_tp_rotate_enable_store=%d\n", val);
	return 4;
}
static ssize_t elan_tp_cursorX_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int val = elan_tp_getcursorX();
	printk(KERN_INFO "elan_tp_cursorX_show=%d\n", val);
	return 4;
}

static ssize_t elan_tp_cursorX_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int val;
	val = *(buf) + ((*(buf+1))<<8) + ((*(buf+2))<<16) + ((*(buf+3))<<24);
#ifdef ELAN_LIVE_WALL
	tmp_X = val; /// for livewallpaper
#endif
	elan_tp_setcursorX(val);
	//printk(KERN_INFO "elan_tp_cursorX_store=%d\n", val);
	return 4;
}

static ssize_t elan_tp_cursorY_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int val = elan_tp_getcursorY();
	printk(KERN_INFO "elan_tp_cursorY_store=%d\n", val);
	return 4;
}

static ssize_t elan_tp_cursorY_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int val;
	val = *(buf) + ((*(buf+1))<<8) + ((*(buf+2))<<16) + ((*(buf+3))<<24);
#ifdef ELAN_LIVE_WALL
	tmp_Y = val; /// for livewallpaper
#endif
	elan_tp_setcursorY(val);
	//printk(KERN_INFO "elan_tp_cursorY_store=%d\n", val);
	return 4;
}

static ssize_t elan_tp_rotate_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int* tmp = (int*)buf;	
	tmp[0] = elan_tp_getrotate();
	printk("elan_tp_getrotate=%d\n",tmp[0]);
	return 4;
}

static ssize_t elan_tp_rotate_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	elan_tp_setrotate( (int)buf[0] );
	//printk("elan_tp_setrotate=%d\n",(int)buf[0]);
	return 4;
}

static ssize_t elan_tp_cursor_speed_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int* tmp = (int*)buf;	
	tmp[0] = elan_tp_getmousespeed();
	printk("elan_tp_getmousespeed=%d\n",tmp[0]);
	return 4;
}

static ssize_t elan_tp_cursor_speed_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	elan_tp_setmousespeed( (int)buf[0] );
	//printk("elan_tp_setmousespeed=%d\n",(int)buf[0]);
	return 4;
}

static ssize_t elan_tp_edge_width_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int* tmp = (int*)buf;	
	tmp[0] = elan_tp_getedgewidth();
	printk("elan_tp_getedgewidth=%d\n",tmp[0]);
	return 4;
}

static ssize_t elan_tp_edge_width_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	elan_tp_setedgewidth( (int)buf[0] );
	//printk("elan_tp_setedgewidth=%d\n",(int)buf[0]);
	return 4;
}

static ssize_t elan_tp_onefinger_type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int* tmp = (int*)buf;	
	tmp[0] = elan_tp_getonefingertype();
	printk("elan_tp_getonefingertype=%d\n",tmp[0]);
	return 4;
}

static ssize_t elan_tp_onefinger_type_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	elan_tp_setonefingertype( (int)buf[0] );
	//printk("elan_tp_setonefingertype=%d\n",(int)buf[0]);
	return 4;
}

#ifdef ELAN_TP_MODE
static DEVICE_ATTR(elan_tp_HomeMode_enable, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH, elan_tp_HomeMode_enable_show, elan_tp_HomeMode_enable_store);
#endif

#ifdef ELAN_LIVE_WALL
/* for LiveWallpaper */
static DEVICE_ATTR(elan_tp_get_coord_x, S_IROTH, elan_tp_get_coord_x_show, NULL);
static DEVICE_ATTR(elan_tp_get_coord_y, S_IROTH, elan_tp_get_coord_y_show, NULL);
#endif
static DEVICE_ATTR(elan_tp_rotate_enable, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, elan_tp_rotate_enable_show, elan_tp_rotate_enable_store);
static DEVICE_ATTR(elan_tp_cursorX, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, elan_tp_cursorX_show, elan_tp_cursorX_store);
static DEVICE_ATTR(elan_tp_cursorY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, elan_tp_cursorY_show, elan_tp_cursorY_store);
static DEVICE_ATTR(elan_tp_rotate, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, elan_tp_rotate_show, elan_tp_rotate_store);
static DEVICE_ATTR(elan_tp_cursor_speed, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, elan_tp_cursor_speed_show, elan_tp_cursor_speed_store);
static DEVICE_ATTR(elan_tp_edge_width, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, elan_tp_edge_width_show, elan_tp_edge_width_store);
static DEVICE_ATTR(elan_tp_onefinger_type, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, elan_tp_onefinger_type_show, elan_tp_onefinger_type_store);

static struct attribute *ets_attributes[] = {
#ifdef ELAN_TP_MODE
		&dev_attr_elan_tp_HomeMode_enable.attr,
#endif
#ifdef ELAN_LIVE_WALL
		&dev_attr_elan_tp_get_coord_x.attr,
		&dev_attr_elan_tp_get_coord_y.attr,
#endif
		&dev_attr_elan_tp_rotate_enable.attr,
		&dev_attr_elan_tp_cursorX.attr,
		&dev_attr_elan_tp_cursorY.attr,
    	&dev_attr_elan_tp_rotate.attr,
    	&dev_attr_elan_tp_cursor_speed.attr,
    	&dev_attr_elan_tp_edge_width.attr,
    	&dev_attr_elan_tp_onefinger_type.attr,
    	NULL,
};

static struct attribute_group ets_attr_group = {
    .attrs = ets_attributes,
};
/*----------------------------------------------------------------------------*/


static int elan_spi_write(struct elan_tp_spi *elanspi, const void *buf, size_t len)
{
	struct spi_message	m;
	uint8_t buf_recv[4]={0,0,0,0};
	int i;

	mutex_lock(&buf_lock);
	spi_message_init(&m);
	for(i = 0; i<len; i++)
	{
		elanspi->t[i].tx_buf = buf+i;
		elanspi->t[i].rx_buf = buf_recv+i;
		elanspi->t[i].len	  = 1;
		elanspi->t[i].delay.value = 100;
		elanspi->t[i].delay.unit = SPI_DELAY_UNIT_USECS;
		spi_message_add_tail(&elanspi->t[i],&m);
	}
	spi_sync(elanspi->spi,&m);
	//printk("-----spi_write_return %x %x %x %x-----\n",buf_recv[0],buf_recv[1],buf_recv[2],buf_recv[3]);
	mutex_unlock(&buf_lock);  

	return 0;
}


static int elan_spi_read(struct elan_tp_spi *elanspi, void *buf, size_t len)
{
	struct spi_message	m;
	const uint8_t buf_send = 0xff;
	int i;

	mutex_lock(&buf_lock);
	spi_message_init(&m);
	for(i = 0; i<len; i++)
	{
		elanspi->t[i].rx_buf = buf+i;
		elanspi->t[i].tx_buf = &buf_send;
		elanspi->t[i].len	  = 1;
		elanspi->t[i].delay.value = 80;
		elanspi->t[i].delay.unit = SPI_DELAY_UNIT_USECS;

		spi_message_add_tail(&elanspi->t[i],&m);
	}
	spi_sync(elanspi->spi,&m);
	mutex_unlock(&buf_lock);
	return 0;
}


static irqreturn_t elan_tp_irq_handler(int this_irq, void *dev_id)
{
	struct elan_tp_spi *elanspi = (struct elan_tp_spi *)dev_id;

	if(elanspi->binitialize)
	{
		disable_irq_nosync(elanspi->irq);
		queue_work(elanspi->workq, &elanspi->work);
	}
	return IRQ_HANDLED;
}

static void elan_tp_spi_work(struct work_struct *work)
{
	struct elan_tp_spi *elanspi = container_of(work, struct elan_tp_spi, work);
	uint8_t buf[14]={0,0,0,0,0,0,0,0,0,0,0,0,0,0}; 
	MULTI_FINGERS package;	
	int fingercnt = 0;
	int x,y;
	
//	elan_tp_irq_OK();

	if(elan_spi_read(elanspi,buf,14)==0)
	{
//printk("%x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11],buf[12],buf[13]);
		fingercnt = (buf[1]&0xC0)>>6;
		if(fingercnt>0)
		{
			if(buf[13]==0x1)
			{
				//1st finger
				x = buf[2] & 0xf;
				x = x<<8;
				x |= buf[3];
				y = buf[5] & 0xf;
				y = y<<8;
				y |= buf[6];
				package.fingers[0].iX = x;
				package.fingers[0].iY = y;
				
				//2nd finger
				if(fingercnt>=2)
				{
					x = buf[8] & 0xf;
					x = x<<8;
					x |= buf[9];
					y = buf[11] & 0xf;
					y = y<<8;
					y |= buf[12];
					package.fingers[1].iX = x;
					package.fingers[1].iY = y;
				}
			}
			else
			{
				goto drop_package;
			}
				
		}		
		package.fCount = fingercnt;
		elan_tp_do_touchpad(elanspi->input_mouse,elanspi->input_touch,&package);	
	}	
drop_package:
	enable_irq(elanspi->irq);
}

#if defined(ENABLE_DELAY_START)
static void elan_tp_init_work(struct work_struct *init_work)
{
    struct delayed_work * dw = container_of(init_work, struct delayed_work, work);
    struct elan_tp_spi *elanspi = container_of(dw, struct elan_tp_spi, init_work);
    struct spi_device *spi = elanspi->spi;
	int status = 0;

    printk(KERN_INFO"Trackpad irq was enabled.\n");

    status = request_irq(elanspi->irq, elan_tp_irq_handler,IRQF_TRIGGER_LOW,spi->dev.driver->name, elanspi);
    if (status < 0){
        printk( "request_irq failed\n");
        status = -ENOMEM;
    }
}
#endif

static int handle_hello_package(struct elan_tp_spi *elanspi)
{
	uint8_t buf_recv[4] = { 0,0,0,0 };	

	int rc;

	rc = elan_spi_read(elanspi, buf_recv, 4);
	if (rc != 0)
	{
		printk("hello package spi read fail\n");		
		return -EINVAL;
	}
	//else
	//	printk("dump hello packet: %x, %x, %x, %x\n",buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3]);
	return rc;
}

static int  initial_touchpad(struct elan_tp_spi *elanspi)
{
	uint8_t buf_cmd[4] = { 0x5B,0x10,0xC,0x1 };
	uint8_t buf[14];
	int rc;
	int status,retry = 10;	

	rc = elan_spi_write(elanspi,buf_cmd,4);

	if (rc != 0)
		return -EINVAL;

	retry = 2;	
	//poll int pin falling
	// status = gpio_get_value(elanspi->gpio);

	while( /*( status == 0 ) && */ retry > 0) {
		//status = gpio_get_value(elanspi->gpio);
		retry--;
		elan_spi_read(elanspi,buf,14);
		mdelay(10);
	}

	/*
	if(status==0) {
		printk(KERN_INFO"ELAN TP: gpio pin not rising!!\n");		
		return -ETIMEDOUT;
	}
	*/
	printk(KERN_INFO"ELAN TP: gpio pin status OK!!\n");		

	return rc;
}

static int elan_tp_spi_probe(struct spi_device *spi)
{
	int status = 0;
	unsigned char rotation = 0;
	struct elan_tp_spi *elanspi;
	struct input_dev* input_touch;
	struct input_dev* input_mouse;
	
	spi->bits_per_word = 8;
	status = spi_setup(spi);


	elanspi = devm_kzalloc(&spi->dev,
			       sizeof(struct elan_tp_spi), GFP_KERNEL);
	if (elanspi == NULL) 
	{
		printk("spi setup failed\n");
		return -ENOMEM;
	}
	

	input_touch = devm_input_allocate_device(&spi->dev);
	if(!input_touch)
	{
		printk( "crate input touch device failed\n");
		return -ENOMEM;
	}
	elanspi->input_touch = input_touch;

	input_mouse = devm_input_allocate_device(&spi->dev);
	if(!input_mouse)
	{
		printk( "crate input mouse device failed\n");
		status = -ENOMEM;
	}
	elanspi->input_mouse = input_mouse;

	elanspi->binitialize = false;
	//elanspi->gpio = (int)(spi->dev.platform_data);
	elanspi->spi = spi;
	elanspi->irq = spi->irq;
	INIT_WORK(&elanspi->work, elan_tp_spi_work);
	spi_set_drvdata(spi,elanspi);
	mutex_init(&buf_lock);

	elanspi->t = devm_kzalloc(&spi->dev, sizeof(struct spi_transfer)*(20),GFP_KERNEL);
	if(!elanspi->t)
	{
		printk( "create spi transfer failed\n");
		return -ENOMEM;
	}	

	elanspi->workq = create_singlethread_workqueue("elan_tp_wq");
	if (!elanspi->workq) 
	{
		printk( "create_workqueue failed\n");
		return -ENOMEM;
	}

#if defined(ENABLE_DELAY_START)
	INIT_DELAYED_WORK(&elanspi->init_work, elan_tp_init_work);

	elanspi->init_workq = create_workqueue("elan_tp_init_wq");
	if (!elanspi->init_workq) 
	{
		printk( "create_workqueue failed\n");
		status = -ENOMEM;
		goto fail2_3;
	}
#endif

	input_mouse->name = "elan-mouse";
   	set_bit(EV_KEY, input_mouse->evbit);    	
    	set_bit(BTN_MOUSE, input_mouse->keybit);
    	set_bit(BTN_WHEEL, input_mouse->keybit);
	set_bit(BTN_LEFT, input_mouse->keybit);
	set_bit(BTN_RIGHT, input_mouse->keybit);
 	set_bit(BTN_MIDDLE, input_mouse->keybit);
	set_bit(KEY_ENTER, input_mouse->keybit);
	set_bit(KEY_REPLY, input_mouse->keybit);
	set_bit(KEY_UP, input_mouse->keybit);
	set_bit(KEY_DOWN, input_mouse->keybit);
	set_bit(KEY_LEFT, input_mouse->keybit);
	set_bit(KEY_RIGHT, input_mouse->keybit);
	
    	set_bit(EV_REL, input_mouse->evbit);
    	set_bit(REL_X, input_mouse->relbit);
    	set_bit(REL_Y, input_mouse->relbit);
	input_set_drvdata(input_mouse, elanspi);
	status = input_register_device(input_mouse);
	if (status < 0) 
	{
		printk( "input_register_device failed\n");
		status = -ENOMEM;
		goto fail3;
	}


	input_touch->name = "elan-touchpad";
	set_bit(EV_KEY, input_touch->evbit);
	set_bit(BTN_TOUCH, input_touch->keybit);
	set_bit(KEY_ENTER, input_touch->keybit);
     	set_bit(KEY_REPLY, input_touch->keybit);
	set_bit(BTN_TOOL_FINGER, input_touch->keybit);
    	set_bit(EV_ABS, input_touch->evbit);
 	set_bit(ABS_MT_TRACKING_ID, input_touch->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, input_touch->absbit);
	set_bit(ABS_MT_TOUCH_MINOR, input_touch->absbit);
	set_bit(ABS_MT_POSITION_X, input_touch->absbit);
	set_bit(ABS_MT_POSITION_Y, input_touch->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_touch->absbit);
	set_bit(INPUT_PROP_DIRECT, input_touch->propbit);
	input_mt_init_slots(input_touch, 2, INPUT_MT_DIRECT | INPUT_MT_POINTER);
	input_set_abs_params(input_touch, ABS_MT_TRACKING_ID, 0, 2, 0, 0);
	input_set_abs_params(input_touch, ABS_MT_POSITION_X,0, TOUCHPAD_WIDTH, 0, 0);
	input_set_abs_params(input_touch, ABS_MT_POSITION_Y,0, TOUCHPAD_HEIGHT,0, 0);
	input_set_abs_params(input_touch, ABS_MT_TOUCH_MAJOR, 0, 100, 0, 0);
	input_set_abs_params(input_touch, ABS_MT_TOUCH_MINOR, 0, 100, 0, 0);
	input_set_abs_params(input_touch, ABS_MT_WIDTH_MAJOR, 0, 100, 0, 0);	
	input_set_drvdata(input_touch, elanspi);
	status = input_register_device(input_touch);
	if (status < 0) 
	{
		printk("input_register_device failed\n");
		status = -ENOMEM;
		goto fail4;
	}

	//get hello package
	if(handle_hello_package(elanspi)<0)
	{
		printk("get hello package fail!\n");
		status = -ENOMEM;
		goto fail5;
	}

  	status = sysfs_create_group(&input_touch->dev.kobj, &ets_attr_group);
	if (status !=0)
	{
		printk( "sysfs_create_group failed\n");
		status = -ENOMEM;
		goto fail5;		
	}	

	elan_tp_initialize(TOUCHPAD_WIDTH,TOUCHPAD_HEIGHT);

	elan_tp_restore_rotate(ROTATE_270);
#if 0
	rotation = bdinf_get_tp_rota();
	if (rotation == 0xFF) {	// 11111111 -> first time
		int ret = 0;
		printk(KERN_INFO "trackpad first time boot\n");
		ret = bdinf_set_tp_rota(0xFB);
		if (ret < 0) {
			printk(KERN_ERR "failed to write rotation data to eeprom, err:%d\n", ret);
		}
		elan_tp_restore_rotate(ROTATE_270);
	} else {	// 111110xx -> not first time
		rotation &= 0x3;
		elan_tp_restore_rotate(rotation);	// read from eeprom to restore last rotation state
	}
#endif

//initial hardware
	if(initial_touchpad(elanspi)<0)
	{
		printk("initial touch pad fail!\n");
		status = -ENOMEM;		
		goto fail5;
	}

	elanspi->binitialize = true;
#if defined(ENABLE_DELAY_START)
	queue_delayed_work(elanspi->init_workq,&elanspi->init_work,msecs_to_jiffies(1000*15));
#else
	status = request_irq(elanspi->irq, elan_tp_irq_handler,IRQF_TRIGGER_LOW,spi->dev.driver->name, elanspi);
	if (status < 0) 
	{
		printk( "request_irq failed\n");
		status = -ENOMEM;
		goto fail5;
	}
#endif

	//init_elan_tp_timer(elanspi->gpio, elanspi->irq);

	printk("**********elan spi touch pad probe success****************\n");

	return 0;

fail5:
	input_unregister_device(input_touch);
fail4:
	input_unregister_device(input_mouse);
fail3:
	destroy_workqueue(elanspi->workq);
#if defined(ENABLE_DELAY_START)
fail2_3:
	destroy_workqueue(elanspi->init_workq);
#endif
fail0:
	printk("elan spi touch pad probe failed!\n");

	return status;

}


static void elan_tp_spi_remove(struct spi_device *spi)
{
	struct elan_tp_spi * elanspi = dev_get_drvdata(&spi->dev);

	input_unregister_device(elanspi->input_mouse);
	input_unregister_device(elanspi->input_touch);
	destroy_workqueue(elanspi->workq);	   
#if defined(ENABLE_DELAY_START)
	destroy_workqueue(elanspi->init_workq);
#endif
	//gpio_free(elanspi->gpio);
	//free_irq(elanspi->irq, elanspi);
	spi_set_drvdata(spi,NULL);
}

#ifdef	CONFIG_PM

static int elan_tp_spi_suspend(struct device *dev)
{
	struct elan_tp_spi *elanspi = spi_get_drvdata(to_spi_device(dev));
	disable_irq(elanspi->irq);
	return 0;
}

static int elan_tp_spi_resume(struct device *dev)
{
	struct elan_tp_spi *elanspi = spi_get_drvdata(to_spi_device(dev));
	enable_irq(elanspi->irq);
	return 0;
}

#endif

static const struct spi_device_id elan_tp_spi_id[] = {
        { "ekt1059", 0 },
	{},
};
MODULE_DEVICE_TABLE(spi, elan_tp_spi_id);

static const struct of_device_id elan_tp_of_spi_match[] = {
	{ .compatible = "elan,ekt1059" },
	{ },
};
MODULE_DEVICE_TABLE(of, elan_tp_of_spi_match);

static SIMPLE_DEV_PM_OPS(elan_tp_spi_pm, elan_tp_spi_suspend, elan_tp_spi_resume);

static struct spi_driver elan_tp_spi_driver = {
	.driver	= {
		.name	 = "elan_tp_spi",
                .of_match_table = elan_tp_of_spi_match,
		.pm = &elan_tp_spi_pm,
	},
        .id_table = elan_tp_spi_id,
	.probe	= elan_tp_spi_probe,
	.remove	= elan_tp_spi_remove,
};

module_spi_driver(elan_tp_spi_driver);

MODULE_DESCRIPTION("Elan eKT1059 SPI touch pad");
MODULE_AUTHOR("Duson Lin (Elan)<dusonlin@emc.com.tw>");
MODULE_LICENSE("GPL");
