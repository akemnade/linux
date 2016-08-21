/*
 * Driver for panels through Solomon Systech SSD2858 rotator chip
 *
 * Device Tree:
 *   this chip is defined with a pair "port {}" elements
 *   to keep the panel separated from the DSI interface
 *   so it is an "encoder" with an input and an output port
 *   Device Tree can config bypass, rotation and some other paramters
 *   that are used to initialize the chip.
 *   The current implementation is still a mix of SSD2858 plus Panel specific
 *   code and needs a lot of work to be finialised.
 *
 * External Bypass:
 *   the ssd2858 can send DCS(MCS) commands to the panel through a special
 *   mode. There is one exception that DCS(MCS) commands starting with 0xff
 *   need to be escaped and are limited in length. If the panel happens
 *   to need a command that is too long (like the boe-wl677) we need an
 *   external switch that bypasses the lane1 of the ssd2858 while the ssd2858
 *   is in reset state. This is assumed to exist and controlled by the same
 *   gpio as the ssd2858 reset.
 *
 *
 * Copyright (C) 2014 Golden Delicious Computers
 * Author: H. Nikolaus Schaller <hns@goldelico.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* here is what we expect to be logged for proper initialization

Panel MIPI:
  Dimensions: 720x1280 in 840x1340
  Pixel CLK: 67536000
  DDR CLK: 202608000
  LPCLK target: 8000000
SSD2858:
  XTAL CLK: 24000000
  LPCLK in: 12000000
  target VTCM PIXEL CLK: 33768000
  target SYS_CLK: 135072000
  target MAIN_CLK: 1350720000
  target PLL: 1350720000
  PLL_MULT / PLL_POST_DIV: 57 / 1
  real PLL: 1368000000
  MAIN_CLK: 1368000000
  SYS_CLK_DIV: 5
  SYS_CLK: 136800000
  PCLK_NUM / PCLK_DEN: 1 / 4
  VTCM PIXEL_CLK: 34200000
  Panel PIXEL_CLK: 68400000
  MTX_CLK_DIV: 3
MTX_CLK_DIV problem: 3 (1 or even divider) - ignored
  MIPITX_BIT_CLK: 456000000
  MIPITX_DDR_CLK: 228000000
  MIPITX_BYTE_CLK: 57000000
  LPCLK out: 7125000
  LOCKCNT: 504
OMAP MIPI:
  Dimensions: 1280x720 in 840x1340
  Pixel CLK: 68400000
  DDR CLK: 228000000
  LPCLK out: 12000000

[   30.534872] dsi: mipi_debug_reset(0)
[   30.539352] dsi: mipi_debug_regulator(1)
[   30.652943] dsi: mipi_debug_reset(1)
[   31.186511]   x_res=1280
[   31.192599]   y_res=720
[   31.199532]   lpclock=12000000
[   31.206693]   pixelclock=68400000
[   31.214667]   hfp=10
[   31.221208]   hsw=10
[   31.227753]   hbp=100
[   31.234135]   vfp=10
[   31.240979]   vsw=2
[   31.247372]   vbp=48
[   31.253663] dsi: mipi_debug_start()
[   31.262107] dsi: mipi_debug_power_on()
[   31.281199] dsi: enabled()
[   31.794830] rx packet size := 1
[   31.800300] dsi: mipi_debug_read( 0b) ->  00
[   31.805721] dsi: mipi_debug_read( 0c) ->  77
[   31.811167] dsi: mipi_debug_read( 45) ->  00
[   31.816344] rx packet size := 4
[   31.819997] dsi: mipi_debug_read(g, 00 04) ->  00 00 00 00
[   31.826675] dsi: mipi_debug_read(g, 00 08) ->  01 f4 01 32
[   31.833549] dsi: mipi_debug_read(g, 00 0c) ->  00 00 00 03
[   31.840396] dsi: mipi_debug_read(g, 00 10) ->  ff ff ff ff
[   31.847401] dsi: mipi_debug_read(g, 00 14) ->  0c 77 80 0f
[   31.854072] dsi: mipi_debug_read(g, 00 1c) ->  00 00 14 01
[   31.860925] dsi: mipi_debug_read(g, 00 20) ->  15 92 56 7d
[   31.867888] dsi: mipi_debug_read(g, 00 24) ->  00 00 33 00
[   31.874562] dsi: mipi_debug_read(g, 00 28) ->  00 00 00 00
[   31.881422] dsi: mipi_debug_read(g, 00 2c) ->  00 00 00 00
[   31.888264] dsi: mipi_debug_read(g, 00 30) ->  00 00 00 00
[   31.894768] dsi: mipi_debug_write(28)
[   31.899425] dsi: mipi_debug_write(10)
[   31.903882] dsi: mipi_debug_write(ff 00)
[   31.908803] dsi: mipi_debug_write(28)
[   31.913253] dsi: mipi_debug_write(10)
[   31.921474] dsi: mipi_debug_write(g,00 08 01 f8 00 39)
[   31.931410] dsi: mipi_debug_write(g,00 0c 00 00 00 24)
[   31.941395] dsi: mipi_debug_write(g,00 14 0c 37 80 0f)
[   31.951350] dsi: mipi_debug_write(g,00 20 15 d2 56 7d)
[   31.957910] dsi: mipi_debug_write(g,00 24 00 00 30 00)
[   31.964289] dsi: mipi_debug_read(g, 00 08) ->  01 f8 00 39
[   31.971150] dsi: mipi_debug_read(g, 00 0c) ->  00 00 00 24
[   31.978021] dsi: mipi_debug_read(g, 00 14) ->  0c 37 80 0f
[   31.984722] dsi: mipi_debug_read(g, 00 20) ->  15 d2 56 7d
[   31.991585] dsi: mipi_debug_read(g, 00 24) ->  00 00 30 00
[   31.999446] dsi: mipi_debug_write(11)
[   32.021459] dsi: mipi_debug_write(2a 00 00 04 ff)
[   32.031042] dsi: mipi_debug_write(2b 00 00 02 cf)
[   32.036681] dsi: mipi_debug_write(g,10 08 01 20 04 45)
[   32.046558] dsi: mipi_debug_write(g,20 0c 00 00 03 02)
[   32.056700] dsi: mipi_debug_write(g,20 10 00 04 00 01)
[   32.066636] dsi: mipi_debug_write(g,20 14 03 48 00 64)
[   32.076455] dsi: mipi_debug_write(g,20 18 05 3c 00 30)
[   32.091503] dsi: mipi_debug_write(g,20 1c 02 d0 05 00)
[   32.101536] dsi: mipi_debug_write(g,20 20 05 00 02 d0)
[   32.115507] dsi: mipi_debug_write(g,20 24 05 00 02 d0)
[   32.125329] dsi: mipi_debug_write(g,20 3c 05 00 02 d0)
[   32.131756] dsi: mipi_debug_write(g,20 34 00 00 00 00)
[   32.142625] dsi: mipi_debug_write(g,20 38 04 ff 02 cf)
[   32.149197] dsi: mipi_debug_write(g,20 30 00 00 00 15)
[   32.155405] dsi: mipi_debug_write(g,20 a0 00 00 00 50)
[   32.161945] dsi: mipi_debug_read(g, 20 14) ->  03 48 00 64
[   32.170486] dsi: mipi_debug_read(g, 20 38) ->  04 ff 02 cf
[   32.177602] dsi: mipi_debug_write(35 02)
[   32.182486] dsi: mipi_debug_write(44 05 00)
[   32.191224] dsi: mipi_debug_write(36 c0)
[   32.200536] dsi: mipi_debug_write(g,60 08 00 c7 00 08)
[   32.211136] dsi: mipi_debug_write(g,60 0c 30 64 02 0a)
[   32.221116] dsi: mipi_debug_write(g,60 10 05 00 0a 0a)
[   32.231103] dsi: mipi_debug_write(g,60 14 01 00 01 02)
[   32.241069] dsi: mipi_debug_write(g,60 84 00 00 02 d0)
[   32.247654] dsi: mipi_debug_read(g, 60 10) ->  05 00 0a 0a
[   32.254434] dsi: mipi_debug_write(g,ff 01)
[   32.259609] rx packet size := 1
[   32.263099] dsi: mipi_debug_read( 05) ->  00
[   32.268295] dsi: mipi_debug_read( 0a) ->  08
[   32.273911] dsi: mipi_debug_read( 0b) ->  00
[   32.279005] dsi: mipi_debug_read( 0c) ->  07
[   32.283601] dsi: mipi_debug_read( 0d) ->  00
[   32.288249] dsi: mipi_debug_read( 0e) ->  00
[   32.292856] dsi: mipi_debug_read( 0f) ->  00
[   32.297415] rx packet size := 2
[   32.300802] dsi: mipi_debug_read( 45) ->  00 00
[   32.305900] rx packet size := 1
[   32.309331] dsi: mipi_debug_read( 0b) ->  00
[   32.314228] dsi: mipi_debug_read( 0c) ->  07
[   32.319172] dsi: mipi_debug_read( 45) ->  00
[   32.324013] dsi: mipi_debug_write(g,ff 01)
[   32.328723] dsi: mipi_debug_write(11)
[   32.436918] dsi: mipi_debug_write(29)
[   32.441109] dsi: mipi_debug_write(g,ff 00)
[   32.567174] dsi: mipi_debug_write(29)

 */

#define REGULATOR 0
#define LOG 1

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "../dss/omapdss.h"
#include <video/omap-panel-data.h>
#include <video/mipi_display.h>

/* extended DCS commands (not defined in mipi_display.h) */

#define MIPI_DCS_SET_ADDRESS_MODE_HFLIP 0x40
#define MIPI_DCS_SET_ADDRESS_MODE_VFLIP 0x80

#if 0	// are not used by ssd!

#define DCS_READ_DDB_START		0x02
#define DCS_READ_NUM_ERRORS		0x05
#define DCS_BRIGHTNESS			0x51	// write brightness
#define DCS_READ_BRIGHTNESS		0x52	// read brightness
#define DCS_CTRL_DISPLAY		0x53	// enable backlight etc.
#define DCS_READ_CTRL_DISPLAY	0x53	// read control
#define DCS_WRITE_CABC			0x55
#define DCS_READ_CABC			0x56
#define MCS_READID1		0xda
#define MCS_READID2		0xdb
#define MCS_READID3		0xdc

/* manufacturer specific commands */
#define MCS_MANUFPROT	0xb0
#define MCS_SETDEEPSTBY	0xb1
#define MCS_IFACESET	0xb3
#define MCS_MIPISPEED	0xb6
#define MCS_DISPLSET1	0xc1
#define MCS_DISPLSET2	0xc2
#define MCS_VSYNCEN		0xc3
#define MCS_SRCTIMING	0xc4
#define MCS_LPTSTIMING	0xc6
#define MCS_GAMMA_A		0xc7
#define MCS_GAMMA_B		0xc8
#define MCS_GAMMA_C		0xc9
#define MCS_PANELIFACE	0xcc
#define MCS_CHARGEPUMP	0xd0
#define MCS_POWERSET	0xd3
#define MCS_VCOMSET		0xd5

#endif

#define IS_MCS(CMD) (((CMD) >= 0xb0 && (CMD) <= 0xff) && !((CMD) == 0xda || (CMD) == 0xdb || (CMD) == 0xdc))

#define SSD2858_PIXELFORMAT		OMAP_DSS_DSI_FMT_RGB888	// 16.7M color = RGB888

struct panel_drv_data {
	struct omap_dss_device dssdev;	/* the output port (where a panel connects) */
	struct omap_dss_device *in;	/* the input port (OMAP to SSD) */

	struct omap_video_timings videomode;
	struct omap_dss_dsi_config dsi_config;
	
	struct platform_device *pdev;

	struct mutex lock;

	int	reset_gpio;
#if REGULATOR
	int	regulator_gpio;
#endif

	bool reset;	/* ssd is in reset state (and hardware bypass) */
	bool bypass;	/* ssd is in bypass mode */

	u32 xtal;	/* xtal clock frequency */
	bool rotate;	/* rotate by 90 degrees */
	int flip;	/* flip code */

	u16 tearline;	/* scanline for tearing impulse */
	bool tear;	/* enable tearing impulse */

	int config_channel;
	int pixel_channel;

	struct omap_video_timings panel_timings;
	struct omap_dss_dsi_config panel_dsi_config;

	/* derived panel parameters */

	u16 PANEL_FRAME_WIDTH;
	u16 PANEL_FRAME_HEIGHT;
	u32 PANEL_PCLK;
	u32 PANEL_MIN_DDR;
	u16 PANEL_FPS;		// frames per second
	u8 PANEL_BPP;		// bits per lane
	u8 PANEL_LANES;		// lanes

	/* SSD2858 mode parameters */

	bool SPLIT_MEM;
	bool TE_SEL;
	bool VBP;
	bool CKE;
	bool VB_MODE;
	bool VBE;
	bool PCLK_NUM;
	u8 PCLK_DEN;	// must be 1:4 for proper video processing
	u8 SYS_CLK_DIV;	// this is something to play with until all parameters fit the constraints
	u16 LOCKCNT;
	u16 PLL_POST_DIV;
	u16 PLL_MULT;
	u16 MTX_CLK_DIV;
	u16 LP_CLK_DIV;

};

#define to_panel_data(p) container_of(p, struct panel_drv_data, dssdev)

/*
 * hardware control
 */

static int ssd2858_reset(struct omap_dss_device *dssdev, bool activate)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);

#if LOG
	printk("dsi: ssd2858_reset(%d)\n", activate);
#endif

	if (gpio_is_valid(ddata->reset_gpio))
		gpio_set_value(ddata->reset_gpio, !activate);	/* assume active low */
	ddata->reset = activate;

	return 0;
}

#if REGULATOR
static int ssd2858_regulator(struct omap_dss_device *dssdev, int activate)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);

#if LOG
	printk("dsi: ssd2858_regulator(%d)\n", state);
#endif

	if (gpio_is_valid(ddata->regulator_gpio))
		gpio_set_value(ddata->regulator_gpio, activate);	/* assume active high */

	return 0;
}
#endif /* REGULATOR */

/*
 * communication with the SSD chip control registers
 */

static int ssd2858_write(struct omap_dss_device *dssdev, u8 *buf, int len)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;
	int r;
	int i;

#if LOG
	printk("dsi: ssd2858_write(%s", IS_MCS(buf[0])?"g":""); for(i=0; i<len; i++) printk("%02x%s", buf[i], i+1 == len?")\n":" ");
#endif

	if (IS_MCS(buf[0]))
		{ // this is a "manufacturer command" that must be sent as a "generic write command"
			r = in->ops.dsi->gen_write_nosync(in, ddata->config_channel, buf, len);
		}
	else
		{ // this is a "user command" that must be sent as "DCS command"
			r = in->ops.dsi->dcs_write_nosync(in, ddata->config_channel, buf, len);
		}

	if (r)
		dev_err(&ddata->pdev->dev, "write cmd/reg(%x) failed: %d\n",
				buf[0], r);

	return r;
}

static inline int ssd2858_write_cmd4(struct omap_dss_device *dssdev,
			     u8 dcs,
			     u8 p1, u8 p2, u8 p3, u8 p4)
{ /* send command with 4 parameter bytes */
	u8 buf[5];
	buf[0] = dcs;
	buf[1] = p1;
	buf[2] = p2;
	buf[3] = p3;
	buf[4] = p4;
	return ssd2858_write(dssdev, buf, sizeof(buf));
}

static inline int ssd2858_write_cmd3(struct omap_dss_device *dssdev,
			     u8 dcs,
			     u8 p1, u8 p2, u8 p3)
{ /* send command with 3 parameter bytes */
	u8 buf[3];
	buf[0] = dcs;
	buf[1] = p1;
	buf[2] = p2;
	buf[3] = p3;
	return ssd2858_write(dssdev, buf, sizeof(buf));
}

static inline int ssd2858_write_cmd2(struct omap_dss_device *dssdev,
			     u8 dcs,
			     u8 p1, u8 p2)
{ /* send command with 2 parameter bytes */
	u8 buf[3];
	buf[0] = dcs;
	buf[1] = p1;
	buf[2] = p2;
	return ssd2858_write(dssdev, buf, sizeof(buf));
}

static inline int ssd2858_write_cmd1(struct omap_dss_device *dssdev,
			     u8 dcs,
			     u8 p1)
{ /* send command with 1 parameter byte */
	u8 buf[2];
	buf[0] = dcs;
	buf[1] = p1;
	return ssd2858_write(dssdev, buf, sizeof(buf));
}

static inline int ssd2858_write_cmd0(struct omap_dss_device *dssdev,
			     u8 dcs)
{ /* send command with no parameter byte */
	return ssd2858_write(dssdev, &dcs, 1);
}

static int ssd2858_read(struct omap_dss_device *dssdev, u8 dcs_cmd, u8 *buf, int len)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;
	int r;
	int i;

	r = in->ops.dsi->set_max_rx_packet_size(in, ddata->config_channel, len);	// tell interface how much we expect
	if (r) {
		dev_err(&ddata->pdev->dev, "can't set max rx packet size\n");
		return -EIO;
	}

	if(IS_MCS(dcs_cmd))
		{ // this is a "manufacturer command" that must be sent as a "generic read command"
			r = in->ops.dsi->gen_read(in, ddata->config_channel, &dcs_cmd, 1, buf, len);
		}
	else
		{ // this is a "user command" that must be sent as "DCS command"
			r = in->ops.dsi->dcs_read(in, ddata->config_channel, dcs_cmd, buf, len);
		}

	if (r)
		dev_err(&ddata->pdev->dev, "read cmd/reg(%02x, %d) failed: %d\n",
				dcs_cmd, len, r);
#if LOG
	printk("dsi: ssd2858_read(%02x,", dcs_cmd); for(i=0; i<len; i++) printk(" %02x", buf[i]);
	printk(") -> %d\n", r);
#endif
	return r;
}

static int ssd2858_pass_to_panel(struct omap_dss_device *dssdev, bool enable)
{ // choose destination of further commands: SSD chip or panel
	struct panel_drv_data *ddata = to_panel_data(dssdev);

	if (ddata->reset) { /* assume hardware bypass is on */
		if (!enable) {
			dev_err(&ddata->pdev->dev, "can't send commands to ssd while in reset\n");
			return -EIO;
		}
	} else if (ddata->bypass != enable)
		{ /* write a "special control packet" prefix to switch pass through modes */
			ddata->bypass = enable;
#if LOG
			int r=ssd2858_write_cmd1(dssdev, 0xff, enable);
			printk("dsi: ssd2858_pass_to_panel(%d) -> %d\n", enable, r);
			return r;
#endif
			return ssd2858_write_cmd1(dssdev, 0xff, enable);
		}
	return 0;
}

/*
 * higher level accessors
 */

static int ssd2858_write_reg(struct omap_dss_device *dssdev, unsigned short address, unsigned long data)
{
	int r;
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;
	u8 buf[6];
	buf[0] = address >> 8;
	buf[1] = address >> 0;
	buf[2] = data >> 24;
	buf[3] = data >> 16;
	buf[4] = data >> 8;
	buf[5] = data >> 0;

	ssd2858_pass_to_panel(dssdev, false);	/* communicate with the SSD */

	r = in->ops.dsi->gen_write(in, ddata->config_channel, buf, 6);

#if LOG
	printk("dsi: ssd2858_write_reg: %04x <- %08lx (r=%d)\n", address, data, r);
#endif
	return r;
}

static int ssd2858_read_reg(struct omap_dss_device *dssdev, unsigned short address, unsigned long *data)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;
	int r;
	u8 buf[6];
	unsigned long val;

	ssd2858_pass_to_panel(dssdev, false);	/* communicate with the SSD */

	r = in->ops.dsi->set_max_rx_packet_size(in, ddata->config_channel, 4);	// tell panel how much we expect
	if (r) {
		dev_err(&ddata->pdev->dev, "can't set max rx packet size\n");
		return -EIO;
	}

	buf[0] = address >> 8;
	buf[1] = address >> 0;
	buf[2] = 0;
	buf[3] = 0;
	buf[4] = 0;
	buf[5] = 0;
	r = in->ops.dsi->gen_read(in, ddata->config_channel, buf, 2, &buf[2], 4);
	val  = (buf[2] << 24) | (buf[3] << 16) | (buf[4] << 8) | (buf[5] << 0);
#if LOG
	printk("dsi: ssd2858_read_reg: %04x -> %08lx (r=%d)\n", address, val, r);
#endif
	if (data)
		*data = val;
	return r;
}

/*
 * calculate timings based on panel and system setup
 *
 * note: timing values depend on rotation on or off
 */

static int ssd2858_get_panel_timings(struct omap_dss_device *dssdev)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);

#if LOG
	printk("ssd2858_get_panel_timings()\n");
#endif
	// we have a problem: dssdev->dst may not yet be initialized here!

	if (dssdev->dst && dssdev->dst->driver && dssdev->dst->driver->get_timings)
		dssdev->dst->driver->get_timings(dssdev->dst, &ddata->panel_timings);
	else {
#if LOG
		printk("ssd2858_get_panel_timings() use hard coded panel setup\n");
#endif
		ddata->panel_timings.x_res = 720;
		ddata->panel_timings.y_res = 1280;
		ddata->PANEL_FPS = 60;	// frames per second
		ddata->PANEL_BPP = 24;	// bits per lane
		ddata->PANEL_LANES = 4;	// lanes
		ddata->panel_timings.hfp = 10;	// front porch
		ddata->panel_timings.hsw = 10;	// sync active
		ddata->panel_timings.hbp = 100;	// back porch
		// NOTE: we must set this so the sum of V* is < ~70 to get a slightly higher pixel and DDR rate or the panel wouldn't sync properly
		// warning: some VBP values are causing horizontal misalignemt:
		// 12..15, 18..23, 26..?, 34..40, ...?
		ddata->panel_timings.vfp = 10;	// top porch
		ddata->panel_timings.vsw = 2;	// sync active
		ddata->panel_timings.vbp = 48;	// bottom porch
		ddata->panel_dsi_config.lp_clk_max = 8000000;	// maximum is 9.2 MHz
	}

	ddata->PANEL_FRAME_WIDTH = ddata->panel_timings.x_res
					+ ddata->panel_timings.hfp
					+ ddata->panel_timings.hsw
					+ ddata->panel_timings.hbp;	// some margin for sync and retrace
	ddata->PANEL_FRAME_HEIGHT = ddata->panel_timings.y_res
					+ ddata->panel_timings.vfp
					+ ddata->panel_timings.vsw
					+ ddata->panel_timings.vbp;	// some margin for sync and retrace
	ddata->PANEL_PCLK = ddata->PANEL_FRAME_WIDTH
					* ddata->PANEL_FRAME_HEIGHT
					* ddata->PANEL_FPS;	// required pixel clock
	ddata->PANEL_MIN_DDR = (ddata->PANEL_PCLK / 2 / ddata->PANEL_LANES) * ddata->PANEL_BPP;	// min is defined by required data bandwidth

	return 0;
}

static int ssd2858_calculate_timings(struct omap_dss_device *dssdev)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);

	u32 PANEL_MAX_DDR=250000000;	// maximum DDR clock (4..25ns) that can be processed by ssd2858 (output)
	u32 SSD_MAX_DDR=450000000;	// maximum DDR clock on ssd2858 input

	/* OMAP timings */

	ddata->dsi_config.mode = OMAP_DSS_DSI_VIDEO_MODE;
	ddata->dsi_config.pixel_format = SSD2858_PIXELFORMAT,
	ddata->dsi_config.timings = &ddata->videomode,
	ddata->dsi_config.ddr_clk_always_on = true,
	ddata->dsi_config.trans_mode = OMAP_DSS_DSI_BURST_MODE,
	ddata->dsi_config.lp_clk_max = ddata->xtal / 2;	// we must drive SSD with this LP clock frequency
	ddata->dsi_config.lp_clk_min = 6000000;

	ddata->videomode = ddata->panel_timings;
	if (ddata->rotate) {
		ddata->videomode.x_res = ddata->panel_timings.y_res;
		ddata->videomode.y_res = ddata->panel_timings.x_res;
	}

	/* calculate timings */

	/* SSD2858 mode parameters */

	ddata->SPLIT_MEM = 1;
	ddata->TE_SEL = 1;
	ddata->VBP = 0;
	ddata->CKE = 0;
	ddata->VB_MODE = 0;
	ddata->VBE = 0;
	ddata->PCLK_NUM = 1;
	ddata->PCLK_DEN = 4; // must be 1:4 for proper video processing

	// check input parameters

#define in_range(val, min, max) ((val) >= (min) && (val) <= (max))

	if (!in_range(ddata->xtal, 20000000, 300000000)) {
		printk("XTAL frequency problem: %u (20 - 30 MHz)\n", ddata->xtal);
		return -EINVAL;
	}

	/* SSD2858 clock divider parameters */

	for(ddata->SYS_CLK_DIV = 5; ddata->SYS_CLK_DIV <= 5; ddata->SYS_CLK_DIV++) {

		u32 TARGET_PIXEL_CLK;
		u32 TARGET_SYS_CLK;
		u32 TARGET_MAIN_CLK;
		u32 TARGET_DIV;	// total required divisor between PLL and MAIN_CLK - rounded down (running the PLL at least at target frequency)
		u32 TARGET_PLL;	// what we expect to see as PLL frequency
		u32 PLL;	// real PLL frequency (us usually higher than target)
		u32 MAIN_CLK;	// real MAIN clock
		u32 SYS_CLK;	// real SYS clock
		u32 PIXEL_CLK;	// real VTCM pixel clock
		u32 MIPITX_BIT_CLK;
		u32 MIPITX_DDR_CLK;	// going to panel
		u32 MIPITX_BYTE_CLK;
		u32 SSD_LPCLK;	// real LP clock output

		/* SSD divider calculations */

		/* 1. VCTM */
		TARGET_PIXEL_CLK = ddata->PANEL_PCLK / 2;
		TARGET_SYS_CLK = ddata->PCLK_DEN * TARGET_PIXEL_CLK / ddata->PCLK_NUM;
		TARGET_MAIN_CLK = 2 * ddata->SYS_CLK_DIV * TARGET_SYS_CLK;
		TARGET_DIV = 1500000000 / TARGET_MAIN_CLK;	// total required divisor between PLL and MAIN_CLK - rounded down (running the PLL at least at required frequency)
		TARGET_PLL = TARGET_DIV * TARGET_MAIN_CLK;	// what we expect to see as PLL frequency
		/* 2. PLL */
		ddata->PLL_MULT = (TARGET_PLL + ddata->xtal - 1) / ddata->xtal;	// required PLL multiplier from XTAL (rounded up)
		PLL = ddata->xtal * ddata->PLL_MULT;	// real PLL frequency
		ddata->PLL_POST_DIV = PLL / TARGET_MAIN_CLK;	// PLL_POST_DIV to get MAIN_CLOCK - should be >= TARGET_DIV
		MAIN_CLK = PLL / ddata->PLL_POST_DIV;	// real MAIN clock
		SYS_CLK = MAIN_CLK / 2 / ddata->SYS_CLK_DIV;	// real SYS clock
		PIXEL_CLK = (SYS_CLK * ddata->PCLK_NUM) / ddata->PCLK_DEN;	// real VTCM pixel clock
		/* 3. MIPITX */
		ddata->MTX_CLK_DIV = MAIN_CLK / ( 2 * ddata->PANEL_MIN_DDR );	// try to run at least with PANEL_MIN_DDR speed
		MIPITX_BIT_CLK = MAIN_CLK / ddata->MTX_CLK_DIV;
		MIPITX_DDR_CLK = MIPITX_BIT_CLK / 2;	// going to panel
		MIPITX_BYTE_CLK = MIPITX_BIT_CLK / 8;
		ddata->LP_CLK_DIV = (MIPITX_BYTE_CLK + ddata->panel_dsi_config.lp_clk_max - 1) / ddata->panel_dsi_config.lp_clk_max;	// divider
		SSD_LPCLK = MIPITX_BYTE_CLK / ddata->LP_CLK_DIV;	// real LP clock output
		/* LOCKCNT - at least 30us - this is the number of LPCLOCK (XTAL / 2); we add 40% safety margin */
		ddata->LOCKCNT=((((ddata->xtal / 2 ) * 30) / 1000000) * 140) / 100;

		/* calculate OMAP parameters */
		ddata->dsi_config.hs_clk_min = MIPITX_DDR_CLK;	// feed at least as fast as ssd output generates 
		ddata->dsi_config.hs_clk_max = SSD_MAX_DDR;

		ddata->videomode.pixelclock = 2 * PIXEL_CLK;	// feed pixels in speed as defined by SSD2858

#if LOG
		printk("Panel MIPI:\n");
		printk("  Dimensions: {%ux%u} in {%ux%u}\n", ddata->panel_timings.x_res, ddata->panel_timings.y_res, ddata->PANEL_FRAME_WIDTH, ddata->PANEL_FRAME_HEIGHT);
		printk("  Pixel CLK: %u\n", ddata->PANEL_PCLK);
		printk("  DDR CLK: %u\n", ddata->PANEL_MIN_DDR);
		printk("  LPCLK target: %lu..%lu\n", ddata->panel_dsi_config.lp_clk_max, ddata->panel_dsi_config.lp_clk_max);

		printk("SSD2858:\n");
		printk("  XTAL CLK: %u\n", ddata->xtal);
		printk("  LPCLK in: %u\n", ddata->xtal / 2);
		printk("  target VTCM Double PIXEL CLK: %u\n", TARGET_PIXEL_CLK);
		printk("  target SYS_CLK: %u\n", TARGET_SYS_CLK);
		printk("  target MAIN_CLK: %u\n", TARGET_MAIN_CLK);
		printk("  target PLL: %u\n", TARGET_PLL);
		printk("  PLL_MULT / PLL_POST_DIV: %u / %u\n", ddata->PLL_MULT, ddata->PLL_POST_DIV);
		printk("  real PLL: %u\n", PLL);
		printk("  MAIN_CLK: %u\n", MAIN_CLK);
		printk("  SYS_CLK_DIV: %ud\n", ddata->SYS_CLK_DIV);
		printk("  SYS_CLK: %u\n", SYS_CLK);
		printk("  PCLK_NUM / PCLK_DEN: %u / %u\n", ddata->PCLK_NUM , ddata->PCLK_DEN);
		printk("  VTCM PIXEL_CLK: %u\n", PIXEL_CLK);
		printk("  Panel PIXEL_CLK: %u\n", 2 * PIXEL_CLK);
		printk("  MTX_CLK_DIV: %u\n", ddata->MTX_CLK_DIV);
		printk("  MIPITX_BIT_CLK: %u\n", MIPITX_BIT_CLK);
		printk("  MIPITX_DDR_CLK: %u\n", MIPITX_DDR_CLK);
		printk("  MIPITX_BYTE_CLK: %u\n", MIPITX_BYTE_CLK);
		printk("  LPCLK out: %u\n", SSD_LPCLK);
		printk("  LOCKCNT: %u\n", ddata->LOCKCNT);

		printk("OMAP MIPI:\n");
		printk("  Dimensions: {%ux%u} in {%ux%u}\n", ddata->videomode.x_res,
			/* FIXME: */ ddata->videomode.y_res, ddata->PANEL_FRAME_WIDTH, ddata->PANEL_FRAME_HEIGHT);
		printk("  Pixel CLK: %u\n", ddata->videomode.pixelclock);
		printk("  DDR CLK: %lu..%lu\n", ddata->dsi_config.hs_clk_min, ddata->dsi_config.hs_clk_max);
		printk("  LPCLK out: %lu..%lu\n", ddata->dsi_config.lp_clk_min, ddata->dsi_config.lp_clk_max);

#endif

	/* check parameters */

// [ $PLL_MULT -ge 1 -a $PLL_MULT -le 128 ] || echo "PLL_MULT problem: $PLL_MULT (1 .. 127)"

		if (!in_range(ddata->PLL_MULT, 1, 128)) {
#if LOG
			printk("PLL_MULT problem: %u (1 .. 128)\n", ddata->PLL_MULT);
#endif
			continue;
		}

// [ $PLL_POST_DIV -ge 1 -a $PLL_POST_DIV -le 64 ] || echo "PLL_POST_DIV problem: $PLL_POST_DIV (1 .. 64)"

		if (!in_range(ddata->PLL_POST_DIV, 1, 64)) {
#if LOG
			printk("PLL_POST_DIV problem: %u (1 .. 64)\n", ddata->PLL_POST_DIV);
#endif
			continue;
		}

// [ $PLL -ge 1000000000 -a $PLL -le 1500000000 ] || echo "PLL frequency problem: $PLL (1.000 .. 1.500 GHz)"

		if (!in_range(PLL, 1000000000, 1500000000)) {
#if LOG
			printk("PLL frequency problem: %u (1.000 .. 1.500 GHz)\n", PLL);
#endif
			continue;
		}

// [ $SYS_CLK_DIV -ge 1 -a $SYS_CLK_DIV -le 16 ] || echo "SYS_CLK_DIV problem: $SYS_CLK_DIV (1 .. 16)"

		if (!in_range(ddata->SYS_CLK_DIV, 1, 16)) {
#if LOG
			printk("SYS_CLK_DIV problem: %u (1 .. 16)\n", ddata->SYS_CLK_DIV);
#endif
			continue;
		}

// [ $SYS_CLK -le 150000000 ] || echo "SYS_CLK problem: $SYS_CLK ( ... 150 MHz)"

		if (!in_range(SYS_CLK, 1, 150000000)) {
#if LOG
			printk("SYS_CLK problem: %u ( .. 150 MHz)\n", SYS_CLK);
#endif
			continue;
		}

// [ $PCLK_NUM -ge 1 -a $PCLK_NUM -le 128 ] || echo "PCLK_NUM problem: $PCLK_NUM (1 .. 128)"

		if (!in_range(ddata->PCLK_NUM, 1, 128)) {
#if LOG
			printk("PCLK_NUM problem: %u (1 .. 16)\n", ddata->PCLK_NUM);
#endif
			continue;
		}

// [ $PCLK_DEN -ge 1 -a $PCLK_DEN -le 256 ] || echo "PCLK_DEN problem: $PCLK_DEN (1 .. 256)"

		if (!in_range(ddata->PCLK_DEN, 1, 256)) {
#if LOG
			printk("PCLK_DEN problem: %u (1 .. 256)\n", ddata->PCLK_DEN);
#endif
			continue;
		}

// [ $MTX_CLK_DIV -ge 1 -a $MTX_CLK_DIV -le 16 ] || echo "MTX_CLK_DIV problem: $MTX_CLK_DIV (1 .. 15)"

		if (!in_range(ddata->MTX_CLK_DIV, 1, 16)) {
#if LOG
			printk("MTX_CLK_DIV problem: %u (1 .. 16)\n", ddata->MTX_CLK_DIV);
#endif
			continue;
		}

// [ $MTX_CLK_DIV -eq 1 -o $((MTX_CLK_DIV % 2)) -eq 0 ] || echo "MTX_CLK_DIV problem: $MTX_CLK_DIV (1 or even divider) - ignored"

		if (!ddata->MTX_CLK_DIV != 1 && !(ddata->MTX_CLK_DIV%2 == 0)) {
#if LOG
			printk("MTX_CLK_DIV problem: %u (1 or even divider) - ignored\n", ddata->MTX_CLK_DIV);
#endif
		//	continue;
		}

// [ $MIPITX_DDR_CLK -ge $PANEL_MIN_DDR ] || echo "MIPITX_DDR_CLK vs. PANEL_MIN_DDR problem: $MIPITX_DDR_CLK < $PANEL_MIN_DDR"
// [ $MIPITX_DDR_CLK -le $PANEL_MAX_DDR ] || echo "MIPITX_DDR_CLK vs. PANEL_MAX_DDR problem: $MIPITX_DDR_CLK < $PANEL_MAX_DDR"

		if (!in_range(MIPITX_DDR_CLK, ddata->PANEL_MIN_DDR, PANEL_MAX_DDR)) {
#if LOG
			printk("MIPITX_DDR_CLK problem: %u (%u .. %u)\n",MIPITX_DDR_CLK, ddata->PANEL_MIN_DDR, PANEL_MAX_DDR);
#endif
			continue;
		}

// [ $LP_CLK_DIV -gt 0 -a $LP_CLK_DIV -le 64 ] || echo "LP_CLK_DIV problem: $LP_CLK_DIV (1 .. 63)"

		if (!in_range(ddata->LP_CLK_DIV, 1, 64)) {
#if LOG
			printk("LP_CLK_DIV problem: %u (1 .. 64)\n", ddata->LP_CLK_DIV);
#endif
			continue;
		}

// [ $LOCKCNT -gt 0 -a $LOCKCNT -le 65535 ] || echo "LOCKCNT problem: $LOCKCNT (1 .. 65535)"

		if (!in_range(ddata->LOCKCNT, 1, 65535)) {
#if LOG
			printk("LOCKCNT problem: %u (1 .. 65535)\n", ddata->LOCKCNT);
#endif
			continue;
		}

		return 0;
	}

	return -EINVAL;	// no sufficiently matching SYS_CLK_DIV found
}

/*
 * set up the ssd2858 controller according to config and timing calculations
 */

static int ssd2858_power_on(struct omap_dss_device *dssdev)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct device *dev = &ddata->pdev->dev;
	struct omap_dss_device *in = ddata->in;
	int r;

#if LOG
	printk("dsi: ssd2858_power_on()\n");
#endif
	ssd2858_reset(dssdev, true);	// activate reset

	/* now initialize OMAP MIPI Interface */

#if 0
	if (ddata->pin_config.num_pins > 0) {
		r = in->ops.dsi->configure_pins(in, &ddata->pin_config);
		if (r) {
			dev_err(&ddata->pdev->dev,
					"failed to configure DSI pins\n");
			goto err0;
		}
	}
#endif

#if 0	/* already triggered by the panel */

/* FIXME: if we can to the next 2 steps here, we do not have to pass them through.
 * In other words: we do not have to pass set_config, enable, enable_hs immediately
 * to the in-ops but can call ssd2858_power_on() in enable_video_output() and call
 * set_config, enable, enable_hs here
 */

	r = in->ops.dsi->set_config(in, &ddata->dsi_config);
	if (r) {
		dev_err(dev, "failed to configure DSI\n");
		goto err0;
	}
	
	r = in->ops.dsi->enable(in);
	if (r) {
		dev_err(dev, "failed to enable DSI\n");
		goto err0;
	}
#endif

#if REGULATOR
	ssd2858_regulator(dssdev, 1);	// switch power on
	msleep(50);
#endif

#if 0	/* already triggered by the panel */
	in->ops.dsi->enable_hs(in, ddata->pixel_channel, true);
#endif
	/* send setup */

	ssd2858_pass_to_panel(dssdev, true);	/* communicate with the panel */

	// may read some registers / status

	ssd2858_write_cmd0(dssdev, MIPI_DCS_SET_DISPLAY_OFF);
	ssd2858_write_cmd0(dssdev, MIPI_DCS_ENTER_SLEEP_MODE);

	ssd2858_reset(dssdev, false);	// release reset
	msleep(10);

	/* setup the panel here */

	/* prepare for getting access to the ssd */

	ssd2858_pass_to_panel(dssdev, false);	/* communicate with the SSD */
	ssd2858_write_cmd0(dssdev, MIPI_DCS_SET_DISPLAY_OFF);
	ssd2858_write_cmd0(dssdev, MIPI_DCS_ENTER_SLEEP_MODE);

#if LOG
	ssd2858_read_reg(dssdev, 0x0008, NULL);
	ssd2858_read_reg(dssdev, 0x000c, NULL);
	ssd2858_read_reg(dssdev, 0x0014, NULL);
	ssd2858_read_reg(dssdev, 0x0020, NULL);
	ssd2858_read_reg(dssdev, 0x0024, NULL);
#endif

	/* start with programming the SCM of the SSD2858 */

	ssd2858_write_reg(dssdev, 0x0008, (ddata->LOCKCNT << 16) | (0 << 15) | (0 << 15) | ((ddata->PLL_POST_DIV-1) << 8) | ddata->PLL_MULT << 0);
	ssd2858_write_reg(dssdev, 0x000c, ((ddata->MTX_CLK_DIV-1) << 4) | ((ddata->SYS_CLK_DIV-1) << 0));
	ssd2858_write_reg(dssdev, 0x0014, 0x0C37800F);	// SCM_MISC2 (0C77800F): MRXEN = enabled
	ssd2858_write_reg(dssdev, 0x0020, 0x1592567D);	// SCM_ANACTRL1 (1592567D): CPEN = enabled
	ssd2858_write_reg(dssdev, 0x0024, 0x00003000);

#if LOG
	ssd2858_read_reg(dssdev, 0x0008, NULL);
	ssd2858_read_reg(dssdev, 0x000c, NULL);
	ssd2858_read_reg(dssdev, 0x0014, NULL);
	ssd2858_read_reg(dssdev, 0x0020, NULL);
	ssd2858_read_reg(dssdev, 0x0024, NULL);
#endif

	/* some DCS */

	ssd2858_write_cmd0(dssdev, MIPI_DCS_EXIT_SLEEP_MODE);
	msleep(1);

	ssd2858_write_cmd4(dssdev, MIPI_DCS_SET_COLUMN_ADDRESS,
			  (ddata->videomode.x_res-1) >> 24,
			  (ddata->videomode.x_res-1) >> 16,
			  (ddata->videomode.x_res-1) >> 8,
			  (ddata->videomode.x_res-1) >> 0);
	ssd2858_write_cmd4(dssdev, MIPI_DCS_SET_PAGE_ADDRESS,
			  (ddata->videomode.y_res-1) >> 24,
			  (ddata->videomode.y_res-1) >> 16,
			  (ddata->videomode.y_res-1) >> 8,
			  (ddata->videomode.y_res-1) >> 0);

	/* MIPIRX */

	ssd2858_write_reg(dssdev, 0x1008, 0x01200445);	// MIPIRX_DCR (01200245): HST=4

	/* VCTM */
	ssd2858_write_reg(dssdev, 0x200c, (ddata->SPLIT_MEM << 9) | (ddata->rotate << 8) | (ddata->VBP << 2) | (ddata->TE_SEL << 1));// VCTM_CFGR (00000000)
	ssd2858_write_reg(dssdev, 0x2010, (ddata->PCLK_DEN << 16) | (ddata->PCLK_NUM << 0));	// VCTM_PCFRR (00010001)
	ssd2858_write_reg(dssdev, 0x2014, (ddata->PANEL_FRAME_WIDTH << 16) | ddata->panel_timings.hbp);	// HDCFGR
	ssd2858_write_reg(dssdev, 0x2018, (ddata->PANEL_FRAME_HEIGHT << 16) | ddata->panel_timings.vbp);	// VDCFGR
	ssd2858_write_reg(dssdev, 0x201c, (ddata->videomode.y_res << 16) | ddata->videomode.x_res);	// MSZR
	ssd2858_write_reg(dssdev, 0x2020, (ddata->panel_timings.y_res << 16) | ddata->panel_timings.x_res);	// DSZR
	ssd2858_write_reg(dssdev, 0x2024, (ddata->panel_timings.y_res << 16) | ddata->panel_timings.x_res);	// PSZR
	ssd2858_write_reg(dssdev, 0x203c, (ddata->panel_timings.y_res << 16) | ddata->panel_timings.x_res);	// ISZR
	ssd2858_write_reg(dssdev, 0x2034, 0x00000000);	// VCTM_POSR (00000000)
	ssd2858_write_reg(dssdev, 0x2038, ((ddata->panel_timings.y_res - 1) << 16) | (ddata->panel_timings.x_res - 1));	// POER
	ssd2858_write_reg(dssdev, 0x2030, 0x00000015);	// URAM refresh period
	ssd2858_write_reg(dssdev, 0x20a0, 0x00000050);	// VTCM_QFBCCTRLR (00004151) - no padding, no pixswap, no fbc

	/* some more DCS */

	if (ddata->tear) {
		ssd2858_write_cmd1(dssdev, MIPI_DCS_SET_TEAR_ON, 0x02);
		ssd2858_write_cmd2(dssdev, MIPI_DCS_SET_TEAR_SCANLINE, ddata->tearline >> 8, ddata->tearline >> 0);
	}

	if (ddata->flip)
		ssd2858_write_cmd1(dssdev, MIPI_DCS_SET_ADDRESS_MODE, ddata->flip);

	ssd2858_write_reg(dssdev, 0x6008, 0x00000008 | ((ddata->PANEL_LANES - 1) << 22) | ((ddata->LP_CLK_DIV-1) << 16) | (ddata->CKE << 0));	// MIPITX_CTLR (00030008)
	ssd2858_write_reg(dssdev, 0x600c, (ddata->panel_timings.vbp << 24) | (ddata->panel_timings.hbp << 16) | (ddata->panel_timings.vsw << 8) | (ddata->panel_timings.hsw << 0));	// MIPITX_VTC1R (0214020A)
	ssd2858_write_reg(dssdev, 0x6010, (ddata->panel_timings.y_res << 16) | (ddata->panel_timings.vfp << 8) | (ddata->panel_timings.hfp << 0));	// MIPITX_VTC2R (0438020A)
	ssd2858_write_reg(dssdev, 0x6014, 0x01000102 | (ddata->VB_MODE << 13) | (ddata->VBE << 30));	// MIPITX_VCFR (01000101): VM=burst mode
	ssd2858_write_reg(dssdev, 0x6084, ddata->panel_timings.x_res << 0);	// MIPITX_DSI0VR (00000400)


#if LOG
	ssd2858_read_reg(dssdev, 0x6010, NULL);
#endif

// this sould come from the panel driver...

	ssd2858_pass_to_panel(dssdev, true);	/* communicate with the panel */

#if LOG
	{
		u8 data[8];
		ssd2858_read(dssdev, 0x0b, data, 1);
		ssd2858_read(dssdev, 0x0c, data, 1);
		ssd2858_read(dssdev, 0x45, data, 1);
	}
#endif

	ssd2858_write_cmd0(dssdev, MIPI_DCS_EXIT_SLEEP_MODE);
	ssd2858_write_cmd0(dssdev, MIPI_DCS_SET_DISPLAY_ON);

	ssd2858_pass_to_panel(dssdev, false);	/* communicate with the SSD */

#if 0	/* already triggered by the panel */
	r = in->ops.dsi->enable_video_output(in, ddata->pixel_channel);
	if (r)
		goto err;

	// do we need this delay?
	
	msleep(120);
#endif

	ssd2858_write_cmd0(dssdev, MIPI_DCS_SET_DISPLAY_ON);

#if LOG
	printk("dsi: powered on()\n");
#endif
	return r;
err:
#if LOG
	printk("dsi: power on error %d\n", r);
#endif
	dev_err(dev, "error powering on ssd2858 - activating reset\n");

#if 0	/* already triggered by the panel */

	in->ops.dsi->disable(in, false, false);
	mdelay(10);
#endif

	ssd2858_reset(dssdev, true);	// activate reset
#if REGULATOR
	ssd2858_regulator(dssdev, 0);	// switch power off
#endif
	mdelay(20);

err0:
	return r;
}

// we simply stop the video stream and assert the RESET
// please note that we don't/can't switch off the VCCIO

static void ssd2858_power_off(struct omap_dss_device *dssdev)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;
#if LOG
	printk("dsi: ssd2858_power_off()\n");
#endif
//	mutex_lock(&ddata->lock);

//	in->ops.dsi->bus_lock(in);

	// FIXME: we should ask the panel driver to send these commands and turn off panel power

	ssd2858_pass_to_panel(dssdev, true);	/* communicate with the panel */

	ssd2858_write_cmd0(dssdev, MIPI_DCS_SET_DISPLAY_OFF);
	ssd2858_write_cmd0(dssdev, MIPI_DCS_ENTER_SLEEP_MODE);

#if 0	/* already triggered by the panel */
	in->ops.dsi->disable_video_output(in, ddata->pixel_channel);
#endif
	ssd2858_pass_to_panel(dssdev, false);	/* communicate with the SSD */

	ssd2858_write_cmd0(dssdev, MIPI_DCS_SET_DISPLAY_OFF);
	ssd2858_write_cmd0(dssdev, MIPI_DCS_ENTER_SLEEP_MODE);

#if 0
	in->ops.dsi->disable(in, false, false);
#endif

	mdelay(10);
	ssd2858_reset(dssdev, true);	// activate reset
#if REGULATOR
	ssd2858_regulator(dssdev, 0);	// switch power off - after stopping video stream
#endif
	mdelay(20);

	/* here we could also power off IOVCC if possible */

//	in->ops.dsi->bus_unlock(in);

//	mutex_unlock(&ddata->lock);
}

/*
 * ops available to be called by attached panel
 *
 * a panel driver may call these operations and we must either
 * - translate them into SSD2858 register settings or
 * - forward to in->ops i.e. our own video source
 *
 * FIXME: we should have gen_write_nosync() and dcs_write()
 */

static int ssd2858_gen_write(struct omap_dss_device *dssdev, int channel, u8 *buf, int len)
{ /* panel driver wants us to send a generic packet to the panel through the SSD/bypass */
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;

#if LOG
	printk("dsi: ssd2858: ssd2858_gen_write\n");
#endif

	// translate channel

	ssd2858_pass_to_panel(dssdev, true);	// switch SSD2858 to pass-through mode

	if (!ddata->reset && buf[0] == 0xff) {
		u8 nbuf[4];
		if(len+1 > sizeof(nbuf)) {
			dev_err(&ddata->pdev->dev, "packet too long (%d bytes) for forwarding mechanism\n", len);
			return -EIO;
		}
		memcpy(&nbuf[1], buf, len);
		nbuf[0] = 0xff;	/* we need to prefix with another 0xff */
		buf = nbuf;	/* send from local copy */
		len += 1;
	}
#if LOG
	{
		int i;
		printk("dsi: ssd2858_gen_write("); for(i=0; i<len; i++) printk("%02x%s", buf[i], i+1 == len?")\n":" ");
	}
#endif
	return in->ops.dsi->gen_write(in, channel, buf, len);
}

static int ssd2858_dcs_write_nosync(struct omap_dss_device *dssdev, int channel, u8 *buf, int len)
{ /* panel driver wants us to send a DCS packet to the panel through the SSD/bypass */
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;

#if LOG
	printk("dsi: ssd2858: ssd2858_dcs_write_nosync\n");
#endif

	// translate channel
	ssd2858_pass_to_panel(dssdev, true);	// switch SSD2858 to pass-through mode

#if LOG
	{
		int i;
		printk("dsi: ssd2858_dcs_write("); for(i=0; i<len; i++) printk("%02x%s", buf[i], i+1 == len?")\n":" ");
	}
#endif
	return in->ops.dsi->dcs_write_nosync(in, channel, buf, len);
}

static int ssd2858_gen_read(struct omap_dss_device *dssdev, int channel,
				u8 *reqdata, int reqlen,
				u8 *data, int len)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;

#if LOG
	printk("dsi: ssd2858: ssd2858_gen_read\n");
#endif

	// translate channel
	ssd2858_pass_to_panel(dssdev, true);	// switch SSD2858 to pass-through mode

	return in->ops.dsi->gen_read(in, channel, reqdata, reqlen, data, len);
}

static int ssd2858_dcs_read(struct omap_dss_device *dssdev, int channel, u8 dcs_cmd,
				u8 *data, int len)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;

#if LOG
	printk("dsi: ssd2858: ssd2858_dcs_read\n");
#endif

	// translate channel
	ssd2858_pass_to_panel(dssdev, true);	// switch SSD2858 to pass-through mode

	return in->ops.dsi->dcs_read(in, channel, dcs_cmd, data, len);
}

static int ssd2858_set_max_rx_packet_size(struct omap_dss_device *dssdev, int channel, u16 plen)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;

#if LOG
	printk("dsi: ssd2858: ssd2858_set_max_rx_packet_size\n");
#endif

	// FIXME: we could lock a little here since the panel may now want to read and we should not interfere by setting our own max_rx_size
	// but it might not be necessary since we have no background activity that could read ssd registers

	// translate channel
	return in->ops.dsi->set_max_rx_packet_size(in, channel, plen);	// tell interface how much the panel expects
}

static int ssd2858_connect(struct omap_dss_device *dssdev, struct omap_dss_device *dst)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;
	struct device *dev = &ddata->pdev->dev;
	int r;

#if LOG
	printk("dsi: ssd2858: ssd2858_connect\n");
#endif
	dev_dbg(dssdev->dev, "connect\n");

	if (omapdss_device_is_connected(dssdev)) {
#if LOG
	printk("dsi: ssd2858 already connected\n");
#endif
		return -EBUSY;
	}

	r = in->ops.dsi->connect(in, dssdev);
	if (r) {
		dev_err(dev, "Failed to connect to video source\n");
		return r;
	}

	dst->src = dssdev;
	dssdev->dst = dst;

	/* channel0 used for video packets */
	r = in->ops.dsi->request_vc(ddata->in, &ddata->pixel_channel);
	if (r) {
		dev_err(dev, "failed to get virtual channel\n");
		goto err_req_vc0;
	}

	r = in->ops.dsi->set_vc_id(ddata->in, ddata->pixel_channel, 0);
	if (r) {
		dev_err(dev, "failed to set VC_ID\n");
		goto err_vc_id0;
	}

	/* channel1 used for registers access in LP mode */
	r = in->ops.dsi->request_vc(ddata->in, &ddata->config_channel);
	if (r) {
		dev_err(dev, "failed to get virtual channel\n");
		goto err_req_vc1;
	}

	r = in->ops.dsi->set_vc_id(ddata->in, ddata->config_channel, 0);
	if (r) {
		dev_err(dev, "failed to set VC_ID\n");
		goto err_vc_id1;
	}

	return 0;

err_vc_id1:
	in->ops.dsi->release_vc(ddata->in, ddata->config_channel);
err_req_vc1:
err_vc_id0:
	in->ops.dsi->release_vc(ddata->in, ddata->pixel_channel);
err_req_vc0:
#if LOG
	printk("dsi: ssd2858_connect error %d\n", r);
#endif
	in->ops.dsi->disconnect(in, dssdev);
	return r;
}

static void ssd2858_disconnect(struct omap_dss_device *dssdev, struct omap_dss_device *dst)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;

#if LOG
	printk("dsi: ssd2858: ssd2858_disconnect\n");
#endif
	dev_dbg(dssdev->dev, "disconnect\n");

	WARN_ON(!omapdss_device_is_connected(dssdev));
	if (!omapdss_device_is_connected(dssdev))
		return;

	WARN_ON(dst != dssdev->dst);
	if (dst != dssdev->dst)
		return;

	dst->src = NULL;
	dssdev->dst = NULL;

	in->ops.dsi->release_vc(in, ddata->pixel_channel);
	in->ops.dsi->release_vc(in, ddata->config_channel);
	in->ops.dsi->disconnect(in, &ddata->dssdev);
}

static int ssd2858_enable(struct omap_dss_device *dssdev)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;
	int r;

#if LOG
	printk("dsi: ssd2858: ssd2858_enable\n");
#endif
	dev_dbg(dssdev->dev, "enable\n");

	if (!omapdss_device_is_connected(dssdev))
		return -ENODEV;

	if (omapdss_device_is_enabled(dssdev))
		return 0;

	/* If we have no hardware bypass, we have to power on the ssd here and use the software bypass */

	in->ops.dsi->bus_lock(in);	/* will stay locked until panel disables */

	r = in->ops.dsi->enable(in);
	if (r) {
		in->ops.dsi->bus_unlock(in);
		return r;
	}

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	/* and program registers here */

	return 0;
}

static void ssd2858_disable(struct omap_dss_device *dssdev, bool disconnect_lanes, bool enter_ulps)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;

#if LOG
	printk("dsi: ssd2858: ssd2858_disable\n");
#endif
	dev_dbg(dssdev->dev, "disable\n");

	if (!omapdss_device_is_enabled(dssdev))
		return;

	in->ops.dsi->disable(in, disconnect_lanes, enter_ulps);

	in->ops.dsi->bus_unlock(in);	/* match bus lock in ssd2858_enable */

	dssdev->state = OMAP_DSS_DISPLAY_DISABLED;
}

static int ssd2858_set_config(struct omap_dss_device *dssdev, const struct omap_dss_dsi_config *cfg)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;

#if LOG
	printk("dsi: ssd2858: ssd2858_set_config\n");
#endif
	dev_dbg(dssdev->dev, "set_config\n");

	ddata->panel_dsi_config = *cfg;	/* save a copy of what the panel wants */

// should we recalculate / reconfigure timings here?
// currently we only use the lp clock max value

	in->ops.dsi->set_config(in, &ddata->dsi_config);	/* pass ssd config to our source */

	return 0;
}

static unsigned long channels;

static int ssd2858_request_vc(struct omap_dss_device *dssdev, int *channel)
{
#if LOG
	printk("dsi: ssd2858: ssd2858_request_vc\n");
#endif
	if (channels == 0xffffffff)
		return -ENOSPC;
	*channel = ffz(channels);
#if LOG
	printk("dsi: ssd2858: ssd2858_request_vc assigned channel %d\n", *channel);
#endif
	channels |= (1 << *channel);	// reserve this channel
	return 0;
}

static void ssd2858_release_vc(struct omap_dss_device *dssdev, int channel)
{
#if LOG
	printk("dsi: ssd2858: ssd2858_release_vc\n");
#endif
	channels &= ~(1 << channel);	// release channel bit
	return;
}

static int ssd2858_set_vc_id(struct omap_dss_device *dssdev, int channel, int vc_id)
{
#if LOG
	printk("dsi: ssd2858: ssd2858_set_vc_id\n");
#endif
	// we use default id values
	return 0;
}

static void ssd2858_enable_hs(struct omap_dss_device *dssdev, int channel, bool enable)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;

#if LOG
	printk("dsi: ssd2858: ssd2858_enable_hs\n");
#endif
	in->ops.dsi->enable_hs(in, channel, enable);
}

static int ssd2858_enable_video_output(struct omap_dss_device *dssdev, int channel)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;
	int r;

#if LOG
	printk("dsi: ssd2858: ssd2858_enable_video_output\n");
#endif
	/*
	 * this is called after the panel has sent its DCS/MCS commands through
	 * the hardware bypass which means that we can now power on and enable the ssd.
	 * Then we have to power on the ssd first (in ssd2858_enable) and use the software bypass.
	 * FIXME: How can we know that the hardware-bypass exists?
	 */

	r = ssd2858_power_on(dssdev);
	if (r) {
		dev_err(&ddata->pdev->dev, "failed to power on\n");
		return r;
	}

	return in->ops.dsi->enable_video_output(in, channel);
}

static void ssd2858_disable_video_output(struct omap_dss_device *dssdev, int channel)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;

#if LOG
	printk("dsi: ssd2858: ssd2858_disable_video_output\n");
#endif

	ssd2858_power_off(dssdev);

	return in->ops.dsi->disable_video_output(in, channel);
}

static void ssd2858_bus_lock(struct omap_dss_device *dssdev)
{
#if LOG
	printk("dsi: ssd2858: ssd2858_bus_lock\n");
#endif
	// no locking implemented - would need for multiple panels on single mipitx
	return;
}

static void ssd2858_bus_unlock(struct omap_dss_device *dssdev)
{
#if LOG
	printk("dsi: ssd2858: ssd2858_bus_unlock\n");
#endif
	// no locking
	return;
}

#if 1

/*
 * ops from the DSI video source
 *
 * Not clear if we need them at all.
 * But it could be necessary to swizzle the panel driver ops.
 * so that we can intercept them and do additional things.
 */

static int driver_ssd2858_connect(struct omap_dss_device *dssdev)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;
	struct device *dev = &ddata->pdev->dev;
	int r;

#if LOG
	printk("dsi: driver_ssd2858_connect\n");
#endif

	return 0;
}

static void driver_ssd2858_disconnect(struct omap_dss_device *dssdev)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;

#if LOG
	printk("dsi: driver_ssd2858_disconnect\n");
#endif
	if (!omapdss_device_is_connected(dssdev))
		return;
}

static void driver_ssd2858_get_timings(struct omap_dss_device *dssdev,
		struct omap_video_timings *timings)
{
#if LOG
	printk("dsi: driver_ssd2858_get_timings\n");
#endif
	*timings = dssdev->panel.timings;
}

static int driver_ssd2858_set_rotate(struct omap_dss_device *dssdev,
		u8 rotate)
{
#if LOG
	printk("dsi: driver_ssd2858_set_rotate %u\n", rotate);
#endif
	return 0;
}

static void driver_ssd2858_set_timings(struct omap_dss_device *dssdev,
		struct omap_video_timings *timings)
{
#if LOG
	printk("dsi: driver_ssd2858_set_timings\n");
#endif
	dssdev->panel.timings.x_res = timings->x_res;
	dssdev->panel.timings.y_res = timings->y_res;
	dssdev->panel.timings.pixelclock = timings->pixelclock;
	dssdev->panel.timings.hsw = timings->hsw;
	dssdev->panel.timings.hfp = timings->hfp;
	dssdev->panel.timings.hbp = timings->hbp;
	dssdev->panel.timings.vsw = timings->vsw;
	dssdev->panel.timings.vfp = timings->vfp;
	dssdev->panel.timings.vbp = timings->vbp;
	// FIXME: forward to panel driver?
}

static int driver_ssd2858_check_timings(struct omap_dss_device *dssdev,
		struct omap_video_timings *timings)
{
#if LOG
	printk("dsi: driver_ssd2858_check_timings\n");
#endif
	return 0;
}

static void driver_ssd2858_get_resolution(struct omap_dss_device *dssdev,
		u16 *xres, u16 *yres)
{
#if LOG
	printk("dsi: driver_ssd2858_get_resolution\n");
#endif
	*xres = dssdev->panel.timings.x_res;
	*yres = dssdev->panel.timings.y_res;
	// FIXME: forward to panel driver?
}

static void driver_ssd2858_disable(struct omap_dss_device *dssdev)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
#if LOG
	printk("dsi: driver_ssd2858_disable()\n");
#endif
	dev_dbg(&ddata->pdev->dev, "disable\n");

	if (dssdev->state == OMAP_DSS_DISPLAY_ACTIVE)
		ssd2858_power_off(dssdev);

	dssdev->state = OMAP_DSS_DISPLAY_DISABLED;
}

static int driver_ssd2858_enable(struct omap_dss_device *dssdev)
{
	struct panel_drv_data *ddata = to_panel_data(dssdev);
	struct omap_dss_device *in = ddata->in;
	int r = 0;

#if LOG
	printk("dsi: driver_ssd2858_enable()\n");
#endif
	dev_dbg(&ddata->pdev->dev, "enable\n");

	if (dssdev->state != OMAP_DSS_DISPLAY_DISABLED)
		return -EINVAL;

#if LOG
	printk("dsi: ssd2858_start()\n");
#endif
	mutex_lock(&ddata->lock);

	in->ops.dsi->bus_lock(in);

	r = ssd2858_power_on(dssdev);

	in->ops.dsi->bus_unlock(in);

	if (r)
		dev_err(&ddata->pdev->dev, "enable failed\n");
	else
		dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	mutex_unlock(&ddata->lock);

	return r;
}

#endif

static struct omap_dss_driver ssd2858_driver_ops = {
	.connect		= driver_ssd2858_connect,
	.disconnect		= driver_ssd2858_disconnect,

	.enable			= driver_ssd2858_enable,
	.disable		= driver_ssd2858_disable,

	.set_rotate		= driver_ssd2858_set_rotate,
	.get_resolution		= driver_ssd2858_get_resolution,

	.set_timings		= driver_ssd2858_set_timings,
	.get_timings		= driver_ssd2858_get_timings,
	.check_timings		= driver_ssd2858_check_timings,

	};

static struct omapdss_dsi_ops ssd2858_dsi_ops = {
	.connect		= ssd2858_connect,
	.disconnect		= ssd2858_disconnect,

	.enable			= ssd2858_enable,
	.disable		= ssd2858_disable,

	.set_config		= ssd2858_set_config,
/*
	.configure_pins		= ssd2858_configure_pins,
*/

	.enable_hs		= ssd2858_enable_hs,
/*
	.enable_te
	.update
*/

	.bus_lock		= ssd2858_bus_lock,
	.bus_unlock		= ssd2858_bus_unlock,

	.enable_video_output	= ssd2858_enable_video_output,
	.disable_video_output	= ssd2858_disable_video_output,

	.request_vc		= ssd2858_request_vc,
	.set_vc_id		= ssd2858_set_vc_id,
	.release_vc		= ssd2858_release_vc,
/*
	.dcs_write		= ssd2858_dcs_write,
*/
	.dcs_write_nosync	= ssd2858_dcs_write_nosync,
	.dcs_read		= ssd2858_dcs_read,
	.gen_write		= ssd2858_gen_write,
/*
	.gen_write_nosync	= ssd2858_gen_write_nosync,
*/
	.gen_read		= ssd2858_gen_read,

/*
	.bta_sync
*/
	.set_max_rx_packet_size	= ssd2858_set_max_rx_packet_size,
};

static int ssd2858_probe_of(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct panel_drv_data *ddata = platform_get_drvdata(pdev);
	struct omap_dss_device *ep;
	int gpio;
	u32 val32;
#if LOG
	printk("dsi: ssd2858_probe_of(%s)\n", node->name);
#endif
	if (node == NULL) {
		dev_err(&pdev->dev, "Unable to find device tree\n");
		return -EINVAL;
	}

	gpio = of_get_gpio(node, 0);
	if (!gpio_is_valid(gpio)) {
		dev_err(&pdev->dev, "failed to parse reset gpio (err=%d)\n", gpio);
		return gpio;
	}
	ddata->reset_gpio = gpio;

#if REGULATOR
	gpio = of_get_gpio(node, 1);
	if (!gpio_is_valid(gpio)) {
		dev_err(&pdev->dev, "failed to parse regulator gpio (err=%d)\n", gpio);
		return gpio;
	}
	ddata->regulator_gpio = gpio;
#endif

	if (of_property_read_u32(node, "xtal-freq-mhz", &val32)) {
		dev_err(&pdev->dev, "failed to find xtal frequency\n");
		return -EINVAL;
	}
	ddata->xtal = val32;

	ddata->rotate = false;
	ddata->flip = 0;

	val32 = 0;
	of_property_read_u32(node, "rotate", &val32);

	switch (val32) {
	case 0:
		break;
	case 90:
		ddata->rotate = true;
		break;
	case 180:
		ddata->flip = MIPI_DCS_SET_ADDRESS_MODE_HFLIP | MIPI_DCS_SET_ADDRESS_MODE_VFLIP;
		break;
	case 270:
		ddata->rotate = true;
		ddata->flip = MIPI_DCS_SET_ADDRESS_MODE_HFLIP | MIPI_DCS_SET_ADDRESS_MODE_VFLIP;
		break;
	default:
		dev_err(&pdev->dev, "rotate property must be 0, 90, 180 or 270 but is %d\n", val32);
		return -EINVAL;
	}

	if (of_property_read_bool(node, "flip-x"))
		ddata->flip ^= MIPI_DCS_SET_ADDRESS_MODE_HFLIP;
	if (of_property_read_bool(node, "flip-y"))
		ddata->flip ^= MIPI_DCS_SET_ADDRESS_MODE_VFLIP;

	if (of_property_read_u32(node, "te-scanline", &val32)) {
		ddata->tearline = val32;
		ddata->tear = true;
	}

#if LOG
	printk("dsi: ssd2858: rotate = %d flip = 0x%02x\n", ddata->rotate, ddata->flip);
#endif
	/*
	 * parse additional dt properties like
	 * - type of compression (e.g. optimal, lossy)
	 * - video-pass-through
	 * - test mode
	 * - channel definitions
	 * - charge pump vs. external power
	 * those are to be reflected by different register settings of the SSD2858 chip
	 */

	ep = omapdss_of_find_source_for_first_ep(node);
	if (IS_ERR(ep)) {
		dev_err(&pdev->dev, "failed to find video source (err=%ld)\n", PTR_ERR(ep));
		return PTR_ERR(ep);
	}

	ddata->in = ep;

	/*
	 * Room for enhancement:
	 * The SSD has a second MIPITX and can also set the output channel if two
	 * panels are connected in parallel.
	 * So we could handle more output ports or multiple endpoints for the second
	 * port here.
	 */

	return 0;
}

static int ssd2858_probe(struct platform_device *pdev)
{
	struct panel_drv_data *ddata;
	struct device *dev = &pdev->dev;
	struct omap_dss_device *dssdev;
	int r;

#if LOG
	printk("dsi: ssd2858_probe()\n");
#endif
	dev_dbg(dev, "ssd2858_probe\n");

	ddata = devm_kzalloc(dev, sizeof(*ddata), GFP_KERNEL);
	if (!ddata)
		return -ENOMEM;

	platform_set_drvdata(pdev, ddata);
	ddata->pdev = pdev;

	r = ssd2858_probe_of(pdev);
	if (r) {
		dev_err(dev, "Failed to probe %d\n", r);
		return r;
	}

	mutex_init(&ddata->lock);

	if (gpio_is_valid(ddata->reset_gpio)) {
		r = devm_gpio_request_one(dev, ddata->reset_gpio,
					  GPIOF_DIR_OUT, "rotator reset");
		if (r) {
			dev_err(dev, "failed to request reset gpio (%d err=%d)\n", ddata->reset_gpio, r);
			return r;
		}
	}

#if REGULATOR
	if (gpio_is_valid(ddata->regulator_gpio)) {
		r = devm_gpio_request_one(dev, ddata->regulator_gpio,
					 GPIOF_DIR_OUT, "rotator DC/DC regulator");
		if (r) {
			dev_err(dev, "failed to request regulator gpio (%d err=%d)\n", ddata->regulator_gpio, r);
			return r;
		}
	}
#endif

	dssdev = &ddata->dssdev;

	ssd2858_reset(dssdev, true);	/* activate reset */
#if REGULATOR
	ssd2858_regulator(dssdev, 0);	/* keep powered off until we need it */
#endif

#if LOG
	printk("change driver %p -> %p\n", dssdev->driver, &ssd2858_driver_ops);
#endif
	dssdev->dev = dev;
	dssdev->driver = &ssd2858_driver_ops;	// coming from video source
	dssdev->ops.dsi = &ssd2858_dsi_ops;	// coming from panel (if it calls in->ops.dsi)

	r = ssd2858_get_panel_timings(dssdev);
	if (r) {
		dev_err(dev, "Failed to calculate timings %d\n", r);
		goto err_reg;
	}

	r = ssd2858_calculate_timings(dssdev);
	if (r) {
		dev_err(dev, "Failed to calculate timings %d\n", r);
		goto err_reg;
	}

	dssdev->panel.timings = ddata->videomode;
	dssdev->type = OMAP_DISPLAY_TYPE_DSI;
	dssdev->output_type = OMAP_DISPLAY_TYPE_DSI;
	dssdev->owner = THIS_MODULE;

	dssdev->panel.dsi_pix_fmt = SSD2858_PIXELFORMAT;
	dssdev->id = OMAP_DSS_OUTPUT_DSI1;
	dssdev->name = "dsi.0";
	dssdev->dispc_channel = 1;

	r = omapdss_register_output(dssdev);
	if (r) {
		dev_err(dev, "Failed to register output\n");
		goto err_reg;
	}

	/* future: if we want to use the second MIPITX, we should
	 * register another output here
	 */

#if LOG
	printk("dsi: ssd2858_probe ok\n");
#endif

	return 0;

err_reg:
	omap_dss_put_device(ddata->in);
	return r;
}


static int __exit ssd2858_remove(struct platform_device *pdev)
{
	struct panel_drv_data *ddata = platform_get_drvdata(pdev);
	struct omap_dss_device *dssdev = &ddata->dssdev;
	struct omap_dss_device *in = ddata->in;

#if LOG
	printk("dsi: ssd2858_remove()\n");
#endif
	omapdss_unregister_output(&ddata->dssdev);

	WARN_ON(omapdss_device_is_enabled(dssdev));
	if (omapdss_device_is_enabled(dssdev))
		ssd2858_disable(dssdev, false, false);	// check parameters!

	WARN_ON(omapdss_device_is_connected(dssdev));
	if (omapdss_device_is_connected(dssdev))
		ssd2858_disconnect(dssdev, 0);	// check parameters!

	omap_dss_put_device(ddata->in);

	mutex_destroy(&ddata->lock);

	return 0;
}

static const struct of_device_id ssd2858_of_match[] = {
	{ .compatible = "omapdss,solomon-systech,ssd2858", },
	{},
};

MODULE_DEVICE_TABLE(of, ssd2858_of_match);

static struct platform_driver ssd2858_driver = {
	.probe = ssd2858_probe,
	.remove = ssd2858_remove,
	.driver = {
		.name = "ssd2858",
		.owner = THIS_MODULE,
		.of_match_table = ssd2858_of_match,
	},
};

module_platform_driver(ssd2858_driver);

MODULE_AUTHOR("H. Nikolaus Schaller <hns@goldelico.com>");
MODULE_DESCRIPTION("ssd2858 video encoder driver");
MODULE_LICENSE("GPL");
