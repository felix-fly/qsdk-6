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
 * nss_virt_if.c
 *	NSS virtual/redirect handler APIs
 */

#include "nss_tx_rx_common.h"
#include "nss_virt_if_stats.h"
#include <net/arp.h>

#define NSS_VIRT_IF_TX_TIMEOUT			3000 /* 3 Seconds */
#define NSS_VIRT_IF_GET_INDEX(if_num)	(if_num-NSS_DYNAMIC_IF_START)

extern int nss_ctl_redirect;

/*
 * Data structure that holds the virtual interface context.
 */
struct nss_virt_if_handle *nss_virt_if_handle_t[NSS_MAX_DYNAMIC_INTERFACES];

/*
 * Spinlock to protect the global data structure virt_handle.
 */
DEFINE_SPINLOCK(nss_virt_if_lock);

/*
 * nss_virt_if_get_context()
 */
struct nss_ctx_instance *nss_virt_if_get_context(void)
{
	return &nss_top_main.nss[nss_top_main.virt_if_handler_id];
}

/*
 * nss_virt_if_verify_if_num()
 *	Verify if_num passed to us.
 */
bool nss_virt_if_verify_if_num(uint32_t if_num)
{
	enum nss_dynamic_interface_type type = nss_dynamic_interface_get_type(nss_virt_if_get_context(), if_num);

	return type == NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR_N2H
		|| type == NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR_H2N;
}
EXPORT_SYMBOL(nss_virt_if_verify_if_num);

/*
 * nss_virt_if_msg_handler()
 *	Handle msg responses from the FW on virtual interfaces
 */
static void nss_virt_if_msg_handler(struct nss_ctx_instance *nss_ctx,
					struct nss_cmn_msg *ncm,
					void *app_data)
{
	struct nss_virt_if_msg *nvim = (struct nss_virt_if_msg *)ncm;
	int32_t if_num;

	nss_virt_if_msg_callback_t cb;
	struct nss_virt_if_handle *handle = NULL;

	/*
	 * Sanity check the message type
	 */
	if (ncm->type > NSS_VIRT_IF_MAX_MSG_TYPES) {
		nss_warning("%p: message type out of range: %d", nss_ctx, ncm->type);
		return;
	}

	/*
	 * Messages value that are within the base class are handled by the base class.
	 */
	if (ncm->type < NSS_IF_MAX_MSG_TYPES) {
		return nss_if_msg_handler(nss_ctx, ncm, app_data);
	}

	if (!nss_virt_if_verify_if_num(ncm->interface)) {
		nss_warning("%p: response for another interface: %d", nss_ctx, ncm->interface);
		return;
	}

	if_num = NSS_VIRT_IF_GET_INDEX(ncm->interface);

	spin_lock_bh(&nss_virt_if_lock);
	if (!nss_virt_if_handle_t[if_num]) {
		spin_unlock_bh(&nss_virt_if_lock);
		nss_warning("%p: virt_if handle is NULL\n", nss_ctx);
		return;
	}

	handle = nss_virt_if_handle_t[if_num];
	spin_unlock_bh(&nss_virt_if_lock);

	switch (nvim->cm.type) {
	case NSS_VIRT_IF_STATS_SYNC_MSG:
		nss_virt_if_stats_sync(handle, &nvim->msg.stats);
		break;
	}

	/*
	 * Log failures
	 */
	nss_core_log_msg_failures(nss_ctx, ncm);

	/*
	 * Update the callback and app_data for NOTIFY messages, IPv4 sends all notify messages
	 * to the same callback/app_data.
	 */
	if (ncm->response == NSS_CMM_RESPONSE_NOTIFY) {
		ncm->cb = (nss_ptr_t)nss_ctx->nss_top->if_rx_msg_callback[ncm->interface];
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
	cb = (nss_virt_if_msg_callback_t)ncm->cb;
	cb((void *)ncm->app_data, ncm);
}

/*
 * nss_virt_if_callback
 *	Callback to handle the completion of NSS ->HLOS messages.
 */
static void nss_virt_if_callback(void *app_data, struct nss_cmn_msg *ncm)
{
	struct nss_virt_if_handle *handle = (struct nss_virt_if_handle *)app_data;
	struct nss_virt_if_pvt *nvip = handle->pvt;

	if (ncm->response != NSS_CMN_RESPONSE_ACK) {
		nss_warning("%p: virt_if Error response %d\n", handle->nss_ctx, ncm->response);
		nvip->response = NSS_TX_FAILURE;
		complete(&nvip->complete);
		return;
	}

	nvip->response = NSS_TX_SUCCESS;
	complete(&nvip->complete);
}

/*
 * nss_virt_if_tx_msg_sync
 *	Send a message from HLOS to NSS synchronously.
 */
static nss_tx_status_t nss_virt_if_tx_msg_sync(struct nss_virt_if_handle *handle,
						struct nss_virt_if_msg *nvim)
{
	nss_tx_status_t status;
	int ret = 0;
	struct nss_virt_if_pvt *nwip = handle->pvt;
	struct nss_ctx_instance *nss_ctx = handle->nss_ctx;

	down(&nwip->sem);

	status = nss_virt_if_tx_msg(nss_ctx, nvim);
	if (status != NSS_TX_SUCCESS) {
		nss_warning("%p: nss_virt_if_msg failed\n", nss_ctx);
		up(&nwip->sem);
		return status;
	}

	ret = wait_for_completion_timeout(&nwip->complete,
						msecs_to_jiffies(NSS_VIRT_IF_TX_TIMEOUT));
	if (!ret) {
		nss_warning("%p: virt_if tx failed due to timeout\n", nss_ctx);
		nwip->response = NSS_TX_FAILURE;
	}

	status = nwip->response;
	up(&nwip->sem);

	return status;
}

/*
 * nss_virt_if_msg_init()
 *	Initialize virt specific message structure.
 */
static void nss_virt_if_msg_init(struct nss_virt_if_msg *nvim,
					uint16_t if_num,
					uint32_t type,
					uint32_t len,
					nss_virt_if_msg_callback_t cb,
					struct nss_virt_if_handle *app_data)
{
	nss_cmn_msg_init(&nvim->cm, if_num, type, len, (void *)cb, (void *)app_data);
}

/*
 * nss_virt_if_handle_destroy_sync()
 *	Destroy the virt handle either due to request from user or due to error, synchronously.
 */
static int nss_virt_if_handle_destroy_sync(struct nss_virt_if_handle *handle)
{
	nss_tx_status_t status;
	int32_t if_num_n2h = handle->if_num_n2h;
	int32_t if_num_h2n = handle->if_num_h2n;
	int32_t index_n2h;
	int32_t index_h2n;

	if (!nss_virt_if_verify_if_num(if_num_n2h) || !nss_virt_if_verify_if_num(if_num_h2n)) {
		nss_warning("%p: bad interface numbers %d %d\n", handle->nss_ctx, if_num_n2h, if_num_h2n);
		return NSS_TX_FAILURE_BAD_PARAM;
	}

	index_n2h = NSS_VIRT_IF_GET_INDEX(if_num_n2h);
	index_h2n = NSS_VIRT_IF_GET_INDEX(if_num_h2n);

	status = nss_dynamic_interface_dealloc_node(if_num_n2h, NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR_N2H);
	if (status != NSS_TX_SUCCESS) {
		nss_warning("%p: Dynamic interface destroy failed status %d\n", handle->nss_ctx, status);
		return status;
	}

	status = nss_dynamic_interface_dealloc_node(if_num_h2n, NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR_H2N);
	if (status != NSS_TX_SUCCESS) {
		nss_warning("%p: Dynamic interface destroy failed status %d\n", handle->nss_ctx, status);
		return status;
	}

	spin_lock_bh(&nss_virt_if_lock);
	nss_virt_if_handle_t[index_n2h] = NULL;
	nss_virt_if_handle_t[index_h2n] = NULL;
	spin_unlock_bh(&nss_virt_if_lock);

	kfree(handle->pvt);
	kfree(handle);

	return status;
}

/*
 * nss_virt_if_handle_create_sync()
 *	Initialize virt handle which holds the if_num and stats per interface.
 */
static struct nss_virt_if_handle *nss_virt_if_handle_create_sync(struct nss_ctx_instance *nss_ctx, int32_t if_num_n2h, int32_t if_num_h2n, int32_t *cmd_rsp)
{
	int32_t index_n2h;
	int32_t index_h2n;
	struct nss_virt_if_handle *handle;

	if (!nss_virt_if_verify_if_num(if_num_n2h) || !nss_virt_if_verify_if_num(if_num_h2n)) {
		nss_warning("%p: bad interface numbers %d %d\n", nss_ctx, if_num_n2h, if_num_h2n);
		return NULL;
	}

	index_n2h = NSS_VIRT_IF_GET_INDEX(if_num_n2h);
	index_h2n = NSS_VIRT_IF_GET_INDEX(if_num_h2n);

	handle = (struct nss_virt_if_handle *)kzalloc(sizeof(struct nss_virt_if_handle),
									GFP_KERNEL);
	if (!handle) {
		nss_warning("%p: handle memory alloc failed\n", nss_ctx);
		*cmd_rsp = NSS_VIRT_IF_ALLOC_FAILURE;
		goto error1;
	}

	handle->nss_ctx = nss_ctx;
	handle->if_num_n2h = if_num_n2h;
	handle->if_num_h2n = if_num_h2n;
	handle->pvt = (struct nss_virt_if_pvt *)kzalloc(sizeof(struct nss_virt_if_pvt),
								GFP_KERNEL);
	if (!handle->pvt) {
		nss_warning("%p: failure allocating memory for nss_virt_if_pvt\n", nss_ctx);
		*cmd_rsp = NSS_VIRT_IF_ALLOC_FAILURE;
		goto error2;
	}

	handle->cb = NULL;
	handle->app_data = NULL;

	spin_lock_bh(&nss_virt_if_lock);
	nss_virt_if_handle_t[index_n2h] = handle;
	nss_virt_if_handle_t[index_h2n] = handle;
	spin_unlock_bh(&nss_virt_if_lock);

	*cmd_rsp = NSS_VIRT_IF_SUCCESS;

	return handle;

error2:
	kfree(handle);
error1:
	return NULL;
}

/*
 * nss_virt_if_register_handler_sync()
 * 	register msg handler for virtual interface and initialize semaphore and completion.
 */
static uint32_t nss_virt_if_register_handler_sync(struct nss_ctx_instance *nss_ctx, struct nss_virt_if_handle *handle)
{
	uint32_t ret;
	struct nss_virt_if_pvt *nvip = NULL;
	int32_t if_num_n2h = handle->if_num_n2h;
	int32_t if_num_h2n = handle->if_num_h2n;

	ret = nss_core_register_handler(nss_ctx, if_num_n2h, nss_virt_if_msg_handler, NULL);
	if (ret != NSS_CORE_STATUS_SUCCESS) {
		nss_warning("%p: Failed to register message handler for redir_n2h interface %d\n", nss_ctx, if_num_n2h);
		return NSS_VIRT_IF_CORE_FAILURE;
	}

	ret = nss_core_register_handler(nss_ctx, if_num_h2n, nss_virt_if_msg_handler, NULL);
	if (ret != NSS_CORE_STATUS_SUCCESS) {
		nss_core_unregister_handler(nss_ctx, if_num_n2h);
		nss_warning("%p: Failed to register message handler for redir_h2n interface %d\n", nss_ctx, if_num_h2n);
		return NSS_VIRT_IF_CORE_FAILURE;
	}

	nvip = handle->pvt;
	if (!nvip->sem_init_done) {
		sema_init(&nvip->sem, 1);
		init_completion(&nvip->complete);
		nvip->sem_init_done = 1;
	}

	nss_virt_if_stats_dentry_create();
	return NSS_VIRT_IF_SUCCESS;
}

/*
 * nss_virt_if_create_sync()
 *	Create redir_n2h and redir_h2n interfaces, synchronously and associate it with same netdev
 */
struct nss_virt_if_handle *nss_virt_if_create_sync(struct net_device *netdev)
{
	struct nss_ctx_instance *nss_ctx = nss_virt_if_get_context();
	struct nss_virt_if_msg nvim;
	struct nss_virt_if_config_msg *nvcm;
	uint32_t ret;
	struct nss_virt_if_handle *handle = NULL;
	int32_t if_num_n2h, if_num_h2n;

	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("%p: Interface could not be created as core not ready\n", nss_ctx);
		return NULL;
	}

	if_num_n2h = nss_dynamic_interface_alloc_node(NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR_N2H);
	if (if_num_n2h < 0) {
		nss_warning("%p: failure allocating redir_n2h\n", nss_ctx);
		return NULL;
	}

	if_num_h2n = nss_dynamic_interface_alloc_node(NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR_H2N);
	if (if_num_h2n < 0) {
		nss_warning("%p: failure allocating redir_h2n\n", nss_ctx);
		nss_dynamic_interface_dealloc_node(if_num_n2h, NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR_N2H);
		return NULL;
	}

	handle = nss_virt_if_handle_create_sync(nss_ctx, if_num_n2h, if_num_h2n, &ret);
	if (!handle) {
		nss_warning("%p: virt_if handle creation failed ret %d\n", nss_ctx, ret);
		nss_dynamic_interface_dealloc_node(if_num_n2h, NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR_N2H);
		nss_dynamic_interface_dealloc_node(if_num_h2n, NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR_H2N);
		return NULL;
	}

	/*
	 * Initializes the semaphore and also sets the msg handler for if_num.
	 */
	ret = nss_virt_if_register_handler_sync(nss_ctx, handle);
	if (ret != NSS_VIRT_IF_SUCCESS) {
		nss_warning("%p: Registration handler failed reason: %d\n", nss_ctx, ret);
		goto error1;
	}

	nss_virt_if_msg_init(&nvim, handle->if_num_n2h, NSS_VIRT_IF_TX_CONFIG_MSG,
				sizeof(struct nss_virt_if_config_msg), nss_virt_if_callback, handle);

	nvcm = &nvim.msg.if_config;
	nvcm->flags = 0;
	nvcm->sibling = if_num_h2n;
	nvcm->nexthop = NSS_N2H_INTERFACE;
	memcpy(nvcm->mac_addr, netdev->dev_addr, ETH_ALEN);

	ret = nss_virt_if_tx_msg_sync(handle, &nvim);
	if (ret != NSS_TX_SUCCESS) {
		nss_warning("%p: nss_virt_if_tx_msg_sync failed %u\n", nss_ctx, ret);
		goto error2;
	}

	nvim.cm.interface = if_num_h2n;
	nvcm->sibling = if_num_n2h;
	nvcm->nexthop = NSS_ETH_RX_INTERFACE;

	ret = nss_virt_if_tx_msg_sync(handle, &nvim);
	if (ret != NSS_TX_SUCCESS) {
		nss_warning("%p: nss_virt_if_tx_msg_sync failed %u\n", nss_ctx, ret);
		goto error2;
	}

	nss_core_register_subsys_dp(nss_ctx, handle->if_num_n2h, NULL, NULL, NULL, netdev, 0);
	nss_core_register_subsys_dp(nss_ctx, handle->if_num_h2n, NULL, NULL, NULL, netdev, 0);

	nss_core_set_subsys_dp_type(nss_ctx, netdev, if_num_n2h, NSS_VIRT_IF_DP_REDIR_N2H);
	nss_core_set_subsys_dp_type(nss_ctx, netdev, if_num_h2n, NSS_VIRT_IF_DP_REDIR_H2N);

	/*
	 * Hold a reference to the net_device
	 */
	dev_hold(netdev);

	/*
	 * The context returned is the handle interface # which contains all the info related to
	 * the interface if_num.
	 */

	return handle;

error2:
	nss_core_unregister_handler(nss_ctx, if_num_n2h);
	nss_core_unregister_handler(nss_ctx, if_num_h2n);

error1:
	nss_virt_if_handle_destroy_sync(handle);
	return NULL;
}
EXPORT_SYMBOL(nss_virt_if_create_sync);

/*
 * nss_virt_if_destroy_sync()
 *	Destroy the virt interface associated with the interface number, synchronously.
 */
nss_tx_status_t nss_virt_if_destroy_sync(struct nss_virt_if_handle *handle)
{
	nss_tx_status_t status;
	struct net_device *dev;
	int32_t if_num_n2h;
	int32_t if_num_h2n;
	struct nss_ctx_instance *nss_ctx;
	uint32_t ret;

	if (!handle) {
		nss_warning("handle is NULL\n");
		return NSS_TX_FAILURE_BAD_PARAM;
	}

	if_num_n2h = handle->if_num_n2h;
	if_num_h2n = handle->if_num_h2n;
	nss_ctx = handle->nss_ctx;

	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("%p: Interface could not be destroyed as core not ready\n", nss_ctx);
		return NSS_TX_FAILURE_NOT_READY;
	}

	spin_lock_bh(&nss_top_main.lock);
	if (!nss_ctx->subsys_dp_register[if_num_n2h].ndev || !nss_ctx->subsys_dp_register[if_num_h2n].ndev) {
		spin_unlock_bh(&nss_top_main.lock);
		nss_warning("%p: Unregister virt interface %d %d: no context\n", nss_ctx, if_num_n2h, if_num_h2n);
		return NSS_TX_FAILURE_BAD_PARAM;
	}

	dev = nss_ctx->subsys_dp_register[if_num_n2h].ndev;
	nss_assert(dev == nss_ctx->subsys_dp_register[if_num_h2n].ndev);
	nss_core_unregister_subsys_dp(nss_ctx, if_num_n2h);
	nss_core_unregister_subsys_dp(nss_ctx, if_num_h2n);
	spin_unlock_bh(&nss_top_main.lock);
	dev_put(dev);

	status = nss_virt_if_handle_destroy_sync(handle);
	if (status != NSS_TX_SUCCESS) {
		nss_warning("%p: handle destroy failed for if_num_n2h %d and if_num_h2n %d\n", nss_ctx, if_num_n2h, if_num_h2n);
		return NSS_TX_FAILURE;
	}

	ret = nss_core_unregister_handler(nss_ctx, if_num_n2h);
	if (ret != NSS_CORE_STATUS_SUCCESS) {
		nss_warning("%p: Not able to unregister handler for redir_n2h interface %d with NSS core\n", nss_ctx, if_num_n2h);
		return NSS_TX_FAILURE_BAD_PARAM;
	}

	ret = nss_core_unregister_handler(nss_ctx, if_num_h2n);
	if (ret != NSS_CORE_STATUS_SUCCESS) {
		nss_warning("%p: Not able to unregister handler for redir_h2n interface %d with NSS core\n", nss_ctx, if_num_h2n);
		return NSS_TX_FAILURE_BAD_PARAM;
	}

	return status;
}
EXPORT_SYMBOL(nss_virt_if_destroy_sync);

/*
 * nss_virt_if_tx_buf()
 *	HLOS interface has received a packet which we redirect to the NSS, if appropriate to do so.
 */
nss_tx_status_t nss_virt_if_tx_buf(struct nss_virt_if_handle *handle,
						struct sk_buff *skb)
{
	int32_t status;
	int32_t if_num = handle->if_num_h2n;
	struct nss_ctx_instance *nss_ctx = handle->nss_ctx;

	if (unlikely(nss_ctl_redirect == 0)) {
		return NSS_TX_FAILURE_NOT_ENABLED;
	}

	if (unlikely(skb->vlan_tci)) {
		return NSS_TX_FAILURE_NOT_SUPPORTED;
	}

	if (!nss_virt_if_verify_if_num(if_num)) {
		nss_warning("%p: bad interface number %d\n", nss_ctx, if_num);
		return NSS_TX_FAILURE_BAD_PARAM;
	}

	nss_trace("%p: Virtual Rx packet, if_num:%d, skb:%p", nss_ctx, if_num, skb);

	/*
	 * Get the NSS context that will handle this packet and check that it is initialised and ready
	 */
	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("%p: Virtual Rx packet dropped as core not ready", nss_ctx);
		return NSS_TX_FAILURE_NOT_READY;
	}

	/*
	 * Sanity check the SKB to ensure that it's suitable for us
	 */
	if (unlikely(skb->len <= ETH_HLEN)) {
		nss_warning("%p: Virtual Rx packet: %p too short", nss_ctx, skb);
		return NSS_TX_FAILURE_TOO_SHORT;
	}

	if (unlikely(skb_shinfo(skb)->nr_frags != 0)) {
		/*
		 * TODO: If we have a connection matching rule for this skbuff,
		 * do we need to flush it??
		 */
		nss_warning("%p: Delivering the packet to Linux because of fragmented skb: %p\n", nss_ctx, skb);
		return NSS_TX_FAILURE_NOT_SUPPORTED;
	}

	/*
	 * Direct the buffer to the NSS
	 */
	status = nss_core_send_buffer(nss_ctx, if_num, skb, NSS_IF_DATA_QUEUE_0,
					H2N_BUFFER_PACKET, H2N_BIT_FLAG_VIRTUAL_BUFFER);
	if (unlikely(status != NSS_CORE_STATUS_SUCCESS)) {
		nss_warning("%p: Virtual Rx packet unable to enqueue\n", nss_ctx);
		return NSS_TX_FAILURE_QUEUE;
	}

	/*
	 * Kick the NSS awake so it can process our new entry.
	 */
	nss_hal_send_interrupt(nss_ctx, NSS_H2N_INTR_DATA_COMMAND_QUEUE);
	NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_TX_PACKET]);
	return NSS_TX_SUCCESS;
}
EXPORT_SYMBOL(nss_virt_if_tx_buf);

/*
 * nss_virt_if_tx_msg()
 */
nss_tx_status_t nss_virt_if_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_virt_if_msg *nvim)
{
	int32_t status;
	struct sk_buff *nbuf;
	struct nss_cmn_msg *ncm = &nvim->cm;
	struct nss_virt_if_msg *nvim2;

	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("Interface could not be created as core not ready");
		return NSS_TX_FAILURE;
	}

	/*
	 * Sanity check the message
	 */
	if (!nss_virt_if_verify_if_num(ncm->interface)) {
		nss_warning("%p: tx request for another interface: %d", nss_ctx, ncm->interface);
		return NSS_TX_FAILURE;
	}

	if (ncm->type > NSS_VIRT_IF_MAX_MSG_TYPES) {
		nss_warning("%p: message type out of range: %d", nss_ctx, ncm->type);
		return NSS_TX_FAILURE;
	}

	if (nss_cmn_get_msg_len(ncm) > sizeof(struct nss_virt_if_msg)) {
		nss_warning("%p: invalid length: %d. Length of virt msg is %d",
				nss_ctx, nss_cmn_get_msg_len(ncm), (int)sizeof(struct nss_virt_if_msg));
		return NSS_TX_FAILURE;
	}

	nbuf = dev_alloc_skb(NSS_NBUF_PAYLOAD_SIZE);
	if (unlikely(!nbuf)) {
		NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_NBUF_ALLOC_FAILS]);
		nss_warning("%p: virtual interface %d: command allocation failed", nss_ctx, ncm->interface);
		return NSS_TX_FAILURE;
	}

	nvim2 = (struct nss_virt_if_msg *)skb_put(nbuf, sizeof(struct nss_virt_if_msg));
	memcpy(nvim2, nvim, sizeof(struct nss_virt_if_msg));

	status = nss_core_send_buffer(nss_ctx, 0, nbuf, NSS_IF_CMD_QUEUE, H2N_BUFFER_CTRL, 0);
	if (status != NSS_CORE_STATUS_SUCCESS) {
		dev_kfree_skb_any(nbuf);
		nss_warning("%p: Unable to enqueue 'virtual interface' command\n", nss_ctx);
		return NSS_TX_FAILURE;
	}

	nss_hal_send_interrupt(nss_ctx, NSS_H2N_INTR_DATA_COMMAND_QUEUE);

	/*
	 * The context returned is the virtual interface # which is, essentially, the index into the if_ctx
	 * array that is holding the net_device pointer
	 */
	return NSS_TX_SUCCESS;
}
EXPORT_SYMBOL(nss_virt_if_tx_msg);

/*
 * nss_virt_if_register()
 */
void nss_virt_if_register(struct nss_virt_if_handle *handle,
				nss_virt_if_data_callback_t data_callback,
				struct net_device *netdev)
{
	struct nss_ctx_instance *nss_ctx;
	int32_t if_num;

	if (!handle) {
		nss_warning("handle is NULL\n");
		return;
	}

	nss_ctx = handle->nss_ctx;

	if (!nss_virt_if_verify_if_num(handle->if_num_h2n)) {
		nss_warning("if_num is invalid\n");
		return;
	}

	if_num = handle->if_num_h2n;

	nss_core_register_subsys_dp(nss_ctx, if_num, data_callback, NULL, NULL, netdev, (uint32_t)netdev->features);
	nss_top_main.if_rx_msg_callback[if_num] = NULL;
}
EXPORT_SYMBOL(nss_virt_if_register);

/*
 * nss_virt_if_unregister()
 */
void nss_virt_if_unregister(struct nss_virt_if_handle *handle)
{
	struct nss_ctx_instance *nss_ctx;
	int32_t if_num;

	if (!handle) {
		nss_warning("handle is NULL\n");
		return;
	}

	nss_ctx = handle->nss_ctx;

	if (!nss_virt_if_verify_if_num(handle->if_num_h2n)) {
		nss_warning("if_num is invalid\n");
		return;
	}

	if_num = handle->if_num_h2n;

	nss_core_unregister_subsys_dp(nss_ctx, if_num);

	nss_top_main.if_rx_msg_callback[if_num] = NULL;
}
EXPORT_SYMBOL(nss_virt_if_unregister);

/*
 * nss_virt_if_get_interface_num()
 *	Get interface number for a virtual interface
 */
int32_t nss_virt_if_get_interface_num(struct nss_virt_if_handle *handle)
{
	if (!handle) {
		nss_warning("virt_if handle is NULL\n");
		return -1;
	}

	/*
	 * Return if_num_n2h whose datapath type is 0.
	 */
	return handle->if_num_n2h;
}
EXPORT_SYMBOL(nss_virt_if_get_interface_num);
