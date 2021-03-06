/*
 **************************************************************************
 * Copyright (c) 2015-2017, The Linux Foundation. All rights reserved.
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

#include <net/sock.h>
#include "nss_tx_rx_common.h"
#include "nss_pptp_stats.h"

#define NSS_PPTP_TX_TIMEOUT 3000 /* 3 Seconds */

/*
 * Data structures to store pptp nss debug stats
 */
static DEFINE_SPINLOCK(nss_pptp_session_debug_stats_lock);
static struct nss_pptp_stats_session_debug nss_pptp_session_debug_stats[NSS_MAX_PPTP_DYNAMIC_INTERFACES];

/*
 * Private data structure
 */
static struct nss_pptp_pvt {
	struct semaphore sem;
	struct completion complete;
	int response;
	void *cb;
	void *app_data;
} pptp_pvt;

/*
 * nss_pptp_session_debug_stats_sync
 *	Per session debug stats for pptp
 */
void nss_pptp_session_debug_stats_sync(struct nss_ctx_instance *nss_ctx, struct nss_pptp_sync_session_stats_msg *stats_msg, uint16_t if_num)
{
	int i, j;
	spin_lock_bh(&nss_pptp_session_debug_stats_lock);
	for (i = 0; i < NSS_MAX_PPTP_DYNAMIC_INTERFACES; i++) {
		if (nss_pptp_session_debug_stats[i].if_num == if_num) {
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_ENCAP_RX_PACKETS] += stats_msg->encap_stats.rx_packets;
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_ENCAP_RX_BYTES] += stats_msg->encap_stats.rx_bytes;
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_ENCAP_TX_PACKETS] += stats_msg->encap_stats.tx_packets;
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_ENCAP_TX_BYTES] += stats_msg->encap_stats.tx_bytes;
			for (j = 0; j < NSS_MAX_NUM_PRI; j++) {
				nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_ENCAP_RX_QUEUE_0_DROP + j] += stats_msg->encap_stats.rx_dropped[j];
			}
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_DECAP_RX_PACKETS] += stats_msg->decap_stats.rx_packets;
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_DECAP_RX_BYTES] += stats_msg->decap_stats.rx_bytes;
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_DECAP_TX_PACKETS] += stats_msg->decap_stats.tx_packets;
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_DECAP_TX_BYTES] += stats_msg->decap_stats.tx_bytes;
			for (j = 0; j < NSS_MAX_NUM_PRI; j++) {
				nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_DECAP_RX_QUEUE_0_DROP + j] += stats_msg->decap_stats.rx_dropped[j];
			}
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_SESSION_ENCAP_HEADROOM_ERR] += stats_msg->exception_events[PPTP_EXCEPTION_EVENT_ENCAP_HEADROOM_ERR];
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_SESSION_ENCAP_SMALL_SIZE] += stats_msg->exception_events[PPTP_EXCEPTION_EVENT_ENCAP_SMALL_SIZE];
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_SESSION_ENCAP_PNODE_ENQUEUE_FAIL] += stats_msg->exception_events[PPTP_EXCEPTION_EVENT_ENCAP_PNODE_ENQUEUE_FAIL];
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_SESSION_DECAP_NO_SEQ_NOR_ACK] += stats_msg->exception_events[PPTP_EXCEPTION_EVENT_DECAP_NO_SEQ_NOR_ACK];
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_SESSION_DECAP_INVAL_GRE_FLAGS] += stats_msg->exception_events[PPTP_EXCEPTION_EVENT_DECAP_INVAL_GRE_FLAGS];
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_SESSION_DECAP_INVAL_GRE_PROTO] += stats_msg->exception_events[PPTP_EXCEPTION_EVENT_DECAP_INVAL_GRE_PROTO];
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_SESSION_DECAP_WRONG_SEQ] += stats_msg->exception_events[PPTP_EXCEPTION_EVENT_DECAP_WRONG_SEQ];
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_SESSION_DECAP_INVAL_PPP_HDR] += stats_msg->exception_events[PPTP_EXCEPTION_EVENT_DECAP_INVAL_PPP_HDR];
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_SESSION_DECAP_PPP_LCP] += stats_msg->exception_events[PPTP_EXCEPTION_EVENT_DECAP_PPP_LCP];
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_SESSION_DECAP_UNSUPPORTED_PPP_PROTO] += stats_msg->exception_events[PPTP_EXCEPTION_EVENT_DECAP_UNSUPPORTED_PPP_PROTO];
			nss_pptp_session_debug_stats[i].stats[NSS_PPTP_STATS_SESSION_DECAP_PNODE_ENQUEUE_FAIL] += stats_msg->exception_events[PPTP_EXCEPTION_EVENT_DECAP_PNODE_ENQUEUE_FAIL];
			break;
		}
	}
	spin_unlock_bh(&nss_pptp_session_debug_stats_lock);
}

/*
 * nss_pptp_global_session_stats_get()
 *	Get session pptp statitics.
 */
void nss_pptp_session_debug_stats_get(void *stats_mem)
{
	struct nss_pptp_stats_session_debug *stats = (struct nss_pptp_stats_session_debug *)stats_mem;
	int i;

	if (!stats) {
		nss_warning("No memory to copy pptp session stats");
		return;
	}

	spin_lock_bh(&nss_pptp_session_debug_stats_lock);
	for (i = 0; i < NSS_MAX_PPTP_DYNAMIC_INTERFACES; i++) {
		if (nss_pptp_session_debug_stats[i].valid) {
			memcpy(stats, &nss_pptp_session_debug_stats[i], sizeof(struct nss_pptp_stats_session_debug));
			stats++;
		}
	}
	spin_unlock_bh(&nss_pptp_session_debug_stats_lock);
}

/*
 * nss_pptp_handler()
 *	Handle NSS -> HLOS messages for pptp tunnel
 */
static void nss_pptp_handler(struct nss_ctx_instance *nss_ctx, struct nss_cmn_msg *ncm, __attribute__((unused))void *app_data)
{
	struct nss_pptp_msg *ntm = (struct nss_pptp_msg *)ncm;
	void *ctx;

	nss_pptp_msg_callback_t cb;

	BUG_ON(!(nss_is_dynamic_interface(ncm->interface) || ncm->interface == NSS_PPTP_INTERFACE));

	/*
	 * Is this a valid request/response packet?
	 */
	if (ncm->type >= NSS_PPTP_MSG_MAX) {
		nss_warning("%p: received invalid message %d for PPTP interface", nss_ctx, ncm->type);
		return;
	}

	if (nss_cmn_get_msg_len(ncm) > sizeof(struct nss_pptp_msg)) {
		nss_warning("%p: Length of message is greater than required: %d", nss_ctx, nss_cmn_get_msg_len(ncm));
		return;
	}

	switch (ntm->cm.type) {

	case NSS_PPTP_MSG_SYNC_STATS:
		/*
		 * session debug stats embeded in session stats msg
		 */
		nss_pptp_session_debug_stats_sync(nss_ctx, &ntm->msg.stats, ncm->interface);
		break;
	}

	/*
	 * Update the callback and app_data for NOTIFY messages, pptp sends all notify messages
	 * to the same callback/app_data.
	 */
	if (ncm->response == NSS_CMM_RESPONSE_NOTIFY) {
		ncm->cb = (nss_ptr_t)nss_ctx->nss_top->pptp_msg_callback;
		ncm->app_data =  (nss_ptr_t)nss_ctx->subsys_dp_register[ncm->interface].app_data;
	}

	/*
	 * Log failures
	 */
	nss_core_log_msg_failures(nss_ctx, ncm);

	/*
	 * Do we have a call back
	 */
	if (!ncm->cb) {
		return;
	}

	/*
	 * callback
	 */
	cb = (nss_pptp_msg_callback_t)ncm->cb;
	ctx = (void *)ncm->app_data;

	/*
	 * call pptp tunnel callback
	 */
	if (!cb) {
		nss_warning("%p: Event received for pptp tunnel interface %d before registration", nss_ctx, ncm->interface);
		return;
	}

	cb(ctx, ntm);
}

/*
 * nss_pptp_tx_msg()
 *	Transmit a pptp message to NSS firmware
 */
static nss_tx_status_t nss_pptp_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_pptp_msg *msg)
{
	struct nss_pptp_msg *nm;
	struct nss_cmn_msg *ncm = &msg->cm;
	struct sk_buff *nbuf;
	int32_t status;

	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("%p: pptp msg dropped as core not ready", nss_ctx);
		return NSS_TX_FAILURE_NOT_READY;
	}

	/*
	 * Sanity check the message
	 */
	if (!nss_is_dynamic_interface(ncm->interface)) {
		nss_warning("%p: tx request for non dynamic interface: %d", nss_ctx, ncm->interface);
		return NSS_TX_FAILURE;
	}

	if (ncm->type > NSS_PPTP_MSG_MAX) {
		nss_warning("%p: message type out of range: %d", nss_ctx, ncm->type);
		return NSS_TX_FAILURE;
	}

	if (nss_cmn_get_msg_len(ncm) > sizeof(struct nss_pptp_msg)) {
		nss_warning("%p: message length is invalid: %d", nss_ctx, nss_cmn_get_msg_len(ncm));
		return NSS_TX_FAILURE;
	}

	nbuf = dev_alloc_skb(NSS_NBUF_PAYLOAD_SIZE);
	if (unlikely(!nbuf)) {
		NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_NBUF_ALLOC_FAILS]);
		nss_warning("%p: msg dropped as command allocation failed", nss_ctx);
		return NSS_TX_FAILURE;
	}

	/*
	 * Copy the message to our skb
	 */
	nm = (struct nss_pptp_msg *)skb_put(nbuf, sizeof(struct nss_pptp_msg));
	memcpy(nm, msg, sizeof(struct nss_pptp_msg));

	status = nss_core_send_buffer(nss_ctx, 0, nbuf, NSS_IF_CMD_QUEUE, H2N_BUFFER_CTRL, 0);
	if (status != NSS_CORE_STATUS_SUCCESS) {
		dev_kfree_skb_any(nbuf);
		nss_warning("%p: Unable to enqueue 'pptp message'\n", nss_ctx);
		if (status == NSS_CORE_STATUS_FAILURE_QUEUE) {
			return NSS_TX_FAILURE_QUEUE;
		}
		return NSS_TX_FAILURE;
	}

	nss_hal_send_interrupt(nss_ctx, NSS_H2N_INTR_DATA_COMMAND_QUEUE);

	NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_TX_CMD_REQ]);
	return NSS_TX_SUCCESS;
}

/*
 * nss_pptp_sync_msg_callback()
 *	Callback to handle the completion of NSS->HLOS messages.
 */
static void nss_pptp_sync_msg_callback(void *app_data, struct nss_pptp_msg *nim)
{
	nss_pptp_msg_callback_t callback = (nss_pptp_msg_callback_t)pptp_pvt.cb;
	void *data = pptp_pvt.app_data;

	pptp_pvt.cb = NULL;
	pptp_pvt.app_data = NULL;

	if (nim->cm.response != NSS_CMN_RESPONSE_ACK) {
		nss_warning("pptp Error response %d\n", nim->cm.response);

		pptp_pvt.response = NSS_TX_FAILURE;
		if (callback) {
			callback(data, nim);
		}

		complete(&pptp_pvt.complete);
		return;
	}

	pptp_pvt.response = NSS_TX_SUCCESS;
	if (callback) {
		callback(data, nim);
	}

	complete(&pptp_pvt.complete);
}

/*
 * nss_pptp_tx_msg()
 *	Transmit a pptp message to NSS firmware synchronously.
 */
nss_tx_status_t nss_pptp_tx_msg_sync(struct nss_ctx_instance *nss_ctx, struct nss_pptp_msg *msg)
{

	nss_tx_status_t status;
	int ret = 0;

	down(&pptp_pvt.sem);
	pptp_pvt.cb = (void *)msg->cm.cb;
	pptp_pvt.app_data = (void *)msg->cm.app_data;

	msg->cm.cb = (nss_ptr_t)nss_pptp_sync_msg_callback;
	msg->cm.app_data = (nss_ptr_t)NULL;

	status = nss_pptp_tx_msg(nss_ctx, msg);
	if (status != NSS_TX_SUCCESS) {
		nss_warning("%p: pptp_tx_msg failed\n", nss_ctx);
		up(&pptp_pvt.sem);
		return status;
	}

	ret = wait_for_completion_timeout(&pptp_pvt.complete, msecs_to_jiffies(NSS_PPTP_TX_TIMEOUT));

	if (!ret) {
		nss_warning("%p: PPTP msg tx failed due to timeout\n", nss_ctx);
		pptp_pvt.response = NSS_TX_FAILURE;
	}

	status = pptp_pvt.response;
	up(&pptp_pvt.sem);
	return status;
}

/*
 * nss_pptp_tx_buf()
 *	Send packet to pptp interface owned by NSS
 */
nss_tx_status_t nss_pptp_tx_buf(struct nss_ctx_instance *nss_ctx, uint32_t if_num, struct sk_buff *skb)
{
	int32_t status;

	nss_trace("%p: pptp If Tx packet, id:%d, data=%p", nss_ctx, if_num, skb->data);

	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("%p: 'PPTP' packet dropped as core not ready", nss_ctx);
		return NSS_TX_FAILURE_NOT_READY;
	}

	status = nss_core_send_buffer(nss_ctx, if_num, skb, NSS_IF_DATA_QUEUE_0, H2N_BUFFER_PACKET, H2N_BIT_FLAG_VIRTUAL_BUFFER);
	if (unlikely(status != NSS_CORE_STATUS_SUCCESS)) {
		nss_warning("%p: Unable to enqueue 'PPTP' packet\n", nss_ctx);
		return NSS_TX_FAILURE_QUEUE;
	}

	/*
	 * Kick the NSS awake so it can process our new entry.
	 */
	nss_hal_send_interrupt(nss_ctx, NSS_H2N_INTR_DATA_COMMAND_QUEUE);

	NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_TX_PACKET]);
	return NSS_TX_SUCCESS;
}

/*
 * nss_register_pptp_if()
 */
struct nss_ctx_instance *nss_register_pptp_if(uint32_t if_num,
					      nss_pptp_callback_t pptp_data_callback,
					      nss_pptp_msg_callback_t notification_callback,
					      struct net_device *netdev,
					      uint32_t features,
					      void *app_ctx)
{
	struct nss_ctx_instance *nss_ctx = (struct nss_ctx_instance *)&nss_top_main.nss[nss_top_main.pptp_handler_id];
	int i = 0;

	nss_assert(nss_ctx);
	nss_assert(nss_is_dynamic_interface(if_num));

	nss_core_register_subsys_dp(nss_ctx, if_num, pptp_data_callback, NULL, app_ctx, netdev, features);

	nss_top_main.pptp_msg_callback = notification_callback;

	nss_core_register_handler(nss_ctx, if_num, nss_pptp_handler, NULL);

	spin_lock_bh(&nss_pptp_session_debug_stats_lock);
	for (i = 0; i < NSS_MAX_PPTP_DYNAMIC_INTERFACES; i++) {
		if (!nss_pptp_session_debug_stats[i].valid) {
			nss_pptp_session_debug_stats[i].valid = true;
			nss_pptp_session_debug_stats[i].if_num = if_num;
			nss_pptp_session_debug_stats[i].if_index = netdev->ifindex;
			break;
		}
	}
	spin_unlock_bh(&nss_pptp_session_debug_stats_lock);

	return nss_ctx;
}

/*
 * nss_unregister_pptp_if()
 */
void nss_unregister_pptp_if(uint32_t if_num)
{
	struct nss_ctx_instance *nss_ctx = (struct nss_ctx_instance *)&nss_top_main.nss[nss_top_main.pptp_handler_id];
	int i;

	nss_assert(nss_ctx);
	nss_assert(nss_is_dynamic_interface(if_num));

	spin_lock_bh(&nss_pptp_session_debug_stats_lock);
	for (i = 0; i < NSS_MAX_PPTP_DYNAMIC_INTERFACES; i++) {
		nss_pptp_session_debug_stats[i].valid = false;
		nss_pptp_session_debug_stats[i].if_num = 0;
		nss_pptp_session_debug_stats[i].if_index = 0;
	}
	spin_unlock_bh(&nss_pptp_session_debug_stats_lock);

	nss_core_unregister_subsys_dp(nss_ctx, if_num);

	nss_top_main.pptp_msg_callback = NULL;

	nss_core_unregister_handler(nss_ctx, if_num);
}

/*
 * nss_get_pptp_context()
 */
struct nss_ctx_instance *nss_pptp_get_context()
{
	return (struct nss_ctx_instance *)&nss_top_main.nss[nss_top_main.pptp_handler_id];
}

/*
 * nss_pptp_msg_init()
 *      Initialize nss_pptp msg.
 */
void nss_pptp_msg_init(struct nss_pptp_msg *ncm, uint16_t if_num, uint32_t type,  uint32_t len, void *cb, void *app_data)
{
	nss_cmn_msg_init(&ncm->cm, if_num, type, len, cb, app_data);
}

/* nss_pptp_register_handler()
 *   debugfs stats msg handler received on static pptp interface
 */
void nss_pptp_register_handler(void)
{
	int i;

	nss_info("nss_pptp_register_handler");
	nss_core_register_handler(nss_pptp_get_context(), NSS_PPTP_INTERFACE, nss_pptp_handler, NULL);

	spin_lock_bh(&nss_pptp_session_debug_stats_lock);
	for (i = 0; i < NSS_MAX_PPTP_DYNAMIC_INTERFACES; i++) {
		nss_pptp_session_debug_stats[i].valid = false;
		nss_pptp_session_debug_stats[i].if_num = 0;
		nss_pptp_session_debug_stats[i].if_index = 0;
	}
	spin_unlock_bh(&nss_pptp_session_debug_stats_lock);

	sema_init(&pptp_pvt.sem, 1);
	init_completion(&pptp_pvt.complete);

	nss_pptp_stats_dentry_create();
}

EXPORT_SYMBOL(nss_pptp_get_context);
EXPORT_SYMBOL(nss_pptp_tx_msg_sync);
EXPORT_SYMBOL(nss_pptp_tx_buf);
EXPORT_SYMBOL(nss_unregister_pptp_if);
EXPORT_SYMBOL(nss_pptp_msg_init);
EXPORT_SYMBOL(nss_register_pptp_if);
