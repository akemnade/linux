/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (C) 2020 Andreas Kemnade */
int mxc_epdc_fb_send_single_update(struct mxcfb_update_data *upd_data,
				   struct mxc_epdc *priv);
void mxc_epdc_draw_mode0(struct mxc_epdc *priv);
int mxc_epdc_init_update(struct mxc_epdc *priv);
void mxc_epdc_flush_updates(struct mxc_epdc *priv);

