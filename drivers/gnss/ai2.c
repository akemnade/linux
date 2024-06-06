// SPDX-License-Identifier: GPL-2.0
/*
 * Texas Instruments AI2 (Air independent interface) protocol device driver
 * Used for some TI WLAN/Bluetooth/GNSS combo chips.
 *
 * Copyright (C) 2024 Andreas Kemnade <andreas@kemnade.info>
 */
#include <asm/byteorder.h>
#include <linux/errno.h>
#include <linux/gnss.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/ti_wilink_st.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

/* Channel-9 details for GPS */
#define GPS_CH9_PKT_NUMBER		0x9
#define GPS_CH9_OP_WRITE		0x1
#define GPS_CH9_OP_READ			0x2
#define GPS_CH9_OP_COMPLETED_EVT	0x3

/* arbitarily chosen, should fit everything seen in the past */
#define MAX_AI2_FRAME_SIZE 2048

#define AI2_ESCAPE 0x10 /* if sent as data, it is doubled */
#define AI2_END_MARKER 0x3
#define AI2_ACK 0x2

/* reports */
#define AI2_REPORT_NMEA 0xd3

#define NMEA_HEADER_LEN 4

/* commands */
#define AI2_CMD_RECEIVER_STATE 2

#define RECEIVER_STATE_OFF 1
#define RECEIVER_STATE_IDLE 2
#define RECEIVER_STATE_ON 3

#define AI2_CMD_CONFIG_NMEA 0xe5
#define NMEA_MASK_GGA (1 << 0)
#define NMEA_MASK_GLL (1 << 1)
#define NMEA_MASK_GSA (1 << 2)
#define NMEA_MASK_GSV (1 << 3)
#define NMEA_MASK_RMC (1 << 4)
#define NMEA_MASK_VTG (1 << 5)

#define NMEA_MASK_ALL (NMEA_MASK_GGA | \
		NMEA_MASK_GLL | \
		NMEA_MASK_GSA | \
		NMEA_MASK_GSV | \
		NMEA_MASK_RMC | \
		NMEA_MASK_VTG)


static bool ai2raw;

struct ai2_device {
	struct mutex gdev_mutex;
	bool gdev_open;
	struct gnss_device *gdev;
	struct device *dev;
	struct sk_buff *recv_skb;
	bool recv_esc;
};

static struct sk_buff *ai2_skb_alloc(unsigned int len, gfp_t how)
{
	struct sk_buff *skb;

	skb = bt_skb_alloc(len + sizeof(struct gps_event_hdr), how);
	if (skb)
		skb_reserve(skb, sizeof(struct gps_event_hdr));

	return skb;
}

static int ai2_send_frame(struct ai2_device *ai2dev,
			  struct sk_buff *skb)
{
	int len;
	struct gps_event_hdr *gnssdrv_hdr;
	struct hci_dev *hdev;

	if (skb->len >= U16_MAX)
		return -EINVAL;

	/*
	 * note: fragmentation at this point not handled yet
	 * not needed for simple config commands
	 */
	len = skb->len;
	gnssdrv_hdr = skb_push(skb, sizeof(struct gps_event_hdr));
	gnssdrv_hdr->opcode = GPS_CH9_OP_WRITE;
	gnssdrv_hdr->plen = __cpu_to_le16(len);

	hci_skb_pkt_type(skb) = GPS_CH9_PKT_NUMBER;
	hdev = st_get_hci(ai2dev->dev->parent);
	return hdev->send(hdev, skb);
}

static void ai2_put_escaped(struct sk_buff *skb, u8 d)
{
	skb_put_u8(skb, d);
	if (d == 0x10)
		skb_put_u8(skb, d);
}

static struct sk_buff *ai2_compose_frame(bool request_ack,
					u8 cmd,
					const u8 *data,
					int len)
{
	u16 sum;
	int i;
	/* duplicate the length to have space for worst case escaping */
	struct sk_buff *skb = ai2_skb_alloc(2 + len * 2 + 2 + 2, GFP_KERNEL);

	skb_put_u8(skb, AI2_ESCAPE);
	skb_put_u8(skb, request_ack ? 1 : 0);

	sum = AI2_ESCAPE;
	if (request_ack)
		sum++;

	ai2_put_escaped(skb, cmd);
	sum += cmd;

	ai2_put_escaped(skb, len & 0xff);
	sum += len & 0xff;

	ai2_put_escaped(skb, len >> 8);
	sum += len >> 8;

	for (i = 0; i < len; i++) {
		sum += data[i];
		ai2_put_escaped(skb, data[i]);
	}

	ai2_put_escaped(skb, sum & 0xFF);
	ai2_put_escaped(skb, sum >> 8);
	skb_put_u8(skb, AI2_ESCAPE);
	skb_put_u8(skb, AI2_END_MARKER);

	return skb;
}

static int ai2_set_receiver_state(struct ai2_device *ai2dev,
					      uint8_t state)
{
	struct sk_buff *skb = ai2_compose_frame(true, AI2_CMD_RECEIVER_STATE,
						&state, 1);
	if (!skb)
		return -ENOMEM;

	return ai2_send_frame(ai2dev, skb);
}

static int ai2_config_nmea_reports(struct ai2_device *ai2dev,
				   uint8_t mask)
{
	u8 buf[4] = {0};
	struct sk_buff *skb;

	buf[0] = mask;
	skb = ai2_compose_frame(true, AI2_CMD_CONFIG_NMEA,
				buf, sizeof(buf));
	if (!skb)
		return -ENOMEM;

	return ai2_send_frame(ai2dev, skb);
}

/*
 * Unknown commands, give some version information, must be sent
 * once, not sure what undoes them besides resetting the whole
 * bluetooth part, but no signs of significant things being still
 * turned on without undoing this.
 */
static int gnss_ai2_init(struct ai2_device *ai2dev)
{
	int ret;
	u8 d = 0x01;
	struct sk_buff *skb = ai2_compose_frame(true, 0xf5, &d, 1);

	if (!skb)
		return -ENOMEM;

	ret = ai2_send_frame(ai2dev, skb);
	if (ret)
		return ret;

	msleep(200); /* seen some 60ms response time here, so wait a bit */
	d = 5;
	skb = ai2_compose_frame(true, 0xf1, &d, 1);
	if (!skb)
		return -ENOMEM;

	return ai2_send_frame(ai2dev, skb);
}

static int gnss_ai2_open(struct gnss_device *gdev)
{
	struct ai2_device *ai2dev = gnss_get_drvdata(gdev);
	int ret;

	mutex_lock(&ai2dev->gdev_mutex);
	ai2dev->gdev_open = true;
	mutex_unlock(&ai2dev->gdev_mutex);
	if (ai2raw)
		return 0;

	ret = gnss_ai2_init(ai2dev);
	if (ret)
		goto err;

	/* TODO: find out on what kind of ack we should wait */
	msleep(50);
	ret = ai2_set_receiver_state(ai2dev, RECEIVER_STATE_IDLE);
	if (ret)
		goto err;

	msleep(100);
	ret = ai2_config_nmea_reports(ai2dev, NMEA_MASK_ALL);
	if (ret)
		goto err;

	msleep(50);
	ret = ai2_set_receiver_state(ai2dev, RECEIVER_STATE_ON);
	if (ret)
		goto err;

	msleep(50);

	return 0;
err:
	mutex_lock(&ai2dev->gdev_mutex);
	ai2dev->gdev_open = false;
	if (ai2dev->recv_skb)
		kfree_skb(ai2dev->recv_skb);

	ai2dev->recv_skb = NULL;
	mutex_unlock(&ai2dev->gdev_mutex);
	return ret;
}

static void gnss_ai2_close(struct gnss_device *gdev)
{
	struct ai2_device *ai2dev = gnss_get_drvdata(gdev);

	/* TODO: find out on what kind of ack we should wait */
	if (!ai2raw) {
		msleep(50);
		ai2_set_receiver_state(ai2dev, RECEIVER_STATE_IDLE);
		msleep(50);
		ai2_set_receiver_state(ai2dev, RECEIVER_STATE_OFF);
		msleep(200); /* seen some longer response time here, so wait */
	}

	mutex_lock(&ai2dev->gdev_mutex);
	ai2dev->gdev_open = false;
	if (ai2dev->recv_skb)
		kfree_skb(ai2dev->recv_skb);

	ai2dev->recv_skb = NULL;
	mutex_unlock(&ai2dev->gdev_mutex);
}


static int gnss_ai2_write_raw(struct gnss_device *gdev,
		const unsigned char *buf, size_t count)
{
	struct ai2_device *ai2dev = gnss_get_drvdata(gdev);
	int err = 0;
	struct sk_buff *skb = NULL;

	if (!ai2raw)
		return -EPERM;

	/* allocate packet */
	skb = ai2_skb_alloc(count, GFP_KERNEL);
	if (!skb) {
		BT_ERR("cannot allocate memory for HCILL packet");
		err = -ENOMEM;
		goto out;
	}

	skb_put_data(skb, buf, count);

	err = ai2_send_frame(ai2dev, skb);
	if (err)
		goto out;

	return count;
out:
	return err;
}

static const struct gnss_operations gnss_ai2_ops = {
	.open		= gnss_ai2_open,
	.close		= gnss_ai2_close,
	.write_raw	= gnss_ai2_write_raw,
};

static void process_ai2_packet(struct ai2_device *ai2dev,
			       u8 cmd, u8 *data, u16 len)
{
	if (cmd != AI2_REPORT_NMEA)
		return;

	if (len <= NMEA_HEADER_LEN)
		return;

	len -= NMEA_HEADER_LEN;
	data += NMEA_HEADER_LEN;

	gnss_insert_raw(ai2dev->gdev, data, len);
}

/* do some sanity checks and split frame into packets */
static void process_ai2_frame(struct ai2_device *ai2dev)
{
	u16 sum;
	int i;
	u8 *head;
	u8 *data;

	sum = 0;
	data = ai2dev->recv_skb->data;
	for (i = 0; i < ai2dev->recv_skb->len - 2; i++)
		sum += data[i];

	print_hex_dump_bytes("ai2 frame: ", DUMP_PREFIX_OFFSET, data, ai2dev->recv_skb->len);

	if (get_unaligned_le16(data + i) != sum) {
		dev_dbg(ai2dev->dev,
			"checksum error in reception, dropping frame\n");
		return;
	}

	/* reached if byte 1 in the command packet is set to 1 */
	if (data[1] == AI2_ACK)
		return;

	head = skb_pull(ai2dev->recv_skb, 2); /* drop frame start marker */
	while (head && (ai2dev->recv_skb->len >= 3)) {
		u8 cmd;
		u16 pktlen;

		cmd = head[0];
		pktlen = get_unaligned_le16(head + 1);
		data = skb_pull(ai2dev->recv_skb, 3);
		if (!data)
			break;

		if (pktlen > ai2dev->recv_skb->len)
			break;

		head = skb_pull(ai2dev->recv_skb, pktlen);

		process_ai2_packet(ai2dev, cmd, data, pktlen);
	}
}

static void process_ai2_data(struct ai2_device *ai2dev,
			     u8 *data, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (!ai2dev->recv_skb) {
			ai2dev->recv_esc = false;
			if (data[i] != AI2_ESCAPE) {
				dev_dbg(ai2dev->dev, "dropping data, trying to resync\n");
				continue;
			}
			ai2dev->recv_skb = alloc_skb(MAX_AI2_FRAME_SIZE, GFP_KERNEL);
			if (!ai2dev->recv_skb)
				return;

			dev_dbg(ai2dev->dev, "starting packet\n");

			/* this initial AI2_ESCAPE is part of checksum computation */
			skb_put_u8(ai2dev->recv_skb, data[i]);
			continue;
		}
		if (ai2dev->recv_skb->len == 1) {
			if (data[i] == AI2_END_MARKER) {
				dev_dbg(ai2dev->dev, "unexpected end of frame received\n");
				kfree_skb(ai2dev->recv_skb);
				ai2dev->recv_skb = NULL;
				continue;
			}
			skb_put_u8(ai2dev->recv_skb, data[i]);
		} else {
			/* drop one of two AI2_ESCAPE */
			if ((!ai2dev->recv_esc) &&
			   (data[i] == AI2_ESCAPE)) {
				ai2dev->recv_esc = true;
				continue;
			}

			if (ai2dev->recv_esc &&
			    (data[i] == AI2_END_MARKER)) {
				process_ai2_frame(ai2dev);
				kfree_skb(ai2dev->recv_skb);
				ai2dev->recv_skb = NULL;
				continue;
			}

			ai2dev->recv_esc = false;
			skb_put_u8(ai2dev->recv_skb, data[i]);
		}
	}
}

static void gnss_recv_frame(struct device *dev, struct sk_buff *skb)
{
	struct ai2_device *ai2dev = dev_get_drvdata(dev);
	struct gps_event_hdr *gnss_hdr;
	u8 *data;

	if (!ai2dev->gdev) {
		kfree_skb(skb);
		return;
	}

	gnss_hdr = (struct gps_event_hdr *)skb->data;

	data = skb_pull(skb, sizeof(*gnss_hdr));
	/*
	 * REVISIT: maybe do something with the completed
	 * event
	 */
	if (gnss_hdr->opcode ==	GPS_CH9_OP_READ) {
		mutex_lock(&ai2dev->gdev_mutex);
		if (ai2dev->gdev_open) {
			if (ai2raw)
				gnss_insert_raw(ai2dev->gdev, data, skb->len);
			else
				process_ai2_data(ai2dev, data, skb->len);
		} else {
			dev_dbg(ai2dev->dev,
				"receiving data while chip should be off\n");
		}
		mutex_unlock(&ai2dev->gdev_mutex);
	}
	kfree_skb(skb);
}

static int gnss_ai2_probe(struct platform_device *pdev)
{
	struct gnss_device *gdev;
	struct ai2_device *ai2dev;
	int ret;

	ai2dev = devm_kzalloc(&pdev->dev, sizeof(*ai2dev), GFP_KERNEL);
	if (!ai2dev)
		return -ENOMEM;

	ai2dev->dev = &pdev->dev;
	gdev = gnss_allocate_device(&pdev->dev);
	if (!gdev)
		return -ENOMEM;

	gdev->ops = &gnss_ai2_ops;
	gdev->type = ai2raw ? GNSS_TYPE_AI2 : GNSS_TYPE_NMEA;
	gnss_set_drvdata(gdev, ai2dev);
	platform_set_drvdata(pdev, ai2dev);
	st_set_gnss_recv_func(pdev->dev.parent, gnss_recv_frame);
	mutex_init(&ai2dev->gdev_mutex);

	ret = gnss_register_device(gdev);
	if (ret)
		goto err;

	ai2dev->gdev = gdev;
	return 0;

err:
	st_set_gnss_recv_func(pdev->dev.parent, NULL);

	if (ai2dev->recv_skb)
		kfree_skb(ai2dev->recv_skb);

	gnss_put_device(gdev);
	return ret;
}

static void gnss_ai2_remove(struct platform_device *pdev)
{
	struct ai2_device *ai2dev =  platform_get_drvdata(pdev);

	st_set_gnss_recv_func(pdev->dev.parent, NULL);
	gnss_deregister_device(ai2dev->gdev);
	gnss_put_device(ai2dev->gdev);
	if (ai2dev->recv_skb)
		kfree_skb(ai2dev->recv_skb);
}

static const struct platform_device_id gnss_ai2_id[] = {
	{
		.name = "ti-ai2-gnss"
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(platform, gnss_ai2_id);

static struct platform_driver gnss_ai2_driver = {
	.driver = {
		.name = "gnss-ai2",
	},
	.probe		= gnss_ai2_probe,
	.remove_new	= gnss_ai2_remove,
	.id_table	= gnss_ai2_id,
};
module_platform_driver(gnss_ai2_driver);

module_param(ai2raw, bool, 0600);
MODULE_DESCRIPTION("AI2 GNSS driver");
MODULE_LICENSE("GPL");
