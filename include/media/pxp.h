/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * i.MX PXP API for internal kernel consumers
 *
 * Copyright (c) 2026 Hugo Osvaldo Barrera
 */
#ifndef __MEDIA_PXP_H__
#define __MEDIA_PXP_H__

#include <linux/types.h>

struct device;
struct drm_rect;

/**
 * struct pxp_epdc_config - PXP configuration for EPDC processing
 * @src_addr: DMA address of source buffer (RGB32 format)
 * @dst_addr: DMA address of destination buffer (grayscale)
 * @src_width: source buffer width in pixels
 * @src_height: source buffer height in pixels
 * @src_stride: source buffer stride in bytes
 * @dst_stride: destination buffer stride in bytes
 * @rotation: rotation angle (0, 90, 180, 270 degrees)
 * @clip: rectangle defining the region to process
 * @grayscale_mode: grayscale output mode
 */
struct pxp_epdc_config {
	dma_addr_t src_addr;
	dma_addr_t dst_addr;
	u32 src_width;
	u32 src_height;
	u32 src_stride;
	u32 dst_stride;
	unsigned int rotation;
	const struct drm_rect *clip;
};

/**
 * enum pxp_grayscale_mode - PXP grayscale output format
 * @PXP_GRAYSCALE_Y8: 8-bit grayscale (full range)
 * @PXP_GRAYSCALE_Y4_UPPER: 4-bit grayscale in upper nibble (for EPDC P4N)
 * @PXP_GRAYSCALE_Y5_SHIFTED: 5-bit grayscale shifted (for EPDC P5N)
 */
enum pxp_grayscale_mode {
	PXP_GRAYSCALE_Y8,
	PXP_GRAYSCALE_Y4_UPPER,
	PXP_GRAYSCALE_Y5_SHIFTED,
};

/**
 * pxp_epdc_process() - Process a framebuffer region for EPDC
 * @dev: PXP device pointer (obtained via pxp_get_device())
 * @cfg: configuration describing the processing operation
 * @mode: grayscale output format mode
 *
 * This function configures the PXP hardware to:
 * - Read RGB32 pixels from source buffer
 * - Convert to grayscale using ITU BT.601
 * - Apply rotation transformation
 * - Write to destination buffer in specified grayscale format
 *
 * The operation is synchronous and blocks until completion.
 *
 * Return: 0 on success, negative error code on failure
 */
#if IS_ENABLED(CONFIG_VIDEO_IMX_PXP)

int pxp_epdc_process(struct device *dev, const struct pxp_epdc_config *cfg,
		      enum pxp_grayscale_mode mode);

/**
 * pxp_get_device() - Get the PXP device for EPDC
 *
 * Returns a pointer to the PXP device for use with pxp_epdc_process().
 * The device is obtained from the platform driver.
 *
 * Return: pointer to PXP device, or NULL if not available
 */
struct device *pxp_get_device(void);

#else

static inline int pxp_epdc_process(struct device *dev,
				   const struct pxp_epdc_config *cfg,
				   enum pxp_grayscale_mode mode)
{
	return -ENODEV;
}

static inline struct device *pxp_get_device(void)
{
	return NULL;
}

#endif /* CONFIG_VIDEO_IMX_PXP */

#endif /* __MEDIA_PXP_H__ */
