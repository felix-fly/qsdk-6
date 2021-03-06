/*
 **************************************************************************
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
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

#include "nss_tx_rx_common.h"
#include "nss_wifili_stats.h"

#define NSS_WIFILI_TX_TIMEOUT 1000 /* Millisecond to jiffies*/

/*
 * nss_wifili_pvt
 *	Private data structure
 */
static struct nss_wifili_pvt {
	struct semaphore sem;
	struct completion complete;
	int response;
	void *cb;
	void *app_data;
} wifili_pvt;

/*
 * nss_wifili_handler()
 *	Handle NSS -> HLOS messages for wifi
 */
static void nss_wifili_handler(struct nss_ctx_instance *nss_ctx, struct nss_cmn_msg *ncm, __attribute__((unused))void *app_data)
{
	struct nss_wifili_msg *ntm = (struct nss_wifili_msg *)ncm;
	void *ctx;
	nss_wifili_msg_callback_t cb;

	nss_info("%p: NSS->HLOS message for wifili\n", nss_ctx);

	/*
	 * The interface number shall be wifili soc interface or wifili radio interface
	 */
	BUG_ON((nss_is_dynamic_interface(ncm->interface)) || ncm->interface != NSS_WIFILI_INTERFACE);

	/*
	 * Is this a valid request/response packet?
	 */
	if (ncm->type >= NSS_WIFILI_MAX_MSG) {
		nss_warning("%p: Received invalid message %d for wifili interface", nss_ctx, ncm->type);
		return;
	}

	if (nss_cmn_get_msg_len(ncm) > sizeof(struct nss_wifili_msg)) {
		nss_warning("%p: Length of message is greater than required: %d", nss_ctx, nss_cmn_get_msg_len(ncm));
		return;
	}

	/*
	 * Snoop messages for local driver and handle
	 */
	switch (ntm->cm.type) {
	case NSS_WIFILI_STATS_MSG:
		nss_wifili_stats_sync(nss_ctx, &ntm->msg.wlsoc_stats, ncm->interface);
		break;
	}

	/*
	 * Update the callback and app_data for notify messages, wifili sends all notify messages
	 * to the same callback/app_data.
	 */
	if (ncm->response == NSS_CMM_RESPONSE_NOTIFY) {
		ncm->cb = (nss_ptr_t)nss_ctx->nss_top->wifili_msg_callback;
	}

	/*
	 * Log failures
	 */
	nss_core_log_msg_failures(nss_ctx, ncm);

	/*
	 * Do we have a call back
	 */
	if (!ncm->cb) {
		nss_info("%p: cb null for wifili interface %d", nss_ctx, ncm->interface);
		return;
	}

	/*
	 * Get callback & context
	 */
	cb = (nss_wifili_msg_callback_t)ncm->cb;
	ctx = nss_ctx->subsys_dp_register[ncm->interface].ndev;

	/*
	 * call wifili msg callback
	 */
	if (!ctx) {
		nss_warning("%p: Event received for wifili interface %d before registration", nss_ctx, ncm->interface);
		return;
	}

	cb(ctx, ntm);
}

/*
 * nss_wifili_callback()
 *	Callback to handle the completion of NSS->HLOS messages.
 */
static void nss_wifili_callback(void *app_data, struct nss_wifili_msg *nvm)
{
	nss_wifili_msg_callback_t callback = (nss_wifili_msg_callback_t)wifili_pvt.cb;
	void *data = wifili_pvt.app_data;

	wifili_pvt.response = NSS_TX_SUCCESS;
	wifili_pvt.cb = NULL;
	wifili_pvt.app_data = NULL;

	if (nvm->cm.response != NSS_CMN_RESPONSE_ACK) {
		nss_warning("wifili error response %d\n", nvm->cm.response);
		wifili_pvt.response = nvm->cm.response;
	}

	if (callback) {
		callback(data, nvm);
	}
	complete(&wifili_pvt.complete);
}

/*
 * nss_wifili_tx_msg
 *	Transmit a wifili message to NSS FW
 *
 * NOTE: The caller is expected to handle synchronous wait for message
 * response if needed.
 */
nss_tx_status_t nss_wifili_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_wifili_msg *msg)
{
	struct nss_wifili_msg *nm;
	struct nss_cmn_msg *ncm = &msg->cm;
	struct sk_buff *nbuf;
	int32_t status;

	NSS_VERIFY_CTX_MAGIC(nss_ctx);

	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("%p: wifili message dropped as core not ready", nss_ctx);
		return NSS_TX_FAILURE_NOT_READY;
	}

	if (ncm->type >= NSS_WIFILI_MAX_MSG) {
		nss_warning("%p: wifili message type out of range: %d", nss_ctx, ncm->type);
		return NSS_TX_FAILURE;
	}

	if (nss_cmn_get_msg_len(ncm) > sizeof(struct nss_wifili_msg)) {
		nss_warning("%p: wifili message length is invalid: %d", nss_ctx, nss_cmn_get_msg_len(ncm));
		return NSS_TX_FAILURE;
	}

	/*
	 * The interface number shall be wifili soc interface or wifili radio interface
	 */
	if (ncm->interface != NSS_WIFILI_INTERFACE) {
		nss_warning("%p: tx request for interface that is not a wifili: %d", nss_ctx, ncm->interface);
		return NSS_TX_FAILURE;
	}

	nbuf = dev_alloc_skb(NSS_NBUF_PAYLOAD_SIZE);
	if (unlikely(!nbuf)) {
		spin_lock_bh(&nss_ctx->nss_top->stats_lock);
		NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_NBUF_ALLOC_FAILS]);
		spin_unlock_bh(&nss_ctx->nss_top->stats_lock);
		nss_warning("%p: wifili message failed as command allocation failed", nss_ctx);
		return NSS_TX_FAILURE;
	}

	/*
	 * Copy the message to our skb
	 */
	nm = (struct nss_wifili_msg *)skb_put(nbuf, sizeof(struct nss_wifili_msg));
	memcpy(nm, msg, sizeof(struct nss_wifili_msg));

	status = nss_core_send_buffer(nss_ctx, 0, nbuf, NSS_IF_CMD_QUEUE, H2N_BUFFER_CTRL, 0);
	if (status != NSS_CORE_STATUS_SUCCESS) {
		dev_kfree_skb_any(nbuf);
		nss_warning("%p: Unable to enqueue 'wifili message'", nss_ctx);
		return NSS_TX_FAILURE;
	}

	nss_hal_send_interrupt(nss_ctx, NSS_H2N_INTR_DATA_COMMAND_QUEUE);

	NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_TX_CMD_REQ]);

	return NSS_TX_SUCCESS;
}
EXPORT_SYMBOL(nss_wifili_tx_msg);

/*
 * nss_wifili_tx_msg_sync()
 *	Transmit a wifili message to NSS firmware synchronously.
 */
nss_tx_status_t nss_wifili_tx_msg_sync(struct nss_ctx_instance *nss_ctx, struct nss_wifili_msg *nvm)
{
	nss_tx_status_t status;
	int ret = 0;

	down(&wifili_pvt.sem);
	wifili_pvt.cb = (void *)nvm->cm.cb;
	wifili_pvt.app_data = (void *)nvm->cm.app_data;

	nvm->cm.cb = (nss_ptr_t)nss_wifili_callback;
	nvm->cm.app_data = (nss_ptr_t)NULL;

	status = nss_wifili_tx_msg(nss_ctx, nvm);
	if (status != NSS_TX_SUCCESS) {
		nss_warning("%p: wifili_tx_msg failed\n", nss_ctx);
		up(&wifili_pvt.sem);
		return status;
	}

	ret = wait_for_completion_timeout(&wifili_pvt.complete, msecs_to_jiffies(NSS_WIFILI_TX_TIMEOUT));
	if (!ret) {
		nss_warning("%p: wifili msg tx failed due to timeout\n", nss_ctx);
		wifili_pvt.response = NSS_TX_FAILURE;
	}

	status = wifili_pvt.response;
	up(&wifili_pvt.sem);
	return status;
}
EXPORT_SYMBOL(nss_wifili_tx_msg_sync);

/*
 * nss_wifili_get_context()
 */
struct nss_ctx_instance *nss_wifili_get_context(void)
{
	return (struct nss_ctx_instance *)&nss_top_main.nss[nss_top_main.wifi_handler_id];
}
EXPORT_SYMBOL(nss_wifili_get_context);

/*
 * nss_wifili_msg_init()
 *	Initialize nss_wifili_msg.
 */
void nss_wifili_msg_init(struct nss_wifili_msg *ncm, uint16_t if_num, uint32_t type, uint32_t len, void *cb, void *app_data)
{
	nss_cmn_msg_init(&ncm->cm, if_num, type, len, cb, app_data);
}
EXPORT_SYMBOL(nss_wifili_msg_init);

/*
 ****************************************
 * Register/Unregister/Miscellaneous APIs
 ****************************************
 */

/*
 * nss_register_wifili_if()
 *	Register wifili with nss driver
 */
struct nss_ctx_instance *nss_register_wifili_if(uint32_t if_num, nss_wifili_callback_t wifili_callback,
			nss_wifili_callback_t wifili_ext_callback,
			nss_wifili_msg_callback_t event_callback, struct net_device *netdev, uint32_t features)
{
	struct nss_ctx_instance *nss_ctx = (struct nss_ctx_instance *)&nss_top_main.nss[nss_top_main.wifi_handler_id];

	/*
	 * The interface number shall be wifili soc interface
	 */
	nss_assert(if_num == NSS_WIFILI_INTERFACE);

	nss_info("nss_register_wifili_if if_num %d wifictx %p", if_num, netdev);

	nss_core_register_subsys_dp(nss_ctx, if_num, wifili_callback, wifili_ext_callback, NULL, netdev, features);

	nss_top_main.wifili_msg_callback = event_callback;

	return (struct nss_ctx_instance *)&nss_top_main.nss[nss_top_main.wifi_handler_id];
}
EXPORT_SYMBOL(nss_register_wifili_if);

/*
 * nss_unregister_wifili_if()
 *	Unregister wifili with nss driver
 */
void nss_unregister_wifili_if(uint32_t if_num)
{
	struct nss_ctx_instance *nss_ctx = (struct nss_ctx_instance *)&nss_top_main.nss[nss_top_main.wifi_handler_id];

	/*
	 * The interface number shall be wifili soc interface
	 */
	nss_assert(if_num == NSS_WIFILI_INTERFACE);

	nss_core_unregister_subsys_dp(nss_ctx, if_num);
}
EXPORT_SYMBOL(nss_unregister_wifili_if);

/*
 * nss_register_wifili_radio_if()
 *	Register wifili radio with nss driver
 */
struct nss_ctx_instance *nss_register_wifili_radio_if(uint32_t if_num, nss_wifili_callback_t wifili_callback,
			nss_wifili_callback_t wifili_ext_callback,
			nss_wifili_msg_callback_t event_callback, struct net_device *netdev, uint32_t features)
{
	struct nss_ctx_instance *nss_ctx = (struct nss_ctx_instance *)&nss_top_main.nss[nss_top_main.wifi_handler_id];

	/*
	 * The interface number shall be wifili radio dynamic interface
	 */
	nss_assert(nss_is_dynamic_interface(if_num));
	nss_info("nss_register_wifili_if if_num %d wifictx %p", if_num, netdev);

	nss_core_register_subsys_dp(nss_ctx, if_num, wifili_callback, wifili_ext_callback, NULL, netdev, features);

	return (struct nss_ctx_instance *)&nss_top_main.nss[nss_top_main.wifi_handler_id];
}
EXPORT_SYMBOL(nss_register_wifili_radio_if);

/*
 * nss_unregister_wifili_radio_if()
 *	Unregister wifili radio with nss driver
 */
void nss_unregister_wifili_radio_if(uint32_t if_num)
{
	struct nss_ctx_instance *nss_ctx = (struct nss_ctx_instance *)&nss_top_main.nss[nss_top_main.wifi_handler_id];

	/*
	 * The interface number shall be wifili radio dynamic interface
	 */
	nss_assert(nss_is_dynamic_interface(if_num));

	nss_core_unregister_subsys_dp(nss_ctx, if_num);
}
EXPORT_SYMBOL(nss_unregister_wifili_radio_if);

/*
 * nss_wifili_register_handler()
 *	Register handle for notfication messages received on wifi interface
 */
void nss_wifili_register_handler(void)
{
	struct nss_ctx_instance *nss_ctx = (struct nss_ctx_instance *)&nss_top_main.nss[nss_top_main.wifi_handler_id];

	nss_info("nss_wifili_register_handler");
	nss_core_register_handler(nss_ctx, NSS_WIFILI_INTERFACE, nss_wifili_handler, NULL);

	nss_wifili_stats_dentry_create();

	sema_init(&wifili_pvt.sem, 1);
	init_completion(&wifili_pvt.complete);
}
