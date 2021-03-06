/*
 **************************************************************************
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
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
#include "nss_gre_redir_lag_ds_stats.h"

#define NSS_GRE_REDIR_LAG_DS_TX_TIMEOUT 3000 /* 3 Seconds */

static struct nss_gre_redir_lag_ds_tun_stats tun_stats[NSS_GRE_REDIR_LAG_MAX_NODE];
static DEFINE_SPINLOCK(nss_gre_redir_lag_ds_stats_lock);

/*
 * Private data structure
 */
static struct {
	struct semaphore sem;
	struct completion complete;
	int response;
	nss_gre_redir_lag_ds_msg_callback_t *cb;
	void *app_data;
} nss_gre_redir_lag_ds_pvt;

/*
 * nss_gre_redir_lag_ds_callback()
 *	Callback to handle the completion of HLOS-->NSS messages.
 */
static void nss_gre_redir_lag_ds_callback(void *app_data, struct nss_gre_redir_lag_ds_msg *nim)
{
	nss_gre_redir_lag_ds_msg_callback_t callback = (nss_gre_redir_lag_ds_msg_callback_t)nss_gre_redir_lag_ds_pvt.cb;
	void *data = nss_gre_redir_lag_ds_pvt.app_data;

	nss_gre_redir_lag_ds_pvt.cb = NULL;
	nss_gre_redir_lag_ds_pvt.app_data = NULL;

	if (nim->cm.response != NSS_CMN_RESPONSE_ACK) {
		nss_warning("GRE LAG DS: error response %d\n", nim->cm.response);
		nss_gre_redir_lag_ds_pvt.response = NSS_TX_FAILURE;
	} else {
		nss_gre_redir_lag_ds_pvt.response = NSS_TX_SUCCESS;
	}

	if (callback) {
		callback(data, &nim->cm);
	}

	complete(&nss_gre_redir_lag_ds_pvt.complete);
}

/*
 * nss_gre_redir_lag_ds_get_node_idx()
 *	Returns index of statistics context.
 */
static inline bool nss_gre_redir_lag_ds_get_node_idx(uint32_t ifnum, uint32_t *idx)
{
	uint32_t node_idx;
	for (node_idx = 0; node_idx < NSS_GRE_REDIR_LAG_MAX_NODE; node_idx++) {
		if ((tun_stats[node_idx].valid) && (tun_stats[node_idx].ifnum == ifnum)) {
			*idx = node_idx;
			return true;
		}
	}

	return false;
}

/*
 * nss_gre_redir_lag_ds_update_sync_stats()
 *	Update synchonized statistics.
 */
static void nss_gre_redir_lag_ds_update_sync_stats(struct nss_ctx_instance *nss_ctx, struct nss_gre_redir_lag_ds_sync_stats_msg *ngss, uint32_t ifnum)
{
	int idx, j;

	spin_lock_bh(&nss_gre_redir_lag_ds_stats_lock);
	if (!nss_gre_redir_lag_ds_get_node_idx(ifnum, &idx)) {
		spin_unlock_bh(&nss_gre_redir_lag_ds_stats_lock);
		nss_warning("%p: Unable to update hash stats msg. Stats context not found.\n", nss_ctx);
		return;
	}

	tun_stats[idx].tx_packets += ngss->node_stats.tx_packets;
	tun_stats[idx].tx_bytes += ngss->node_stats.tx_bytes;
	tun_stats[idx].rx_packets += ngss->node_stats.rx_packets;
	tun_stats[idx].rx_bytes += ngss->node_stats.rx_bytes;
	for (j = 0; j < NSS_MAX_NUM_PRI; j++) {
		tun_stats[idx].rx_dropped[j] += ngss->node_stats.rx_dropped[j];
	}
	spin_unlock_bh(&nss_gre_redir_lag_ds_stats_lock);
}

/*
 * nss_gre_redir_lag_ds_verify_ifnum()
 *	Verify interface type.
 */
static bool nss_gre_redir_lag_ds_verify_ifnum(uint32_t if_num)
{
	return nss_dynamic_interface_get_type(nss_gre_redir_lag_ds_get_context(), if_num) == NSS_DYNAMIC_INTERFACE_TYPE_GRE_REDIR_LAG_DS;
}

/*
 * nss_gre_redir_lag_ds_handler()
 *	Handle NSS -> HLOS messages for gre tunnel
 */
static void nss_gre_redir_lag_ds_msg_handler(struct nss_ctx_instance *nss_ctx, struct nss_cmn_msg *ncm, __attribute__((unused))void *app_data)
{
	void *ctx;
	struct nss_gre_redir_lag_ds_msg *ngrm = (struct nss_gre_redir_lag_ds_msg *)ncm;
	nss_gre_redir_lag_ds_msg_callback_t cb;

	/*
	 * Interface should be a dynamic interface of type NSS_DYNAMIC_INTERFACE_TYPE_GRE_REDIR_LAG_DS.
	 */
	BUG_ON(!nss_gre_redir_lag_ds_verify_ifnum(ncm->interface));

	/*
	 * Is this a valid request/response packet?
	 */
	if (ncm->type >=  NSS_GRE_REDIR_LAG_DS_MAX_MSG_TYPES) {
		nss_warning("%p: received invalid message %d for gre interface\n", nss_ctx, ncm->type);
		return;
	}

	if (nss_cmn_get_msg_len(ncm) > sizeof(struct nss_gre_redir_lag_ds_msg)) {
		nss_warning("%p: Length of message is greater than required: %d\n", nss_ctx, nss_cmn_get_msg_len(ncm));
		return;
	}

	/*
	 * Update the callback and app_data for NOTIFY messages, gre sends all notify messages
	 * to the same callback/app_data.
	 */
	if (ncm->response == NSS_CMM_RESPONSE_NOTIFY) {
		ncm->cb = (nss_ptr_t)nss_ctx->nss_top->if_rx_msg_callback[ncm->interface];
		ncm->app_data = (nss_ptr_t)nss_ctx->nss_rx_interface_handlers[nss_ctx->id][ncm->interface].app_data;
	}

	/*
	 * Log failures
	 */
	nss_core_log_msg_failures(nss_ctx, ncm);

	switch (ncm->type) {
	case NSS_GRE_REDIR_LAG_DS_STATS_SYNC_MSG:
		nss_gre_redir_lag_ds_update_sync_stats(nss_ctx, &ngrm->msg.ds_sync_stats, ncm->interface);
		break;
	}

	/*
	 * Do we have a call back
	 */
	if (!ncm->cb) {
		return;
	}

	/*
	 * callback
	 */
	cb = (nss_gre_redir_lag_ds_msg_callback_t)ncm->cb;
	ctx = (void *)ncm->app_data;

	/*
	 * call gre tunnel callback
	 */
	cb(ctx, ncm);
}

/*
 * nss_gre_redir_lag_ds_unregister_if()
 *	Unregister GRE redirect LAG downstream node.
 */
static enum nss_gre_redir_lag_err_types nss_gre_redir_lag_ds_unregister_if(uint32_t if_num)
{
	struct nss_ctx_instance *nss_ctx __maybe_unused = nss_gre_redir_lag_ds_get_context();
	uint32_t idx, status;

	nss_assert(nss_ctx);
	nss_assert(!nss_gre_redir_lag_ds_verify_ifnum(if_num));

	status = nss_core_unregister_handler(nss_ctx, if_num);
	if (status != NSS_CORE_STATUS_SUCCESS) {
		nss_warning("%p: Not able to unregister handler for gre_lag interface %d with NSS core\n", nss_ctx, if_num);
		return NSS_GRE_REDIR_LAG_ERR_CORE_UNREGISTER_FAILED;
	}

	nss_core_unregister_subsys_dp(nss_ctx, if_num);
	nss_top_main.if_rx_msg_callback[if_num] = NULL;
	spin_lock_bh(&nss_gre_redir_lag_ds_stats_lock);
	if (!nss_gre_redir_lag_ds_get_node_idx(if_num, &idx)) {
		spin_unlock_bh(&nss_gre_redir_lag_ds_stats_lock);
		nss_warning("%p: Stats context not found.\n", nss_ctx);
		return NSS_GRE_REDIR_LAG_ERR_STATS_INDEX_NOT_FOUND;
	}

	tun_stats[idx].valid = false;
	spin_unlock_bh(&nss_gre_redir_lag_ds_stats_lock);
	return NSS_GRE_REDIR_LAG_SUCCESS;
}

/*
 * nss_gre_redir_lag_ds_register_if()
 *	Register GRE redirect LAG downstream node.
 */
static struct nss_ctx_instance *nss_gre_redir_lag_ds_register_if(uint32_t if_num, struct net_device *netdev,
		nss_gre_redir_lag_ds_data_callback_t cb_func_data,
		nss_gre_redir_lag_ds_msg_callback_t cb_func_msg, uint32_t features, uint32_t type, void *app_ctx)
{
	struct nss_ctx_instance *nss_ctx = nss_gre_redir_lag_ds_get_context();
	uint32_t status, i;
	nss_assert(nss_ctx);
	nss_assert(!nss_gre_redir_lag_ds_verify_ifnum(if_num));

	/*
	 * Registering handler for sending tunnel interface msgs to NSS.
	 */
	status = nss_core_register_handler(nss_ctx, if_num, nss_gre_redir_lag_ds_msg_handler, app_ctx);
	if (status != NSS_CORE_STATUS_SUCCESS) {
		nss_warning("%p: Not able to register handler for gre_lag interface %d with NSS core\n", nss_ctx, if_num);
		return NULL;
	}

	nss_core_register_subsys_dp(nss_ctx, if_num, cb_func_data, NULL, NULL, netdev, features);
	nss_core_set_subsys_dp_type(nss_ctx, netdev, if_num, type);
	nss_top_main.if_rx_msg_callback[if_num] = cb_func_msg;
	spin_lock_bh(&nss_gre_redir_lag_ds_stats_lock);
	for (i = 0; i < NSS_GRE_REDIR_LAG_MAX_NODE; i++) {
		if (!tun_stats[i].valid) {
			tun_stats[i].ifnum = if_num;
			tun_stats[i].valid = true;
			break;
		}
	}

	spin_unlock_bh(&nss_gre_redir_lag_ds_stats_lock);

	return nss_ctx;
}

/*
 * nss_gre_redir_lag_ds_get_context()
 *	Retrieves context GRE redirect LAG downstream node.
 */
struct nss_ctx_instance *nss_gre_redir_lag_ds_get_context(void)
{
	return (struct nss_ctx_instance *)&nss_top_main.nss[nss_top_main.gre_redir_lag_ds_handler_id];
}
EXPORT_SYMBOL(nss_gre_redir_lag_ds_get_context);

/*
 * nss_gre_redir_lag_ds_get_cmn_stats()
 *	Get statistics for downstream LAG node.
 */
bool nss_gre_redir_lag_ds_get_cmn_stats(struct nss_gre_redir_lag_ds_tun_stats *cmn_stats, uint32_t index)
{
	if (index >= NSS_GRE_REDIR_LAG_MAX_NODE) {
		return false;
	}

	spin_lock_bh(&nss_gre_redir_lag_ds_stats_lock);
	if (!tun_stats[index].valid) {
		spin_unlock_bh(&nss_gre_redir_lag_ds_stats_lock);
		return false;
	}
	memcpy((void *)cmn_stats, (void *)&tun_stats[index], sizeof(*cmn_stats));
	spin_unlock_bh(&nss_gre_redir_lag_ds_stats_lock);
	return true;
}

/*
 * nss_gre_redir_lag_ds_tx_msg()
 *	Transmit a gre message to NSS.
 */
nss_tx_status_t nss_gre_redir_lag_ds_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_gre_redir_lag_ds_msg *msg)
{
	struct nss_gre_redir_lag_ds_msg *nm;
	struct nss_cmn_msg *ncm = &msg->cm;
	struct sk_buff *nbuf;
	int32_t status;

	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("%p: GRE redir LAG DS msg dropped as core not ready\n", nss_ctx);
		return NSS_TX_FAILURE_NOT_READY;
	}

	/*
	 * Sanity check the message. Interface should be a dynamic interface
	 * of type NSS_DYNAMIC_INTERFACE_TYPE_GRE_REDIR_LAG_DS.
	 */
	if (!nss_gre_redir_lag_ds_verify_ifnum(ncm->interface)) {
		nss_warning("%p: tx request for another interface: %d\n", nss_ctx, ncm->interface);
		return NSS_TX_FAILURE;
	}

	if (ncm->type >= NSS_GRE_REDIR_LAG_DS_MAX_MSG_TYPES) {
		nss_warning("%p: message type out of range: %d\n", nss_ctx, ncm->type);
		return NSS_TX_FAILURE;
	}

	if (nss_cmn_get_msg_len(ncm) > sizeof(struct nss_gre_redir_lag_ds_msg)) {
		nss_warning("%p: message length is invalid: %d\n", nss_ctx, nss_cmn_get_msg_len(ncm));
		return NSS_TX_FAILURE;
	}

	nbuf = dev_alloc_skb(NSS_NBUF_PAYLOAD_SIZE);
	if (unlikely(!nbuf)) {
		NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_NBUF_ALLOC_FAILS]);
		nss_warning("%p: msg dropped as command allocation failed\n", nss_ctx);
		return NSS_TX_FAILURE;
	}

	/*
	 * Copy the message to our skb
	 */
	nm = (struct nss_gre_redir_lag_ds_msg *)skb_put(nbuf, sizeof(struct nss_gre_redir_lag_ds_msg));
	memcpy(nm, msg, sizeof(struct nss_gre_redir_lag_ds_msg));

	status = nss_core_send_buffer(nss_ctx, 0, nbuf, NSS_IF_CMD_QUEUE, H2N_BUFFER_CTRL, 0);
	if (status != NSS_CORE_STATUS_SUCCESS) {
		dev_kfree_skb_any(nbuf);
		nss_warning("%p: Unable to enqueue 'gre message'\n", nss_ctx);
		return NSS_TX_FAILURE;
	}

	nss_hal_send_interrupt(nss_ctx, NSS_H2N_INTR_DATA_COMMAND_QUEUE);

	NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_TX_CMD_REQ]);
	return NSS_TX_SUCCESS;
}
EXPORT_SYMBOL(nss_gre_redir_lag_ds_tx_msg);

/*
 * nss_gre_redir_lag_ds_tx_msg_sync()
 *	Transmit a GRE lag message to NSS firmware synchronously.
 */
nss_tx_status_t nss_gre_redir_lag_ds_tx_msg_sync(struct nss_ctx_instance *nss_ctx, struct nss_gre_redir_lag_ds_msg *ngrm)
{
	nss_tx_status_t status;
	int ret = 0;

	down(&nss_gre_redir_lag_ds_pvt.sem);
	nss_gre_redir_lag_ds_pvt.cb = (void *)ngrm->cm.cb;
	nss_gre_redir_lag_ds_pvt.app_data = (void *)ngrm->cm.app_data;
	ngrm->cm.cb = (nss_ptr_t)nss_gre_redir_lag_ds_callback;
	ngrm->cm.app_data = (nss_ptr_t)NULL;

	status = nss_gre_redir_lag_ds_tx_msg(nss_ctx, ngrm);
	if (status != NSS_TX_SUCCESS) {
		nss_warning("%p: GRE LAG DS msg tx failed\n", nss_ctx);
		up(&nss_gre_redir_lag_ds_pvt.sem);
		return status;
	}

	ret = wait_for_completion_timeout(&nss_gre_redir_lag_ds_pvt.complete, msecs_to_jiffies(NSS_GRE_REDIR_LAG_DS_TX_TIMEOUT));
	if (!ret) {
		nss_warning("%p: GRE LAG DS tx sync failed due to timeout\n", nss_ctx);
		nss_gre_redir_lag_ds_pvt.response = NSS_TX_FAILURE;
	}

	status = nss_gre_redir_lag_ds_pvt.response;
	up(&nss_gre_redir_lag_ds_pvt.sem);
	return status;
}
EXPORT_SYMBOL(nss_gre_redir_lag_ds_tx_msg_sync);

/*
 * nss_gre_redir_lag_ds_unregister_and_dealloc()
 *	Unregister and deallocate nss gre redirect LAG DS node.
 */
enum nss_gre_redir_lag_err_types nss_gre_redir_lag_ds_unregister_and_dealloc(uint32_t ifnum)
{
	uint32_t ret;
	struct nss_ctx_instance *nss_ctx __maybe_unused = nss_gre_redir_lag_ds_get_context();
	nss_tx_status_t status;

	if (!nss_gre_redir_lag_ds_verify_ifnum(ifnum)) {
		nss_warning("%p: Unknown interface type %u.\n", nss_ctx, ifnum);
		return NSS_GRE_REDIR_LAG_ERR_INCORRECT_IFNUM;
	}

	ret = nss_gre_redir_lag_ds_unregister_if(ifnum);
	if (ret) {
		nss_warning("%p: Unable to unregister interface %u.\n", nss_ctx, ifnum);
		return ret;
	}

	status = nss_dynamic_interface_dealloc_node(ifnum, NSS_DYNAMIC_INTERFACE_TYPE_GRE_REDIR_LAG_DS);
	if (status != NSS_TX_SUCCESS) {
		nss_warning("%p: Unable to deallocate node %u\n", nss_ctx, ifnum);
		return NSS_GRE_REDIR_LAG_ERR_DEALLOC_FAILED;
	}

	return NSS_GRE_REDIR_LAG_SUCCESS;
}
EXPORT_SYMBOL(nss_gre_redir_lag_ds_unregister_and_dealloc);

/*
 * nss_gre_redir_lag_ds_alloc_and_register_node()
 *	Allocates and registers GRE downstream type dynamic nodes with NSS.
 */
int nss_gre_redir_lag_ds_alloc_and_register_node(struct net_device *dev,
		nss_gre_redir_lag_ds_data_callback_t cb_func_data,
		nss_gre_redir_lag_ds_msg_callback_t cb_func_msg, void *app_ctx)
{
	int ifnum;
	nss_tx_status_t status;
	struct nss_ctx_instance *nss_ctx;

	ifnum = nss_dynamic_interface_alloc_node(NSS_DYNAMIC_INTERFACE_TYPE_GRE_REDIR_LAG_DS);
	if (ifnum == -1) {
		nss_warning("%p: Unable to allocate GRE_LAG node of type = %u\n", dev, NSS_DYNAMIC_INTERFACE_TYPE_GRE_REDIR_LAG_DS);
		return -1;
	}

	nss_ctx = nss_gre_redir_lag_ds_register_if(ifnum, dev, cb_func_data,
			cb_func_msg, 0, NSS_DYNAMIC_INTERFACE_TYPE_GRE_REDIR_LAG_DS, app_ctx);
	if (!nss_ctx) {
		nss_warning("%p: Unable to register GRE_LAG node of type = %u\n", dev, NSS_DYNAMIC_INTERFACE_TYPE_GRE_REDIR_LAG_DS);
		status = nss_dynamic_interface_dealloc_node(ifnum, NSS_DYNAMIC_INTERFACE_TYPE_GRE_REDIR_LAG_DS);
		if (status != NSS_TX_SUCCESS) {
			nss_warning("%p: Unable to deallocate node of type = %u.\n", dev, NSS_DYNAMIC_INTERFACE_TYPE_GRE_REDIR_LAG_DS);
		}

		return -1;
	}

	return ifnum;
}
EXPORT_SYMBOL(nss_gre_redir_lag_ds_alloc_and_register_node);

/*
 * nss_gre_redir_lag_ds_register_handler()
 *	Registering handler for sending msg to base gre_lag node on NSS.
 */
void nss_gre_redir_lag_ds_register_handler(void)
{
	if (!nss_gre_redir_lag_ds_stats_dentry_create()) {
		nss_warning(" Unable to create debugfs entry for LAG DS node.\n");
		return;
	}

	nss_gre_redir_lag_ds_pvt.cb = NULL;
	nss_gre_redir_lag_ds_pvt.app_data = NULL;
	sema_init(&nss_gre_redir_lag_ds_pvt.sem, 1);
	init_completion(&nss_gre_redir_lag_ds_pvt.complete);
}
