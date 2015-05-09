/*
 * linux/arch/arm/mach-omap2/board-omap3gta04.c
 *
 * Copyright (C) 2008 Texas Instruments
 *
 * Modified from mach-omap2/board-omap3beagle.c
 *
 * Initial code: Syed Mohammed Khasim
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/backlight.h>
#include <linux/pwm.h>
#include <linux/pwm_backlight.h>
#include <linux/rfkill-regulator.h>
#include <linux/gpio-reg.h>
#include <linux/gpio-w2sg0004.h>
#include <linux/extcon/extcon-gpio.h>
#include <linux/opp.h>
#include <linux/cpu.h>

#include <linux/usb/phy.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio.h>

#include <linux/regulator/fixed.h>
#include <linux/regulator/machine.h>
#include <linux/i2c/twl.h>
#include <linux/i2c/tsc2007.h>

#include <linux/i2c/bmp085.h>
#include <linux/lis3lv02d.h>
#ifdef CONFIG_LEDS_TCA6507
#include <linux/leds-tca6507.h>
#endif
#if defined(CONFIG_VIDEO_OV9655) || defined(CONFIG_VIDEO_OV9655_MODULE)
#include <media/ov9655.h>
#include <media/omap3isp.h>
#include <media/v4l2-int-device.h>
/* needed for: v4l2_dev_to_isp_device */
#include <../../../drivers/media/platform/omap3isp/isp.h>
#include "devices.h"	/* omap3_init_camera */
#endif
#include <linux/input/tca8418_keypad.h>
#include <linux/mfd/tps6105x.h>

#include <linux/power/twl4030_madc_battery.h>

#include <linux/sysfs.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>
#include <asm/setup.h>

#include "common.h"
#include <video/omapdss.h>
#include <video/omap-panel-data.h>
#include "gpmc.h"
#include <linux/platform_data/mtd-nand-omap2.h>
#include "clock.h"
#include "omap-pm.h"
#include <linux/platform_data/gpio-omap.h>
#include <linux/platform_data/spi-omap2-mcspi.h>
#include <linux/platform_data/omap-pwm.h>
#include <linux/platform_data/omap-twl4030.h>
#include <linux/platform_data/serial-omap.h>
#include "omap_device.h"

#include "soc.h"
#include "mux.h"
#include "hsmmc.h"
#include "pm.h"
#include "board-flash.h"
#include "common-board-devices.h"
#include "control.h"

#if CONFIG_TOUCHSCREEN_TSC2007_GTA04
#define CONFIG_TOUCHSCREEN_TSC2007
#endif

#define GPMC_CS0_BASE  0x60
#define GPMC_CS_SIZE   0x30

#define MAX_12BIT		((1 << 12) - 1)

#define NAND_BLOCK_SIZE		SZ_128K

/* hardware specific GPIO assignments - may depend on board version and variant */

#define	AUX_BUTTON_GPIO		7
#define TWL4030_MSECURE_GPIO	22
#define	TS_PENIRQ_GPIO		160
#define	WO3G_GPIO		(gta04_version >= 4 ? 10 : 176)	/* changed on A4 boards */
#define KEYIRQ_GPIO		(gta04_version >= 4 ? 176 : 10)	/* changed on A4 boards */
#define WWAN_RESET_GPIO (gta04_version >= 4 ? 186 : -1)	/* introduced on A4 boards */
#define	RS232_ENABLE_GPIO		13
#define	IRDA_SHUTDOWN_GPIO		(gta04_version >= 5 ? 175 : RS232_ENABLE_GPIO)	/* separated on A5 boards */
#define BMP085_EOC_IRQ_GPIO		113	/* BMP085 end of conversion GPIO */
#define TV_OUT_GPIO	23
#define ANTENNA_SWITCH_GPIO	144
#define CAMERA_RESET_GPIO	98	/* CAM_FLD */
#define CAMERA_PWDN_GPIO	165	/* CAM_WEN */
#define CAMERA_STROBE_GPIO	126	/* CAM_STROBE */
#define AUX_HEADSET_GPIO	55
#define USB_PHY_RESET_GPIO	174
#define GPS_ON_OFF_GPIO	145
#define GPS_RX_GPIO 147
#define BOARD_VERSION_0_GPIO 171
#define BOARD_VERSION_1_GPIO 172
#define BOARD_VERSION_2_GPIO 173
#define LIS302_IRQ1_GPIO 114
#define LIS302_IRQ2_GPIO -EINVAL  /* Not yet in use */
#define ITG3200_IRQ_GPIO 56
#define HMC5883L_IRQ_GPIO 112

/*
 * Board peripheral code name passed through a "mux="
 * argument on the command line.
 * It mainly describes which Pinmux variant has been
 * used for U-Boot.
 * This can be used to differently configure the platform
 * devices, e.g. switch gpio numbers, drivers etc.
 * mainly this is used to differentiate between GTA04
 * GTA04b2 (Letux 3704), GTA04b3 (Letux 7004) etc. and
 * the BeagleBoard.
 * So this is sort of a "major board version" name
 * while the gta04_revision is a number to denote
 * different wiring of the core GTA04 board (A2, A3, A4,
 * ...)
 *
 * Maybe we can replace this functionality with different
 * Device Tree files that the U-Boot is providing.
 */

static char gta04_bymux[20];

/* see: https://patchwork.kernel.org/patch/120449/
 * OMAP3 gta04 revision
 * Run time detection of gta04 revision is done by reading GPIO.
 */

static u8 gta04_version;	/* counts 2..9 */

static void __init gta04_init_rev(void)
{
	int ret;
	u16 gta04_rev = 0;
	static char revision[8] = {	/* revision table defined by pull-down
					 * R305, R306, R307 */
		9,
		6,
		7,
		3,
		8,
		4,
		5,
		2
	};
//	printk("Running gta04_init_rev()\n");

	omap_mux_init_gpio(BOARD_VERSION_0_GPIO, OMAP_PIN_INPUT_PULLUP);
	omap_mux_init_gpio(BOARD_VERSION_1_GPIO, OMAP_PIN_INPUT_PULLUP);
	omap_mux_init_gpio(BOARD_VERSION_2_GPIO, OMAP_PIN_INPUT_PULLUP);

	udelay(100);

	ret = gpio_request(BOARD_VERSION_0_GPIO, "rev_id_0");
	if (ret < 0)
		goto fail0;

	ret = gpio_request(BOARD_VERSION_1_GPIO, "rev_id_1");
	if (ret < 0)
		goto fail1;

	ret = gpio_request(BOARD_VERSION_2_GPIO, "rev_id_2");
	if (ret < 0)
		goto fail2;

	udelay(100);

	gpio_direction_input(BOARD_VERSION_0_GPIO);
	gpio_direction_input(BOARD_VERSION_1_GPIO);
	gpio_direction_input(BOARD_VERSION_2_GPIO);

	udelay(100);

	gta04_rev = gpio_get_value(BOARD_VERSION_0_GPIO)
				| (gpio_get_value(BOARD_VERSION_1_GPIO) << 1)
				| (gpio_get_value(BOARD_VERSION_2_GPIO) << 2);

//	printk("gta04_rev %u\n", gta04_rev);

	gta04_version = revision[gta04_rev];

	return;

fail2:
	gpio_free(BOARD_VERSION_1_GPIO);
fail1:
	gpio_free(BOARD_VERSION_0_GPIO);
fail0:
	printk(KERN_ERR "Unable to get revision detection GPIO pins\n");
	gta04_version = 0;

	return;
}


static struct mtd_partition gta04_nand_partitions[] = {
	/* All the partition sizes are listed in terms of NAND block size */
	{
		.name		= "X-Loader",
		.offset		= 0,
		.size		= 4 * NAND_BLOCK_SIZE,
		.mask_flags	= MTD_WRITEABLE,	/* force read-only */
	},
	{
		.name		= "U-Boot",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x80000 */
		.size		= 15 * NAND_BLOCK_SIZE,
	},
	{
		.name		= "U-Boot Env",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x260000 */
		.size		= 1 * NAND_BLOCK_SIZE,
	},
	{
		.name		= "Kernel",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x280000 */
		.size		= 32 * NAND_BLOCK_SIZE,
	},
	{
		.name		= "File System",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x680000 */
		.size		= MTDPART_SIZ_FULL,
	},
};

static struct omap_pwm_pdata pwm_pdata = {
	.timer_id = 11,
};

static struct pwm_lookup board_pwm_lookup[] = {
	// GPT11_PWM_EVT - Active High
	PWM_LOOKUP("omap-pwm", 0, "pwm-backlight", NULL),
};

static struct platform_device pwm_device = {
	.name           = "omap-pwm",
	.id		= 0,
	.dev.platform_data = &pwm_pdata,
};

static struct platform_pwm_backlight_data pwm_backlight = {
	.pwm_id		= 0,
	.max_brightness = 100,
	.dft_brightness = 100,
	.pwm_period_ns  = 2000000, /* 500 Hz */
	.lth_brightness = 11, /* Below 11% display appears as off */
};
static struct platform_device backlight_device = {
	.name = "pwm-backlight",
	.dev = {
		.platform_data = &pwm_backlight,
	},
	.id = -1,
};

extern int gta04_jack_probe(struct snd_soc_codec *codec);
extern void gta04_jack_remove(struct snd_soc_codec *codec);
static struct omap_tw4030_pdata audio_pdata = {
//	.voice_connected = true,
	.custom_routing = true,

	.has_hs		= OMAP_TWL4030_LEFT | OMAP_TWL4030_RIGHT,
	.has_hf		= OMAP_TWL4030_LEFT | OMAP_TWL4030_RIGHT,
	.has_ear	= true,

	.has_mainmic	= true,
	.has_submic	= false,
	.has_hsmic	= true,
	.has_linein	= OMAP_TWL4030_LEFT | OMAP_TWL4030_RIGHT,

	.card_name = "gta04",

	.jack_init	= gta04_jack_probe,
	.jack_remove	= gta04_jack_remove,
};
static struct platform_device twl4030_audio_device = {
	.name = "omap-twl4030",
	.dev = {
		.platform_data = &audio_pdata,
	},
	.id = -1,
};

static struct panel_tpo_td028tec1_platform_data gta04_panel_data = {
	.cs_gpio = 19,
	.scl_gpio = 12,
	.din_gpio = 18,
	.dout_gpio = 20,

	.name = "lcd",
	.source = "dpi.0",
	.data_lines = 24,
};

static struct platform_device gta04_lcd_device = {
	.name = "panel-tpo-td028ttec1",
	.id = 0,
	.dev.platform_data = &gta04_panel_data,
};

static struct platform_device *gta04_panel = &gta04_lcd_device;

static struct display_timing gta04b2_panel_timing = { // GTA04b2 - ortustech_com37h3m05dtc/ortustech_com37h3m099dtc - OpenPhoenux 3704
	.pixelclock	= { 0, 22153000, 0 },	/* min, typ, max */

	.hactive = { 0, 480, 0 },
	.hfront_porch = { 0, 8, 0 },
	.hback_porch = { 0, 10, 0 },
	.hsync_len = { 0, 10, 0 },

	.vactive = { 0, 640, 0 },
	.vfront_porch = { 0, 4, 0 },
	.vback_porch = { 0, 4, 0 },
	.vsync_len = { 0, 3, 0 },

	.flags = DISPLAY_FLAGS_HSYNC_LOW | DISPLAY_FLAGS_VSYNC_LOW |
			DISPLAY_FLAGS_DE_HIGH | DISPLAY_FLAGS_PIXDATA_POSEDGE,
};

static struct panel_dpi_platform_data gta04b2_panel_data = {
	.name = "lcd",
	.source = "dpi.0",
	.data_lines = 24,
	.display_timing = &gta04b2_panel_timing,
	// #define GPIO_STBY 158	/* V1 adapter board for BeagleBoard */
	.backlight_gpio = -1,
	.enable_gpio = 20,	/* STBY */
};

static struct platform_device gta04b2_lcd_device = {
	.name = "panel-dpi",
	.id = 0,
	.dev.platform_data = &gta04b2_panel_data,
};

static struct display_timing gta04b3_panel_timing = { // GTA04b3 - sharp_lq070y3dg3b - OpenPhoenux 7004

	.pixelclock	= { 0, 33260000, 0 },

	.hactive = { 0, 800, 0 },
	.hfront_porch = { 0, 64, 0 },
	.hback_porch = { 0, 64, 0 },
	.hsync_len = { 0, 128, 0 },

	.vactive = { 0, 480, 0 },
	.vfront_porch = { 0, 8, 0 },
	.vback_porch = { 0, 35, 0 },
	.vsync_len = { 0, 2, 0 },

	.flags = DISPLAY_FLAGS_HSYNC_LOW | DISPLAY_FLAGS_VSYNC_LOW |
			DISPLAY_FLAGS_DE_HIGH | DISPLAY_FLAGS_PIXDATA_POSEDGE,
};

static struct panel_dpi_platform_data gta04b3_panel_data = {
	.name = "lcd",
	.source = "dpi.0",
	.data_lines = 24,
	.display_timing = &gta04b3_panel_timing,
	.backlight_gpio = -1,
	// switch to GPIO162 on BeagleBoardB3 */
	.enable_gpio = 12,	/* 3.3V LDO controlled through McBSP5-CLKX of GTA04 */
};

static struct platform_device gta04b3_lcd_device = {
	.name = "panel-dpi",
	.id = 0,
	.dev.platform_data = &gta04b3_panel_data,
};

/* for GTA04b7 = Neo900
 static struct panel_acx565akm_platform_data acx_pdata = {
	.name		= "lcd",
	.source		= "sdi.0",
	.reset_gpio	= RX51_LCD_RESET_GPIO,
	.datapairs	= 2,
 };

 static struct platform_device gta04b7_lcd_device = {
	.name = "panel-acx565akm",
	.id = 0,
	.dev.platform_data = &acx_pdata,
}; */

/* DSS */
/* currently not used on GTA04 but BeagleBoard hardware */

static struct connector_dvi_platform_data gta04_dvi_connector_pdata = {
	.name                   = "dvi",
	.source                 = "tfp410.0",
	.i2c_bus_num            = 3,
};

static struct platform_device gta04_dvi_connector_device = {
	.name                   = "connector-dvi",
	.id                     = 0,
	.dev.platform_data      = &gta04_dvi_connector_pdata,
};

static struct encoder_tfp410_platform_data gta04_tfp410_pdata = {
	.name                   = "tfp410.0",
	.source                 = "dpi.0",
	.data_lines             = 24,
	.power_down_gpio        = -1,
};

static struct platform_device gta04_tfp410_device = {
	.name                   = "tfp410",
	.id                     = 0,
	.dev.platform_data      = &gta04_tfp410_pdata,
};

static struct amplifier_opa362_platform_data gta04_opa362_pdata = {
	.name                   = "opa362.0",
	.source                 = "venc.0",
	.enable_gpio            = TV_OUT_GPIO,
	.bypass			= true,
	.acbias			= true,
};

static struct platform_device gta04_opa362_device = {
	.name                   = "amplifier-opa362",
	.id                     = 0,
	.dev.platform_data      = &gta04_opa362_pdata,
};

static struct connector_atv_platform_data gta04_tv_pdata = {
	.name                   = "tv",
	.source                 = "opa362.0",
	.connector_type         = OMAP_DSS_VENC_TYPE_COMPOSITE,
	.invert_polarity        = true,	/* needed if we use external video driver */
};

static struct platform_device gta04_tv_connector_device = {
	.name                   = "connector-analog-tv",
	.id                     = 0,
	.dev.platform_data      = &gta04_tv_pdata,
};

static struct omap_dss_board_info gta04_dss_data = {
	.default_display_name = "lcd",
};

static struct regulator_consumer_supply gta04_vdvi_supplies[] = {
	REGULATOR_SUPPLY("vdds_sdi", "omapdss"),
	REGULATOR_SUPPLY("vdds_dsi", "omapdss_dpi.0"),
	REGULATOR_SUPPLY("vdds_dsi", "omapdss_dsi.0"),
};

#include "sdram-micron-mt46h32m32lf-6.h"

/* "+2" because TWL4030 adds 2 LED drives as gpio outputs */
#define GPIO_WIFI_RESET (OMAP_MAX_GPIO_LINES + TWL4030_GPIO_MAX + 2)
#define GPIO_BT_REG (GPIO_WIFI_RESET + 1)	// regulator-gpio.0
#define GPIO_GPS_CTRL (GPIO_BT_REG + 1)	// regulator-gpio.1


static struct omap2_hsmmc_info mmc[] = {
	{
		.mmc		= 1,
		 // only 4 wires are connected, and they cannot be removed...
		.caps		= (MMC_CAP_4_BIT_DATA
				   |MMC_CAP_NONREMOVABLE
				   |MMC_CAP_POWER_OFF_CARD),
		.gpio_cd	= -EINVAL,	// no card detect
		.gpio_wp	= -EINVAL,	// no write protect
		.gpio_reset	= -EINVAL,
	},
	{ // this is the WiFi SDIO interface
		.mmc		= 2,
		.caps		= (MMC_CAP_4_BIT_DATA // only 4 wires are connected
				   // |MMC_CAP_NONREMOVABLE
				   |MMC_CAP_POWER_OFF_CARD),
		.gpio_cd	= -EINVAL, // virtual card detect
		.gpio_wp	= -EINVAL,	// no write protect
		.gpio_reset	= GPIO_WIFI_RESET,
		.transceiver	= true,	// external transceiver
		.ocr_mask	= MMC_VDD_31_32,	/* 3.15 is what we want */
		.deferred	= true,
	},
	{}	/* Terminator */
};

static struct regulator_consumer_supply gta04_vmmc1_supply[] = {
	REGULATOR_SUPPLY("vmmc", "omap_hsmmc.0"),
};

static struct regulator_consumer_supply gta04_vsim_supply[] = {
	REGULATOR_SUPPLY("vrfkill", "rfkill-regulator.0"),
};

static struct twl4030_gpio_platform_data gta04_gpio_data = {
	.use_leds	= true,
	.pullups	= BIT(1),
	.pulldowns	= BIT(2) | BIT(6) | BIT(7) | BIT(8) | BIT(13)
				| BIT(15) | BIT(16) | BIT(17),
};

static struct twl4030_clock_init_data gta04_clock = {
	.ck32k_lowpwr_enable = 1, /* Reduce power when on backup battery */
};

static struct twl4030_madc_bat_calibration gta04_battery_charging_data[] = {
	{ 4200, 100 },
	{ 4100, 75 },
	{ 4000, 55 },
	{ 3900, 25 },
	{ 3800, 5 },
	{ 3700, 2 },
	{ 3600, 1 },
	{ 3300, 0 },
};

static struct twl4030_madc_bat_calibration gta04_battery_discharging_data[] = {
	{ 4200, 100 },
	{ 4100, 95 },
	{ 4000, 70 },
	{ 3800, 50 },
	{ 3700, 10 },
	{ 3600, 5 },
	{ 3300, 0 },
};

static struct twl4030_madc_bat_platform_data gta04_battery_data = {
	.capacity = 1200000,	/* total capacity in uAh */
	.charging = gta04_battery_charging_data,
	.charging_size = ARRAY_SIZE(gta04_battery_charging_data),
	.discharging = gta04_battery_discharging_data,
	.discharging_size = ARRAY_SIZE(gta04_battery_discharging_data),
};

static struct platform_device twl4030_madc_bat = {
        .name = "twl4030_madc_battery",
        .id = -1,
	.dev            = {
		.platform_data = &gta04_battery_data,
	},
};


/* VMMC1 for MMC1 pins CMD, CLK, DAT0..DAT3 (20 mA, plus card == max 220 mA) */
static struct regulator_init_data gta04_vmmc1 = {
	.constraints = {
		.name			= "VMMC1",
		.min_uV			= 1850000,
		.max_uV			= 3150000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= gta04_vmmc1_supply,
};

/* VAUX4 is enabled if Bluetooth /or/ WLAN needs it */

static struct regulator_consumer_supply gta04_vaux4_supply[] = {
	REGULATOR_SUPPLY("vgpio", "regulator-gpio.0"),
	REGULATOR_SUPPLY("vmmc", "omap_hsmmc.1")
};

static struct twl_regulator_driver_data vaux4_data = {
	.features = TWL4030_ALLOW_UNSUPPORTED,
};

static struct regulator_init_data gta04_vaux4 = {
	.constraints = {
		.name			= "VAUX4",
		.min_uV			= 2800000,
		/* FIXME: this is a HW issue - 3.15V or 3.3V isn't supported
		 * officially - set CONFIG_TWL4030_ALLOW_UNSUPPORTED */
		.max_uV			= 3150000,
		.valid_modes_mask	= (REGULATOR_MODE_NORMAL
					   | REGULATOR_MODE_STANDBY),
		.valid_ops_mask		= (REGULATOR_CHANGE_VOLTAGE
					   | REGULATOR_CHANGE_MODE
					   | REGULATOR_CHANGE_STATUS),
	},
	.num_consumer_supplies	= ARRAY_SIZE(gta04_vaux4_supply),
	.consumer_supplies	= gta04_vaux4_supply,
	.driver_data = &vaux4_data,
};

/* VAUX3 for Camera */

static struct regulator_consumer_supply gta04_vaux3_supply[] = {
	REGULATOR_SUPPLY("vaux3", "2-0030"),
};

static struct regulator_init_data gta04_vaux3 = {
	.constraints = {
		.name			= "VAUX3",
		.min_uV			= 2500000,
		.max_uV			= 2500000,
		.apply_uV		= true,
		.valid_modes_mask	= (REGULATOR_MODE_NORMAL
					   | REGULATOR_MODE_STANDBY),
		.valid_ops_mask		= (REGULATOR_CHANGE_MODE
					   | REGULATOR_CHANGE_STATUS),
	},
	.num_consumer_supplies	= ARRAY_SIZE(gta04_vaux3_supply),
	.consumer_supplies	= gta04_vaux3_supply,
};

/* VAUX2 for Sensors ITG3200 (and LIS302/LSM303) and HMC5883L */

static struct regulator_consumer_supply gta04_vaux2_supply[] = {
	REGULATOR_SUPPLY("vaux2", "2-0068"),
	REGULATOR_SUPPLY("vaux2", "2-001e"),
	REGULATOR_SUPPLY("Vdd", "2-001d"),
	REGULATOR_SUPPLY("Vdd_IO", "2-001d"),
};

static struct regulator_init_data gta04_vaux2 = {
	.constraints = {
		.name			= "VAUX2",
		.min_uV			= 2800000,
		.max_uV			= 2800000,
		.always_on		= 1,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= 0,
	},
	.num_consumer_supplies	= ARRAY_SIZE(gta04_vaux2_supply),
	.consumer_supplies	= gta04_vaux2_supply,
};

/* VAUX1 unused */

static struct regulator_consumer_supply gta04_vaux1_supply = {
	.supply			= "vaux1",
};

static struct regulator_init_data gta04_vaux1 = {
	.constraints = {
		.name			= "VAUX1",
		.min_uV			= 2500000,
		.max_uV			= 3000000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
		| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
		| REGULATOR_CHANGE_MODE
		| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &gta04_vaux1_supply,
};

/* VSIM used for powering the external GPS Antenna */

static struct regulator_init_data gta04_vsim = {
	.constraints = {
		.name			= "VSIM",
		.min_uV			= 2800000,
		.max_uV			= 2800000,
		.apply_uV		= 1,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(gta04_vsim_supply),
	.consumer_supplies	= gta04_vsim_supply,
};

// CHECKME: the devkit8000 uses VPLL1 for this purpose?

/* VPLL2 for digital video outputs */
static struct regulator_init_data gta04_vpll2 = {
	.constraints = {
		.name			= "VDVI",
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(gta04_vdvi_supplies),
	.consumer_supplies	= gta04_vdvi_supplies,
};

/* rfkill devices for GPS and Bluetooth to control regulators */

static struct rfkill_regulator_platform_data gps_rfkill_data = {
	.name = "GPS",
	.type = RFKILL_TYPE_GPS,
};

static struct platform_device gps_rfkill_device = {
	.name = "rfkill-regulator",
	.id = 0,
	.dev.platform_data = &gps_rfkill_data,
};

static struct gpio_reg_data bt_gpio_data = {
	.gpio = GPIO_BT_REG,
};

static struct platform_device bt_gpio_reg_device = {
	.name = "regulator-gpio",	// enables VAUX4 if we connect to /dev/ttyO0
	.id = 0,
	.dev.platform_data = &bt_gpio_data,
};

static struct gpio_w2sg_data gps_gpio_data = {
	.ctrl_gpio	= GPIO_GPS_CTRL,	// powers the LNA
	.on_off_gpio	= GPS_ON_OFF_GPIO,	// allows to reset the GPS module
	.rx_gpio	= GPS_RX_GPIO,	// used to check for feedback from the GPS module
	.on_state	= OMAP_PIN_INPUT_PULLUP | OMAP_MUX_MODE0,
	.off_state	= OMAP_PIN_INPUT_PULLUP | OMAP_MUX_MODE4,
};

static struct platform_device gps_gpio_device = {
	.name = "w2s-gpio",
	.id = 0,
	.dev.platform_data = &gps_gpio_data,
};

static struct gpio_extcon_platform_data antenna_extcon_data = {
	.name = "gps_antenna",
	.gpio = ANTENNA_SWITCH_GPIO,
	.debounce = 10,
	.irq_flags = IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING,
	.state_on = "external",
	.state_off = "internal",
};

static struct platform_device antenna_extcon_dev = {
	.name = "extcon-gpio",
	.id = -1,
	.dev.platform_data = &antenna_extcon_data,
};

static struct twl4030_usb_data gta04_usb_data = {
	.usb_mode	= T2_USB_MODE_ULPI,
};

static struct twl4030_codec_data omap3_codec;

static struct twl4030_vibra_data gta04_vibra_data = {
	.coexist	=	0,
};

static struct twl4030_audio_data omap3_audio_pdata = {
	.audio_mclk = 26000000,
	.codec = &omap3_codec,
	.vibra = &gta04_vibra_data,
};

static struct twl4030_madc_platform_data gta04_madc_data = {
	.irq_line	= 1,
};

#define TWL_RES_CFG(_res, _devg) { .resource = _res, .devgroup = _devg, \
	.type = TWL4030_RESCONFIG_UNDEF, .type2 = TWL4030_RESCONFIG_UNDEF,}

#define DEV_GRP_ALL (DEV_GRP_P1|DEV_GRP_P2|DEV_GRP_P3)
static struct twl4030_resconfig twl4030_rconfig[] = {
	TWL_RES_CFG(RES_HFCLKOUT, DEV_GRP_P3),
	TWL_RES_CFG(RES_VINTANA1, DEV_GRP_ALL),
	TWL_RES_CFG(RES_VINTANA1, DEV_GRP_P1),
	TWL_RES_CFG(RES_VINTANA2, DEV_GRP_ALL),
	TWL_RES_CFG(RES_VINTANA2, DEV_GRP_P1),
	TWL_RES_CFG(RES_VINTDIG, DEV_GRP_ALL),
	TWL_RES_CFG(RES_VINTDIG, DEV_GRP_P1),
	{ 0, 0},
};

struct twl4030_power_data gta04_power_scripts = {
	.resource_config = twl4030_rconfig,
	.use_poweroff	= 1,
};

/* override TWL defaults */

static int gta04_batt_table[] = {
	/* 0 C*/
	30800, 29500, 28300, 27100,
	26000, 24900, 23900, 22900, 22000, 21100, 20300, 19400, 18700, 17900,
	17200, 16500, 15900, 15300, 14700, 14100, 13600, 13100, 12600, 12100,
	11600, 11200, 10800, 10400, 10000, 9630,  9280,  8950,  8620,  8310,
	8020,  7730,  7460,  7200,  6950,  6710,  6470,  6250,  6040,  5830,
	5640,  5450,  5260,  5090,  4920,  4760,  4600,  4450,  4310,  4170,
	4040,  3910,  3790,  3670,  3550
};

static struct twl4030_bci_platform_data gta04_bci_data = {
	.battery_tmp_tbl        = gta04_batt_table,
	.tblsize                = ARRAY_SIZE(gta04_batt_table),
	.bb_uvolt		= 3200000,
	.bb_uamp		= 150,
};


static struct twl4030_platform_data gta04_twldata = {
	/* platform_data for children goes here */
	.bci		= &gta04_bci_data,
	.gpio		= &gta04_gpio_data,
	.madc		= &gta04_madc_data,
	.power		= &gta04_power_scripts,	/* empty but if not present, pm_power_off is not initialized */
	.usb		= &gta04_usb_data,
	.audio		= &omap3_audio_pdata,
	.clock		= &gta04_clock,

	.vaux1		= &gta04_vaux1,
	.vaux2		= &gta04_vaux2,
	.vaux3		= &gta04_vaux3,
	.vaux4		= &gta04_vaux4,
	.vmmc1		= &gta04_vmmc1,
	.vsim		= &gta04_vsim,
	.vpll2		= &gta04_vpll2,
};


#if defined(CONFIG_SND_SOC_GTM601)

static struct platform_device gta04_gtm601_codec_audio_device = {
	.name	= "gtm601_codec_audio",
	.id	= -1,
	.dev	= {
		.platform_data	= NULL,
	},
};
#endif

#if defined(CONFIG_SND_SOC_SI47XX)

#if 0	// this is what I assume should be in the .platform data - but in a different format
static struct si47xx_setup_data gta04_fm_soc_data = {
	.i2c_bus	 = 2,
	.i2c_address	 = 0x11,
};
#endif

static struct platform_device gta04_si47xx_codec_audio_device = {
	.name	= "si47xx_codec_audio",
	.id	= -1,
	.dev	= {
		.platform_data	= NULL,
//		.platform_data	= gta04_fm_soc_data,
	},
};
#endif

#if defined(CONFIG_SND_SOC_W2CBW003)

static struct platform_device gta04_w2cbw003_codec_audio_device = {
	.name	= "w2cbw003_codec_audio",
	.id	= -1,
	.dev	= {
		.platform_data	= NULL,
	},
};
#endif

#ifdef CONFIG_TOUCHSCREEN_TSC2007

// TODO: see also http://e2e.ti.com/support/arm174_microprocessors/omap_applications_processors/f/42/t/33262.aspx for an example...
// and http://www.embedded-bits.co.uk/?tag=struct-i2c_board_info for a description of how struct i2c_board_info works

/* TouchScreen */

static int ts_get_pendown_state(void)
{
	int val = 0;

	val = gpio_get_value(TS_PENIRQ_GPIO);

//	printk("ts_get_pendown_state() -> %d\n", val);
	return val ? 0 : 1;
}

static int __init tsc2007_init(void)
{
#if 0
	printk("tsc2007_init()\n");
#endif
	omap_mux_init_gpio(TS_PENIRQ_GPIO, OMAP_PIN_INPUT_PULLUP);
	if (gpio_request(TS_PENIRQ_GPIO, "tsc2007_pen_down")) {
		printk(KERN_ERR "Failed to request GPIO %d for "
			   "TSC2007 pen down IRQ\n", TS_PENIRQ_GPIO);
		return  -ENODEV;
	}

	if (gpio_direction_input(TS_PENIRQ_GPIO)) {
		printk(KERN_WARNING "GPIO#%d cannot be configured as "
			   "input\n", TS_PENIRQ_GPIO);
		return -ENXIO;
	}
//	debounce isn't handled properly when power-saving and we lose
//	interrupts, so don't bother for now.
//	gpio_set_debounce(TS_PENIRQ_GPIO, (0x0a+1)*31);
	irq_set_irq_type(gpio_to_irq(TS_PENIRQ_GPIO), IRQ_TYPE_EDGE_FALLING);
	return 0;
}

static void tsc2007_exit(void)
{
	gpio_free(TS_PENIRQ_GPIO);
}

struct tsc2007_platform_data __initdata tsc2007_info = {
	.model			= 2007,
	.x_plate_ohms	= 550,			// GTA04: 250 - 900
	.min_x			= 0x0100,
	.min_y			= 0x00c0,
	.min_z			= 0,
	.max_x			= 0x0f00,
	.max_y			= 0x0f00,
	.max_z			= 0x0fff,
	.flip_x			= false,
	.flip_y			= true,
	.swap_xy		= false,
	.get_pendown_state	= ts_get_pendown_state,
	.init_platform_hw	= tsc2007_init,
	.exit_platform_hw	= tsc2007_exit,
};

#endif


#if defined(CONFIG_BMP085) || defined(CONFIG_BMP085_MODULE)

struct bmp085_platform_data __initdata bmp085_info = {
	.eoc_gpio = BMP085_EOC_IRQ_GPIO,
};

#endif

#ifdef CONFIG_LEDS_TCA6507

void tca6507_setup(unsigned gpio_base, unsigned ngpio)
{
	// do it as early as we have the reset wire fo the WiFi chip
	omap_hsmmc_late_init(mmc);
}

static struct led_info tca6507_leds[] = {
	[0] = { .name = "gta04:red:aux" },
	[1] = { .name = "gta04:green:aux" },
	[3] = { .name = "gta04:red:power", .default_trigger = "default-on" },
	[4] = { .name = "gta04:green:power" },
	[6] = { .name = "gta04:wlan:reset", .flags = TCA6507_MAKE_GPIO },
};

static struct led_info tca6507_leds_b2[] = {
	[0] = { .name = "gta04:left" },
	[1] = { .name = "gta04:right", .default_trigger = "default-on" },
	[6] = { .name = "gta04:wlan:reset", .flags = TCA6507_MAKE_GPIO },
};

static struct led_info tca6507_leds_b3[] = {
	[0] = { .name = "gta04:red:aux" },
	[1] = { .name = "gta04:green:aux" },
	[2] = { .name = "gta04:blue:aux" },
	[3] = { .name = "gta04:red:power", .default_trigger = "default-on" },
	[4] = { .name = "gta04:green:power" },
	[5] = { .name = "gta04:blue:power" },
	[6] = { .name = "gta04:wlan:reset", .flags = TCA6507_MAKE_GPIO },
};

static struct tca6507_platform_data tca6507_info = {
	.leds = {
		.num_leds = 7,
		.leds = tca6507_leds,
	},
	.gpio_base = GPIO_WIFI_RESET,
	.setup = tca6507_setup,
};
#endif

#if defined(CONFIG_TPS6105X) || defined(CONFIG_TPS6105X_MODULE)

static struct tps6105x_platform_data tps6105x_info = {
	.mode = TPS6105X_MODE_TORCH_FLASH,	/* unsupported by driver! */
};

#endif

#if defined(CONFIG_KEYBOARD_TCA8418) || defined(CONFIG_KEYBOARD_TCA8418_MODULE)

static int __init tca8418_init(void)
{
	omap_mux_init_gpio(KEYIRQ_GPIO, OMAP_PIN_INPUT_PULLUP);	// gpio 10 or 176
	
	if (gpio_request(KEYIRQ_GPIO, "keyirq")) {
		printk(KERN_ERR "Failed to request GPIO %d for "
			   "KEYIRQ\n", KEYIRQ_GPIO);
	}
	
	if (gpio_direction_input(KEYIRQ_GPIO)) {
		printk(KERN_WARNING "GPIO#%d cannot be configured as "
			   "input\n", KEYIRQ_GPIO);
	}
	gpio_set_debounce(KEYIRQ_GPIO, (0xa+1)*31);
	irq_set_irq_type(gpio_to_irq(KEYIRQ_GPIO), IRQ_TYPE_EDGE_FALLING);

	return 0;

}

const uint32_t gta04_keymap[] = {
	/* KEY(row, col, val) - see include/linux/input.h */
	KEY(0, 0, KEY_LEFTCTRL),
	KEY(0, 1, KEY_RIGHTCTRL),
	KEY(0, 2, KEY_Y),
	KEY(0, 3, KEY_A),
	KEY(0, 4, KEY_Q),
	KEY(0, 5, KEY_1),
	//	KEY(0, 6, KEY_RESERVED),
	//	KEY(0, 7, KEY_RESERVED),
	KEY(0, 8, KEY_SPACE),
	KEY(0, 9, KEY_OK),
	
	KEY(1, 0, KEY_LEFTALT),
	KEY(1, 1, KEY_FN),
	KEY(1, 2, KEY_SPACE),
	KEY(1, 3, KEY_SPACE),
	KEY(1, 4, KEY_COMMA),
	KEY(1, 5, KEY_DOT),
	KEY(1, 6, KEY_PAGEDOWN),
	KEY(1, 7, KEY_END),
	KEY(1, 8, KEY_LEFT),
	KEY(1, 9, KEY_RIGHT),
	
	KEY(2, 0, KEY_DELETE),
	//	KEY(2, 1, KEY_RESERVED),
	KEY(2, 2, KEY_TAB),
	KEY(2, 3, KEY_BACKSPACE),
	KEY(2, 4, KEY_ENTER),
	KEY(2, 5, KEY_GRAVE),
	KEY(2, 6, KEY_PAGEUP),
	KEY(2, 7, KEY_HOME),
	KEY(2, 8, KEY_UP),
	KEY(2, 9, KEY_DOWN),
	
	KEY(3, 0, KEY_RIGHTALT),
	KEY(3, 1, KEY_CAPSLOCK),
	KEY(3, 2, KEY_ESC),
	//	KEY(3, 3, KEY_RESERVED),
	//	KEY(3, 4, KEY_RESERVED),
	//	KEY(3, 5, KEY_RESERVED),
	KEY(3, 6, KEY_LEFTBRACE),
	KEY(3, 7, KEY_RIGHTBRACE),
	KEY(3, 8, KEY_SEMICOLON),
	KEY(3, 9, KEY_APOSTROPHE),
	
	KEY(4, 0, KEY_LEFTSHIFT),
	KEY(4, 1, KEY_X),
	KEY(4, 2, KEY_C),
	KEY(4, 3, KEY_V),
	KEY(4, 4, KEY_B),
	KEY(4, 5, KEY_N),
	KEY(4, 6, KEY_M),
	KEY(4, 7, KEY_MINUS),
	KEY(4, 8, KEY_EQUAL),
	KEY(4, 9, KEY_KPASTERISK),
	
	KEY(5, 0, KEY_RIGHTSHIFT),
	KEY(5, 1, KEY_S),
	KEY(5, 2, KEY_D),
	KEY(5, 3, KEY_F),
	KEY(5, 4, KEY_G),
	KEY(5, 5, KEY_H),
	KEY(5, 6, KEY_J),
	KEY(5, 7, KEY_K),
	KEY(5, 8, KEY_L),
	KEY(5, 9, KEY_APOSTROPHE),
	
	KEY(6, 0, KEY_LEFTMETA),
	KEY(6, 1, KEY_W),
	KEY(6, 2, KEY_E),
	KEY(6, 3, KEY_R),
	KEY(6, 4, KEY_T),
	KEY(6, 5, KEY_Z),
	KEY(6, 6, KEY_U),
	KEY(6, 7, KEY_I),
	KEY(6, 8, KEY_O),
	KEY(6, 9, KEY_P),
	
	KEY(7, 0, KEY_RIGHTMETA),
	KEY(7, 1, KEY_2),
	KEY(7, 2, KEY_3),
	KEY(7, 3, KEY_4),
	KEY(7, 4, KEY_5),
	KEY(7, 5, KEY_6),
	KEY(7, 6, KEY_7),
	KEY(7, 7, KEY_8),
	KEY(7, 8, KEY_9),
	KEY(7, 9, KEY_0),
};

struct matrix_keymap_data tca8418_keymap = {
	.keymap = gta04_keymap,
	.keymap_size = ARRAY_SIZE(gta04_keymap),
};

struct tca8418_keypad_platform_data tca8418_pdata = {
	.keymap_data = &tca8418_keymap,
	.rows = 8,
	.cols = 10,
	.rep = 1,
	.irq_is_gpio = 1,
};

#endif

#if defined(CONFIG_SENSORS_LIS3_I2C) || defined(CONFIG_SENSORS_LIS3_I2C_MODULE)
static int lis302_setup(void)
{
	int err;
	int irq1 = LIS302_IRQ1_GPIO;
	int irq2 = LIS302_IRQ2_GPIO;

	if (gpio_is_valid(irq1)) {
        /* gpio for interrupt pin 1 */
        err = gpio_request(irq1, "lis3lv02dl_irq1");
        if (err) {
			printk(KERN_ERR "lis3lv02dl: gpio request failed\n");
			goto out;
        }

		gpio_direction_input(irq1);
	}

	if (gpio_is_valid(irq2)) {
		/* gpio for interrupt pin 2 */
		err = gpio_request(irq2, "lis3lv02dl_irq2");
		if (err) {
			gpio_free(irq1);
			printk(KERN_ERR "lis3lv02dl: gpio request failed\n");
			goto out;
		}

		gpio_direction_input(irq2);
	}

out:
	return err;
}

static int lis302_release(void)
{
	if (gpio_is_valid(LIS302_IRQ1_GPIO))
		gpio_free(LIS302_IRQ1_GPIO);
	if (gpio_is_valid(LIS302_IRQ2_GPIO))
		gpio_free(LIS302_IRQ2_GPIO);

	return 0;
}

static struct lis3lv02d_platform_data lis302_info = {
	.click_flags    = LIS3_CLICK_SINGLE_X | LIS3_CLICK_SINGLE_Y |
	LIS3_CLICK_SINGLE_Z,
	/* Limits are 0.5g * value */
	.click_thresh_x = 8,
	.click_thresh_y = 8,
	.click_thresh_z = 10,
	/* Click must be longer than time limit */
	.click_time_limit = 9,
	.click_time_limit = 9,
	/* Kind of debounce filter */
	.click_latency    = 50,

	/* Limits for all axis. millig-value / 18 to get HW values */
	.wakeup_flags = LIS3_WAKEUP_X_HI | LIS3_WAKEUP_Y_HI,
	.wakeup_thresh = 800 / 18,
	.wakeup_flags2 = LIS3_WAKEUP_Z_HI ,
	.wakeup_thresh2 = 900 / 18,

	.hipass_ctrl = LIS3_HIPASS1_DISABLE | LIS3_HIPASS2_DISABLE,

	/* Interrupt line 2 for click detection, line 1 for thresholds */
	.irq_cfg = LIS3_IRQ2_CLICK | LIS3_IRQ1_FF_WU_12,

	.axis_x = LIS3_DEV_X,
	.axis_y = LIS3_INV_DEV_Y,
	.axis_z = LIS3_INV_DEV_Z,
	.setup_resources = lis302_setup,
	.release_resources = lis302_release,
	.st_min_limits = {-32, 3, 3},
	.st_max_limits = {-3, 32, 32},
};
#endif

static struct i2c_board_info __initdata gta04_i2c2_boardinfo[] = {
#ifdef CONFIG_TOUCHSCREEN_TSC2007
{
	I2C_BOARD_INFO("tsc2007", 0x48),
	.platform_data	= &tsc2007_info,
	.irq		= -EINVAL,	// will be modified dynamically by code
},
#endif
#if defined(CONFIG_BMP085) || defined(CONFIG_BMP085_MODULE)
{
	I2C_BOARD_INFO("bmp085", 0x77),
	.platform_data	= &bmp085_info,
},
#endif
#if defined(CONFIG_LEDS_TCA6507)
{
	I2C_BOARD_INFO("tca6507", 0x45),
	.platform_data	= &tca6507_info,
},
#endif
#if defined(CONFIG_INPUT_BMA150) || defined(CONFIG_INPUT_BMA150_MODULE)
{
	I2C_BOARD_INFO("bma150", 0x41),	/* supports our bma180 */
},
#endif
#if defined(CONFIG_SENSORS_LIS3_I2C) || defined(CONFIG_SENSORS_LIS3_I2C_MODULE)
{
	I2C_BOARD_INFO("lis3lv02d", 0x1d),      /* supports our lis302 */
	 .platform_data = &lis302_info,
},
#endif
#if defined(CONFIG_SENSORS_HMC5843) || defined(CONFIG_SENSORS_HMC5843_MODULE)
{
	I2C_BOARD_INFO("hmc5843", 0x1e),	/* supports our hmc5883l */
	.type = "hmc5883l",
},
#endif
#if defined(CONFIG_ITG3200) || defined(CONFIG_ITG3200_MODULE)
	{
	I2C_BOARD_INFO("itg3200", 0x68),
	},
#endif
#if defined(CONFIG_BMA250) || defined(CONFIG_BMA250_MODULE)
	{
	I2C_BOARD_INFO("bma250", 0x18),
	.irq		= 115,
	},	
#endif
#if defined(CONFIG_BMC050) || defined(CONFIG_BMC050_MODULE)
	{
	I2C_BOARD_INFO("bmc050", 0x10),
	.irq		= 111,
	},	
#endif
#if defined(CONFIG_TPS6105X) || defined(CONFIG_TPS6105X_MODULE)
	{
	I2C_BOARD_INFO("tps6105x", 0x33),
	.platform_data = &tps6105x_info,
	},	
#endif
#ifdef CONFIG_EEPROM_AT24
	{
	I2C_BOARD_INFO("24c64", 0x50),	/* supports our mt24lr64 RFID EEPROM */
	},	
#endif
#if defined(CONFIG_KEYBOARD_TCA8418) || defined(CONFIG_KEYBOARD_TCA8418_MODULE)
	{
	I2C_BOARD_INFO("tca8418_keyboard", 0x34),	/* /sys/.../name */
	.platform_data	= &tca8418_pdata,
	.irq		= -EINVAL,	// will be modified dynamically by code
	},	
#endif
};

#if defined(CONFIG_VIDEO_OV9655) || defined(CONFIG_VIDEO_OV9655_MODULE)

struct ov9655_platform_data ov9655_pdata = {
	.reset		= CAMERA_RESET_GPIO,
	.ext_freq	= 24000000,
};

static struct i2c_board_info gta04_camera_i2c_device[] = {
	{  
		I2C_BOARD_INFO("ov9655", 0x30),
		.platform_data = &ov9655_pdata,		
	},
	/* add more records for secondary camera */
};

static struct isp_subdev_i2c_board_info gta04_camera_subdevs[] = {
	{
	.board_info = &gta04_camera_i2c_device[0],
	.i2c_adapter_id = 2,	/* connected to I2C2 */
	},
	/* add more records for secondary camera */
	{ NULL, 0, },
};

static struct isp_v4l2_subdevs_group gta04_camera_subdevs_group[] = {
	{
		.subdevs = gta04_camera_subdevs,
		.interface = ISP_INTERFACE_PARALLEL,
		.bus = {
			/* see http://lxr.free-electrons.com/source/include/media/omap3isp.h?a=arm#L30 */
			.parallel = {
				.data_lane_shift = ISP_LANE_SHIFT_2,	/* CAMEXT[13:0] -> CAM[13:0] */
				.clk_pol = 1,	/* sample on falling edge */
				.hs_pol = 0,	/* active high */
				.vs_pol = 0,	/* axctive high */
				.data_pol = 0,	/* normal */
			}
		},
	},
	/* add more records for secondary camera */
	{ },
};

static struct isp_platform_data gta04_isp_platform_data = {
	.subdevs = gta04_camera_subdevs_group,
	.xclks[0] = { /* XCLK_A */
		.dev_id = "2-0030",
	},
	/* .set_constraints = & void (*set_constraints)(struct isp_device *isp, bool enable) */
};

static void __init gta04_camera_setup(void) {
	struct platform_device *r;
	pr_info("GTA04 camera: setup\n");
	clk_add_alias(NULL, "2-0030", "cam_xclka", NULL);
	r = regulator_register_fixed(0, gta04_vaux3_supply, ARRAY_SIZE(gta04_vaux3_supply));
	if(IS_ERR(r))
		pr_err("%s: cannot register vaux3 regulator\n", __func__);
	else if (omap3_init_camera(&gta04_isp_platform_data) < 0)
		pr_warn("%s: failed registering camera device!\n", __func__);
}

#endif 
	
static struct i2c_board_info __initdata gta04_i2c3_boardinfo[] = {
	/* Bus 3 is currently not used */
	/* add your I2C_BOARD_INFO records here */
};

static int __init gta04_i2c_init(void)
{
	omap3_pmic_get_config(&gta04_twldata,
			TWL_COMMON_PDATA_USB | TWL_COMMON_PDATA_MADC |
			TWL_COMMON_PDATA_AUDIO,
			TWL_COMMON_REGULATOR_VDAC | TWL_COMMON_REGULATOR_VPLL2);

	omap_pmic_init(1, 2600, "twl4030", 7 + OMAP_INTC_START,
		       &gta04_twldata);	
	omap_register_i2c_bus(2, 400,  gta04_i2c2_boardinfo,
				ARRAY_SIZE(gta04_i2c2_boardinfo));
	omap_register_i2c_bus(3, 100, gta04_i2c3_boardinfo,
						  ARRAY_SIZE(gta04_i2c3_boardinfo));
	return 0;
}

#if 0	// FIXME: initialize spare SPIs and McBSPs

static struct spi_board_info gta04fpga_mcspi_board_info[] = {
	// spi 4.0
	{
		.modalias	= "spidev",
		.max_speed_hz	= 48000000, //48 Mbps
		.bus_num	= 4,	// McSPI4
		.chip_select	= 0,
		.mode = SPI_MODE_1,
	},
};

static void __init gta04fpga_init_spi(void)
{
		/* hook the spi ports to the spidev driver */
		spi_register_board_info(gta04fpga_mcspi_board_info,
			ARRAY_SIZE(gta04fpga_mcspi_board_info));
}
#endif

static struct gpio_keys_button gpio_buttons[] = {
	{
		.code			= KEY_PHONE,
		.gpio			= AUX_BUTTON_GPIO,
		.desc			= "AUX",
		.wakeup			= 1,
	},
};
static struct gpio_keys_button gpio_3G_buttons[] = {
	{
		.code			= KEY_UNKNOWN,
		.gpio			= -1	/* WO3G_GPIO will be inserted dynamically */,
		.debounce_interval	= 0,
		.desc			= "3G_WOE",
		.wakeup			= 1,
	},
};

static struct gpio_keys_platform_data gpio_key_info = {
	.buttons	= gpio_buttons,
	.nbuttons	= ARRAY_SIZE(gpio_buttons),
	.name		= "Phone button",
};
static struct gpio_keys_platform_data gpio_3G_info = {
	.buttons	= gpio_3G_buttons,
	.nbuttons	= ARRAY_SIZE(gpio_3G_buttons),
	.name		= "3G Wakeup",
};

static struct platform_device keys_gpio = {
	.name	= "gpio-keys",
	.id	= 0,
	.dev	= {
		.platform_data	= &gpio_key_info,
	},
};
static struct platform_device keys_3G_gpio = {
	.name	= "gpio-keys",
	.id	= 1,
	.dev	= {
		.platform_data	= &gpio_3G_info,
	},
};

static void __init gta04_init_early(void)
{
// 	printk("Doing gta04_init_early()\n");
	omap3_init_early();
}

#if defined(CONFIG_REGULATOR_VIRTUAL_CONSUMER)

static struct platform_device gta04_vaux1_virtual_regulator_device = {
	.name		= "reg-virt-consumer",
	.id			= 1,
	.dev		= {
		.platform_data	= "vaux1",
	},
};

#endif


static struct platform_device twl4030_madc_hwmon = {
	.name	= "twl4030_madc_hwmon",
	.id	= -1,
};

static struct platform_device *gta04_devices[] __initdata = {
	&pwm_device,
	&backlight_device,
	&twl4030_audio_device,
//	&leds_gpio,
	&keys_gpio,
	&keys_3G_gpio,
	&gta04_tfp410_device,
	&gta04_dvi_connector_device,
	&gta04_opa362_device,
	&gta04_tv_connector_device,
	&gps_rfkill_device,
	&bt_gpio_reg_device,
	&gps_gpio_device,
	&antenna_extcon_dev,

#if defined(CONFIG_REGULATOR_VIRTUAL_CONSUMER)
	&gta04_vaux1_virtual_regulator_device,
#endif
#if defined(CONFIG_SND_SOC_GTM601)
	&gta04_gtm601_codec_audio_device,
#endif
#if defined(CONFIG_SND_SOC_SI47XX)
	&gta04_si47xx_codec_audio_device,
#endif
#if defined(CONFIG_SND_SOC_W2CBW003)
	&gta04_w2cbw003_codec_audio_device,
#endif
#ifdef CONFIG_SOC_CAMERA_OV9655
	&gta04_camera_device,
#endif
	&twl4030_madc_hwmon,
    &twl4030_madc_bat,
};

static struct usbhs_phy_data phy_data[] __initdata = {
	{
		.port = 2,
		.reset_gpio = USB_PHY_RESET_GPIO,
		.vcc_gpio = -EINVAL,
	},
};

static struct usbhs_omap_platform_data usbhs_bdata __initdata = {

	/* HSUSB0 - is not a EHCI port; TPS65950 configured by twl4030.c and musb driver */
	.port_mode[1] = OMAP_EHCI_PORT_MODE_PHY,		/* HSUSB2 - USB3322C <-> WWAN */

};

static struct omap_board_mux board_mux[] __initdata = {
	/* Enable GPT11_PWM_EVT instead of GPIO-57 */
	OMAP3_MUX(GPMC_NCS6, OMAP_MUX_MODE3),

	{ .reg_offset = OMAP_MUX_TERMINATOR },
};

#define DEFAULT_RXDMA_POLLRATE		1	/* RX DMA polling rate (us) */
#define DEFAULT_RXDMA_BUFSIZE		4096	/* RX DMA buffer size */
#define DEFAULT_RXDMA_TIMEOUT		(3 * HZ)/* RX DMA timeout (jiffies) */
#define DEFAULT_AUTOSUSPEND_DELAY	-1

static void gta04_serial_init(void)
{
	struct omap_board_data bdata;
	struct omap_uart_port_info info = {
		.dma_enabled	= false,
		.dma_rx_buf_size = DEFAULT_RXDMA_BUFSIZE,
		.dma_rx_poll_rate = DEFAULT_RXDMA_POLLRATE,
		.dma_rx_timeout = DEFAULT_RXDMA_TIMEOUT,
		.autosuspend_timeout = DEFAULT_AUTOSUSPEND_DELAY,
		.DTR_present = 1,
	};

	bdata.flags = 0;
	bdata.pads = NULL;

	/*
	 * BT port.  modem lines are used for on/off management
	 * of the WLAN/BT chip
	 */
	bdata.id = 0;
	info.DTR_gpio = GPIO_BT_REG;
	omap_serial_init_port(&bdata, &info);

	/* GPS port.  modem lines are used for on/off management
	 * and antenna detection.
	 */
	bdata.id = 1;
	info.DTR_gpio = GPIO_GPS_CTRL;
	omap_serial_init_port(&bdata, &info);

	/*
	 * Console/IrDA port.
	 * not additional info
	 */
	bdata.id = 2;
	omap_serial_init_port(&bdata, NULL);
}

static int __init gta04_opp_init(void)
{
	int r = 0;

	/* Initialize the omap3 opp table */
	r = omap3_opp_init();
	if (r < 0 && r != -EEXIST) {
		pr_err("%s: opp default init failed\n", __func__);
		return r;
	}

	/* Custom OPP enabled for all xM versions */
	if (cpu_is_omap3630()) {
		struct device *mpu_dev, *iva_dev;

		u32 status;
		int mpu1GHz = 0;

		status = omap_ctrl_readl(OMAP3_CONTROL_OMAP_STATUS);

		printk("gta04_opp_init %08x\n", status);

		mpu_dev = get_cpu_device(0);
		iva_dev = omap_device_get_by_hwmod_name("iva");

		if (!mpu_dev || !iva_dev) {
			pr_err("%s: Aiee.. no mpu/dsp devices? %p %p\n",
				__func__, mpu_dev, iva_dev);
			return -ENODEV;
		}

		/* check the "Speed Binned" bit for AM/DM37xx
		 in the Control Device Status Register */
		mpu1GHz = (status & (1<<9)) != 0;

		if(mpu1GHz)
			printk("Enable MPU 1GHz and lower opps\n");

		/* Enable MPU 1GHz and lower opps */
		r  = opp_enable(mpu_dev,  800000000);
		if (mpu1GHz)
			r |= opp_enable(mpu_dev, 1000000000);

		/* Enable IVA 800MHz and lower opps */
		r |= opp_enable(iva_dev, 660000000);
		if (mpu1GHz)
			r |= opp_enable(iva_dev, 800000000);

		if (r) {
			pr_err("%s: failed to enable higher opp %d\n",
				__func__, r);
			/*
			 * Cleanup - disable the higher freqs - we dont care
			 * about the results
			 */
			opp_disable(mpu_dev,1000000000);
			opp_disable(mpu_dev, 800000000);
			opp_disable(iva_dev, 800000000);
			opp_disable(iva_dev, 660000000);
		}
		else {
			/* OMAP3530 like we find it in BeagleBoard C4 */
			/* may be 600 or 720 mHz variant */
		}
	}
	return 0;
}
device_initcall(gta04_opp_init);

static void __init gta04_init(void)
{
	int i;
	printk("running gta04_init()\n");
	omap3_mux_init(board_mux, OMAP_PACKAGE_CBB);
	gta04_init_rev();
	
	/* insert the correct IRQ numbers which aren't known at compile time */
	
	for(i=0; i<ARRAY_SIZE(gta04_i2c2_boardinfo); i++) {			
#ifdef CONFIG_TOUCHSCREEN_TSC2007
		if(gta04_i2c2_boardinfo[i].addr == 0x48)
			gta04_i2c2_boardinfo[i].irq = gpio_to_irq(TS_PENIRQ_GPIO);
#endif
#ifdef CONFIG_KEYBOARD_TCA8418
		if(gta04_i2c2_boardinfo[i].addr == 0x34)
			gta04_i2c2_boardinfo[i].irq = gpio_to_irq(KEYIRQ_GPIO);
#endif
#if defined(CONFIG_ITG3200) || defined(CONFIG_ITG3200_MODULE)
		if(gta04_i2c2_boardinfo[i].addr == 0x68)
			gta04_i2c2_boardinfo[i].irq = gpio_to_irq(ITG3200_IRQ_GPIO);
#endif
#if defined(CONFIG_SENSORS_HMC5843) || defined(CONFIG_SENSORS_HMC5843_MODULE)
		if(gta04_i2c2_boardinfo[i].addr == 0x1e)
			gta04_i2c2_boardinfo[i].irq = gpio_to_irq(HMC5883L_IRQ_GPIO);
#endif
	}
	
#ifdef CONFIG_KEYBOARD_TCA8418
	tca8418_init();
#endif
	gta04_i2c_init();
	
	regulator_has_full_constraints/*_listed*/(/*all_reg_data*/);
	gta04_serial_init();
	omap_sdrc_init(mt46h32m32lf6_sdrc_params,
		       mt46h32m32lf6_sdrc_params);

	omap_mux_init_gpio(WO3G_GPIO, OMAP_PIN_INPUT | OMAP_WAKEUP_EN);
	gpio_3G_buttons[0].gpio = WO3G_GPIO;
	
	/* add the panel first so that it becomes display0 and tvout becomes display1 */
	platform_device_register(gta04_panel);

	platform_add_devices(gta04_devices,
			     ARRAY_SIZE(gta04_devices));

	omap_display_init(&gta04_dss_data);

	omap_hsmmc_init(mmc);

// #ifdef CONFIG_OMAP_MUX

	// for a definition of the mux names see arch/arm/mach-omap2/mux34xx.c
	// the syntax of the first paramter to omap_mux_init_signal() is "muxname" or "m0name.muxname" (for ambiguous modes)
	// note: calling omap_mux_init_signal() overwrites the parameter string...

// 	omap_mux_init_signal("mcbsp3_clkx.uart2_tx", OMAP_PIN_OUTPUT);	// gpio 142 / GPS TX
// 	omap_mux_init_signal("mcbsp3_fsx.uart2_rx", OMAP_PIN_INPUT);	// gpio 143 / GPS RX

// #else
// #error we need CONFIG_OMAP_MUX
// #endif

	printk(KERN_INFO "Revision GTA04A%d\n", gta04_version);
	// gpio_export() allows to access through /sys/devices/virtual/gpio/gpio*/value

#if 0	// available on beagleboard only
	//	omap_mux_init_gpio(170, OMAP_PIN_INPUT);
	omap_mux_init_gpio(170, OMAP_PIN_OUTPUT);
	gpio_request(170, "DVI_nPD");
	gpio_direction_output(170, false);	/* leave DVI powered down until it's needed ... */
	gpio_export(170, 0);	// no direction change
#endif

	gpio_request(IRDA_SHUTDOWN_GPIO, "IrDA_select");
	gpio_direction_output(IRDA_SHUTDOWN_GPIO, true);

	if(WWAN_RESET_GPIO >= 0) { /* feature of GTA04A4 and later */
		omap_mux_init_gpio(WWAN_RESET_GPIO, OMAP_PIN_OUTPUT);
		gpio_request(WWAN_RESET_GPIO, "WWAN_RESET");
		gpio_direction_output(WWAN_RESET_GPIO, 0); // keep initial value 
		gpio_export(WWAN_RESET_GPIO, 0);    // no direction change
        }

#ifdef GTA04A2
	// has different pins but relevant chips have never been installed

#else

	// enable AUX out/Headset switch
	gpio_request(AUX_HEADSET_GPIO, "AUX_OUT");
	gpio_direction_output(AUX_HEADSET_GPIO, true);
	gpio_export(AUX_HEADSET_GPIO, 0);	// no direction change
#endif

	pwm_add_table(board_pwm_lookup, ARRAY_SIZE(board_pwm_lookup));

	usb_bind_phy("musb-hdrc.1.auto", 0, "twl4030_usb");
	usb_musb_init(NULL);

	usbhs_init_phys(phy_data, ARRAY_SIZE(phy_data));
	usbhs_init(&usbhs_bdata);

	board_nand_init(gta04_nand_partitions,
			ARRAY_SIZE(gta04_nand_partitions), 0,
			NAND_BUSWIDTH_16, NULL);

	/* Ensure SDRC pins are mux'd for self-refresh */
	omap_mux_init_signal("sdrc_cke0", OMAP_PIN_OUTPUT);
	omap_mux_init_signal("sdrc_cke1", OMAP_PIN_OUTPUT);

	/* TPS65950 mSecure initialization for write access enabling to RTC registers */
	omap_mux_init_gpio(TWL4030_MSECURE_GPIO, OMAP_PIN_OUTPUT);
	gpio_request(TWL4030_MSECURE_GPIO, "mSecure");
	gpio_direction_output(TWL4030_MSECURE_GPIO, true);

	omap_mux_init_gpio(145, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(174, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(TV_OUT_GPIO, OMAP_PIN_OUTPUT); // enable TV out
	omap_mux_init_gpio(AUX_HEADSET_GPIO, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(13, OMAP_PIN_OUTPUT);

	pm_set_vt_switch(0);

	/* handle special wiring of our Si47xx
	 * FSX, CLKX, DX, DR are wired as usual for 4-wire
	 * FSR must be the 6-wire FSR output and have the same signal as FSX
	 * CLKR must be ignored (Interrupt GPIO!) and be internally driven by CLKX
	 */
	{ /* disconnect CLKR from McBSP1 and drive from CLKX
	   * see https://git.kernel.org/cgit/linux/kernel/git/stable/linux-stable.git/commit/sound/soc/omap/omap-mcbsp.c?id=8fef6263ea68f6160637f370a5864d 0a455c620d
	   */
		u32 devconf0;
		devconf0 = omap_ctrl_readl(OMAP2_CONTROL_DEVCONF0);
		devconf0 |= OMAP2_MCBSP1_CLKR_MASK;
		omap_ctrl_writel(devconf0, OMAP2_CONTROL_DEVCONF0);
		printk("CONTROL_DEVCONF0 = %08lx\n", (unsigned long int) devconf0);
	}

	printk("gta04_init done...\n");
}

static void __init gta04_init_late(void)
{
	omap3630_init_late();
#if OFF_MODE_IS_STABLE
	omap_pm_enable_off_mode();
	omap3_pm_off_mode_enable(1);
#endif
//	omap_hsmmc_late_init(mmc);	// if we do it here, we must comment out the other call in the led setup
#if defined(CONFIG_VIDEO_OV9655) || defined(CONFIG_VIDEO_OV9655_MODULE)
	gta04_camera_setup();
#endif
}

/*
 * for the GTA04 custom based devices we may have different
 * displays and other peripherals which also need different
 * pinmux. This has been delegated to U-Boot which passes
 * us the pinmux configuration in the 'mux=' command line
 * argument.
 * We now configure the right display driver and adapt for
 * the leds and touch screen.
 * The defaults of all these configs are for the standard
 * GTA04 so that this kernel works without a mux= argument
 * (older U-Boot) on such devices.
 */

static int __init gta04_init_mux(char *str)
{
	printk(KERN_INFO "gta04_init_bymux: %s\n", str);
	strncpy(gta04_bymux, str, sizeof(gta04_bymux)-1);

	if(strcmp(gta04_bymux, "GTA04") == 0 || strcmp(gta04_bymux, "GTA04A2") == 0 || strcmp(gta04_bymux, "GTA04A3+") == 0) {
		// configure for TPO display (2804) - also the default (could have been called GTA04B1)
		tsc2007_info.x_plate_ohms = 550;			// GTA04: 250 - 900
		tca6507_info.leds.leds = tca6507_leds;
		gta04_panel = &gta04_lcd_device;
	}
	else if(strcmp(gta04_bymux, "GTA04B2") == 0 || strcmp(gta04_bymux, "GTA04B6") == 0) {
		// configure for Ortus display (3704)
		gta04_battery_data.capacity = 3900000;
		tsc2007_info.x_plate_ohms = 600;		// GTA04b2: 200 - 900
		tca6507_info.leds.leds = tca6507_leds_b2;
		gta04_panel = &gta04b2_lcd_device;
		// FIXME: configure RFID driver
	}
	else if(strcmp(gta04_bymux, "GTA04B3") == 0) {
		// configure for 7" Sharp display (7004)
		gta04_battery_data.capacity = 3900000;
		tsc2007_info.x_plate_ohms = 450;			// GTA04b3: 100 - 900
		tsc2007_info.swap_xy = 1,		/* x and y axes are swapped */
		tca6507_info.leds.leds = tca6507_leds_b3;
		gta04_panel = &gta04b3_lcd_device;
	}
	else if(strcmp(gta04_bymux, "GTA04B4") == 0) {
		// configure for 5" Sharp display (5004)
		tsc2007_info.x_plate_ohms = 400;			// GTA04b4: 100 - 850 (very asymmetric between X and Y!)
		// FIXME: configure display and LEDs
	}
	/*
	 else if(strcmp(gta04_bymux, "GTA04B5") == 0) {
	 }
	 else if(strcmp(gta04_bymux, "GTA04B7") == 0) {
	 }
	 else if(strcmp(gta04_bymux, "GTA04B8") == 0) {
	 }
	 else if(strcmp(gta04_bymux, "BeagleBoardB1") == 0) {
		// configure for TPO display (2804)
		tsc2007_info.x_plate_ohms = 550;			// GTA04: 250 - 900
		tca6507_info.leds.leds = tca6507_leds;
		gta04_panel = &gta04_lcd_device;
	 // FIXME: move this to gta04_init and differentiate by gta04_bymux
		gta04_tv_pdata.connector_type = OMAP_DSS_VENC_TYPE_SVIDEO;
		gta04_tv_pdata.invert = false;
		beagle_tfp410_pdata.power_down_gpio = -1;
	 }
	 else if(strcmp(gta04_bymux, "BeagleBoardB2") == 0) {
	 // configure for Ortus display (3704)
		gta04_battery_data.capacity = 3900000;
		tsc2007_info.x_plate_ohms = 600;		// GTA04b2: 200 - 900
		tca6507_info.leds.leds = tca6507_leds_b2;
		gta04_panel = &gta04b2_lcd_device;
		gta04_tv_pdata.connector_type = OMAP_DSS_VENC_TYPE_SVIDEO;
		gta04_tv_pdata.invert = false;
	 // FIXME: configure RFID driver
	 }
	 else if(strcmp(gta04_bymux, "BeagleBoardB4") == 0) {
		// configure for 5" Sharp display (5004)
		tsc2007_info.x_plate_ohms = 400;			// GTA04b4: 100 - 850 (very asymmetric between X and Y!)
		gta04_tv_pdata.connector_type = OMAP_DSS_VENC_TYPE_SVIDEO;
		gta04_tv_pdata.invert = false;
		// FIXME: configure display and LEDs
	 }
	 else if(strcmp(gta04_bymux, "PandaBoardB1") == 0) {
	 }
	 */
	else {
		printk(KERN_EMERG "UNKNOWN U-Boot PINMUX %s!\n", str);
		// maybe we should stop booting here before we risk to damage some unidentified hardware!
	}
	return 1;
}

__setup("mux=", gta04_init_mux);

/* see http://elinux.org/images/4/48/Experiences_With_Device_Tree_Support_Development_For_ARM-Based_SOC's.pdf */

static char const *gta04_dt_compat[] __initdata = {
	"goldelico,gta04",
	// FIXME: do we still need this?
	"goldelico,gta04b2",
	"goldelico,gta04b3",
	NULL
};

MACHINE_START(GTA04, "GTA04")
	/* Maintainer: Nikolaus Schaller - http://www.gta04.org */
	.atag_offset	=	0x100,
	.reserve	=	omap_reserve,
	.map_io		=	omap3_map_io,
	.init_irq	=	omap3_init_irq,
	.handle_irq	=	omap3_intc_handle_irq,
	.init_early	=	gta04_init_early,
	.init_machine	=	gta04_init,
	.init_late	=	gta04_init_late,
	.init_time	=	omap3_secure_sync32k_timer_init,
	.restart	=	omap3xxx_restart,
	.dt_compat	=	gta04_dt_compat,
MACHINE_END
