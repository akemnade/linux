#define DEBUG
// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Marek Vasut <marex@denx.de>
 *
 * Based on tc358764.c by
 *  Andrzej Hajda <a.hajda@samsung.com>
 *  Maciej Purski <m.purski@samsung.com>
 *
 * Based on rpi_touchscreen.c by
 *  Eric Anholt <eric@anholt.net>
 */

#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/property.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/unaligned.h>

#include <video/mipi_display.h>
#include <video/videomode.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_bridge.h>
#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_of.h>
#include <drm/drm_print.h>
#include <drm/drm_probe_helper.h>

/* PPI layer registers */
#define PPI_STARTPPI		0x0104 /* START control bit */
#define PPI_STARTPPI_STARTPPI	BIT(0)

#define PPI_LPTXTIMECNT		0x0114 /* LPTX timing signal */
#define PPI_D0S_ATMR		0x0144
#define PPI_D1S_ATMR		0x0148
#define PPI_D0S_CLRSIPOCOUNT	0x0164 /* Assertion timer for Lane 0 */
#define PPI_D1S_CLRSIPOCOUNT	0x0168 /* Assertion timer for Lane 1 */

/* DSI layer registers */
#define DSI_STARTDSI		0x0204 /* START control bit of DSI-TX */
#define DSI_STARTDSI_STARTDSI	BIT(0)

#define DSI_LANEENABLE		0x0210 /* Enables each lane */
#define DSI_LANEENABLE_CLEN	BIT(0)
#define DSI_LANEENABLE_L0EN	BIT(1)
#define DSI_LANEENABLE_L1EN	BIT(2)

#define RDPKTLN			0x0410 /* Packet length */


/* LCDC/DPI Registers */
#define LCDCTRL			0x0420 /* Video Path Control */
#define LCDCTRL_MSF		BIT(0) /* Magic square in RGB666 */
#define LCDCTRL_VTGEN		BIT(1) /* Use chip clock for timing */
#define LCDCTRL_PXLFORM		GENMASK_U32(6, 4)
#define LCDCTRL_PXLFORM_RGB666		0	/* x:R:G:B 6:8:8:8 */
#define LCDCTRL_PXLFORM_RGB666_24	1	/* x:R:x:G:x:B 2:6:2:6:2:6 */
#define LCDCTRL_PXLFORM_RGB565		2	/* x:R:G:B 8:5:6:5 */
#define LCDCTRL_PXLFORM_RGB565_1	3	/* x:R:x:G:x:B 3:5:2:6:3:5 */
#define LCDCTRL_PXLFORM_RGB565_2	4	/* x:R:x:G:x:B:x 2:5:3:6:2:5:1 */
#define LCDCTRL_PXLFORM_RGB888		5	/* R:G:B 8:8:8 */
#define LCDCTRL_DPI_EN		BIT(8)
#define LCDCTRL_HSYNC_POL	BIT(17) /* Polarity of HSYNC signal */
#define LCDCTRL_DE_POL		BIT(18) /* Polarity of DE signal */
#define LCDCTRL_VSYNC_POL	BIT(19) /* Polarity of VSYNC signal */
#define LCDCTRL_DCLK_POL	BIT(20) /* Polarity of pixel clock */

#define LCDC_HSR_HBPR		0x0424
#define LCDC_HDISPR_HFPR	0x0428
#define LCDC_VSR_VBPR		0x042C
#define LCDC_VDISPR_VFPR	0x0430
#define LCDC_VFUEN		0x0434

/* SPI Master Registers */
#define SPICMR			0x0450
#define SPI_SEL_CS0		0x0002

#define SPITCR1			0x0454

#define WCMDQUE			0x0500



/* System Controller Registers */
#define SYSCTRL			0x0464
#define SYSCTRL_DPIDATA_IO_MASK GENMASK_U32(1, 0)
#define SYSCTRL_DPIDATA_IO_1MA	0
#define SYSCTRL_DPIDATA_IO_2MA	1
#define SYSCTRL_DPIDATA_IO_3MA	2
#define SYSCTRL_DPIDATA_IO_4MA	3
#define SYSCTRL_DPISTB_IO_MASK	GENMASK_U32(3, 2)
#define SYSCTRL_DPISTB_IO_1MA	0
#define SYSCTRL_DPISTB_IO_2MA	1
#define SYSCTRL_DPISTB_IO_3MA	2
#define SYSCTRL_DPISTB_IO_4MA	3
#define SYSCTRL_PCLKDIV_MASK	GENMASK_U32(11, 8)
#define SYSCTRL_PCLKDIV_DIV_2	2
#define SYSCTRL_PCLKDIV_DIV_3	4

#define IDREG			0x04A0 /* Chip and Revision ID */

#define LPX_PERIOD		7

struct tc358762 {
	struct device *dev;
	struct drm_bridge bridge;
	struct regulator *regulator;
	struct drm_bridge *panel_bridge;
	struct gpio_desc *reset_gpio;
	struct spi_controller *spi;
	bool pre_enabled;
	int error;
	bool use_vtg;
};

static int tc358762_clear_error(struct tc358762 *ctx)
{
	int ret = ctx->error;

	ctx->error = 0;
	return ret;
}

static int tc358762_read(struct tc358762 *ctx, u16 addr, u32 *val)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;
	u8 addr_buf[2];
	u8 buf[4];

	put_unaligned_le16(addr, addr_buf);
	dev_info(ctx->dev,"read addr %02x %02x\n", addr_buf[0], addr_buf[1]);

	ret = mipi_dsi_generic_read(dsi, addr_buf, 2, buf, sizeof(buf));
	if (ret < 0) {
		dev_err(ctx->dev, "gen read failed: %d\n", ret);
		return ret;
	}
	dev_info(ctx->dev,"read return %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);

	*val = get_unaligned_le32(buf);
	return ret;
}

static void tc358762_write(struct tc358762 *ctx, u16 addr, u32 val)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;
	u8 data[6];

	if (ctx->error)
		return;

	data[0] = addr;
	data[1] = addr >> 8;
	data[2] = val;
	data[3] = val >> 8;
	data[4] = val >> 16;
	data[5] = val >> 24;

	printk("tc358762_write: %04x %d %x\n", (int)addr, val, val);
	ret = mipi_dsi_generic_write(dsi, data, sizeof(data));
	if (ret < 0)
		ctx->error = ret;
}

static inline struct tc358762 *bridge_to_tc358762(struct drm_bridge *bridge)
{
	return container_of(bridge, struct tc358762, bridge);
}

static void tc358762_post_disable(struct drm_bridge *bridge,
				  struct drm_atomic_commit *state)
{
	struct tc358762 *ctx = bridge_to_tc358762(bridge);
	int ret;

	/*
	 * The post_disable hook might be called multiple times.
	 * We want to avoid regulator imbalance below.
	 */
	if (!ctx->pre_enabled)
		return;

	ctx->pre_enabled = false;

	/* Turn off the DPI output */
	tc358762_write(ctx, LCDCTRL, 0);

	if (ctx->reset_gpio)
		gpiod_set_value_cansleep(ctx->reset_gpio, 0);

	ret = regulator_disable(ctx->regulator);
	if (ret < 0)
		dev_err(ctx->dev, "error disabling regulators (%d)\n", ret);
}

static void tc358762_pre_enable(struct drm_bridge *bridge,
				struct drm_atomic_commit *state)
{
	struct tc358762 *ctx = bridge_to_tc358762(bridge);
	struct drm_connector_state *conn_state;
	struct drm_bridge_state *bridge_state;
	struct drm_crtc_state *crtc_state;
	struct drm_connector *connector;
	struct drm_display_mode *mode;
	u32 lcdctrl;
	int ret;

	dev_dbg(ctx->dev, "pre enable");
	ret = regulator_enable(ctx->regulator);
	if (ret < 0)
		dev_err(ctx->dev, "error enabling regulators (%d)\n", ret);

	if (ctx->reset_gpio) {
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		usleep_range(5000, 10000);
	}

	ctx->pre_enabled = true;

	bridge_state = drm_atomic_get_new_bridge_state(state, bridge);

	connector = drm_atomic_get_new_connector_for_encoder(state, bridge->encoder);
	conn_state = drm_atomic_get_new_connector_state(state, connector);
	crtc_state = drm_atomic_get_new_crtc_state(state, conn_state->crtc);
	mode = &crtc_state->mode;

	/*
	 * DPIENABLE has reset default of 1. Make sure we don't output on
	 * DPI until we have finished the coniguration.
	 */
	tc358762_write(ctx, LCDCTRL, 0);

	tc358762_write(ctx, SYSCTRL,
		       FIELD_PREP(SYSCTRL_DPIDATA_IO_MASK, SYSCTRL_DPIDATA_IO_4MA) |
		       FIELD_PREP(SYSCTRL_DPISTB_IO_MASK, SYSCTRL_DPISTB_IO_4MA) |
		       FIELD_PREP(SYSCTRL_PCLKDIV_MASK, SYSCTRL_PCLKDIV_DIV_3));

	msleep(100);

	tc358762_write(ctx, DSI_LANEENABLE,
		       DSI_LANEENABLE_L0EN | DSI_LANEENABLE_CLEN);
	tc358762_write(ctx, PPI_D0S_CLRSIPOCOUNT, 5);
	tc358762_write(ctx, PPI_D1S_CLRSIPOCOUNT, 5);
	tc358762_write(ctx, PPI_D0S_ATMR, 0);
	tc358762_write(ctx, PPI_D1S_ATMR, 0);
	tc358762_write(ctx, PPI_LPTXTIMECNT, LPX_PERIOD);

	if (ctx->use_vtg) {
		struct videomode vm = { 0 };

		drm_display_mode_to_videomode(mode, &vm);

		tc358762_write(ctx, LCDC_HSR_HBPR,
			       vm.hsync_len | (vm.hback_porch << 16));
		tc358762_write(ctx, LCDC_HDISPR_HFPR,
			       vm.hactive | (vm.hfront_porch << 16));

		tc358762_write(ctx, LCDC_VSR_VBPR,
			       vm.vsync_len | (vm.vback_porch << 16));
		tc358762_write(ctx, LCDC_VDISPR_VFPR,
			       vm.vactive | (vm.vfront_porch << 16));

		/* Upload VTG timings */
		tc358762_write(ctx, LCDC_VFUEN, BIT(0));
	}

	lcdctrl = FIELD_PREP(LCDCTRL_PXLFORM, LCDCTRL_PXLFORM_RGB888) |
		  LCDCTRL_DPI_EN;

	if (ctx->use_vtg)
		lcdctrl |= LCDCTRL_VTGEN;

	/* Note: DCLK_POL affects pixdata, de and syncs */
	if (bridge_state->output_bus_cfg.flags & DRM_BUS_FLAG_PIXDATA_DRIVE_POSEDGE)
		lcdctrl |= LCDCTRL_DCLK_POL;

	if (mode->flags & DRM_MODE_FLAG_PHSYNC)
		lcdctrl |= LCDCTRL_HSYNC_POL;

	if (mode->flags & DRM_MODE_FLAG_PVSYNC)
		lcdctrl |= LCDCTRL_VSYNC_POL;

	if (bridge_state->output_bus_cfg.flags & DRM_BUS_FLAG_DE_LOW)
		lcdctrl |= LCDCTRL_DE_POL;

	tc358762_write(ctx, LCDCTRL, lcdctrl);

	tc358762_write(ctx, PPI_STARTPPI, PPI_STARTPPI_STARTPPI);
	tc358762_write(ctx, DSI_STARTDSI, DSI_STARTDSI_STARTDSI);

	msleep(100);

	ret = tc358762_clear_error(ctx);
	if (ret < 0)
		dev_err(ctx->dev, "error initializing bridge (%d)\n", ret);
}

static void tc358762_enable(struct drm_bridge *bridge,
			    struct drm_atomic_commit *state)
{
	struct tc358762 *ctx = bridge_to_tc358762(bridge);
	int ret;

	dev_dbg(ctx->dev, "enable");
}

static int tc358762_attach(struct drm_bridge *bridge,
			   struct drm_encoder *encoder,
			   enum drm_bridge_attach_flags flags)
{
	struct tc358762 *ctx = bridge_to_tc358762(bridge);

	dev_dbg(ctx->dev, "attach\n");
	return drm_bridge_attach(encoder, ctx->panel_bridge,
				 bridge, flags);
}

static const struct drm_bridge_funcs tc358762_bridge_funcs = {
	.atomic_post_disable = tc358762_post_disable,
	.atomic_pre_enable = tc358762_pre_enable,
	.atomic_enable = tc358762_enable,
	.atomic_duplicate_state = drm_atomic_helper_bridge_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_bridge_destroy_state,
	.atomic_create_state = drm_atomic_helper_bridge_create_state,
	.attach = tc358762_attach,
};

static int tc358762_parse_dt(struct tc358762 *ctx)
{
	struct drm_bridge *panel_bridge;
	struct device *dev = ctx->dev;
	u32 lanes;

	panel_bridge = devm_drm_of_get_bridge(dev, dev->of_node, 1, 0);
	if (IS_ERR(panel_bridge))
		return PTR_ERR(panel_bridge);

	ctx->panel_bridge = panel_bridge;

	/* Reset GPIO is optional */
	ctx->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset_gpio))
		return PTR_ERR(ctx->reset_gpio);

	if (!device_property_read_u32(dev, "dsi-lanes", &lanes))
		to_mipi_dsi_device(ctx->dev)->lanes = lanes;

	return 0;
}

static int tc358762_spi_transfer_one(struct spi_controller *ctlr,
				     struct spi_device *spi,
				     struct spi_transfer *t)
{
	struct tc358762 *ctx = spi_controller_get_devdata(ctlr);
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	/* 
	 * limits to be determined, just define something which is
	 * enough for current use case.
	 */
	u8 data[8];

	if (t->len > sizeof(data) - 2)
	       return -EOVERFLOW;	

	/*
	 * half duplex is supported bi the bridge,
	 * but due to lack of testing, support only simplex write
	 */
	if (t->rx_buf)
		return -EINVAL;

	if (!t->tx_buf)
		return -EINVAL;

	put_unaligned_le16(WCMDQUE, data);
	memcpy(data + 2, t->tx_buf, t->len - 2);

	//return 0;
	return mipi_dsi_generic_write(dsi, data, t->len + 2);
}

static int tc358762_spi_setup(struct spi_device *spi)
{
	return 0;
}


static int tc358762_configure_regulators(struct tc358762 *ctx)
{
	ctx->regulator = devm_regulator_get(ctx->dev, "vddc");
	if (IS_ERR(ctx->regulator))
		return PTR_ERR(ctx->regulator);

	return 0;
}

static int tc358762_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct tc358762 *ctx;
	struct device_node *spi_node;
	int ret;

	ctx = devm_drm_bridge_alloc(dev, struct tc358762, bridge,
				    &tc358762_bridge_funcs);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	ctx->pre_enabled = false;

	spi_node = of_get_child_by_name(dev->of_node, "spi");
	if (spi_node) {
		ctx->spi = devm_spi_alloc_host(dev, 0);
		spi_controller_set_devdata(ctx->spi, ctx);
		//ctx->spi->setup = tc358762_spi_setup;
		ctx->spi->transfer_one = tc358762_spi_transfer_one;
		ctx->spi->dev.of_node = spi_node;
		ret = devm_spi_register_controller(dev, ctx->spi);
		if (ret < 0)
			return dev_err_probe(dev, ret, "register spi controller  failed\n");
	}

	/* Always use VTG */
	ctx->use_vtg = true;

	/*
	 * When using DSI clk for pixel clock (only mode supported in the driver),
	 * the pclk is derived directly from the DSI byteclk via simple divider,
	 * which is either 2 or 3.
	 * The required divider can be calculated with bitspp / 8 / nlanes. Thus,
	 * for RGB888, only nlanes = 1 works as nlanes = 2 would require divider
	 * of 1.5.
	 */
	dsi->lanes = 1;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS | 
			  MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_VIDEO_HSE;

	ret = tc358762_parse_dt(ctx);
	if (ret < 0)
		return ret;

	ret = tc358762_configure_regulators(ctx);
	if (ret < 0)
		return ret;

	ctx->bridge.type = DRM_MODE_CONNECTOR_DPI;
	ctx->bridge.of_node = dev->of_node;
	ctx->bridge.pre_enable_prev_first = true;

	ret = devm_drm_bridge_add(dev, &ctx->bridge);
	if (ret < 0)
		return ret;

	ret = devm_mipi_dsi_attach(dev, dsi);
	if (ret < 0)
		dev_err(dev, "failed to attach dsi\n");

	return ret;
}

static const struct of_device_id tc358762_of_match[] = {
	{ .compatible = "toshiba,tc358762" },
	{ }
};
MODULE_DEVICE_TABLE(of, tc358762_of_match);

static struct mipi_dsi_driver tc358762_driver = {
	.probe = tc358762_probe,
	.driver = {
		.name = "tc358762",
		.of_match_table = tc358762_of_match,
	},
};
module_mipi_dsi_driver(tc358762_driver);

MODULE_AUTHOR("Marek Vasut <marex@denx.de>");
MODULE_DESCRIPTION("MIPI-DSI based Driver for TC358762 DSI/DPI Bridge");
MODULE_LICENSE("GPL v2");
