// SPDX-License-Identifier: GPL-2.0-or-later
#define DEBUG

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>
#include <drm/drm_probe_helper.h>
#include "panel-tc358762.h"

static const struct drm_display_mode default_mode = {
	.clock			= 41600,	/* kHz */
	.hdisplay		= 960,
	.hsync_start		= 960 + 176,
	.hsync_end		= 960 + 176 + 20,
	.htotal			= 960 + 176 + 20 + 86,
	.vdisplay		= 540,
	.vsync_start		= 540 + 9,
	.vsync_end		= 540 + 9 + 3,
	.vtotal			= 540 + 9 + 3 + 12,
	.flags			= 0,
	.width_mm		= 63,
	.height_mm		= 112,
};

struct bt200 {
	struct device *dev;
	struct drm_panel panel;
	bool prepared;
	bool enabled;
};

static int tc358762_read_register(struct bt200 *ctx, u16 reg)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	u8 buf[4];
	u8 addr_buf[2];
	u32 val;
	int r = 0;

	addr_buf[0] = reg;
	addr_buf[1] = reg >> 8;
	r = mipi_dsi_generic_read(dsi, addr_buf, 2, buf, sizeof(buf));
	if (r < 0) {
		dev_err(ctx->dev, "gen read failed\n");
		return r;
	}

	val = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	dev_dbg(ctx->dev, "reg read %x, val=%08x\n", reg, val);

	return r;
}

static int tc358762_write_register(struct bt200 *ctx, u16 addr, u32 val, int value_len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	u8 data[6];

	data[0] = addr;
	data[1] = addr >> 8;
	data[2] = val;
	data[3] = val >> 8;
	data[4] = val >> 16;
	data[5] = val >> 24;

	return mipi_dsi_generic_write(dsi, data, sizeof(addr) + value_len);
}

static inline int tc358762_write_lcd(struct bt200 *ctx, u8 addr, u8 data )
{
//	u16 reg = ( ( (u16)addr << 8 ) | data );
	u16 reg = ( ( (u16)data << 8 ) | addr );
	return tc358762_write_register(ctx, WCMDQUE, reg, sizeof(u16));
}

static inline struct bt200 *panel_to_bt200(struct drm_panel *panel)
{
	return container_of(panel, struct bt200, panel);
}

static int bt200_disable(struct drm_panel *panel)
{
	struct bt200 *ctx = panel_to_bt200(panel);
	tc358762_write_lcd(ctx, 0x0A, 0 );
	return 0;
}

static int bt200_unprepare(struct drm_panel *panel)
{
	int r = 0;
	struct bt200 *ctx = panel_to_bt200(panel);

	dev_dbg(ctx->dev, "%s\n", __func__);

	if (!ctx->prepared)
		return 0;

//	r = init_seq(ctx);

	ctx->prepared = false;

	return r;

}

static struct {
	u8 addr;
	u8 data;
} lcddr_init[] = {
	{0x01,0x00},{0x02,0x00},{0x03,0x00},{0x05,0x01},	//   4
	{0x07,0x00},{0x0A,0x00},{0x10,0x03},{0x11,0x44},	//   8
	{0x12,0x44},{0x13,0x55},{0x14,0x03},{0x15,0x00},	//  12
	{0x16,0x2A},{0x17,0x20},{0x18,0x00},{0x19,0x10},	//  16
	{0x1A,0x12},{0x1B,0x0E},{0x1C,0x0F},{0x1D,0x10},	//  20
	{0x1E,0x0F},{0x1F,0x1B},{0x20,0x0F},{0x21,0x00},	//  24
	{0x22,0x00},{0x23,0x00},{0x24,0x00},{0x28,0x14},	//  28
	{0x29,0x19},{0x2A,0x17},{0x2B,0x2B},{0x2C,0x99},	//  32
	{0x2D,0x13},{0x2E,0x2A},{0x30,0x0B},{0x31,0x00},	//  36
	{0x32,0x00},{0x33,0x01},{0x34,0x00},{0x35,0x0B},	//  40
	{0x36,0x04},{0x37,0x21},{0x38,0x00},{0x39,0x46},	//  44
	{0x3A,0x01},{0x3B,0x06},{0x3C,0x03},{0x3D,0x00},	//  48
	{0x3E,0x06},{0x3F,0x04},{0x40,0x00},{0x41,0x0D},	//  52
	{0x42,0x00},{0x43,0x2E},{0x45,0x08},{0x46,0x00},	//  56
	{0x47,0x01},{0x48,0x00},{0x49,0x00},{0x4A,0x0B},	//  60
	{0x4B,0x38},{0x4C,0x03},{0x4D,0x04},{0x50,0x0F},	//  64
	{0x51,0x04},{0x52,0x01},{0x53,0x0E},{0x54,0x11},	//  68
	{0x55,0x9F},{0x56,0x36},{0x57,0x00},{0x58,0x68},	//  72
	{0x59,0x01},{0x5A,0xE0},{0x5B,0x00},{0x5C,0x00},	//  76
	{0x5D,0x10},{0x5E,0x36},{0x5F,0x36},{0x60,0x00},	//  80
	{0x61,0x04},{0x62,0x12},{0x63,0x00},{0x64,0x80},	//  84
	{0x65,0x00},{0x66,0x80},{0x67,0x1C},{0x68,0x00},	//  88
	{0x69,0x00},{0x6A,0x00},{0x6B,0x00},{0x6C,0x00},	//  92
	{0x70,0x00},{0x71,0x55},{0x72,0x44},{0x73,0x33},	//  96
	{0x74,0x22},{0x75,0x33},{0x76,0x22},{0x77,0x22},	// 100
	{0x78,0x33},{0x79,0x22},{0x7A,0x44},{0x7B,0x55},	// 104
	{0x7C,0x00},{0x80,0x00},{0x81,0x66},{0x82,0x22},	// 108
	{0x83,0x33},{0x84,0x22},{0x85,0x33},{0x86,0x22},	// 112
	{0x87,0x22},{0x88,0x33},{0x89,0x44},{0x8A,0x44},	// 116
	{0x8B,0x44},{0x8C,0x00},{0x90,0x00},{0x91,0x77},	// 120
	{0x92,0x66},{0x93,0x33},{0x94,0x11},{0x95,0x33},	// 124
	{0x96,0x22},{0x97,0x22},{0x98,0x44},{0x99,0x44},	// 128
	{0x9A,0x33},{0x9B,0x44},{0x9C,0x00},{0xA0,0x88},	// 132
	{0xA1,0x88},{0xA2,0x88},{0xA3,0x88},{0xA4,0x88},	// 136
	{0xA5,0x88},{0xA6,0x88},{0xA7,0x88},{0xA8,0x88},	// 140
	{0xA9,0x88},{0xAA,0x88},{0xAB,0x88},{0xAC,0x88},	// 144
	{0xAD,0x88},{0xAE,0x88},{0xAF,0x88},{0xB0,0x88},	// 148
	{0xB1,0x88},{0xB2,0x88},{0xB3,0x88},{0xB4,0x88},	// 152
	{0xB5,0x88},{0xB6,0x88},{0xB7,0x88},{0xB8,0x88},	// 156
	{0xB9,0x88},{0xBA,0x88},{0xBB,0xA6},{0xBC,0x88},	// 160
	{0xBD,0x88},{0xBE,0x88},{0xBF,0x88},{0xC0,0x88},	// 164
	{0xC1,0x88},{0xC2,0x88},{0xC3,0x88},{0xC4,0x88},	// 168
	{0xC5,0x88},{0xC6,0x88},{0xC7,0x88},{0xC8,0x88},	// 172
	{0xC9,0x88},{0xD0,0x36},{0xD1,0x26},{0xD2,0x21},	// 176
	{0xD3,0x1F},{0xD4,0x17},{0xD5,0x15},{0xD6,0x13},	// 180
	{0xD7,0x10},{0xD8,0x08},{0xD9,0x08},{0xDA,0x18},	// 184
	{0xDB,0x1D},{0xDC,0x1F},{0xDD,0x27},{0xDE,0x29},	// 188
	{0xDF,0x2B},{0xE0,0x2E},{0xE1,0x36},{0xE6,0x00},	// 192
	{0xF1,0x00},{0xF2,0x00},{0xF3,0x00}			// 195
};

static int init_lcd(struct bt200 *ctx)
{
	int i;
	int r;
	r = tc358762_write_register(ctx, SPICTRL, SPI_SEL_CS0, sizeof(u32) );
	for (i = 0; i < ARRAY_SIZE(lcddr_init); ++i) {
		//printk("addr = %02x value = %02x\n",(u16)lcddr_init[i].addr, lcddr_init[i].data);
		r = tc358762_write_lcd(ctx, (u16)lcddr_init[i].addr, lcddr_init[i].data);
		if (r) {
			dev_err(ctx->dev, "failed to write initial config (write) %d\n", i);
			return r;
		}
	}

	return 0;
}

static int init_seq(struct bt200 *ctx)
{
	static const struct
	{
		u16 reg;
		u32 data;
	} tc358762_init_seq[] = {

		{ RDPKTLN, 0x00000003 },		// set read packet size
		{ DSI_LANEENABLE, 0x00000007 },
		/* D*S_CLRSIPOCOUNT = [(THS-SETTLE + THS-ZERO) / HS_byte_clock_period ] */
		{ PPI_D0S_CLRSIPOCOUNT, 0x00000003 },
		{ PPI_D1S_CLRSIPOCOUNT, 0x00000003 },
		/* */
		{ PPI_D0S_ATMR, 0x00000000 },
		{ PPI_D1S_ATMR, 0x00000000 },
		/* SYSLPTX Timing Generation Counter */
		{ PPI_LPTXTIMECNT, 0x00000007 },

		/* SPI */
		{ SPICTRL, 0x00000002 },
		{ SPITCR1, 0x00000122 },

		// DE
		{ LCDCTRL0, 0x00000152 },
		{ HSR, 0x00560014 },
		{ HDISPR, 0x002103C0 },	// HFP=33
		{ VSR, 0x000C0003 },
		{ VDISPR, 0x0009021C },
		{ VFUEN, 0x00000001 },

		{ SYSCTRL, 0x0000020F },

		/* Changed to 1 */
		{ PPI_STARTPPI, 0x00000001 },
		/* Changed to 1 */
		{ DSI_STARTDSI, 0x00000001 },

		{ SYSPLL3, 0xB8640000 },
	};
	
	int i;
	int r;
	dev_info(ctx->dev, "id reg %d\n" ,tc358762_read_register(ctx, IDREG));
	dev_info(ctx->dev, "id reg %d\n" ,tc358762_read_register(ctx, IDREG));
	dev_info(ctx->dev, "id reg %d\n" ,tc358762_read_register(ctx, IDREG));
	// Rx read
	r = tc358762_read_register(ctx, IDREG);
	if (r < 0) {
		dev_err(ctx->dev,
			"failed to read id reg %d\n", r);

		return r;
	}

	for (i = 0; i < ARRAY_SIZE(tc358762_init_seq); ++i) {
		u16 reg = tc358762_init_seq[i].reg;
		u32 data = tc358762_init_seq[i].data;

		r = tc358762_write_register(ctx, reg, data, sizeof(u32));
		if (r) {
			dev_err(ctx->dev,
				"failed to write initial config (write) %d\n", i);
			return r;
		}
	}


	r = init_lcd(ctx);
//	if (r < 0)
//		goto err_write_init;
	return r;
}

static int bt200_prepare(struct drm_panel *panel)
{
	struct bt200 *ctx = panel_to_bt200(panel);
	int r;

	dev_dbg(ctx->dev, "%s\n", __func__);

	if (ctx->prepared)
		return 0;

	r = init_seq(ctx);

	ctx->prepared = true;

	return r;
	
}

static int bt200_enable(struct drm_panel *panel)
{
	struct bt200 *ctx = panel_to_bt200(panel);
	return tc358762_write_lcd(ctx, 0x0A, 1 );
}

static int bt200_get_modes(struct drm_panel *panel, struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct bt200 *ctx = panel_to_bt200(panel);
	
	dev_dbg(panel->dev, "%s\n", __func__);

	mode = drm_mode_duplicate(connector->dev, &default_mode);
	if (!mode) {
		dev_err(panel->dev, "failed to add mode %ux%u@%u\n",
			default_mode.hdisplay, default_mode.vdisplay,
			drm_mode_vrefresh(&default_mode));
		return -ENOMEM;
	}

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;	// REVISIT: do we need this?
	drm_mode_probed_add(connector, mode);

#if 0
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
#endif

	dev_dbg(panel->dev, "%s done\n", __func__);

	return 1;
}

static const struct drm_panel_funcs bt200_panel_funcs = {
	.disable = bt200_disable,
	.unprepare = bt200_unprepare,
	.prepare = bt200_prepare,
	.enable = bt200_enable,
	.get_modes = bt200_get_modes,
};

static int bt200_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct bt200 *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);
	
	ctx->dev = dev;

	dsi->lanes = 2;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_CLOCK_NON_CONTINUOUS |
			  MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
		          MIPI_DSI_MODE_VIDEO_BURST | 
			  MIPI_DSI_MODE_LPM;
	dsi->hs_rate = 1050 * 41600 * 24 / (dsi->lanes * 2);
	dsi->lp_rate = 9200000;

#if 0
	dsi->hs_rate = 105 * (W677L_HS_CLOCK / 100);	/* allow for 5% overclocking */
	dsi->lp_rate = W677L_LP_CLOCK;
#endif

	drm_panel_init(&ctx->panel, dev, &bt200_panel_funcs, DRM_MODE_CONNECTOR_DSI);
	drm_panel_add(&ctx->panel);
	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		goto err_dsi_attach;

	dev_dbg(dev, "%s ok\n", __func__);

	return 0;	

err_dsi_attach:
	drm_panel_remove(&ctx->panel);
	dev_dbg(dev, "%s nok\n", __func__);

	return ret;
}

static void bt200_remove(struct mipi_dsi_device *dsi)
{
	struct bt200 *ctx = mipi_dsi_get_drvdata(dsi);

	dev_dbg(&dsi->dev, "%s\n", __func__);

	mipi_dsi_detach(dsi);

	drm_panel_remove(&ctx->panel);

}

static const struct of_device_id bt200_of_match[] = {
	{ .compatible = "epson,bt200", },
	{},
};

MODULE_DEVICE_TABLE(of, bt200_of_match);

static struct mipi_dsi_driver bt200_driver = {
	.probe = bt200_probe,
	.remove = bt200_remove,
	.driver = {
		.name = "panel-bt200",
		.of_match_table = bt200_of_match,
		.suppress_bind_attrs = true,
	},
};

module_mipi_dsi_driver(bt200_driver);

MODULE_AUTHOR("Andreas Kemnade <andreas@kemnade.info>");
MODULE_DESCRIPTION("bt200 panel driver");
MODULE_LICENSE("GPL");
