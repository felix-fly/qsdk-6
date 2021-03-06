/*
 **************************************************************************
 * Copyright (c) 2014-2017, The Linux Foundation. All rights reserved.
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 **************************************************************************
 */

/*
 * nss_phy_if.c
 *	NSS physical interface functions
 */

#include "nss_tx_rx_common.h"
#include "nss_tstamp.h"

#define NSS_PHYS_IF_TX_TIMEOUT 3000 /* 3 Seconds */

/*
 * NSS phys_if modes
 */
#define NSS_PHYS_IF_MODE0	0	/* phys_if mode 0 */
#define NSS_PHYS_IF_MODE1	1	/* phys_if mode 1 */
#define NSS_PHYS_IF_MODE2	2	/* phys_if mode 2 */

/*
 * Private data structure for phys_if interface
 */
static struct nss_phys_if_pvt {
	struct semaphore sem;
	struct completion complete;
	int response;
} phif;

static int nss_phys_if_sem_init_done;

/*
 * nss_phys_if_update_driver_stats()
 *	Snoop the extended message and update driver statistics.
 */
static void nss_phys_if_update_driver_stats(struct nss_ctx_instance *nss_ctx, uint32_t id, struct nss_phys_if_stats *stats)
{
	struct nss_top_instance *nss_top = nss_ctx->nss_top;
	uint64_t *top_stats = &(nss_top->stats_gmac[id][0]);

	spin_lock_bh(&nss_top->stats_lock);
	top_stats[NSS_STATS_GMAC_TOTAL_TICKS] += stats->estats.gmac_total_ticks;
	if (unlikely(top_stats[NSS_STATS_GMAC_WORST_CASE_TICKS] < stats->estats.gmac_worst_case_ticks)) {
		top_stats[NSS_STATS_GMAC_WORST_CASE_TICKS] = stats->estats.gmac_worst_case_ticks;
	}
	top_stats[NSS_STATS_GMAC_ITERATIONS] += stats->estats.gmac_iterations;
	spin_unlock_bh(&nss_top->stats_lock);
}

/*
 * nss_phys_if_msg_handler()
 *	Handle NSS -> HLOS messages for physical interface/gmacs
 */
static void nss_phys_if_msg_handler(struct nss_ctx_instance *nss_ctx, struct nss_cmn_msg *ncm,
		__attribute__((unused))void *app_data)
{
	struct nss_phys_if_msg *nim = (struct nss_phys_if_msg *)ncm;
	nss_phys_if_msg_callback_t cb;

	/*
	 * Sanity check the message type
	 */
	if (ncm->type > NSS_PHYS_IF_MAX_MSG_TYPES) {
		nss_warning("%p: message type out of range: %d", nss_ctx, ncm->type);
		return;
	}

	if (!NSS_IS_IF_TYPE(PHYSICAL, ncm->interface)) {
		nss_warning("%p: response for another interface: %d", nss_ctx, ncm->interface);
		return;
	}

	if (nss_cmn_get_msg_len(ncm) > sizeof(struct nss_phys_if_msg)) {
		nss_warning("%p: message length too big: %d", nss_ctx, nss_cmn_get_msg_len(ncm));
		return;
	}

	/*
	 * Messages value that are within the base class are handled by the base class.
	 */
	if (ncm->type < NSS_IF_MAX_MSG_TYPES) {
		return nss_if_msg_handler(nss_ctx, ncm, app_data);
	}

	/*
	 * Log failures
	 */
	nss_core_log_msg_failures(nss_ctx, ncm);

	/*
	 * Snoop messages for local driver and handle deprecated interfaces.
	 */
	switch (nim->cm.type) {
	case NSS_PHYS_IF_EXTENDED_STATS_SYNC:
		/*
		 * To create the old API gmac statistics, we use the new extended GMAC stats.
		 */
		nss_phys_if_update_driver_stats(nss_ctx, ncm->interface, &nim->msg.stats);
		nss_top_main.data_plane_ops->data_plane_stats_sync(&nim->msg.stats, ncm->interface);
		break;
	}

	/*
	 * Update the callback and app_data for NOTIFY messages, IPv4 sends all notify messages
	 * to the same callback/app_data.
	 */
	if (ncm->response == NSS_CMM_RESPONSE_NOTIFY) {
		ncm->cb = (nss_ptr_t)nss_ctx->nss_top->phys_if_msg_callback[ncm->interface];
		ncm->app_data = (nss_ptr_t)nss_ctx->subsys_dp_register[ncm->interface].ndev;
	}

	/*
	 * Do we have a callback?
	 */
	if (!ncm->cb) {
		return;
	}

	/*
	 * Callback
	 */
	cb = (nss_phys_if_msg_callback_t)ncm->cb;
	cb((void *)ncm->app_data, nim);
}

/*
 * nss_phys_if_callback
 *	Callback to handle the completion of NSS ->HLOS messages.
 */
static void nss_phys_if_callback(void *app_data, struct nss_phys_if_msg *nim)
{
	if(nim->cm.response != NSS_CMN_RESPONSE_ACK) {
		nss_warning("phys_if Error response %d\n", nim->cm.response);
		phif.response = NSS_TX_FAILURE;
		complete(&phif.complete);
		return;
	}

	phif.response = NSS_TX_SUCCESS;
	complete(&phif.complete);
}

/*
 * nss_phys_if_buf()
 *	Send packet to physical interface owned by NSS
 */
nss_tx_status_t nss_phys_if_buf(struct nss_ctx_instance *nss_ctx, struct sk_buff *os_buf, uint32_t if_num)
{
	int32_t status;
	uint16_t flags = 0;

	nss_trace("%p: Phys If Tx packet, id:%d, data=%p", nss_ctx, if_num, os_buf->data);

	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("%p: 'Phys If Tx' packet dropped as core not ready", nss_ctx);
		return NSS_TX_FAILURE_NOT_READY;
	}

	/*
	 * If we need the packet to be timestamped by GMAC Hardware at Tx
	 * send the packet to tstamp NSS module
	 */
	if (unlikely(skb_shinfo(os_buf)->tx_flags & SKBTX_HW_TSTAMP)) {
		return nss_tstamp_tx_buf(nss_ctx, os_buf, if_num);
	}

	status = nss_core_send_buffer(nss_ctx, if_num, os_buf, NSS_IF_DATA_QUEUE_0, H2N_BUFFER_PACKET, flags);
	if (unlikely(status != NSS_CORE_STATUS_SUCCESS)) {
		nss_warning("%p: Unable to enqueue 'Phys If Tx' packet\n", nss_ctx);
		if (status == NSS_CORE_STATUS_FAILURE_QUEUE) {
			return NSS_TX_FAILURE_QUEUE;
		}

		return NSS_TX_FAILURE;
	}

	/*
	 * Kick the NSS awake so it can process our new entry.
	 */
	nss_hal_send_interrupt(nss_ctx, NSS_H2N_INTR_DATA_COMMAND_QUEUE);

	NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_TX_PACKET]);

	return NSS_TX_SUCCESS;
}

/*
 * nss_phys_if_msg()
 */
nss_tx_status_t nss_phys_if_msg(struct nss_ctx_instance *nss_ctx, struct nss_phys_if_msg *nim)
{
	struct nss_cmn_msg *ncm = &nim->cm;
	struct nss_phys_if_msg *nim2;
	struct net_device *dev;
	struct sk_buff *nbuf;
	uint32_t if_num;
	int32_t status;

	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("Interface could not be created as core not ready");
		return NSS_TX_FAILURE;
	}

	/*
	 * Sanity check the message
	 */
	if (!NSS_IS_IF_TYPE(PHYSICAL, ncm->interface)) {
		nss_warning("%p: tx request for another interface: %d", nss_ctx, ncm->interface);
		return NSS_TX_FAILURE;
	}

	if (ncm->type > NSS_PHYS_IF_MAX_MSG_TYPES) {
		nss_warning("%p: message type out of range: %d", nss_ctx, ncm->type);
		return NSS_TX_FAILURE;
	}

	if (nss_cmn_get_msg_len(ncm) > sizeof(struct nss_phys_if_msg)) {
		nss_warning("%p: invalid length: %d", nss_ctx, nss_cmn_get_msg_len(ncm));
		return NSS_TX_FAILURE;
	}

	if_num = ncm->interface;
	dev = nss_ctx->subsys_dp_register[if_num].ndev;
	if (!dev) {
		nss_warning("%p: Unregister physical interface %d: no context", nss_ctx, if_num);
		return NSS_TX_FAILURE_BAD_PARAM;
	}

	nbuf = dev_alloc_skb(NSS_NBUF_PAYLOAD_SIZE);
	if (unlikely(!nbuf)) {
		NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_NBUF_ALLOC_FAILS]);
		nss_warning("%p: physical interface %p: command allocation failed", nss_ctx, dev);
		return NSS_TX_FAILURE;
	}

	nim2 = (struct nss_phys_if_msg *)skb_put(nbuf, sizeof(struct nss_phys_if_msg));
	memcpy(nim2, nim, sizeof(struct nss_phys_if_msg));

	status = nss_core_send_buffer(nss_ctx, 0, nbuf, NSS_IF_CMD_QUEUE, H2N_BUFFER_CTRL, 0);
	if (status != NSS_CORE_STATUS_SUCCESS) {
		dev_kfree_skb_any(nbuf);
		nss_warning("%p: Unable to enqueue 'physical interface' command\n", nss_ctx);
		return NSS_TX_FAILURE;
	}

	nss_hal_send_interrupt(nss_ctx, NSS_H2N_INTR_DATA_COMMAND_QUEUE);

	return NSS_TX_SUCCESS;
}

/*
 * nss_phys_if_tx_msg_sync()
 *	Send a message to physical interface & wait for the response.
 */
nss_tx_status_t nss_phys_if_msg_sync(struct nss_ctx_instance *nss_ctx, struct nss_phys_if_msg *nim)
{
	nss_tx_status_t status;
	int ret = 0;

	down(&phif.sem);

	status = nss_phys_if_msg(nss_ctx, nim);
	if(status != NSS_TX_SUCCESS)
	{
		nss_warning("%p: nss_phys_if_msg failed\n", nss_ctx);
		up(&phif.sem);
		return status;
	}

	ret = wait_for_completion_timeout(&phif.complete, msecs_to_jiffies(NSS_PHYS_IF_TX_TIMEOUT));

	if(!ret)
	{
		nss_warning("%p: phys_if tx failed due to timeout\n", nss_ctx);
		phif.response = NSS_TX_FAILURE;
	}

	status = phif.response;
	up(&phif.sem);

	return status;
}

/*
 **********************************
 Register/Unregister/Miscellaneous APIs
 **********************************
 */

/*
 * nss_phys_if_register()
 */
struct nss_ctx_instance *nss_phys_if_register(uint32_t if_num,
				nss_phys_if_rx_callback_t rx_callback,
				nss_phys_if_msg_callback_t msg_callback,
				struct net_device *netdev,
				uint32_t features)
{
	uint8_t id = nss_top_main.phys_if_handler_id[if_num];
	struct nss_ctx_instance *nss_ctx = &nss_top_main.nss[id];

	nss_assert(nss_ctx);
	nss_assert(if_num <= NSS_MAX_PHYSICAL_INTERFACES);

	nss_core_register_subsys_dp(nss_ctx, if_num, rx_callback, NULL, NULL, netdev, features);

	nss_top_main.phys_if_msg_callback[if_num] = msg_callback;

	nss_ctx->phys_if_mtu[if_num] = ETH_DATA_LEN;
	return nss_ctx;
}

/*
 * nss_phys_if_unregister()
 */
void nss_phys_if_unregister(uint32_t if_num)
{
	uint8_t id = nss_top_main.phys_if_handler_id[if_num];
	struct nss_ctx_instance *nss_ctx = &nss_top_main.nss[id];

	nss_assert(nss_ctx);
	nss_assert(if_num < NSS_MAX_PHYSICAL_INTERFACES);

	nss_core_unregister_subsys_dp(nss_ctx, if_num);

	nss_top_main.phys_if_msg_callback[if_num] = NULL;

	nss_top_main.nss[0].phys_if_mtu[if_num] = 0;
	nss_top_main.nss[1].phys_if_mtu[if_num] = 0;
}

/*
 * nss_phys_if_register_handler()
 */
void nss_phys_if_register_handler(struct nss_ctx_instance *nss_ctx, uint32_t if_num)
{
	uint32_t ret;

	ret = nss_core_register_handler(nss_ctx, if_num, nss_phys_if_msg_handler, NULL);

	if (ret != NSS_CORE_STATUS_SUCCESS) {
		nss_warning("Message handler FAILED to be registered for interface %d", if_num);
		return;
	}

	if(!nss_phys_if_sem_init_done) {
		sema_init(&phif.sem, 1);
		init_completion(&phif.complete);
		nss_phys_if_sem_init_done = 1;
	}
}

/*
 * nss_phys_if_open()
 *	Send open command to physical interface
 */
nss_tx_status_t nss_phys_if_open(struct nss_ctx_instance *nss_ctx, uint32_t tx_desc_ring, uint32_t rx_desc_ring, uint32_t mode, uint32_t if_num, uint32_t bypass_nw_process)
{
	struct nss_phys_if_msg nim;
	struct nss_if_open *nio;

	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	nss_info("%p: Phys If Open, id:%d, TxDesc: %x, RxDesc: %x\n", nss_ctx, if_num, tx_desc_ring, rx_desc_ring);

	nss_cmn_msg_init(&nim.cm, if_num, NSS_PHYS_IF_OPEN,
			sizeof(struct nss_if_open), nss_phys_if_callback, NULL);

	nio = &nim.msg.if_msg.open;
	nio->tx_desc_ring = tx_desc_ring;
	nio->rx_desc_ring = rx_desc_ring;

	if (mode == NSS_PHYS_IF_MODE0) {
		nio->rx_forward_if = NSS_ETH_RX_INTERFACE;
		nio->alignment_mode = NSS_IF_DATA_ALIGN_2BYTE;
	} else if (mode == NSS_PHYS_IF_MODE1) {
		nio->rx_forward_if = NSS_SJACK_INTERFACE;
		nio->alignment_mode = NSS_IF_DATA_ALIGN_4BYTE;
	} else if (mode == NSS_PHYS_IF_MODE2) {
		nio->rx_forward_if = NSS_PORTID_INTERFACE;
		nio->alignment_mode = NSS_IF_DATA_ALIGN_2BYTE;
	} else {
		nss_info("%p: Phys If Open, unknown mode %d\n", nss_ctx, mode);
		return NSS_TX_FAILURE;
	}

	/*
	 * If Network processing in NSS is bypassed
	 * update next hop and alignment accordingly
	 */
	if (bypass_nw_process) {
		nio->rx_forward_if = NSS_N2H_INTERFACE;
		nio->alignment_mode = NSS_IF_DATA_ALIGN_2BYTE;
	}

	return nss_phys_if_msg_sync(nss_ctx, &nim);
}

/*
 * nss_phys_if_close()
 *	Send close command to physical interface
 */
nss_tx_status_t nss_phys_if_close(struct nss_ctx_instance *nss_ctx, uint32_t if_num)
{
	struct nss_phys_if_msg nim;

	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	nss_info("%p: Phys If Close, id:%d \n", nss_ctx, if_num);

	nss_cmn_msg_init(&nim.cm, if_num, NSS_PHYS_IF_CLOSE,
			sizeof(struct nss_if_close), nss_phys_if_callback, NULL);

	return nss_phys_if_msg_sync(nss_ctx, &nim);
}

/*
 * nss_phys_if_link_state()
 *	Send link state to physical interface
 */
nss_tx_status_t nss_phys_if_link_state(struct nss_ctx_instance *nss_ctx, uint32_t link_state, uint32_t if_num)
{
	struct nss_phys_if_msg nim;
	struct nss_if_link_state_notify *nils;

	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	nss_info("%p: Phys If Link State, id:%d, State: %x\n", nss_ctx, if_num, link_state);

	nss_cmn_msg_init(&nim.cm, if_num, NSS_PHYS_IF_LINK_STATE_NOTIFY,
			sizeof(struct nss_if_link_state_notify), nss_phys_if_callback, NULL);

	nils = &nim.msg.if_msg.link_state_notify;
	nils->state = link_state;
	return nss_phys_if_msg_sync(nss_ctx, &nim);
}

/*
 * nss_phys_if_mac_addr()
 *	Send a MAC address to physical interface
 */
nss_tx_status_t nss_phys_if_mac_addr(struct nss_ctx_instance *nss_ctx, uint8_t *addr, uint32_t if_num)
{
	struct nss_phys_if_msg nim;
	struct nss_if_mac_address_set *nmas;

	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	nss_info("%p: Phys If MAC Address, id:%d\n", nss_ctx, if_num);
	nss_assert(addr != 0);

	nss_cmn_msg_init(&nim.cm, if_num, NSS_PHYS_IF_MAC_ADDR_SET,
			sizeof(struct nss_if_mac_address_set), nss_phys_if_callback, NULL);

	nmas = &nim.msg.if_msg.mac_address_set;
	memcpy(nmas->mac_addr, addr, ETH_ALEN);
	return nss_phys_if_msg_sync(nss_ctx, &nim);
}

/*
 * nss_phys_if_change_mtu()
 *	Send a MTU change command
 */
nss_tx_status_t nss_phys_if_change_mtu(struct nss_ctx_instance *nss_ctx, uint32_t mtu, uint32_t if_num)
{
	struct nss_phys_if_msg nim;
	struct nss_if_mtu_change *nimc;
	uint16_t mtu_sz, max_mtu;
	int i;
	nss_tx_status_t status;

	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	nss_info("%p: Phys If Change MTU, id:%d, mtu=%d\n", nss_ctx, if_num, mtu);

	nss_cmn_msg_init(&nim.cm, if_num, NSS_PHYS_IF_MTU_CHANGE,
			sizeof(struct nss_if_mtu_change), nss_phys_if_callback, NULL);

	nimc = &nim.msg.if_msg.mtu_change;
	nimc->min_buf_size = mtu;

	status = nss_phys_if_msg_sync(nss_ctx, &nim);
	if (status != NSS_TX_SUCCESS) {
		return status;
	}

	/*
	 * Update the mtu and max_buf_size accordingly
	 */
	nss_ctx->phys_if_mtu[if_num] = (uint16_t)mtu;

	/*
	 * Loop through MTU values of all Physical
	 * interfaces and get the maximum one of all
	 */
	max_mtu = nss_ctx->phys_if_mtu[0];
	for (i = 1; i < NSS_MAX_PHYSICAL_INTERFACES; i++) {
		if (max_mtu < nss_ctx->phys_if_mtu[i]) {
			max_mtu = nss_ctx->phys_if_mtu[i];
		}
	}

	mtu_sz = nss_top_main.data_plane_ops->data_plane_get_mtu_sz(max_mtu);
	nss_ctx->max_buf_size = ((mtu_sz + ETH_HLEN + SMP_CACHE_BYTES - 1) & ~(SMP_CACHE_BYTES - 1)) + NSS_NBUF_ETH_EXTRA + NSS_NBUF_PAD_EXTRA;

	/*
	 * max_buf_size should not be lesser than NSS_NBUF_PAYLOAD_SIZE
	 */
	if (nss_ctx->max_buf_size < NSS_NBUF_PAYLOAD_SIZE) {
		nss_ctx->max_buf_size = NSS_NBUF_PAYLOAD_SIZE;
	}

	nss_info("Current mtu:%u mtu_sz:%u max_buf_size:%d\n", mtu, mtu_sz, nss_ctx->max_buf_size);

	if (mtu_sz > nss_ctx->nss_top->prev_mtu_sz) {

		/* If crypto is enabled on platform
		 * Send the flush payloads message
		 */
		if (nss_ctx->nss_top->crypto_enabled) {
			if (nss_n2h_flush_payloads(nss_ctx) != NSS_TX_SUCCESS) {
				nss_info("Unable to send flush payloads command to NSS\n");
			}
		}
	}
	nss_ctx->nss_top->prev_mtu_sz = mtu_sz;

	return status;
}

/*
 * nss_phys_if_vsi_assign()
 *	Send a vsi assign to physical interface
 */
nss_tx_status_t nss_phys_if_vsi_assign(struct nss_ctx_instance *nss_ctx, uint32_t vsi, uint32_t if_num)
{
	struct nss_phys_if_msg nim;

	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	nss_info("%p: Phys If VSI Assign, id:%d\n", nss_ctx, if_num);

	memset(&nim, 0, sizeof(struct nss_phys_if_msg));

	nss_cmn_msg_init(&nim.cm, if_num, NSS_PHYS_IF_VSI_ASSIGN,
			sizeof(struct nss_if_vsi_assign), nss_phys_if_callback, NULL);

	nim.msg.if_msg.vsi_assign.vsi = vsi;
	return nss_phys_if_msg_sync(nss_ctx, &nim);
}

/*
 * nss_phys_if_vsi_unassign()
 *	Send a vsi unassign to physical interface
 */
nss_tx_status_t nss_phys_if_vsi_unassign(struct nss_ctx_instance *nss_ctx, uint32_t vsi, uint32_t if_num)
{
	struct nss_phys_if_msg nim;

	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	nss_info("%p: Phys If VSI Unassign, id:%d\n", nss_ctx, if_num);

	memset(&nim, 0, sizeof(struct nss_phys_if_msg));

	nss_cmn_msg_init(&nim.cm, if_num, NSS_PHYS_IF_VSI_UNASSIGN,
			sizeof(struct nss_if_vsi_unassign), nss_phys_if_callback, NULL);

	nim.msg.if_msg.vsi_unassign.vsi = vsi;
	return nss_phys_if_msg_sync(nss_ctx, &nim);
}

/*
 * nss_phys_if_pause_on_off()
 *	Send a pause enabled/disabled message to GMAC
 */
nss_tx_status_t nss_phys_if_pause_on_off(struct nss_ctx_instance *nss_ctx, uint32_t pause_on, uint32_t if_num)
{
	struct nss_phys_if_msg nim;
	struct nss_if_pause_on_off *nipe;

	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	nss_info("%p: phys if pause is set to %d, id:%d\n", nss_ctx, pause_on, if_num);

	nss_cmn_msg_init(&nim.cm, if_num, NSS_PHYS_IF_PAUSE_ON_OFF,
			sizeof(struct nss_if_pause_on_off), nss_phys_if_callback, NULL);

	nipe = &nim.msg.if_msg.pause_on_off;
	nipe->pause_on = pause_on;

	return nss_phys_if_msg_sync(nss_ctx, &nim);
}

/*
 * nss_get_state()
 *	Return the NSS initialization state
 */
nss_state_t nss_get_state(void *ctx)
{
	return nss_cmn_get_state(ctx);
}

EXPORT_SYMBOL(nss_get_state);
