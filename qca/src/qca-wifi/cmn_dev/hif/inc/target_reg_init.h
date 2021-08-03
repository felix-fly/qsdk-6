/*
 * Copyright (c) 2016 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef TARGET_REG_INIT_H
#define TARGET_REG_INIT_H
#include "reg_struct.h"
#include "targaddrs.h"
/*** WARNING : Add to the end of the TABLE! do not change the order ****/
typedef struct targetdef_s TARGET_REGISTER_TABLE;



#define ATH_UNSUPPORTED_REG_OFFSET UNSUPPORTED_REGISTER_OFFSET
#define ATH_SUPPORTED_BY_TARGET(reg_offset) \
	((reg_offset) != ATH_UNSUPPORTED_REG_OFFSET)

#if defined(MY_TARGET_DEF)

/* Cross-platform compatibility */
#if !defined(SOC_RESET_CONTROL_OFFSET) && defined(RESET_CONTROL_OFFSET)
#define SOC_RESET_CONTROL_OFFSET RESET_CONTROL_OFFSET
#endif

#if !defined(CLOCK_GPIO_OFFSET)
#define CLOCK_GPIO_OFFSET ATH_UNSUPPORTED_REG_OFFSET
#define CLOCK_GPIO_BT_CLK_OUT_EN_LSB 0
#define CLOCK_GPIO_BT_CLK_OUT_EN_MASK 0
#endif

#if !defined(WLAN_MAC_BASE_ADDRESS)
#define WLAN_MAC_BASE_ADDRESS        ATH_UNSUPPORTED_REG_OFFSET
#endif

#if !defined(CE0_BASE_ADDRESS)
#define CE0_BASE_ADDRESS ATH_UNSUPPORTED_REG_OFFSET
#define CE1_BASE_ADDRESS ATH_UNSUPPORTED_REG_OFFSET
#define CE_COUNT 0
#endif

#if !defined(MSI_NUM_REQUEST)
#define MSI_NUM_REQUEST              0
#define MSI_ASSIGN_FW                0
#define MSI_ASSIGN_CE_INITIAL        0
#endif

#if !defined(FW_INDICATOR_ADDRESS)
#define FW_INDICATOR_ADDRESS     ATH_UNSUPPORTED_REG_OFFSET
#endif

#if !defined(FW_CPU_PLL_CONFIG)
#define FW_CPU_PLL_CONFIG     ATH_UNSUPPORTED_REG_OFFSET
#endif

#if !defined(DRAM_BASE_ADDRESS)
#define DRAM_BASE_ADDRESS            ATH_UNSUPPORTED_REG_OFFSET
#endif

#if !defined(SOC_CORE_BASE_ADDRESS)
#define SOC_CORE_BASE_ADDRESS        ATH_UNSUPPORTED_REG_OFFSET
#endif

#if !defined(CPU_INTR_ADDRESS)
#define CPU_INTR_ADDRESS        ATH_UNSUPPORTED_REG_OFFSET
#endif

#if !defined(SOC_LF_TIMER_CONTROL0_ADDRESS)
#define SOC_LF_TIMER_CONTROL0_ADDRESS        ATH_UNSUPPORTED_REG_OFFSET
#define SOC_LF_TIMER_CONTROL0_ENABLE_MASK        ATH_UNSUPPORTED_REG_OFFSET
#endif

#if !defined(SOC_RESET_CONTROL_ADDRESS)
#define SOC_RESET_CONTROL_ADDRESS    ATH_UNSUPPORTED_REG_OFFSET
#define SOC_RESET_CONTROL_CE_RST_MASK    ATH_UNSUPPORTED_REG_OFFSET
#define SOC_RESET_CONTROL_CPU_WARM_RST_MASK    ATH_UNSUPPORTED_REG_OFFSET
#endif

#if !defined(CORE_CTRL_ADDRESS)
#define CORE_CTRL_ADDRESS            ATH_UNSUPPORTED_REG_OFFSET
#define CORE_CTRL_CPU_INTR_MASK      0
#endif

#if !defined(PCIE_INTR_ENABLE_ADDRESS)
#define PCIE_INTR_ENABLE_ADDRESS     ATH_UNSUPPORTED_REG_OFFSET
#define PCIE_INTR_CLR_ADDRESS        ATH_UNSUPPORTED_REG_OFFSET
#define PCIE_INTR_FIRMWARE_MASK      ATH_UNSUPPORTED_REG_OFFSET
#define PCIE_INTR_CE_MASK_ALL        ATH_UNSUPPORTED_REG_OFFSET
#define PCIE_INTR_CAUSE_ADDRESS      ATH_UNSUPPORTED_REG_OFFSET
#endif

#if !defined(WIFICMN_PCIE_BAR_REG_ADDRESS)
#define WIFICMN_PCIE_BAR_REG_ADDRESS    ATH_UNSUPPORTED_REG_OFFSET
#endif

#if !defined(WIFICMN_INT_STATUS_ADDRESS)
#define WIFICMN_INT_STATUS_ADDRESS    ATH_UNSUPPORTED_REG_OFFSET
#endif

#if !defined(FW_AXI_MSI_ADDR)
#define FW_AXI_MSI_ADDR    ATH_UNSUPPORTED_REG_OFFSET
#endif

#if !defined(FW_AXI_MSI_DATA)
#define FW_AXI_MSI_DATA    ATH_UNSUPPORTED_REG_OFFSET
#endif

#if !defined(WLAN_SUBSYSTEM_CORE_ID_ADDRESS)
#define WLAN_SUBSYSTEM_CORE_ID_ADDRESS ATH_UNSUPPORTED_REG_OFFSET
#endif

#if !defined(FPGA_VERSION_ADDRESS)
#define FPGA_VERSION_ADDRESS    ATH_UNSUPPORTED_REG_OFFSET
#endif

#if !defined(SI_CONFIG_ADDRESS)
#define SI_CONFIG_ADDRESS ATH_UNSUPPORTED_REG_OFFSET
#define SI_CONFIG_BIDIR_OD_DATA_LSB 0
#define SI_CONFIG_BIDIR_OD_DATA_MASK 0
#define SI_CONFIG_I2C_LSB 0
#define SI_CONFIG_I2C_MASK 0
#define SI_CONFIG_POS_SAMPLE_LSB 0
#define SI_CONFIG_POS_SAMPLE_MASK 0
#define SI_CONFIG_INACTIVE_CLK_LSB 0
#define SI_CONFIG_INACTIVE_CLK_MASK 0
#define SI_CONFIG_INACTIVE_DATA_LSB 0
#define SI_CONFIG_INACTIVE_DATA_MASK 0
#define SI_CONFIG_DIVIDER_LSB 0
#define SI_CONFIG_DIVIDER_MASK 0
#define SI_CONFIG_OFFSET 0
#define SI_TX_DATA0_OFFSET ATH_UNSUPPORTED_REG_OFFSET
#define SI_TX_DATA1_OFFSET ATH_UNSUPPORTED_REG_OFFSET
#define SI_RX_DATA0_OFFSET ATH_UNSUPPORTED_REG_OFFSET
#define SI_RX_DATA1_OFFSET ATH_UNSUPPORTED_REG_OFFSET
#define SI_CS_OFFSET ATH_UNSUPPORTED_REG_OFFSET
#define SI_CS_DONE_ERR_MASK 0
#define SI_CS_DONE_INT_MASK 0
#define SI_CS_START_LSB 0
#define SI_CS_START_MASK 0
#define SI_CS_RX_CNT_LSB 0
#define SI_CS_RX_CNT_MASK 0
#define SI_CS_TX_CNT_LSB 0
#define SI_CS_TX_CNT_MASK 0
#endif

#ifndef SI_BASE_ADDRESS
#define SI_BASE_ADDRESS ATH_UNSUPPORTED_REG_OFFSET
#endif

#ifndef WLAN_GPIO_PIN10_ADDRESS
#define WLAN_GPIO_PIN10_ADDRESS ATH_UNSUPPORTED_REG_OFFSET
#endif

#ifndef WLAN_GPIO_PIN11_ADDRESS
#define WLAN_GPIO_PIN11_ADDRESS ATH_UNSUPPORTED_REG_OFFSET
#endif

#ifndef WLAN_GPIO_PIN12_ADDRESS
#define WLAN_GPIO_PIN12_ADDRESS ATH_UNSUPPORTED_REG_OFFSET
#endif

#ifndef WLAN_GPIO_PIN13_ADDRESS
#define WLAN_GPIO_PIN13_ADDRESS ATH_UNSUPPORTED_REG_OFFSET
#endif

#ifndef WIFICMN_INT_STATUS_ADDRESS
#define WIFICMN_INT_STATUS_ADDRESS  ATH_UNSUPPORTED_REG_OFFSET
#endif

static struct targetdef_s my_target_def = {
	.d_RTC_SOC_BASE_ADDRESS = RTC_SOC_BASE_ADDRESS,
	.d_RTC_WMAC_BASE_ADDRESS = RTC_WMAC_BASE_ADDRESS,
	.d_SYSTEM_SLEEP_OFFSET = WLAN_SYSTEM_SLEEP_OFFSET,
	.d_WLAN_SYSTEM_SLEEP_OFFSET = WLAN_SYSTEM_SLEEP_OFFSET,
	.d_WLAN_SYSTEM_SLEEP_DISABLE_LSB = WLAN_SYSTEM_SLEEP_DISABLE_LSB,
	.d_WLAN_SYSTEM_SLEEP_DISABLE_MASK = WLAN_SYSTEM_SLEEP_DISABLE_MASK,
	.d_CLOCK_CONTROL_OFFSET = CLOCK_CONTROL_OFFSET,
	.d_CLOCK_CONTROL_SI0_CLK_MASK = CLOCK_CONTROL_SI0_CLK_MASK,
	.d_RESET_CONTROL_OFFSET = SOC_RESET_CONTROL_OFFSET,
	.d_RESET_CONTROL_SI0_RST_MASK = RESET_CONTROL_SI0_RST_MASK,
	.d_WLAN_RESET_CONTROL_OFFSET = WLAN_RESET_CONTROL_OFFSET,
	.d_WLAN_RESET_CONTROL_COLD_RST_MASK = WLAN_RESET_CONTROL_COLD_RST_MASK,
	.d_WLAN_RESET_CONTROL_WARM_RST_MASK = WLAN_RESET_CONTROL_WARM_RST_MASK,
	.d_GPIO_BASE_ADDRESS = GPIO_BASE_ADDRESS,
	.d_GPIO_PIN0_OFFSET = GPIO_PIN0_OFFSET,
	.d_GPIO_PIN1_OFFSET = GPIO_PIN1_OFFSET,
	.d_GPIO_PIN0_CONFIG_MASK = GPIO_PIN0_CONFIG_MASK,
	.d_GPIO_PIN1_CONFIG_MASK = GPIO_PIN1_CONFIG_MASK,
	.d_SI_CONFIG_BIDIR_OD_DATA_LSB = SI_CONFIG_BIDIR_OD_DATA_LSB,
	.d_SI_CONFIG_BIDIR_OD_DATA_MASK = SI_CONFIG_BIDIR_OD_DATA_MASK,
	.d_SI_CONFIG_I2C_LSB = SI_CONFIG_I2C_LSB,
	.d_SI_CONFIG_I2C_MASK = SI_CONFIG_I2C_MASK,
	.d_SI_CONFIG_POS_SAMPLE_LSB = SI_CONFIG_POS_SAMPLE_LSB,
	.d_SI_CONFIG_POS_SAMPLE_MASK = SI_CONFIG_POS_SAMPLE_MASK,
	.d_SI_CONFIG_INACTIVE_CLK_LSB = SI_CONFIG_INACTIVE_CLK_LSB,
	.d_SI_CONFIG_INACTIVE_CLK_MASK = SI_CONFIG_INACTIVE_CLK_MASK,
	.d_SI_CONFIG_INACTIVE_DATA_LSB = SI_CONFIG_INACTIVE_DATA_LSB,
	.d_SI_CONFIG_INACTIVE_DATA_MASK = SI_CONFIG_INACTIVE_DATA_MASK,
	.d_SI_CONFIG_DIVIDER_LSB = SI_CONFIG_DIVIDER_LSB,
	.d_SI_CONFIG_DIVIDER_MASK = SI_CONFIG_DIVIDER_MASK,
	.d_SI_BASE_ADDRESS = SI_BASE_ADDRESS,
	.d_SI_CONFIG_OFFSET = SI_CONFIG_OFFSET,
	.d_SI_TX_DATA0_OFFSET = SI_TX_DATA0_OFFSET,
	.d_SI_TX_DATA1_OFFSET = SI_TX_DATA1_OFFSET,
	.d_SI_RX_DATA0_OFFSET = SI_RX_DATA0_OFFSET,
	.d_SI_RX_DATA1_OFFSET = SI_RX_DATA1_OFFSET,
	.d_SI_CS_OFFSET = SI_CS_OFFSET,
	.d_SI_CS_DONE_ERR_MASK = SI_CS_DONE_ERR_MASK,
	.d_SI_CS_DONE_INT_MASK = SI_CS_DONE_INT_MASK,
	.d_SI_CS_START_LSB = SI_CS_START_LSB,
	.d_SI_CS_START_MASK = SI_CS_START_MASK,
	.d_SI_CS_RX_CNT_LSB = SI_CS_RX_CNT_LSB,
	.d_SI_CS_RX_CNT_MASK = SI_CS_RX_CNT_MASK,
	.d_SI_CS_TX_CNT_LSB = SI_CS_TX_CNT_LSB,
	.d_SI_CS_TX_CNT_MASK = SI_CS_TX_CNT_MASK,
	.d_BOARD_DATA_SZ = MY_TARGET_BOARD_DATA_SZ,
	.d_BOARD_EXT_DATA_SZ = MY_TARGET_BOARD_EXT_DATA_SZ,
	.d_MBOX_BASE_ADDRESS = MBOX_BASE_ADDRESS,
	.d_LOCAL_SCRATCH_OFFSET = LOCAL_SCRATCH_OFFSET,
	.d_CPU_CLOCK_OFFSET = CPU_CLOCK_OFFSET,
	.d_GPIO_PIN10_OFFSET = GPIO_PIN10_OFFSET,
	.d_GPIO_PIN11_OFFSET = GPIO_PIN11_OFFSET,
	.d_GPIO_PIN12_OFFSET = GPIO_PIN12_OFFSET,
	.d_GPIO_PIN13_OFFSET = GPIO_PIN13_OFFSET,
	.d_CLOCK_GPIO_OFFSET = CLOCK_GPIO_OFFSET,
	.d_CPU_CLOCK_STANDARD_LSB = CPU_CLOCK_STANDARD_LSB,
	.d_CPU_CLOCK_STANDARD_MASK = CPU_CLOCK_STANDARD_MASK,
	.d_LPO_CAL_ENABLE_LSB = LPO_CAL_ENABLE_LSB,
	.d_LPO_CAL_ENABLE_MASK = LPO_CAL_ENABLE_MASK,
	.d_CLOCK_GPIO_BT_CLK_OUT_EN_LSB = CLOCK_GPIO_BT_CLK_OUT_EN_LSB,
	.d_CLOCK_GPIO_BT_CLK_OUT_EN_MASK = CLOCK_GPIO_BT_CLK_OUT_EN_MASK,
	.d_ANALOG_INTF_BASE_ADDRESS = ANALOG_INTF_BASE_ADDRESS,
	.d_WLAN_MAC_BASE_ADDRESS = WLAN_MAC_BASE_ADDRESS,
	.d_CE0_BASE_ADDRESS = CE0_BASE_ADDRESS,
	.d_CE1_BASE_ADDRESS = CE1_BASE_ADDRESS,
	.d_FW_INDICATOR_ADDRESS = FW_INDICATOR_ADDRESS,
	.d_FW_CPU_PLL_CONFIG = FW_CPU_PLL_CONFIG,
	.d_DRAM_BASE_ADDRESS = DRAM_BASE_ADDRESS,
	.d_SOC_CORE_BASE_ADDRESS = SOC_CORE_BASE_ADDRESS,
	.d_CORE_CTRL_ADDRESS = CORE_CTRL_ADDRESS,
	.d_CE_COUNT = CE_COUNT,
	.d_MSI_NUM_REQUEST = MSI_NUM_REQUEST,
	.d_MSI_ASSIGN_FW = MSI_ASSIGN_FW,
	.d_MSI_ASSIGN_CE_INITIAL = MSI_ASSIGN_CE_INITIAL,
	.d_PCIE_INTR_ENABLE_ADDRESS = PCIE_INTR_ENABLE_ADDRESS,
	.d_PCIE_INTR_CLR_ADDRESS = PCIE_INTR_CLR_ADDRESS,
	.d_PCIE_INTR_FIRMWARE_MASK = PCIE_INTR_FIRMWARE_MASK,
	.d_PCIE_INTR_CE_MASK_ALL = PCIE_INTR_CE_MASK_ALL,
	.d_CORE_CTRL_CPU_INTR_MASK = CORE_CTRL_CPU_INTR_MASK,
	.d_WIFICMN_PCIE_BAR_REG_ADDRESS = WIFICMN_PCIE_BAR_REG_ADDRESS,
	/* htt_rx.c */
	/* htt tx */
	.d_MSDU_LINK_EXT_3_TCP_OVER_IPV4_CHECKSUM_EN_MASK
		= MSDU_LINK_EXT_3_TCP_OVER_IPV4_CHECKSUM_EN_MASK,
	.d_MSDU_LINK_EXT_3_TCP_OVER_IPV6_CHECKSUM_EN_MASK
		= MSDU_LINK_EXT_3_TCP_OVER_IPV6_CHECKSUM_EN_MASK,
	.d_MSDU_LINK_EXT_3_UDP_OVER_IPV4_CHECKSUM_EN_MASK
		= MSDU_LINK_EXT_3_UDP_OVER_IPV4_CHECKSUM_EN_MASK,
	.d_MSDU_LINK_EXT_3_UDP_OVER_IPV6_CHECKSUM_EN_MASK
		= MSDU_LINK_EXT_3_UDP_OVER_IPV6_CHECKSUM_EN_MASK,
	.d_MSDU_LINK_EXT_3_TCP_OVER_IPV4_CHECKSUM_EN_LSB
		= MSDU_LINK_EXT_3_TCP_OVER_IPV4_CHECKSUM_EN_LSB,
	.d_MSDU_LINK_EXT_3_TCP_OVER_IPV6_CHECKSUM_EN_LSB
		= MSDU_LINK_EXT_3_TCP_OVER_IPV6_CHECKSUM_EN_LSB,
	.d_MSDU_LINK_EXT_3_UDP_OVER_IPV4_CHECKSUM_EN_LSB
		= MSDU_LINK_EXT_3_UDP_OVER_IPV4_CHECKSUM_EN_LSB,
	.d_MSDU_LINK_EXT_3_UDP_OVER_IPV6_CHECKSUM_EN_LSB
		= MSDU_LINK_EXT_3_UDP_OVER_IPV6_CHECKSUM_EN_LSB,
	/* copy_engine.c  */
	.d_DST_WR_INDEX_ADDRESS = DST_WR_INDEX_ADDRESS,
	.d_SRC_WATERMARK_ADDRESS = SRC_WATERMARK_ADDRESS,
	.d_SRC_WATERMARK_LOW_MASK = SRC_WATERMARK_LOW_MASK,
	.d_SRC_WATERMARK_HIGH_MASK = SRC_WATERMARK_HIGH_MASK,
	.d_DST_WATERMARK_LOW_MASK = DST_WATERMARK_LOW_MASK,
	.d_DST_WATERMARK_HIGH_MASK = DST_WATERMARK_HIGH_MASK,
	.d_CURRENT_SRRI_ADDRESS = CURRENT_SRRI_ADDRESS,
	.d_CURRENT_DRRI_ADDRESS = CURRENT_DRRI_ADDRESS,
	.d_HOST_IS_SRC_RING_HIGH_WATERMARK_MASK
		= HOST_IS_SRC_RING_HIGH_WATERMARK_MASK,
	.d_HOST_IS_SRC_RING_LOW_WATERMARK_MASK
		= HOST_IS_SRC_RING_LOW_WATERMARK_MASK,
	.d_HOST_IS_DST_RING_HIGH_WATERMARK_MASK
		= HOST_IS_DST_RING_HIGH_WATERMARK_MASK,
	.d_HOST_IS_DST_RING_LOW_WATERMARK_MASK
		= HOST_IS_DST_RING_LOW_WATERMARK_MASK,
	.d_HOST_IS_ADDRESS = HOST_IS_ADDRESS,
	.d_HOST_IS_COPY_COMPLETE_MASK = HOST_IS_COPY_COMPLETE_MASK,
	.d_CE_CMD_ADDRESS = CE_CMD_ADDRESS,
	.d_CE_CMD_HALT_MASK = CE_CMD_HALT_MASK,
	.d_CE_WRAPPER_BASE_ADDRESS = CE_WRAPPER_BASE_ADDRESS,
	.d_CE_WRAPPER_INTERRUPT_SUMMARY_ADDRESS
		= CE_WRAPPER_INTERRUPT_SUMMARY_ADDRESS,
	.d_HOST_IE_ADDRESS = HOST_IE_ADDRESS,
	.d_HOST_IE_COPY_COMPLETE_MASK = HOST_IE_COPY_COMPLETE_MASK,
	.d_SR_BA_ADDRESS = SR_BA_ADDRESS,
	.d_SR_SIZE_ADDRESS = SR_SIZE_ADDRESS,
	.d_CE_CTRL1_ADDRESS = CE_CTRL1_ADDRESS,
	.d_CE_CTRL1_DMAX_LENGTH_MASK = CE_CTRL1_DMAX_LENGTH_MASK,
	.d_DR_BA_ADDRESS = DR_BA_ADDRESS,
	.d_DR_SIZE_ADDRESS = DR_SIZE_ADDRESS,
	.d_MISC_IE_ADDRESS = MISC_IE_ADDRESS,
	.d_MISC_IS_AXI_ERR_MASK = MISC_IS_AXI_ERR_MASK,
	.d_MISC_IS_DST_ADDR_ERR_MASK = MISC_IS_DST_ADDR_ERR_MASK,
	.d_MISC_IS_SRC_LEN_ERR_MASK = MISC_IS_SRC_LEN_ERR_MASK,
	.d_MISC_IS_DST_MAX_LEN_VIO_MASK = MISC_IS_DST_MAX_LEN_VIO_MASK,
	.d_MISC_IS_DST_RING_OVERFLOW_MASK = MISC_IS_DST_RING_OVERFLOW_MASK,
	.d_MISC_IS_SRC_RING_OVERFLOW_MASK = MISC_IS_SRC_RING_OVERFLOW_MASK,
	.d_SRC_WATERMARK_LOW_LSB = SRC_WATERMARK_LOW_LSB,
	.d_SRC_WATERMARK_HIGH_LSB = SRC_WATERMARK_HIGH_LSB,
	.d_DST_WATERMARK_LOW_LSB = DST_WATERMARK_LOW_LSB,
	.d_DST_WATERMARK_HIGH_LSB = DST_WATERMARK_HIGH_LSB,
	.d_CE_WRAPPER_INTERRUPT_SUMMARY_HOST_MSI_MASK
		= CE_WRAPPER_INTERRUPT_SUMMARY_HOST_MSI_MASK,
	.d_CE_WRAPPER_INTERRUPT_SUMMARY_HOST_MSI_LSB
		= CE_WRAPPER_INTERRUPT_SUMMARY_HOST_MSI_LSB,
	.d_CE_CTRL1_DMAX_LENGTH_LSB = CE_CTRL1_DMAX_LENGTH_LSB,
	.d_CE_CTRL1_SRC_RING_BYTE_SWAP_EN_MASK
		= CE_CTRL1_SRC_RING_BYTE_SWAP_EN_MASK,
	.d_CE_CTRL1_DST_RING_BYTE_SWAP_EN_MASK
		= CE_CTRL1_DST_RING_BYTE_SWAP_EN_MASK,
	.d_CE_CTRL1_SRC_RING_BYTE_SWAP_EN_LSB
		= CE_CTRL1_SRC_RING_BYTE_SWAP_EN_LSB,
	.d_CE_CTRL1_DST_RING_BYTE_SWAP_EN_LSB
		= CE_CTRL1_DST_RING_BYTE_SWAP_EN_LSB,
	.d_CE_CMD_HALT_STATUS_MASK = CE_CMD_HALT_STATUS_MASK,
	.d_CE_CMD_HALT_STATUS_LSB = CE_CMD_HALT_STATUS_LSB,
	.d_SR_WR_INDEX_ADDRESS = SR_WR_INDEX_ADDRESS,
	.d_DST_WATERMARK_ADDRESS = DST_WATERMARK_ADDRESS,
	.d_PCIE_INTR_CAUSE_ADDRESS = PCIE_INTR_CAUSE_ADDRESS,
	.d_SOC_RESET_CONTROL_ADDRESS = SOC_RESET_CONTROL_ADDRESS,
	.d_SOC_RESET_CONTROL_CE_RST_MASK = SOC_RESET_CONTROL_CE_RST_MASK,
	.d_SOC_RESET_CONTROL_CPU_WARM_RST_MASK
		= SOC_RESET_CONTROL_CPU_WARM_RST_MASK,
	.d_CPU_INTR_ADDRESS = CPU_INTR_ADDRESS,
	.d_SOC_LF_TIMER_CONTROL0_ADDRESS = SOC_LF_TIMER_CONTROL0_ADDRESS,
	.d_SOC_LF_TIMER_CONTROL0_ENABLE_MASK
		= SOC_LF_TIMER_CONTROL0_ENABLE_MASK,
	.d_SI_CONFIG_ERR_INT_MASK = SI_CONFIG_ERR_INT_MASK,
	.d_SI_CONFIG_ERR_INT_LSB = SI_CONFIG_ERR_INT_LSB,
	.d_GPIO_ENABLE_W1TS_LOW_ADDRESS = GPIO_ENABLE_W1TS_LOW_ADDRESS,
	.d_GPIO_PIN0_CONFIG_LSB = GPIO_PIN0_CONFIG_LSB,
	.d_GPIO_PIN0_PAD_PULL_LSB = GPIO_PIN0_PAD_PULL_LSB,
	.d_GPIO_PIN0_PAD_PULL_MASK = GPIO_PIN0_PAD_PULL_MASK,
	.d_SOC_CHIP_ID_ADDRESS = SOC_CHIP_ID_ADDRESS,
	.d_SOC_CHIP_ID_REVISION_MASK = SOC_CHIP_ID_REVISION_MASK,
	.d_SOC_CHIP_ID_REVISION_LSB = SOC_CHIP_ID_REVISION_LSB,
	.d_SOC_CHIP_ID_REVISION_MSB = SOC_CHIP_ID_REVISION_MSB,
	.d_WIFICMN_PCIE_BAR_REG_ADDRESS = WIFICMN_PCIE_BAR_REG_ADDRESS,
	.d_FW_AXI_MSI_ADDR = FW_AXI_MSI_ADDR,
	.d_FW_AXI_MSI_DATA = FW_AXI_MSI_DATA,
	.d_WLAN_SUBSYSTEM_CORE_ID_ADDRESS = WLAN_SUBSYSTEM_CORE_ID_ADDRESS,
	.d_FPGA_VERSION_ADDRESS = FPGA_VERSION_ADDRESS,
	.d_WIFICMN_INT_STATUS_ADDRESS = WIFICMN_INT_STATUS_ADDRESS,
};

struct targetdef_s *MY_TARGET_DEF = &my_target_def;
#else
#endif

#if defined(MY_CEREG_DEF)

#if !defined(CE_DDR_ADDRESS_FOR_RRI_LOW)
#define CE_DDR_ADDRESS_FOR_RRI_LOW  ATH_UNSUPPORTED_REG_OFFSET
#endif
#if !defined(CE_DDR_ADDRESS_FOR_RRI_HIGH)
#define CE_DDR_ADDRESS_FOR_RRI_HIGH ATH_UNSUPPORTED_REG_OFFSET
#endif
#if !defined(SR_BA_ADDRESS_HIGH)
#define SR_BA_ADDRESS_HIGH ATH_UNSUPPORTED_REG_OFFSET
#endif
#if !defined(DR_BA_ADDRESS_HIGH)
#define DR_BA_ADDRESS_HIGH ATH_UNSUPPORTED_REG_OFFSET
#endif
#if !defined(CE_CMD_REGISTER)
#define CE_CMD_REGISTER ATH_UNSUPPORTED_REG_OFFSET
#endif
#if !defined(CE_MSI_ADDRESS)
#define CE_MSI_ADDRESS ATH_UNSUPPORTED_REG_OFFSET
#endif
#if !defined(CE_MSI_ADDRESS_HIGH)
#define CE_MSI_ADDRESS_HIGH ATH_UNSUPPORTED_REG_OFFSET
#endif
#if !defined(CE_MSI_DATA)
#define CE_MSI_DATA ATH_UNSUPPORTED_REG_OFFSET
#endif
#if !defined(CE_MSI_ENABLE_BIT)
#define CE_MSI_ENABLE_BIT ATH_UNSUPPORTED_REG_OFFSET
#endif
#if !defined(CE_CTRL1_IDX_UPD_EN_MASK)
#define CE_CTRL1_IDX_UPD_EN_MASK ATH_UNSUPPORTED_REG_OFFSET
#endif
#if !defined(CE_WRAPPER_DEBUG_OFFSET)
#define CE_WRAPPER_DEBUG_OFFSET ATH_UNSUPPORTED_REG_OFFSET
#endif
#if !defined(CE_DEBUG_OFFSET)
#define CE_DEBUG_OFFSET ATH_UNSUPPORTED_REG_OFFSET
#endif
#if !defined(A_WIFI_APB_3_A_WCMN_APPS_CE_INTR_ENABLES)
#define A_WIFI_APB_3_A_WCMN_APPS_CE_INTR_ENABLES ATH_UNSUPPORTED_REG_OFFSET
#endif
#if !defined(A_WIFI_APB_3_A_WCMN_APPS_CE_INTR_STATUS)
#define A_WIFI_APB_3_A_WCMN_APPS_CE_INTR_STATUS ATH_UNSUPPORTED_REG_OFFSET
#endif

static struct ce_reg_def my_ce_reg_def = {
	/* copy_engine.c */
	.d_DST_WR_INDEX_ADDRESS = DST_WR_INDEX_ADDRESS,
	.d_SRC_WATERMARK_ADDRESS = SRC_WATERMARK_ADDRESS,
	.d_SRC_WATERMARK_LOW_MASK = SRC_WATERMARK_LOW_MASK,
	.d_SRC_WATERMARK_HIGH_MASK = SRC_WATERMARK_HIGH_MASK,
	.d_DST_WATERMARK_LOW_MASK = DST_WATERMARK_LOW_MASK,
	.d_DST_WATERMARK_HIGH_MASK = DST_WATERMARK_HIGH_MASK,
	.d_CURRENT_SRRI_ADDRESS = CURRENT_SRRI_ADDRESS,
	.d_CURRENT_DRRI_ADDRESS = CURRENT_DRRI_ADDRESS,
	.d_HOST_IS_SRC_RING_HIGH_WATERMARK_MASK
		= HOST_IS_SRC_RING_HIGH_WATERMARK_MASK,
	.d_HOST_IS_SRC_RING_LOW_WATERMARK_MASK
		= HOST_IS_SRC_RING_LOW_WATERMARK_MASK,
	.d_HOST_IS_DST_RING_HIGH_WATERMARK_MASK
		= HOST_IS_DST_RING_HIGH_WATERMARK_MASK,
	.d_HOST_IS_DST_RING_LOW_WATERMARK_MASK
		= HOST_IS_DST_RING_LOW_WATERMARK_MASK,
	.d_HOST_IS_ADDRESS = HOST_IS_ADDRESS,
	.d_MISC_IS_ADDRESS = MISC_IS_ADDRESS,
	.d_HOST_IS_COPY_COMPLETE_MASK = HOST_IS_COPY_COMPLETE_MASK,
	.d_CE_WRAPPER_BASE_ADDRESS = CE_WRAPPER_BASE_ADDRESS,
	.d_CE_WRAPPER_INTERRUPT_SUMMARY_ADDRESS
		= CE_WRAPPER_INTERRUPT_SUMMARY_ADDRESS,
	.d_CE_DDR_ADDRESS_FOR_RRI_LOW = CE_DDR_ADDRESS_FOR_RRI_LOW,
	.d_CE_DDR_ADDRESS_FOR_RRI_HIGH = CE_DDR_ADDRESS_FOR_RRI_HIGH,
	.d_HOST_IE_ADDRESS = HOST_IE_ADDRESS,
	.d_HOST_IE_COPY_COMPLETE_MASK = HOST_IE_COPY_COMPLETE_MASK,
	.d_SR_BA_ADDRESS = SR_BA_ADDRESS,
	.d_SR_BA_ADDRESS_HIGH = SR_BA_ADDRESS_HIGH,
	.d_SR_SIZE_ADDRESS = SR_SIZE_ADDRESS,
	.d_CE_CTRL1_ADDRESS = CE_CTRL1_ADDRESS,
	.d_CE_CTRL1_DMAX_LENGTH_MASK = CE_CTRL1_DMAX_LENGTH_MASK,
	.d_DR_BA_ADDRESS = DR_BA_ADDRESS,
	.d_DR_BA_ADDRESS_HIGH = DR_BA_ADDRESS_HIGH,
	.d_DR_SIZE_ADDRESS = DR_SIZE_ADDRESS,
	.d_CE_CMD_REGISTER = CE_CMD_REGISTER,
	.d_CE_MSI_ADDRESS = CE_MSI_ADDRESS,
	.d_CE_MSI_ADDRESS_HIGH = CE_MSI_ADDRESS_HIGH,
	.d_CE_MSI_DATA = CE_MSI_DATA,
	.d_CE_MSI_ENABLE_BIT = CE_MSI_ENABLE_BIT,
	.d_MISC_IE_ADDRESS = MISC_IE_ADDRESS,
	.d_MISC_IS_AXI_ERR_MASK = MISC_IS_AXI_ERR_MASK,
	.d_MISC_IS_DST_ADDR_ERR_MASK = MISC_IS_DST_ADDR_ERR_MASK,
	.d_MISC_IS_SRC_LEN_ERR_MASK = MISC_IS_SRC_LEN_ERR_MASK,
	.d_MISC_IS_DST_MAX_LEN_VIO_MASK = MISC_IS_DST_MAX_LEN_VIO_MASK,
	.d_MISC_IS_DST_RING_OVERFLOW_MASK = MISC_IS_DST_RING_OVERFLOW_MASK,
	.d_MISC_IS_SRC_RING_OVERFLOW_MASK = MISC_IS_SRC_RING_OVERFLOW_MASK,
	.d_SRC_WATERMARK_LOW_LSB = SRC_WATERMARK_LOW_LSB,
	.d_SRC_WATERMARK_HIGH_LSB = SRC_WATERMARK_HIGH_LSB,
	.d_DST_WATERMARK_LOW_LSB = DST_WATERMARK_LOW_LSB,
	.d_DST_WATERMARK_HIGH_LSB = DST_WATERMARK_HIGH_LSB,
	.d_CE_WRAPPER_INTERRUPT_SUMMARY_HOST_MSI_MASK
		= CE_WRAPPER_INTERRUPT_SUMMARY_HOST_MSI_MASK,
	.d_CE_WRAPPER_INTERRUPT_SUMMARY_HOST_MSI_LSB
		= CE_WRAPPER_INTERRUPT_SUMMARY_HOST_MSI_LSB,
	.d_CE_CTRL1_DMAX_LENGTH_LSB = CE_CTRL1_DMAX_LENGTH_LSB,
	.d_CE_CTRL1_SRC_RING_BYTE_SWAP_EN_MASK
		= CE_CTRL1_SRC_RING_BYTE_SWAP_EN_MASK,
	.d_CE_CTRL1_DST_RING_BYTE_SWAP_EN_MASK
		= CE_CTRL1_DST_RING_BYTE_SWAP_EN_MASK,
	.d_CE_CTRL1_SRC_RING_BYTE_SWAP_EN_LSB
		= CE_CTRL1_SRC_RING_BYTE_SWAP_EN_LSB,
	.d_CE_CTRL1_DST_RING_BYTE_SWAP_EN_LSB
		= CE_CTRL1_DST_RING_BYTE_SWAP_EN_LSB,
	.d_CE_CTRL1_IDX_UPD_EN_MASK = CE_CTRL1_IDX_UPD_EN_MASK,
	.d_CE_WRAPPER_DEBUG_OFFSET = CE_WRAPPER_DEBUG_OFFSET,
	.d_CE_WRAPPER_DEBUG_SEL_MSB = CE_WRAPPER_DEBUG_SEL_MSB,
	.d_CE_WRAPPER_DEBUG_SEL_LSB = CE_WRAPPER_DEBUG_SEL_LSB,
	.d_CE_WRAPPER_DEBUG_SEL_MASK = CE_WRAPPER_DEBUG_SEL_MASK,
	.d_CE_DEBUG_OFFSET = CE_DEBUG_OFFSET,
	.d_CE_DEBUG_SEL_MSB = CE_DEBUG_SEL_MSB,
	.d_CE_DEBUG_SEL_LSB = CE_DEBUG_SEL_LSB,
	.d_CE_DEBUG_SEL_MASK = CE_DEBUG_SEL_MASK,
	.d_CE0_BASE_ADDRESS = CE0_BASE_ADDRESS,
	.d_CE1_BASE_ADDRESS = CE1_BASE_ADDRESS,
	.d_A_WIFI_APB_3_A_WCMN_APPS_CE_INTR_ENABLES
		= A_WIFI_APB_3_A_WCMN_APPS_CE_INTR_ENABLES,
	.d_A_WIFI_APB_3_A_WCMN_APPS_CE_INTR_STATUS
		= A_WIFI_APB_3_A_WCMN_APPS_CE_INTR_STATUS
};

struct ce_reg_def *MY_CEREG_DEF = &my_ce_reg_def;

#else
#endif
#endif
