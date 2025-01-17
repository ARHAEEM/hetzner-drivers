/******************************************************************************

  Copyright (c) 2013-2019, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   3. Neither the name of the Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/
/*$FreeBSD$*/
#include "iavf_type.h"
#include "iavf_adminq.h"
#include "iavf_prototype.h"
#include "virtchnl.h"

/**
 * iavf_set_mac_type - Sets MAC type
 * @hw: pointer to the HW structure
 *
 * This function sets the mac type of the adapter based on the
 * vendor ID and device ID stored in the hw structure.
 **/
enum iavf_status iavf_set_mac_type(struct iavf_hw *hw)
{
	enum iavf_status status = IAVF_SUCCESS;

	DEBUGFUNC("iavf_set_mac_type\n");

	if (hw->vendor_id == IAVF_INTEL_VENDOR_ID) {
		switch (hw->device_id) {
		case IAVF_DEV_ID_X722_VF:
			hw->mac.type = IAVF_MAC_X722_VF;
			break;
		case IAVF_DEV_ID_VF:
		case IAVF_DEV_ID_VF_HV:
		case IAVF_DEV_ID_ADAPTIVE_VF:
		case IAVF_DEV_ID_VDEV:
			hw->mac.type = IAVF_MAC_VF;
			break;
		default:
			hw->mac.type = IAVF_MAC_GENERIC;
			break;
		}
	} else {
		status = IAVF_ERR_DEVICE_NOT_SUPPORTED;
	}

	DEBUGOUT2("iavf_set_mac_type found mac: %d, returns: %d\n",
		  hw->mac.type, status);
	return status;
}

/**
 * iavf_aq_str - convert AQ err code to a string
 * @hw: pointer to the HW structure
 * @aq_err: the AQ error code to convert
 **/
const char *iavf_aq_str(struct iavf_hw *hw, enum iavf_admin_queue_err aq_err)
{
	switch (aq_err) {
	case IAVF_AQ_RC_OK:
		return "OK";
	case IAVF_AQ_RC_EPERM:
		return "IAVF_AQ_RC_EPERM";
	case IAVF_AQ_RC_ENOENT:
		return "IAVF_AQ_RC_ENOENT";
	case IAVF_AQ_RC_ESRCH:
		return "IAVF_AQ_RC_ESRCH";
	case IAVF_AQ_RC_EINTR:
		return "IAVF_AQ_RC_EINTR";
	case IAVF_AQ_RC_EIO:
		return "IAVF_AQ_RC_EIO";
	case IAVF_AQ_RC_ENXIO:
		return "IAVF_AQ_RC_ENXIO";
	case IAVF_AQ_RC_E2BIG:
		return "IAVF_AQ_RC_E2BIG";
	case IAVF_AQ_RC_EAGAIN:
		return "IAVF_AQ_RC_EAGAIN";
	case IAVF_AQ_RC_ENOMEM:
		return "IAVF_AQ_RC_ENOMEM";
	case IAVF_AQ_RC_EACCES:
		return "IAVF_AQ_RC_EACCES";
	case IAVF_AQ_RC_EFAULT:
		return "IAVF_AQ_RC_EFAULT";
	case IAVF_AQ_RC_EBUSY:
		return "IAVF_AQ_RC_EBUSY";
	case IAVF_AQ_RC_EEXIST:
		return "IAVF_AQ_RC_EEXIST";
	case IAVF_AQ_RC_EINVAL:
		return "IAVF_AQ_RC_EINVAL";
	case IAVF_AQ_RC_ENOTTY:
		return "IAVF_AQ_RC_ENOTTY";
	case IAVF_AQ_RC_ENOSPC:
		return "IAVF_AQ_RC_ENOSPC";
	case IAVF_AQ_RC_ENOSYS:
		return "IAVF_AQ_RC_ENOSYS";
	case IAVF_AQ_RC_ERANGE:
		return "IAVF_AQ_RC_ERANGE";
	case IAVF_AQ_RC_EFLUSHED:
		return "IAVF_AQ_RC_EFLUSHED";
	case IAVF_AQ_RC_BAD_ADDR:
		return "IAVF_AQ_RC_BAD_ADDR";
	case IAVF_AQ_RC_EMODE:
		return "IAVF_AQ_RC_EMODE";
	case IAVF_AQ_RC_EFBIG:
		return "IAVF_AQ_RC_EFBIG";
	}

	snprintf(hw->err_str, sizeof(hw->err_str), "%d", aq_err);
	return hw->err_str;
}

/**
 * iavf_stat_str - convert status err code to a string
 * @hw: pointer to the HW structure
 * @stat_err: the status error code to convert
 **/
const char *iavf_stat_str(struct iavf_hw *hw, enum iavf_status stat_err)
{
	switch (stat_err) {
	case IAVF_SUCCESS:
		return "OK";
	case IAVF_ERR_NVM:
		return "IAVF_ERR_NVM";
	case IAVF_ERR_NVM_CHECKSUM:
		return "IAVF_ERR_NVM_CHECKSUM";
	case IAVF_ERR_PHY:
		return "IAVF_ERR_PHY";
	case IAVF_ERR_CONFIG:
		return "IAVF_ERR_CONFIG";
	case IAVF_ERR_PARAM:
		return "IAVF_ERR_PARAM";
	case IAVF_ERR_MAC_TYPE:
		return "IAVF_ERR_MAC_TYPE";
	case IAVF_ERR_UNKNOWN_PHY:
		return "IAVF_ERR_UNKNOWN_PHY";
	case IAVF_ERR_LINK_SETUP:
		return "IAVF_ERR_LINK_SETUP";
	case IAVF_ERR_ADAPTER_STOPPED:
		return "IAVF_ERR_ADAPTER_STOPPED";
	case IAVF_ERR_INVALID_MAC_ADDR:
		return "IAVF_ERR_INVALID_MAC_ADDR";
	case IAVF_ERR_DEVICE_NOT_SUPPORTED:
		return "IAVF_ERR_DEVICE_NOT_SUPPORTED";
	case IAVF_ERR_PRIMARY_REQUESTS_PENDING:
		return "IAVF_ERR_PRIMARY_REQUESTS_PENDING";
	case IAVF_ERR_INVALID_LINK_SETTINGS:
		return "IAVF_ERR_INVALID_LINK_SETTINGS";
	case IAVF_ERR_AUTONEG_NOT_COMPLETE:
		return "IAVF_ERR_AUTONEG_NOT_COMPLETE";
	case IAVF_ERR_RESET_FAILED:
		return "IAVF_ERR_RESET_FAILED";
	case IAVF_ERR_SWFW_SYNC:
		return "IAVF_ERR_SWFW_SYNC";
	case IAVF_ERR_NO_AVAILABLE_VSI:
		return "IAVF_ERR_NO_AVAILABLE_VSI";
	case IAVF_ERR_NO_MEMORY:
		return "IAVF_ERR_NO_MEMORY";
	case IAVF_ERR_BAD_PTR:
		return "IAVF_ERR_BAD_PTR";
	case IAVF_ERR_RING_FULL:
		return "IAVF_ERR_RING_FULL";
	case IAVF_ERR_INVALID_PD_ID:
		return "IAVF_ERR_INVALID_PD_ID";
	case IAVF_ERR_INVALID_QP_ID:
		return "IAVF_ERR_INVALID_QP_ID";
	case IAVF_ERR_INVALID_CQ_ID:
		return "IAVF_ERR_INVALID_CQ_ID";
	case IAVF_ERR_INVALID_CEQ_ID:
		return "IAVF_ERR_INVALID_CEQ_ID";
	case IAVF_ERR_INVALID_AEQ_ID:
		return "IAVF_ERR_INVALID_AEQ_ID";
	case IAVF_ERR_INVALID_SIZE:
		return "IAVF_ERR_INVALID_SIZE";
	case IAVF_ERR_INVALID_ARP_INDEX:
		return "IAVF_ERR_INVALID_ARP_INDEX";
	case IAVF_ERR_INVALID_FPM_FUNC_ID:
		return "IAVF_ERR_INVALID_FPM_FUNC_ID";
	case IAVF_ERR_QP_INVALID_MSG_SIZE:
		return "IAVF_ERR_QP_INVALID_MSG_SIZE";
	case IAVF_ERR_QP_TOOMANY_WRS_POSTED:
		return "IAVF_ERR_QP_TOOMANY_WRS_POSTED";
	case IAVF_ERR_INVALID_FRAG_COUNT:
		return "IAVF_ERR_INVALID_FRAG_COUNT";
	case IAVF_ERR_QUEUE_EMPTY:
		return "IAVF_ERR_QUEUE_EMPTY";
	case IAVF_ERR_INVALID_ALIGNMENT:
		return "IAVF_ERR_INVALID_ALIGNMENT";
	case IAVF_ERR_FLUSHED_QUEUE:
		return "IAVF_ERR_FLUSHED_QUEUE";
	case IAVF_ERR_INVALID_PUSH_PAGE_INDEX:
		return "IAVF_ERR_INVALID_PUSH_PAGE_INDEX";
	case IAVF_ERR_INVALID_IMM_DATA_SIZE:
		return "IAVF_ERR_INVALID_IMM_DATA_SIZE";
	case IAVF_ERR_TIMEOUT:
		return "IAVF_ERR_TIMEOUT";
	case IAVF_ERR_OPCODE_MISMATCH:
		return "IAVF_ERR_OPCODE_MISMATCH";
	case IAVF_ERR_CQP_COMPL_ERROR:
		return "IAVF_ERR_CQP_COMPL_ERROR";
	case IAVF_ERR_INVALID_VF_ID:
		return "IAVF_ERR_INVALID_VF_ID";
	case IAVF_ERR_INVALID_HMCFN_ID:
		return "IAVF_ERR_INVALID_HMCFN_ID";
	case IAVF_ERR_BACKING_PAGE_ERROR:
		return "IAVF_ERR_BACKING_PAGE_ERROR";
	case IAVF_ERR_NO_PBLCHUNKS_AVAILABLE:
		return "IAVF_ERR_NO_PBLCHUNKS_AVAILABLE";
	case IAVF_ERR_INVALID_PBLE_INDEX:
		return "IAVF_ERR_INVALID_PBLE_INDEX";
	case IAVF_ERR_INVALID_SD_INDEX:
		return "IAVF_ERR_INVALID_SD_INDEX";
	case IAVF_ERR_INVALID_PAGE_DESC_INDEX:
		return "IAVF_ERR_INVALID_PAGE_DESC_INDEX";
	case IAVF_ERR_INVALID_SD_TYPE:
		return "IAVF_ERR_INVALID_SD_TYPE";
	case IAVF_ERR_MEMCPY_FAILED:
		return "IAVF_ERR_MEMCPY_FAILED";
	case IAVF_ERR_INVALID_HMC_OBJ_INDEX:
		return "IAVF_ERR_INVALID_HMC_OBJ_INDEX";
	case IAVF_ERR_INVALID_HMC_OBJ_COUNT:
		return "IAVF_ERR_INVALID_HMC_OBJ_COUNT";
	case IAVF_ERR_INVALID_SRQ_ARM_LIMIT:
		return "IAVF_ERR_INVALID_SRQ_ARM_LIMIT";
	case IAVF_ERR_SRQ_ENABLED:
		return "IAVF_ERR_SRQ_ENABLED";
	case IAVF_ERR_ADMIN_QUEUE_ERROR:
		return "IAVF_ERR_ADMIN_QUEUE_ERROR";
	case IAVF_ERR_ADMIN_QUEUE_TIMEOUT:
		return "IAVF_ERR_ADMIN_QUEUE_TIMEOUT";
	case IAVF_ERR_BUF_TOO_SHORT:
		return "IAVF_ERR_BUF_TOO_SHORT";
	case IAVF_ERR_ADMIN_QUEUE_FULL:
		return "IAVF_ERR_ADMIN_QUEUE_FULL";
	case IAVF_ERR_ADMIN_QUEUE_NO_WORK:
		return "IAVF_ERR_ADMIN_QUEUE_NO_WORK";
	case IAVF_ERR_BAD_IWARP_CQE:
		return "IAVF_ERR_BAD_IWARP_CQE";
	case IAVF_ERR_NVM_BLANK_MODE:
		return "IAVF_ERR_NVM_BLANK_MODE";
	case IAVF_ERR_NOT_IMPLEMENTED:
		return "IAVF_ERR_NOT_IMPLEMENTED";
	case IAVF_ERR_PE_DOORBELL_NOT_ENABLED:
		return "IAVF_ERR_PE_DOORBELL_NOT_ENABLED";
	case IAVF_ERR_DIAG_TEST_FAILED:
		return "IAVF_ERR_DIAG_TEST_FAILED";
	case IAVF_ERR_NOT_READY:
		return "IAVF_ERR_NOT_READY";
	case IAVF_NOT_SUPPORTED:
		return "IAVF_NOT_SUPPORTED";
	case IAVF_ERR_FIRMWARE_API_VERSION:
		return "IAVF_ERR_FIRMWARE_API_VERSION";
	case IAVF_ERR_ADMIN_QUEUE_CRITICAL_ERROR:
		return "IAVF_ERR_ADMIN_QUEUE_CRITICAL_ERROR";
	}

	snprintf(hw->err_str, sizeof(hw->err_str), "%d", stat_err);
	return hw->err_str;
}

/**
 * iavf_debug_aq
 * @hw: debug mask related to admin queue
 * @mask: debug mask
 * @desc: pointer to admin queue descriptor
 * @buffer: pointer to command buffer
 * @buf_len: max length of buffer
 *
 * Dumps debug log about adminq command with descriptor contents.
 **/
void iavf_debug_aq(struct iavf_hw *hw, enum iavf_debug_mask mask, void *desc,
		   void *buffer, u16 buf_len)
{
	struct iavf_aq_desc *aq_desc = (struct iavf_aq_desc *)desc;
	u8 *buf = (u8 *)buffer;
	u16 len;
	u16 i = 0;

	if ((!(mask & hw->debug_mask)) || (desc == NULL))
		return;

	len = LE16_TO_CPU(aq_desc->datalen);

	iavf_debug(hw, mask,
		   "AQ CMD: opcode 0x%04X, flags 0x%04X, datalen 0x%04X, retval 0x%04X\n",
		   LE16_TO_CPU(aq_desc->opcode),
		   LE16_TO_CPU(aq_desc->flags),
		   LE16_TO_CPU(aq_desc->datalen),
		   LE16_TO_CPU(aq_desc->retval));
	iavf_debug(hw, mask, "\tcookie (h,l) 0x%08X 0x%08X\n",
		   LE32_TO_CPU(aq_desc->cookie_high),
		   LE32_TO_CPU(aq_desc->cookie_low));
	iavf_debug(hw, mask, "\tparam (0,1)  0x%08X 0x%08X\n",
		   LE32_TO_CPU(aq_desc->params.internal.param0),
		   LE32_TO_CPU(aq_desc->params.internal.param1));
	iavf_debug(hw, mask, "\taddr (h,l)   0x%08X 0x%08X\n",
		   LE32_TO_CPU(aq_desc->params.external.addr_high),
		   LE32_TO_CPU(aq_desc->params.external.addr_low));

	if ((buffer != NULL) && (aq_desc->datalen != 0)) {
		iavf_debug(hw, mask, "AQ CMD Buffer:\n");
		if (buf_len < len)
			len = buf_len;
		/* write the full 16-byte chunks */
		for (i = 0; i < (len - 16); i += 16)
			iavf_debug(hw, mask,
				   "\t0x%04X  %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
				   i, buf[i], buf[i+1], buf[i+2], buf[i+3],
				   buf[i+4], buf[i+5], buf[i+6], buf[i+7],
				   buf[i+8], buf[i+9], buf[i+10], buf[i+11],
				   buf[i+12], buf[i+13], buf[i+14], buf[i+15]);
		/* the most we could have left is 16 bytes, pad with zeros */
		if (i < len) {
			char d_buf[16];
			int j, i_sav;

			i_sav = i;
			memset(d_buf, 0, sizeof(d_buf));
			for (j = 0; i < len; j++, i++)
				d_buf[j] = buf[i];
			iavf_debug(hw, mask,
				   "\t0x%04X  %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
				   i_sav, d_buf[0], d_buf[1], d_buf[2], d_buf[3],
				   d_buf[4], d_buf[5], d_buf[6], d_buf[7],
				   d_buf[8], d_buf[9], d_buf[10], d_buf[11],
				   d_buf[12], d_buf[13], d_buf[14], d_buf[15]);
		}
	}
}

/**
 * iavf_check_asq_alive
 * @hw: pointer to the hw struct
 *
 * Returns true if Queue is enabled else false.
 **/
bool iavf_check_asq_alive(struct iavf_hw *hw)
{
	if (hw->aq.asq.len)
		return !!(rd32(hw, hw->aq.asq.len) &
			IAVF_VF_ATQLEN1_ATQENABLE_MASK);
	else
		return false;
}

/**
 * iavf_aq_queue_shutdown
 * @hw: pointer to the hw struct
 * @unloading: is the driver unloading itself
 *
 * Tell the Firmware that we're shutting down the AdminQ and whether
 * or not the driver is unloading as well.
 **/
enum iavf_status iavf_aq_queue_shutdown(struct iavf_hw *hw,
					bool unloading)
{
	struct iavf_aq_desc desc;
	struct iavf_aqc_queue_shutdown *cmd =
		(struct iavf_aqc_queue_shutdown *)&desc.params.raw;
	enum iavf_status status;

	iavf_fill_default_direct_cmd_desc(&desc,
					  iavf_aqc_opc_queue_shutdown);

	if (unloading)
		cmd->driver_unloading = CPU_TO_LE32(IAVF_AQ_DRIVER_UNLOADING);
	status = iavf_asq_send_command(hw, &desc, NULL, 0, NULL);

	return status;
}

/**
 * iavf_aq_get_set_rss_lut
 * @hw: pointer to the hardware structure
 * @vsi_id: vsi fw index
 * @pf_lut: for PF table set true, for VSI table set false
 * @lut: pointer to the lut buffer provided by the caller
 * @lut_size: size of the lut buffer
 * @set: set true to set the table, false to get the table
 *
 * Internal function to get or set RSS look up table
 **/
STATIC enum iavf_status iavf_aq_get_set_rss_lut(struct iavf_hw *hw,
						u16 vsi_id, bool pf_lut,
						u8 *lut, u16 lut_size,
						bool set)
{
	enum iavf_status status;
	struct iavf_aq_desc desc;
	struct iavf_aqc_get_set_rss_lut *cmd_resp =
		   (struct iavf_aqc_get_set_rss_lut *)&desc.params.raw;

	if (set)
		iavf_fill_default_direct_cmd_desc(&desc,
						  iavf_aqc_opc_set_rss_lut);
	else
		iavf_fill_default_direct_cmd_desc(&desc,
						  iavf_aqc_opc_get_rss_lut);

	/* Indirect command */
	desc.flags |= CPU_TO_LE16((u16)IAVF_AQ_FLAG_BUF);
	desc.flags |= CPU_TO_LE16((u16)IAVF_AQ_FLAG_RD);

	cmd_resp->vsi_id =
			CPU_TO_LE16((u16)((vsi_id <<
					  IAVF_AQC_SET_RSS_LUT_VSI_ID_SHIFT) &
					  IAVF_AQC_SET_RSS_LUT_VSI_ID_MASK));
	cmd_resp->vsi_id |= CPU_TO_LE16((u16)IAVF_AQC_SET_RSS_LUT_VSI_VALID);

	if (pf_lut)
		cmd_resp->flags |= CPU_TO_LE16((u16)
					((IAVF_AQC_SET_RSS_LUT_TABLE_TYPE_PF <<
					IAVF_AQC_SET_RSS_LUT_TABLE_TYPE_SHIFT) &
					IAVF_AQC_SET_RSS_LUT_TABLE_TYPE_MASK));
	else
		cmd_resp->flags |= CPU_TO_LE16((u16)
					((IAVF_AQC_SET_RSS_LUT_TABLE_TYPE_VSI <<
					IAVF_AQC_SET_RSS_LUT_TABLE_TYPE_SHIFT) &
					IAVF_AQC_SET_RSS_LUT_TABLE_TYPE_MASK));

	status = iavf_asq_send_command(hw, &desc, lut, lut_size, NULL);

	return status;
}

/**
 * iavf_aq_get_rss_lut
 * @hw: pointer to the hardware structure
 * @vsi_id: vsi fw index
 * @pf_lut: for PF table set true, for VSI table set false
 * @lut: pointer to the lut buffer provided by the caller
 * @lut_size: size of the lut buffer
 *
 * get the RSS lookup table, PF or VSI type
 **/
enum iavf_status iavf_aq_get_rss_lut(struct iavf_hw *hw, u16 vsi_id,
				     bool pf_lut, u8 *lut, u16 lut_size)
{
	return iavf_aq_get_set_rss_lut(hw, vsi_id, pf_lut, lut, lut_size,
				       false);
}

/**
 * iavf_aq_set_rss_lut
 * @hw: pointer to the hardware structure
 * @vsi_id: vsi fw index
 * @pf_lut: for PF table set true, for VSI table set false
 * @lut: pointer to the lut buffer provided by the caller
 * @lut_size: size of the lut buffer
 *
 * set the RSS lookup table, PF or VSI type
 **/
enum iavf_status iavf_aq_set_rss_lut(struct iavf_hw *hw, u16 vsi_id,
				     bool pf_lut, u8 *lut, u16 lut_size)
{
	return iavf_aq_get_set_rss_lut(hw, vsi_id, pf_lut, lut, lut_size, true);
}

/**
 * iavf_aq_get_set_rss_key
 * @hw: pointer to the hw struct
 * @vsi_id: vsi fw index
 * @key: pointer to key info struct
 * @set: set true to set the key, false to get the key
 *
 * get the RSS key per VSI
 **/
STATIC enum iavf_status iavf_aq_get_set_rss_key(struct iavf_hw *hw,
				      u16 vsi_id,
				      struct iavf_aqc_get_set_rss_key_data *key,
				      bool set)
{
	enum iavf_status status;
	struct iavf_aq_desc desc;
	struct iavf_aqc_get_set_rss_key *cmd_resp =
			(struct iavf_aqc_get_set_rss_key *)&desc.params.raw;
	u16 key_size = sizeof(struct iavf_aqc_get_set_rss_key_data);

	if (set)
		iavf_fill_default_direct_cmd_desc(&desc,
						  iavf_aqc_opc_set_rss_key);
	else
		iavf_fill_default_direct_cmd_desc(&desc,
						  iavf_aqc_opc_get_rss_key);

	/* Indirect command */
	desc.flags |= CPU_TO_LE16((u16)IAVF_AQ_FLAG_BUF);
	desc.flags |= CPU_TO_LE16((u16)IAVF_AQ_FLAG_RD);

	cmd_resp->vsi_id =
			CPU_TO_LE16((u16)((vsi_id <<
					  IAVF_AQC_SET_RSS_KEY_VSI_ID_SHIFT) &
					  IAVF_AQC_SET_RSS_KEY_VSI_ID_MASK));
	cmd_resp->vsi_id |= CPU_TO_LE16((u16)IAVF_AQC_SET_RSS_KEY_VSI_VALID);

	status = iavf_asq_send_command(hw, &desc, key, key_size, NULL);

	return status;
}

/**
 * iavf_aq_get_rss_key
 * @hw: pointer to the hw struct
 * @vsi_id: vsi fw index
 * @key: pointer to key info struct
 *
 **/
enum iavf_status iavf_aq_get_rss_key(struct iavf_hw *hw,
				     u16 vsi_id,
				     struct iavf_aqc_get_set_rss_key_data *key)
{
	return iavf_aq_get_set_rss_key(hw, vsi_id, key, false);
}

/**
 * iavf_aq_set_rss_key
 * @hw: pointer to the hw struct
 * @vsi_id: vsi fw index
 * @key: pointer to key info struct
 *
 * set the RSS key per VSI
 **/
enum iavf_status iavf_aq_set_rss_key(struct iavf_hw *hw,
				     u16 vsi_id,
				     struct iavf_aqc_get_set_rss_key_data *key)
{
	return iavf_aq_get_set_rss_key(hw, vsi_id, key, true);
}

/* The iavf_ptype_lookup table is used to convert from the 8-bit and 10-bit
 * ptype in the hardware to a bit-field that can be used by SW to more easily
 * determine the packet type.
 *
 * Macros are used to shorten the table lines and make this table human
 * readable.
 *
 * We store the PTYPE in the top byte of the bit field - this is just so that
 * we can check that the table doesn't have a row missing, as the index into
 * the table should be the PTYPE.
 *
 * Typical work flow:
 *
 * IF NOT iavf_ptype_lookup[ptype].known
 * THEN
 *      Packet is unknown
 * ELSE IF iavf_ptype_lookup[ptype].outer_ip == IAVF_RX_PTYPE_OUTER_IP
 *      Use the rest of the fields to look at the tunnels, inner protocols, etc
 * ELSE
 *      Use the enum iavf_rx_l2_ptype to decode the packet type
 * ENDIF
 */

/* macro to make the table lines short */
#define IAVF_PTT(PTYPE, OUTER_IP, OUTER_IP_VER, OUTER_FRAG, T, TE, TEF, I, PL)\
	{	PTYPE, \
		1, \
		IAVF_RX_PTYPE_OUTER_##OUTER_IP, \
		IAVF_RX_PTYPE_OUTER_##OUTER_IP_VER, \
		IAVF_RX_PTYPE_##OUTER_FRAG, \
		IAVF_RX_PTYPE_TUNNEL_##T, \
		IAVF_RX_PTYPE_TUNNEL_END_##TE, \
		IAVF_RX_PTYPE_##TEF, \
		IAVF_RX_PTYPE_INNER_PROT_##I, \
		IAVF_RX_PTYPE_PAYLOAD_LAYER_##PL }

#define IAVF_PTT_UNUSED_ENTRY(PTYPE) \
		{ PTYPE, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

/* shorter macros makes the table fit but are terse */
#define IAVF_RX_PTYPE_NOF		IAVF_RX_PTYPE_NOT_FRAG
#define IAVF_RX_PTYPE_FRG		IAVF_RX_PTYPE_FRAG
#define IAVF_RX_PTYPE_INNER_PROT_TS	IAVF_RX_PTYPE_INNER_PROT_TIMESYNC

/* Lookup table mapping the HW PTYPE to the bit field for decoding */
struct iavf_rx_ptype_decoded iavf_ptype_lookup[] = {
	/* L2 Packet types */
	IAVF_PTT_UNUSED_ENTRY(0),
	IAVF_PTT(1,  L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY2),
	IAVF_PTT(2,  L2, NONE, NOF, NONE, NONE, NOF, TS,   PAY2),
	IAVF_PTT(3,  L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY2),
	IAVF_PTT_UNUSED_ENTRY(4),
	IAVF_PTT_UNUSED_ENTRY(5),
	IAVF_PTT(6,  L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY2),
	IAVF_PTT(7,  L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY2),
	IAVF_PTT_UNUSED_ENTRY(8),
	IAVF_PTT_UNUSED_ENTRY(9),
	IAVF_PTT(10, L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY2),
	IAVF_PTT(11, L2, NONE, NOF, NONE, NONE, NOF, NONE, NONE),
	IAVF_PTT(12, L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY3),
	IAVF_PTT(13, L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY3),
	IAVF_PTT(14, L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY3),
	IAVF_PTT(15, L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY3),
	IAVF_PTT(16, L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY3),
	IAVF_PTT(17, L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY3),
	IAVF_PTT(18, L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY3),
	IAVF_PTT(19, L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY3),
	IAVF_PTT(20, L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY3),
	IAVF_PTT(21, L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY3),

	/* Non Tunneled IPv4 */
	IAVF_PTT(22, IP, IPV4, FRG, NONE, NONE, NOF, NONE, PAY3),
	IAVF_PTT(23, IP, IPV4, NOF, NONE, NONE, NOF, NONE, PAY3),
	IAVF_PTT(24, IP, IPV4, NOF, NONE, NONE, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(25),
	IAVF_PTT(26, IP, IPV4, NOF, NONE, NONE, NOF, TCP,  PAY4),
	IAVF_PTT(27, IP, IPV4, NOF, NONE, NONE, NOF, SCTP, PAY4),
	IAVF_PTT(28, IP, IPV4, NOF, NONE, NONE, NOF, ICMP, PAY4),

	/* IPv4 --> IPv4 */
	IAVF_PTT(29, IP, IPV4, NOF, IP_IP, IPV4, FRG, NONE, PAY3),
	IAVF_PTT(30, IP, IPV4, NOF, IP_IP, IPV4, NOF, NONE, PAY3),
	IAVF_PTT(31, IP, IPV4, NOF, IP_IP, IPV4, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(32),
	IAVF_PTT(33, IP, IPV4, NOF, IP_IP, IPV4, NOF, TCP,  PAY4),
	IAVF_PTT(34, IP, IPV4, NOF, IP_IP, IPV4, NOF, SCTP, PAY4),
	IAVF_PTT(35, IP, IPV4, NOF, IP_IP, IPV4, NOF, ICMP, PAY4),

	/* IPv4 --> IPv6 */
	IAVF_PTT(36, IP, IPV4, NOF, IP_IP, IPV6, FRG, NONE, PAY3),
	IAVF_PTT(37, IP, IPV4, NOF, IP_IP, IPV6, NOF, NONE, PAY3),
	IAVF_PTT(38, IP, IPV4, NOF, IP_IP, IPV6, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(39),
	IAVF_PTT(40, IP, IPV4, NOF, IP_IP, IPV6, NOF, TCP,  PAY4),
	IAVF_PTT(41, IP, IPV4, NOF, IP_IP, IPV6, NOF, SCTP, PAY4),
	IAVF_PTT(42, IP, IPV4, NOF, IP_IP, IPV6, NOF, ICMP, PAY4),

	/* IPv4 --> GRE/NAT */
	IAVF_PTT(43, IP, IPV4, NOF, IP_GRENAT, NONE, NOF, NONE, PAY3),

	/* IPv4 --> GRE/NAT --> IPv4 */
	IAVF_PTT(44, IP, IPV4, NOF, IP_GRENAT, IPV4, FRG, NONE, PAY3),
	IAVF_PTT(45, IP, IPV4, NOF, IP_GRENAT, IPV4, NOF, NONE, PAY3),
	IAVF_PTT(46, IP, IPV4, NOF, IP_GRENAT, IPV4, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(47),
	IAVF_PTT(48, IP, IPV4, NOF, IP_GRENAT, IPV4, NOF, TCP,  PAY4),
	IAVF_PTT(49, IP, IPV4, NOF, IP_GRENAT, IPV4, NOF, SCTP, PAY4),
	IAVF_PTT(50, IP, IPV4, NOF, IP_GRENAT, IPV4, NOF, ICMP, PAY4),

	/* IPv4 --> GRE/NAT --> IPv6 */
	IAVF_PTT(51, IP, IPV4, NOF, IP_GRENAT, IPV6, FRG, NONE, PAY3),
	IAVF_PTT(52, IP, IPV4, NOF, IP_GRENAT, IPV6, NOF, NONE, PAY3),
	IAVF_PTT(53, IP, IPV4, NOF, IP_GRENAT, IPV6, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(54),
	IAVF_PTT(55, IP, IPV4, NOF, IP_GRENAT, IPV6, NOF, TCP,  PAY4),
	IAVF_PTT(56, IP, IPV4, NOF, IP_GRENAT, IPV6, NOF, SCTP, PAY4),
	IAVF_PTT(57, IP, IPV4, NOF, IP_GRENAT, IPV6, NOF, ICMP, PAY4),

	/* IPv4 --> GRE/NAT --> MAC */
	IAVF_PTT(58, IP, IPV4, NOF, IP_GRENAT_MAC, NONE, NOF, NONE, PAY3),

	/* IPv4 --> GRE/NAT --> MAC --> IPv4 */
	IAVF_PTT(59, IP, IPV4, NOF, IP_GRENAT_MAC, IPV4, FRG, NONE, PAY3),
	IAVF_PTT(60, IP, IPV4, NOF, IP_GRENAT_MAC, IPV4, NOF, NONE, PAY3),
	IAVF_PTT(61, IP, IPV4, NOF, IP_GRENAT_MAC, IPV4, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(62),
	IAVF_PTT(63, IP, IPV4, NOF, IP_GRENAT_MAC, IPV4, NOF, TCP,  PAY4),
	IAVF_PTT(64, IP, IPV4, NOF, IP_GRENAT_MAC, IPV4, NOF, SCTP, PAY4),
	IAVF_PTT(65, IP, IPV4, NOF, IP_GRENAT_MAC, IPV4, NOF, ICMP, PAY4),

	/* IPv4 --> GRE/NAT -> MAC --> IPv6 */
	IAVF_PTT(66, IP, IPV4, NOF, IP_GRENAT_MAC, IPV6, FRG, NONE, PAY3),
	IAVF_PTT(67, IP, IPV4, NOF, IP_GRENAT_MAC, IPV6, NOF, NONE, PAY3),
	IAVF_PTT(68, IP, IPV4, NOF, IP_GRENAT_MAC, IPV6, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(69),
	IAVF_PTT(70, IP, IPV4, NOF, IP_GRENAT_MAC, IPV6, NOF, TCP,  PAY4),
	IAVF_PTT(71, IP, IPV4, NOF, IP_GRENAT_MAC, IPV6, NOF, SCTP, PAY4),
	IAVF_PTT(72, IP, IPV4, NOF, IP_GRENAT_MAC, IPV6, NOF, ICMP, PAY4),

	/* IPv4 --> GRE/NAT --> MAC/VLAN */
	IAVF_PTT(73, IP, IPV4, NOF, IP_GRENAT_MAC_VLAN, NONE, NOF, NONE, PAY3),

	/* IPv4 ---> GRE/NAT -> MAC/VLAN --> IPv4 */
	IAVF_PTT(74, IP, IPV4, NOF, IP_GRENAT_MAC_VLAN, IPV4, FRG, NONE, PAY3),
	IAVF_PTT(75, IP, IPV4, NOF, IP_GRENAT_MAC_VLAN, IPV4, NOF, NONE, PAY3),
	IAVF_PTT(76, IP, IPV4, NOF, IP_GRENAT_MAC_VLAN, IPV4, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(77),
	IAVF_PTT(78, IP, IPV4, NOF, IP_GRENAT_MAC_VLAN, IPV4, NOF, TCP,  PAY4),
	IAVF_PTT(79, IP, IPV4, NOF, IP_GRENAT_MAC_VLAN, IPV4, NOF, SCTP, PAY4),
	IAVF_PTT(80, IP, IPV4, NOF, IP_GRENAT_MAC_VLAN, IPV4, NOF, ICMP, PAY4),

	/* IPv4 -> GRE/NAT -> MAC/VLAN --> IPv6 */
	IAVF_PTT(81, IP, IPV4, NOF, IP_GRENAT_MAC_VLAN, IPV6, FRG, NONE, PAY3),
	IAVF_PTT(82, IP, IPV4, NOF, IP_GRENAT_MAC_VLAN, IPV6, NOF, NONE, PAY3),
	IAVF_PTT(83, IP, IPV4, NOF, IP_GRENAT_MAC_VLAN, IPV6, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(84),
	IAVF_PTT(85, IP, IPV4, NOF, IP_GRENAT_MAC_VLAN, IPV6, NOF, TCP,  PAY4),
	IAVF_PTT(86, IP, IPV4, NOF, IP_GRENAT_MAC_VLAN, IPV6, NOF, SCTP, PAY4),
	IAVF_PTT(87, IP, IPV4, NOF, IP_GRENAT_MAC_VLAN, IPV6, NOF, ICMP, PAY4),

	/* Non Tunneled IPv6 */
	IAVF_PTT(88, IP, IPV6, FRG, NONE, NONE, NOF, NONE, PAY3),
	IAVF_PTT(89, IP, IPV6, NOF, NONE, NONE, NOF, NONE, PAY3),
	IAVF_PTT(90, IP, IPV6, NOF, NONE, NONE, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(91),
	IAVF_PTT(92, IP, IPV6, NOF, NONE, NONE, NOF, TCP,  PAY4),
	IAVF_PTT(93, IP, IPV6, NOF, NONE, NONE, NOF, SCTP, PAY4),
	IAVF_PTT(94, IP, IPV6, NOF, NONE, NONE, NOF, ICMP, PAY4),

	/* IPv6 --> IPv4 */
	IAVF_PTT(95,  IP, IPV6, NOF, IP_IP, IPV4, FRG, NONE, PAY3),
	IAVF_PTT(96,  IP, IPV6, NOF, IP_IP, IPV4, NOF, NONE, PAY3),
	IAVF_PTT(97,  IP, IPV6, NOF, IP_IP, IPV4, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(98),
	IAVF_PTT(99,  IP, IPV6, NOF, IP_IP, IPV4, NOF, TCP,  PAY4),
	IAVF_PTT(100, IP, IPV6, NOF, IP_IP, IPV4, NOF, SCTP, PAY4),
	IAVF_PTT(101, IP, IPV6, NOF, IP_IP, IPV4, NOF, ICMP, PAY4),

	/* IPv6 --> IPv6 */
	IAVF_PTT(102, IP, IPV6, NOF, IP_IP, IPV6, FRG, NONE, PAY3),
	IAVF_PTT(103, IP, IPV6, NOF, IP_IP, IPV6, NOF, NONE, PAY3),
	IAVF_PTT(104, IP, IPV6, NOF, IP_IP, IPV6, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(105),
	IAVF_PTT(106, IP, IPV6, NOF, IP_IP, IPV6, NOF, TCP,  PAY4),
	IAVF_PTT(107, IP, IPV6, NOF, IP_IP, IPV6, NOF, SCTP, PAY4),
	IAVF_PTT(108, IP, IPV6, NOF, IP_IP, IPV6, NOF, ICMP, PAY4),

	/* IPv6 --> GRE/NAT */
	IAVF_PTT(109, IP, IPV6, NOF, IP_GRENAT, NONE, NOF, NONE, PAY3),

	/* IPv6 --> GRE/NAT -> IPv4 */
	IAVF_PTT(110, IP, IPV6, NOF, IP_GRENAT, IPV4, FRG, NONE, PAY3),
	IAVF_PTT(111, IP, IPV6, NOF, IP_GRENAT, IPV4, NOF, NONE, PAY3),
	IAVF_PTT(112, IP, IPV6, NOF, IP_GRENAT, IPV4, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(113),
	IAVF_PTT(114, IP, IPV6, NOF, IP_GRENAT, IPV4, NOF, TCP,  PAY4),
	IAVF_PTT(115, IP, IPV6, NOF, IP_GRENAT, IPV4, NOF, SCTP, PAY4),
	IAVF_PTT(116, IP, IPV6, NOF, IP_GRENAT, IPV4, NOF, ICMP, PAY4),

	/* IPv6 --> GRE/NAT -> IPv6 */
	IAVF_PTT(117, IP, IPV6, NOF, IP_GRENAT, IPV6, FRG, NONE, PAY3),
	IAVF_PTT(118, IP, IPV6, NOF, IP_GRENAT, IPV6, NOF, NONE, PAY3),
	IAVF_PTT(119, IP, IPV6, NOF, IP_GRENAT, IPV6, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(120),
	IAVF_PTT(121, IP, IPV6, NOF, IP_GRENAT, IPV6, NOF, TCP,  PAY4),
	IAVF_PTT(122, IP, IPV6, NOF, IP_GRENAT, IPV6, NOF, SCTP, PAY4),
	IAVF_PTT(123, IP, IPV6, NOF, IP_GRENAT, IPV6, NOF, ICMP, PAY4),

	/* IPv6 --> GRE/NAT -> MAC */
	IAVF_PTT(124, IP, IPV6, NOF, IP_GRENAT_MAC, NONE, NOF, NONE, PAY3),

	/* IPv6 --> GRE/NAT -> MAC -> IPv4 */
	IAVF_PTT(125, IP, IPV6, NOF, IP_GRENAT_MAC, IPV4, FRG, NONE, PAY3),
	IAVF_PTT(126, IP, IPV6, NOF, IP_GRENAT_MAC, IPV4, NOF, NONE, PAY3),
	IAVF_PTT(127, IP, IPV6, NOF, IP_GRENAT_MAC, IPV4, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(128),
	IAVF_PTT(129, IP, IPV6, NOF, IP_GRENAT_MAC, IPV4, NOF, TCP,  PAY4),
	IAVF_PTT(130, IP, IPV6, NOF, IP_GRENAT_MAC, IPV4, NOF, SCTP, PAY4),
	IAVF_PTT(131, IP, IPV6, NOF, IP_GRENAT_MAC, IPV4, NOF, ICMP, PAY4),

	/* IPv6 --> GRE/NAT -> MAC -> IPv6 */
	IAVF_PTT(132, IP, IPV6, NOF, IP_GRENAT_MAC, IPV6, FRG, NONE, PAY3),
	IAVF_PTT(133, IP, IPV6, NOF, IP_GRENAT_MAC, IPV6, NOF, NONE, PAY3),
	IAVF_PTT(134, IP, IPV6, NOF, IP_GRENAT_MAC, IPV6, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(135),
	IAVF_PTT(136, IP, IPV6, NOF, IP_GRENAT_MAC, IPV6, NOF, TCP,  PAY4),
	IAVF_PTT(137, IP, IPV6, NOF, IP_GRENAT_MAC, IPV6, NOF, SCTP, PAY4),
	IAVF_PTT(138, IP, IPV6, NOF, IP_GRENAT_MAC, IPV6, NOF, ICMP, PAY4),

	/* IPv6 --> GRE/NAT -> MAC/VLAN */
	IAVF_PTT(139, IP, IPV6, NOF, IP_GRENAT_MAC_VLAN, NONE, NOF, NONE, PAY3),

	/* IPv6 --> GRE/NAT -> MAC/VLAN --> IPv4 */
	IAVF_PTT(140, IP, IPV6, NOF, IP_GRENAT_MAC_VLAN, IPV4, FRG, NONE, PAY3),
	IAVF_PTT(141, IP, IPV6, NOF, IP_GRENAT_MAC_VLAN, IPV4, NOF, NONE, PAY3),
	IAVF_PTT(142, IP, IPV6, NOF, IP_GRENAT_MAC_VLAN, IPV4, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(143),
	IAVF_PTT(144, IP, IPV6, NOF, IP_GRENAT_MAC_VLAN, IPV4, NOF, TCP,  PAY4),
	IAVF_PTT(145, IP, IPV6, NOF, IP_GRENAT_MAC_VLAN, IPV4, NOF, SCTP, PAY4),
	IAVF_PTT(146, IP, IPV6, NOF, IP_GRENAT_MAC_VLAN, IPV4, NOF, ICMP, PAY4),

	/* IPv6 --> GRE/NAT -> MAC/VLAN --> IPv6 */
	IAVF_PTT(147, IP, IPV6, NOF, IP_GRENAT_MAC_VLAN, IPV6, FRG, NONE, PAY3),
	IAVF_PTT(148, IP, IPV6, NOF, IP_GRENAT_MAC_VLAN, IPV6, NOF, NONE, PAY3),
	IAVF_PTT(149, IP, IPV6, NOF, IP_GRENAT_MAC_VLAN, IPV6, NOF, UDP,  PAY4),
	IAVF_PTT_UNUSED_ENTRY(150),
	IAVF_PTT(151, IP, IPV6, NOF, IP_GRENAT_MAC_VLAN, IPV6, NOF, TCP,  PAY4),
	IAVF_PTT(152, IP, IPV6, NOF, IP_GRENAT_MAC_VLAN, IPV6, NOF, SCTP, PAY4),
	IAVF_PTT(153, IP, IPV6, NOF, IP_GRENAT_MAC_VLAN, IPV6, NOF, ICMP, PAY4),

	/* unused entries */
	IAVF_PTT_UNUSED_ENTRY(154),
	IAVF_PTT_UNUSED_ENTRY(155),
	IAVF_PTT_UNUSED_ENTRY(156),
	IAVF_PTT_UNUSED_ENTRY(157),
	IAVF_PTT_UNUSED_ENTRY(158),
	IAVF_PTT_UNUSED_ENTRY(159),

	IAVF_PTT_UNUSED_ENTRY(160),
	IAVF_PTT_UNUSED_ENTRY(161),
	IAVF_PTT_UNUSED_ENTRY(162),
	IAVF_PTT_UNUSED_ENTRY(163),
	IAVF_PTT_UNUSED_ENTRY(164),
	IAVF_PTT_UNUSED_ENTRY(165),
	IAVF_PTT_UNUSED_ENTRY(166),
	IAVF_PTT_UNUSED_ENTRY(167),
	IAVF_PTT_UNUSED_ENTRY(168),
	IAVF_PTT_UNUSED_ENTRY(169),

	IAVF_PTT_UNUSED_ENTRY(170),
	IAVF_PTT_UNUSED_ENTRY(171),
	IAVF_PTT_UNUSED_ENTRY(172),
	IAVF_PTT_UNUSED_ENTRY(173),
	IAVF_PTT_UNUSED_ENTRY(174),
	IAVF_PTT_UNUSED_ENTRY(175),
	IAVF_PTT_UNUSED_ENTRY(176),
	IAVF_PTT_UNUSED_ENTRY(177),
	IAVF_PTT_UNUSED_ENTRY(178),
	IAVF_PTT_UNUSED_ENTRY(179),

	IAVF_PTT_UNUSED_ENTRY(180),
	IAVF_PTT_UNUSED_ENTRY(181),
	IAVF_PTT_UNUSED_ENTRY(182),
	IAVF_PTT_UNUSED_ENTRY(183),
	IAVF_PTT_UNUSED_ENTRY(184),
	IAVF_PTT_UNUSED_ENTRY(185),
	IAVF_PTT_UNUSED_ENTRY(186),
	IAVF_PTT_UNUSED_ENTRY(187),
	IAVF_PTT_UNUSED_ENTRY(188),
	IAVF_PTT_UNUSED_ENTRY(189),

	IAVF_PTT_UNUSED_ENTRY(190),
	IAVF_PTT_UNUSED_ENTRY(191),
	IAVF_PTT_UNUSED_ENTRY(192),
	IAVF_PTT_UNUSED_ENTRY(193),
	IAVF_PTT_UNUSED_ENTRY(194),
	IAVF_PTT_UNUSED_ENTRY(195),
	IAVF_PTT_UNUSED_ENTRY(196),
	IAVF_PTT_UNUSED_ENTRY(197),
	IAVF_PTT_UNUSED_ENTRY(198),
	IAVF_PTT_UNUSED_ENTRY(199),

	IAVF_PTT_UNUSED_ENTRY(200),
	IAVF_PTT_UNUSED_ENTRY(201),
	IAVF_PTT_UNUSED_ENTRY(202),
	IAVF_PTT_UNUSED_ENTRY(203),
	IAVF_PTT_UNUSED_ENTRY(204),
	IAVF_PTT_UNUSED_ENTRY(205),
	IAVF_PTT_UNUSED_ENTRY(206),
	IAVF_PTT_UNUSED_ENTRY(207),
	IAVF_PTT_UNUSED_ENTRY(208),
	IAVF_PTT_UNUSED_ENTRY(209),

	IAVF_PTT_UNUSED_ENTRY(210),
	IAVF_PTT_UNUSED_ENTRY(211),
	IAVF_PTT_UNUSED_ENTRY(212),
	IAVF_PTT_UNUSED_ENTRY(213),
	IAVF_PTT_UNUSED_ENTRY(214),
	IAVF_PTT_UNUSED_ENTRY(215),
	IAVF_PTT_UNUSED_ENTRY(216),
	IAVF_PTT_UNUSED_ENTRY(217),
	IAVF_PTT_UNUSED_ENTRY(218),
	IAVF_PTT_UNUSED_ENTRY(219),

	IAVF_PTT_UNUSED_ENTRY(220),
	IAVF_PTT_UNUSED_ENTRY(221),
	IAVF_PTT_UNUSED_ENTRY(222),
	IAVF_PTT_UNUSED_ENTRY(223),
	IAVF_PTT_UNUSED_ENTRY(224),
	IAVF_PTT_UNUSED_ENTRY(225),
	IAVF_PTT_UNUSED_ENTRY(226),
	IAVF_PTT_UNUSED_ENTRY(227),
	IAVF_PTT_UNUSED_ENTRY(228),
	IAVF_PTT_UNUSED_ENTRY(229),

	IAVF_PTT_UNUSED_ENTRY(230),
	IAVF_PTT_UNUSED_ENTRY(231),
	IAVF_PTT_UNUSED_ENTRY(232),
	IAVF_PTT_UNUSED_ENTRY(233),
	IAVF_PTT_UNUSED_ENTRY(234),
	IAVF_PTT_UNUSED_ENTRY(235),
	IAVF_PTT_UNUSED_ENTRY(236),
	IAVF_PTT_UNUSED_ENTRY(237),
	IAVF_PTT_UNUSED_ENTRY(238),
	IAVF_PTT_UNUSED_ENTRY(239),

	IAVF_PTT_UNUSED_ENTRY(240),
	IAVF_PTT_UNUSED_ENTRY(241),
	IAVF_PTT_UNUSED_ENTRY(242),
	IAVF_PTT_UNUSED_ENTRY(243),
	IAVF_PTT_UNUSED_ENTRY(244),
	IAVF_PTT_UNUSED_ENTRY(245),
	IAVF_PTT_UNUSED_ENTRY(246),
	IAVF_PTT_UNUSED_ENTRY(247),
	IAVF_PTT_UNUSED_ENTRY(248),
	IAVF_PTT_UNUSED_ENTRY(249),

	IAVF_PTT_UNUSED_ENTRY(250),
	IAVF_PTT_UNUSED_ENTRY(251),
	IAVF_PTT_UNUSED_ENTRY(252),
	IAVF_PTT_UNUSED_ENTRY(253),
	IAVF_PTT_UNUSED_ENTRY(254),
	IAVF_PTT_UNUSED_ENTRY(255),
	IAVF_PTT_UNUSED_ENTRY(256),
	IAVF_PTT_UNUSED_ENTRY(257),
	IAVF_PTT_UNUSED_ENTRY(258),
	IAVF_PTT_UNUSED_ENTRY(259),

	IAVF_PTT_UNUSED_ENTRY(260),
	IAVF_PTT_UNUSED_ENTRY(261),
	IAVF_PTT_UNUSED_ENTRY(262),
	IAVF_PTT_UNUSED_ENTRY(263),
	IAVF_PTT_UNUSED_ENTRY(264),
	IAVF_PTT_UNUSED_ENTRY(265),
	IAVF_PTT_UNUSED_ENTRY(266),
	IAVF_PTT_UNUSED_ENTRY(267),
	IAVF_PTT_UNUSED_ENTRY(268),
	IAVF_PTT_UNUSED_ENTRY(269),

	IAVF_PTT_UNUSED_ENTRY(270),
	IAVF_PTT_UNUSED_ENTRY(271),
	IAVF_PTT_UNUSED_ENTRY(272),
	IAVF_PTT_UNUSED_ENTRY(273),
	IAVF_PTT_UNUSED_ENTRY(274),
	IAVF_PTT_UNUSED_ENTRY(275),
	IAVF_PTT_UNUSED_ENTRY(276),
	IAVF_PTT_UNUSED_ENTRY(277),
	IAVF_PTT_UNUSED_ENTRY(278),
	IAVF_PTT_UNUSED_ENTRY(279),

	IAVF_PTT_UNUSED_ENTRY(280),
	IAVF_PTT_UNUSED_ENTRY(281),
	IAVF_PTT_UNUSED_ENTRY(282),
	IAVF_PTT_UNUSED_ENTRY(283),
	IAVF_PTT_UNUSED_ENTRY(284),
	IAVF_PTT_UNUSED_ENTRY(285),
	IAVF_PTT_UNUSED_ENTRY(286),
	IAVF_PTT_UNUSED_ENTRY(287),
	IAVF_PTT_UNUSED_ENTRY(288),
	IAVF_PTT_UNUSED_ENTRY(289),

	IAVF_PTT_UNUSED_ENTRY(290),
	IAVF_PTT_UNUSED_ENTRY(291),
	IAVF_PTT_UNUSED_ENTRY(292),
	IAVF_PTT_UNUSED_ENTRY(293),
	IAVF_PTT_UNUSED_ENTRY(294),
	IAVF_PTT_UNUSED_ENTRY(295),
	IAVF_PTT_UNUSED_ENTRY(296),
	IAVF_PTT_UNUSED_ENTRY(297),
	IAVF_PTT_UNUSED_ENTRY(298),
	IAVF_PTT_UNUSED_ENTRY(299),

	IAVF_PTT_UNUSED_ENTRY(300),
	IAVF_PTT_UNUSED_ENTRY(301),
	IAVF_PTT_UNUSED_ENTRY(302),
	IAVF_PTT_UNUSED_ENTRY(303),
	IAVF_PTT_UNUSED_ENTRY(304),
	IAVF_PTT_UNUSED_ENTRY(305),
	IAVF_PTT_UNUSED_ENTRY(306),
	IAVF_PTT_UNUSED_ENTRY(307),
	IAVF_PTT_UNUSED_ENTRY(308),
	IAVF_PTT_UNUSED_ENTRY(309),

	IAVF_PTT_UNUSED_ENTRY(310),
	IAVF_PTT_UNUSED_ENTRY(311),
	IAVF_PTT_UNUSED_ENTRY(312),
	IAVF_PTT_UNUSED_ENTRY(313),
	IAVF_PTT_UNUSED_ENTRY(314),
	IAVF_PTT_UNUSED_ENTRY(315),
	IAVF_PTT_UNUSED_ENTRY(316),
	IAVF_PTT_UNUSED_ENTRY(317),
	IAVF_PTT_UNUSED_ENTRY(318),
	IAVF_PTT_UNUSED_ENTRY(319),

	IAVF_PTT_UNUSED_ENTRY(320),
	IAVF_PTT_UNUSED_ENTRY(321),
	IAVF_PTT_UNUSED_ENTRY(322),
	IAVF_PTT_UNUSED_ENTRY(323),
	IAVF_PTT_UNUSED_ENTRY(324),
	IAVF_PTT_UNUSED_ENTRY(325),
	IAVF_PTT_UNUSED_ENTRY(326),
	IAVF_PTT_UNUSED_ENTRY(327),
	IAVF_PTT_UNUSED_ENTRY(328),
	IAVF_PTT_UNUSED_ENTRY(329),

	IAVF_PTT_UNUSED_ENTRY(330),
	IAVF_PTT_UNUSED_ENTRY(331),
	IAVF_PTT_UNUSED_ENTRY(332),
	IAVF_PTT_UNUSED_ENTRY(333),
	IAVF_PTT_UNUSED_ENTRY(334),
	IAVF_PTT_UNUSED_ENTRY(335),
	IAVF_PTT_UNUSED_ENTRY(336),
	IAVF_PTT_UNUSED_ENTRY(337),
	IAVF_PTT_UNUSED_ENTRY(338),
	IAVF_PTT_UNUSED_ENTRY(339),

	IAVF_PTT_UNUSED_ENTRY(340),
	IAVF_PTT_UNUSED_ENTRY(341),
	IAVF_PTT_UNUSED_ENTRY(342),
	IAVF_PTT_UNUSED_ENTRY(343),
	IAVF_PTT_UNUSED_ENTRY(344),
	IAVF_PTT_UNUSED_ENTRY(345),
	IAVF_PTT_UNUSED_ENTRY(346),
	IAVF_PTT_UNUSED_ENTRY(347),
	IAVF_PTT_UNUSED_ENTRY(348),
	IAVF_PTT_UNUSED_ENTRY(349),

	IAVF_PTT_UNUSED_ENTRY(350),
	IAVF_PTT_UNUSED_ENTRY(351),
	IAVF_PTT_UNUSED_ENTRY(352),
	IAVF_PTT_UNUSED_ENTRY(353),
	IAVF_PTT_UNUSED_ENTRY(354),
	IAVF_PTT_UNUSED_ENTRY(355),
	IAVF_PTT_UNUSED_ENTRY(356),
	IAVF_PTT_UNUSED_ENTRY(357),
	IAVF_PTT_UNUSED_ENTRY(358),
	IAVF_PTT_UNUSED_ENTRY(359),

	IAVF_PTT_UNUSED_ENTRY(360),
	IAVF_PTT_UNUSED_ENTRY(361),
	IAVF_PTT_UNUSED_ENTRY(362),
	IAVF_PTT_UNUSED_ENTRY(363),
	IAVF_PTT_UNUSED_ENTRY(364),
	IAVF_PTT_UNUSED_ENTRY(365),
	IAVF_PTT_UNUSED_ENTRY(366),
	IAVF_PTT_UNUSED_ENTRY(367),
	IAVF_PTT_UNUSED_ENTRY(368),
	IAVF_PTT_UNUSED_ENTRY(369),

	IAVF_PTT_UNUSED_ENTRY(370),
	IAVF_PTT_UNUSED_ENTRY(371),
	IAVF_PTT_UNUSED_ENTRY(372),
	IAVF_PTT_UNUSED_ENTRY(373),
	IAVF_PTT_UNUSED_ENTRY(374),
	IAVF_PTT_UNUSED_ENTRY(375),
	IAVF_PTT_UNUSED_ENTRY(376),
	IAVF_PTT_UNUSED_ENTRY(377),
	IAVF_PTT_UNUSED_ENTRY(378),
	IAVF_PTT_UNUSED_ENTRY(379),

	IAVF_PTT_UNUSED_ENTRY(380),
	IAVF_PTT_UNUSED_ENTRY(381),
	IAVF_PTT_UNUSED_ENTRY(382),
	IAVF_PTT_UNUSED_ENTRY(383),
	IAVF_PTT_UNUSED_ENTRY(384),
	IAVF_PTT_UNUSED_ENTRY(385),
	IAVF_PTT_UNUSED_ENTRY(386),
	IAVF_PTT_UNUSED_ENTRY(387),
	IAVF_PTT_UNUSED_ENTRY(388),
	IAVF_PTT_UNUSED_ENTRY(389),

	IAVF_PTT_UNUSED_ENTRY(390),
	IAVF_PTT_UNUSED_ENTRY(391),
	IAVF_PTT_UNUSED_ENTRY(392),
	IAVF_PTT_UNUSED_ENTRY(393),
	IAVF_PTT_UNUSED_ENTRY(394),
	IAVF_PTT_UNUSED_ENTRY(395),
	IAVF_PTT_UNUSED_ENTRY(396),
	IAVF_PTT_UNUSED_ENTRY(397),
	IAVF_PTT_UNUSED_ENTRY(398),
	IAVF_PTT_UNUSED_ENTRY(399),

	IAVF_PTT_UNUSED_ENTRY(400),
	IAVF_PTT_UNUSED_ENTRY(401),
	IAVF_PTT_UNUSED_ENTRY(402),
	IAVF_PTT_UNUSED_ENTRY(403),
	IAVF_PTT_UNUSED_ENTRY(404),
	IAVF_PTT_UNUSED_ENTRY(405),
	IAVF_PTT_UNUSED_ENTRY(406),
	IAVF_PTT_UNUSED_ENTRY(407),
	IAVF_PTT_UNUSED_ENTRY(408),
	IAVF_PTT_UNUSED_ENTRY(409),

	IAVF_PTT_UNUSED_ENTRY(410),
	IAVF_PTT_UNUSED_ENTRY(411),
	IAVF_PTT_UNUSED_ENTRY(412),
	IAVF_PTT_UNUSED_ENTRY(413),
	IAVF_PTT_UNUSED_ENTRY(414),
	IAVF_PTT_UNUSED_ENTRY(415),
	IAVF_PTT_UNUSED_ENTRY(416),
	IAVF_PTT_UNUSED_ENTRY(417),
	IAVF_PTT_UNUSED_ENTRY(418),
	IAVF_PTT_UNUSED_ENTRY(419),

	IAVF_PTT_UNUSED_ENTRY(420),
	IAVF_PTT_UNUSED_ENTRY(421),
	IAVF_PTT_UNUSED_ENTRY(422),
	IAVF_PTT_UNUSED_ENTRY(423),
	IAVF_PTT_UNUSED_ENTRY(424),
	IAVF_PTT_UNUSED_ENTRY(425),
	IAVF_PTT_UNUSED_ENTRY(426),
	IAVF_PTT_UNUSED_ENTRY(427),
	IAVF_PTT_UNUSED_ENTRY(428),
	IAVF_PTT_UNUSED_ENTRY(429),

	IAVF_PTT_UNUSED_ENTRY(430),
	IAVF_PTT_UNUSED_ENTRY(431),
	IAVF_PTT_UNUSED_ENTRY(432),
	IAVF_PTT_UNUSED_ENTRY(433),
	IAVF_PTT_UNUSED_ENTRY(434),
	IAVF_PTT_UNUSED_ENTRY(435),
	IAVF_PTT_UNUSED_ENTRY(436),
	IAVF_PTT_UNUSED_ENTRY(437),
	IAVF_PTT_UNUSED_ENTRY(438),
	IAVF_PTT_UNUSED_ENTRY(439),

	IAVF_PTT_UNUSED_ENTRY(440),
	IAVF_PTT_UNUSED_ENTRY(441),
	IAVF_PTT_UNUSED_ENTRY(442),
	IAVF_PTT_UNUSED_ENTRY(443),
	IAVF_PTT_UNUSED_ENTRY(444),
	IAVF_PTT_UNUSED_ENTRY(445),
	IAVF_PTT_UNUSED_ENTRY(446),
	IAVF_PTT_UNUSED_ENTRY(447),
	IAVF_PTT_UNUSED_ENTRY(448),
	IAVF_PTT_UNUSED_ENTRY(449),

	IAVF_PTT_UNUSED_ENTRY(450),
	IAVF_PTT_UNUSED_ENTRY(451),
	IAVF_PTT_UNUSED_ENTRY(452),
	IAVF_PTT_UNUSED_ENTRY(453),
	IAVF_PTT_UNUSED_ENTRY(454),
	IAVF_PTT_UNUSED_ENTRY(455),
	IAVF_PTT_UNUSED_ENTRY(456),
	IAVF_PTT_UNUSED_ENTRY(457),
	IAVF_PTT_UNUSED_ENTRY(458),
	IAVF_PTT_UNUSED_ENTRY(459),

	IAVF_PTT_UNUSED_ENTRY(460),
	IAVF_PTT_UNUSED_ENTRY(461),
	IAVF_PTT_UNUSED_ENTRY(462),
	IAVF_PTT_UNUSED_ENTRY(463),
	IAVF_PTT_UNUSED_ENTRY(464),
	IAVF_PTT_UNUSED_ENTRY(465),
	IAVF_PTT_UNUSED_ENTRY(466),
	IAVF_PTT_UNUSED_ENTRY(467),
	IAVF_PTT_UNUSED_ENTRY(468),
	IAVF_PTT_UNUSED_ENTRY(469),

	IAVF_PTT_UNUSED_ENTRY(470),
	IAVF_PTT_UNUSED_ENTRY(471),
	IAVF_PTT_UNUSED_ENTRY(472),
	IAVF_PTT_UNUSED_ENTRY(473),
	IAVF_PTT_UNUSED_ENTRY(474),
	IAVF_PTT_UNUSED_ENTRY(475),
	IAVF_PTT_UNUSED_ENTRY(476),
	IAVF_PTT_UNUSED_ENTRY(477),
	IAVF_PTT_UNUSED_ENTRY(478),
	IAVF_PTT_UNUSED_ENTRY(479),

	IAVF_PTT_UNUSED_ENTRY(480),
	IAVF_PTT_UNUSED_ENTRY(481),
	IAVF_PTT_UNUSED_ENTRY(482),
	IAVF_PTT_UNUSED_ENTRY(483),
	IAVF_PTT_UNUSED_ENTRY(484),
	IAVF_PTT_UNUSED_ENTRY(485),
	IAVF_PTT_UNUSED_ENTRY(486),
	IAVF_PTT_UNUSED_ENTRY(487),
	IAVF_PTT_UNUSED_ENTRY(488),
	IAVF_PTT_UNUSED_ENTRY(489),

	IAVF_PTT_UNUSED_ENTRY(490),
	IAVF_PTT_UNUSED_ENTRY(491),
	IAVF_PTT_UNUSED_ENTRY(492),
	IAVF_PTT_UNUSED_ENTRY(493),
	IAVF_PTT_UNUSED_ENTRY(494),
	IAVF_PTT_UNUSED_ENTRY(495),
	IAVF_PTT_UNUSED_ENTRY(496),
	IAVF_PTT_UNUSED_ENTRY(497),
	IAVF_PTT_UNUSED_ENTRY(498),
	IAVF_PTT_UNUSED_ENTRY(499),

	IAVF_PTT_UNUSED_ENTRY(500),
	IAVF_PTT_UNUSED_ENTRY(501),
	IAVF_PTT_UNUSED_ENTRY(502),
	IAVF_PTT_UNUSED_ENTRY(503),
	IAVF_PTT_UNUSED_ENTRY(504),
	IAVF_PTT_UNUSED_ENTRY(505),
	IAVF_PTT_UNUSED_ENTRY(506),
	IAVF_PTT_UNUSED_ENTRY(507),
	IAVF_PTT_UNUSED_ENTRY(508),
	IAVF_PTT_UNUSED_ENTRY(509),

	IAVF_PTT_UNUSED_ENTRY(510),
	IAVF_PTT_UNUSED_ENTRY(511),
	IAVF_PTT_UNUSED_ENTRY(512),
	IAVF_PTT_UNUSED_ENTRY(513),
	IAVF_PTT_UNUSED_ENTRY(514),
	IAVF_PTT_UNUSED_ENTRY(515),
	IAVF_PTT_UNUSED_ENTRY(516),
	IAVF_PTT_UNUSED_ENTRY(517),
	IAVF_PTT_UNUSED_ENTRY(518),
	IAVF_PTT_UNUSED_ENTRY(519),

	IAVF_PTT_UNUSED_ENTRY(520),
	IAVF_PTT_UNUSED_ENTRY(521),
	IAVF_PTT_UNUSED_ENTRY(522),
	IAVF_PTT_UNUSED_ENTRY(523),
	IAVF_PTT_UNUSED_ENTRY(524),
	IAVF_PTT_UNUSED_ENTRY(525),
	IAVF_PTT_UNUSED_ENTRY(526),
	IAVF_PTT_UNUSED_ENTRY(527),
	IAVF_PTT_UNUSED_ENTRY(528),
	IAVF_PTT_UNUSED_ENTRY(529),

	IAVF_PTT_UNUSED_ENTRY(530),
	IAVF_PTT_UNUSED_ENTRY(531),
	IAVF_PTT_UNUSED_ENTRY(532),
	IAVF_PTT_UNUSED_ENTRY(533),
	IAVF_PTT_UNUSED_ENTRY(534),
	IAVF_PTT_UNUSED_ENTRY(535),
	IAVF_PTT_UNUSED_ENTRY(536),
	IAVF_PTT_UNUSED_ENTRY(537),
	IAVF_PTT_UNUSED_ENTRY(538),
	IAVF_PTT_UNUSED_ENTRY(539),

	IAVF_PTT_UNUSED_ENTRY(540),
	IAVF_PTT_UNUSED_ENTRY(541),
	IAVF_PTT_UNUSED_ENTRY(542),
	IAVF_PTT_UNUSED_ENTRY(543),
	IAVF_PTT_UNUSED_ENTRY(544),
	IAVF_PTT_UNUSED_ENTRY(545),
	IAVF_PTT_UNUSED_ENTRY(546),
	IAVF_PTT_UNUSED_ENTRY(547),
	IAVF_PTT_UNUSED_ENTRY(548),
	IAVF_PTT_UNUSED_ENTRY(549),

	IAVF_PTT_UNUSED_ENTRY(550),
	IAVF_PTT_UNUSED_ENTRY(551),
	IAVF_PTT_UNUSED_ENTRY(552),
	IAVF_PTT_UNUSED_ENTRY(553),
	IAVF_PTT_UNUSED_ENTRY(554),
	IAVF_PTT_UNUSED_ENTRY(555),
	IAVF_PTT_UNUSED_ENTRY(556),
	IAVF_PTT_UNUSED_ENTRY(557),
	IAVF_PTT_UNUSED_ENTRY(558),
	IAVF_PTT_UNUSED_ENTRY(559),

	IAVF_PTT_UNUSED_ENTRY(560),
	IAVF_PTT_UNUSED_ENTRY(561),
	IAVF_PTT_UNUSED_ENTRY(562),
	IAVF_PTT_UNUSED_ENTRY(563),
	IAVF_PTT_UNUSED_ENTRY(564),
	IAVF_PTT_UNUSED_ENTRY(565),
	IAVF_PTT_UNUSED_ENTRY(566),
	IAVF_PTT_UNUSED_ENTRY(567),
	IAVF_PTT_UNUSED_ENTRY(568),
	IAVF_PTT_UNUSED_ENTRY(569),

	IAVF_PTT_UNUSED_ENTRY(570),
	IAVF_PTT_UNUSED_ENTRY(571),
	IAVF_PTT_UNUSED_ENTRY(572),
	IAVF_PTT_UNUSED_ENTRY(573),
	IAVF_PTT_UNUSED_ENTRY(574),
	IAVF_PTT_UNUSED_ENTRY(575),
	IAVF_PTT_UNUSED_ENTRY(576),
	IAVF_PTT_UNUSED_ENTRY(577),
	IAVF_PTT_UNUSED_ENTRY(578),
	IAVF_PTT_UNUSED_ENTRY(579),

	IAVF_PTT_UNUSED_ENTRY(580),
	IAVF_PTT_UNUSED_ENTRY(581),
	IAVF_PTT_UNUSED_ENTRY(582),
	IAVF_PTT_UNUSED_ENTRY(583),
	IAVF_PTT_UNUSED_ENTRY(584),
	IAVF_PTT_UNUSED_ENTRY(585),
	IAVF_PTT_UNUSED_ENTRY(586),
	IAVF_PTT_UNUSED_ENTRY(587),
	IAVF_PTT_UNUSED_ENTRY(588),
	IAVF_PTT_UNUSED_ENTRY(589),

	IAVF_PTT_UNUSED_ENTRY(590),
	IAVF_PTT_UNUSED_ENTRY(591),
	IAVF_PTT_UNUSED_ENTRY(592),
	IAVF_PTT_UNUSED_ENTRY(593),
	IAVF_PTT_UNUSED_ENTRY(594),
	IAVF_PTT_UNUSED_ENTRY(595),
	IAVF_PTT_UNUSED_ENTRY(596),
	IAVF_PTT_UNUSED_ENTRY(597),
	IAVF_PTT_UNUSED_ENTRY(598),
	IAVF_PTT_UNUSED_ENTRY(599),

	IAVF_PTT_UNUSED_ENTRY(600),
	IAVF_PTT_UNUSED_ENTRY(601),
	IAVF_PTT_UNUSED_ENTRY(602),
	IAVF_PTT_UNUSED_ENTRY(603),
	IAVF_PTT_UNUSED_ENTRY(604),
	IAVF_PTT_UNUSED_ENTRY(605),
	IAVF_PTT_UNUSED_ENTRY(606),
	IAVF_PTT_UNUSED_ENTRY(607),
	IAVF_PTT_UNUSED_ENTRY(608),
	IAVF_PTT_UNUSED_ENTRY(609),

	IAVF_PTT_UNUSED_ENTRY(610),
	IAVF_PTT_UNUSED_ENTRY(611),
	IAVF_PTT_UNUSED_ENTRY(612),
	IAVF_PTT_UNUSED_ENTRY(613),
	IAVF_PTT_UNUSED_ENTRY(614),
	IAVF_PTT_UNUSED_ENTRY(615),
	IAVF_PTT_UNUSED_ENTRY(616),
	IAVF_PTT_UNUSED_ENTRY(617),
	IAVF_PTT_UNUSED_ENTRY(618),
	IAVF_PTT_UNUSED_ENTRY(619),

	IAVF_PTT_UNUSED_ENTRY(620),
	IAVF_PTT_UNUSED_ENTRY(621),
	IAVF_PTT_UNUSED_ENTRY(622),
	IAVF_PTT_UNUSED_ENTRY(623),
	IAVF_PTT_UNUSED_ENTRY(624),
	IAVF_PTT_UNUSED_ENTRY(625),
	IAVF_PTT_UNUSED_ENTRY(626),
	IAVF_PTT_UNUSED_ENTRY(627),
	IAVF_PTT_UNUSED_ENTRY(628),
	IAVF_PTT_UNUSED_ENTRY(629),

	IAVF_PTT_UNUSED_ENTRY(630),
	IAVF_PTT_UNUSED_ENTRY(631),
	IAVF_PTT_UNUSED_ENTRY(632),
	IAVF_PTT_UNUSED_ENTRY(633),
	IAVF_PTT_UNUSED_ENTRY(634),
	IAVF_PTT_UNUSED_ENTRY(635),
	IAVF_PTT_UNUSED_ENTRY(636),
	IAVF_PTT_UNUSED_ENTRY(637),
	IAVF_PTT_UNUSED_ENTRY(638),
	IAVF_PTT_UNUSED_ENTRY(639),

	IAVF_PTT_UNUSED_ENTRY(640),
	IAVF_PTT_UNUSED_ENTRY(641),
	IAVF_PTT_UNUSED_ENTRY(642),
	IAVF_PTT_UNUSED_ENTRY(643),
	IAVF_PTT_UNUSED_ENTRY(644),
	IAVF_PTT_UNUSED_ENTRY(645),
	IAVF_PTT_UNUSED_ENTRY(646),
	IAVF_PTT_UNUSED_ENTRY(647),
	IAVF_PTT_UNUSED_ENTRY(648),
	IAVF_PTT_UNUSED_ENTRY(649),

	IAVF_PTT_UNUSED_ENTRY(650),
	IAVF_PTT_UNUSED_ENTRY(651),
	IAVF_PTT_UNUSED_ENTRY(652),
	IAVF_PTT_UNUSED_ENTRY(653),
	IAVF_PTT_UNUSED_ENTRY(654),
	IAVF_PTT_UNUSED_ENTRY(655),
	IAVF_PTT_UNUSED_ENTRY(656),
	IAVF_PTT_UNUSED_ENTRY(657),
	IAVF_PTT_UNUSED_ENTRY(658),
	IAVF_PTT_UNUSED_ENTRY(659),

	IAVF_PTT_UNUSED_ENTRY(660),
	IAVF_PTT_UNUSED_ENTRY(661),
	IAVF_PTT_UNUSED_ENTRY(662),
	IAVF_PTT_UNUSED_ENTRY(663),
	IAVF_PTT_UNUSED_ENTRY(664),
	IAVF_PTT_UNUSED_ENTRY(665),
	IAVF_PTT_UNUSED_ENTRY(666),
	IAVF_PTT_UNUSED_ENTRY(667),
	IAVF_PTT_UNUSED_ENTRY(668),
	IAVF_PTT_UNUSED_ENTRY(669),

	IAVF_PTT_UNUSED_ENTRY(670),
	IAVF_PTT_UNUSED_ENTRY(671),
	IAVF_PTT_UNUSED_ENTRY(672),
	IAVF_PTT_UNUSED_ENTRY(673),
	IAVF_PTT_UNUSED_ENTRY(674),
	IAVF_PTT_UNUSED_ENTRY(675),
	IAVF_PTT_UNUSED_ENTRY(676),
	IAVF_PTT_UNUSED_ENTRY(677),
	IAVF_PTT_UNUSED_ENTRY(678),
	IAVF_PTT_UNUSED_ENTRY(679),

	IAVF_PTT_UNUSED_ENTRY(680),
	IAVF_PTT_UNUSED_ENTRY(681),
	IAVF_PTT_UNUSED_ENTRY(682),
	IAVF_PTT_UNUSED_ENTRY(683),
	IAVF_PTT_UNUSED_ENTRY(684),
	IAVF_PTT_UNUSED_ENTRY(685),
	IAVF_PTT_UNUSED_ENTRY(686),
	IAVF_PTT_UNUSED_ENTRY(687),
	IAVF_PTT_UNUSED_ENTRY(688),
	IAVF_PTT_UNUSED_ENTRY(689),

	IAVF_PTT_UNUSED_ENTRY(690),
	IAVF_PTT_UNUSED_ENTRY(691),
	IAVF_PTT_UNUSED_ENTRY(692),
	IAVF_PTT_UNUSED_ENTRY(693),
	IAVF_PTT_UNUSED_ENTRY(694),
	IAVF_PTT_UNUSED_ENTRY(695),
	IAVF_PTT_UNUSED_ENTRY(696),
	IAVF_PTT_UNUSED_ENTRY(697),
	IAVF_PTT_UNUSED_ENTRY(698),
	IAVF_PTT_UNUSED_ENTRY(699),

	IAVF_PTT_UNUSED_ENTRY(700),
	IAVF_PTT_UNUSED_ENTRY(701),
	IAVF_PTT_UNUSED_ENTRY(702),
	IAVF_PTT_UNUSED_ENTRY(703),
	IAVF_PTT_UNUSED_ENTRY(704),
	IAVF_PTT_UNUSED_ENTRY(705),
	IAVF_PTT_UNUSED_ENTRY(706),
	IAVF_PTT_UNUSED_ENTRY(707),
	IAVF_PTT_UNUSED_ENTRY(708),
	IAVF_PTT_UNUSED_ENTRY(709),

	IAVF_PTT_UNUSED_ENTRY(710),
	IAVF_PTT_UNUSED_ENTRY(711),
	IAVF_PTT_UNUSED_ENTRY(712),
	IAVF_PTT_UNUSED_ENTRY(713),
	IAVF_PTT_UNUSED_ENTRY(714),
	IAVF_PTT_UNUSED_ENTRY(715),
	IAVF_PTT_UNUSED_ENTRY(716),
	IAVF_PTT_UNUSED_ENTRY(717),
	IAVF_PTT_UNUSED_ENTRY(718),
	IAVF_PTT_UNUSED_ENTRY(719),

	IAVF_PTT_UNUSED_ENTRY(720),
	IAVF_PTT_UNUSED_ENTRY(721),
	IAVF_PTT_UNUSED_ENTRY(722),
	IAVF_PTT_UNUSED_ENTRY(723),
	IAVF_PTT_UNUSED_ENTRY(724),
	IAVF_PTT_UNUSED_ENTRY(725),
	IAVF_PTT_UNUSED_ENTRY(726),
	IAVF_PTT_UNUSED_ENTRY(727),
	IAVF_PTT_UNUSED_ENTRY(728),
	IAVF_PTT_UNUSED_ENTRY(729),

	IAVF_PTT_UNUSED_ENTRY(730),
	IAVF_PTT_UNUSED_ENTRY(731),
	IAVF_PTT_UNUSED_ENTRY(732),
	IAVF_PTT_UNUSED_ENTRY(733),
	IAVF_PTT_UNUSED_ENTRY(734),
	IAVF_PTT_UNUSED_ENTRY(735),
	IAVF_PTT_UNUSED_ENTRY(736),
	IAVF_PTT_UNUSED_ENTRY(737),
	IAVF_PTT_UNUSED_ENTRY(738),
	IAVF_PTT_UNUSED_ENTRY(739),

	IAVF_PTT_UNUSED_ENTRY(740),
	IAVF_PTT_UNUSED_ENTRY(741),
	IAVF_PTT_UNUSED_ENTRY(742),
	IAVF_PTT_UNUSED_ENTRY(743),
	IAVF_PTT_UNUSED_ENTRY(744),
	IAVF_PTT_UNUSED_ENTRY(745),
	IAVF_PTT_UNUSED_ENTRY(746),
	IAVF_PTT_UNUSED_ENTRY(747),
	IAVF_PTT_UNUSED_ENTRY(748),
	IAVF_PTT_UNUSED_ENTRY(749),

	IAVF_PTT_UNUSED_ENTRY(750),
	IAVF_PTT_UNUSED_ENTRY(751),
	IAVF_PTT_UNUSED_ENTRY(752),
	IAVF_PTT_UNUSED_ENTRY(753),
	IAVF_PTT_UNUSED_ENTRY(754),
	IAVF_PTT_UNUSED_ENTRY(755),
	IAVF_PTT_UNUSED_ENTRY(756),
	IAVF_PTT_UNUSED_ENTRY(757),
	IAVF_PTT_UNUSED_ENTRY(758),
	IAVF_PTT_UNUSED_ENTRY(759),

	IAVF_PTT_UNUSED_ENTRY(760),
	IAVF_PTT_UNUSED_ENTRY(761),
	IAVF_PTT_UNUSED_ENTRY(762),
	IAVF_PTT_UNUSED_ENTRY(763),
	IAVF_PTT_UNUSED_ENTRY(764),
	IAVF_PTT_UNUSED_ENTRY(765),
	IAVF_PTT_UNUSED_ENTRY(766),
	IAVF_PTT_UNUSED_ENTRY(767),
	IAVF_PTT_UNUSED_ENTRY(768),
	IAVF_PTT_UNUSED_ENTRY(769),

	IAVF_PTT_UNUSED_ENTRY(770),
	IAVF_PTT_UNUSED_ENTRY(771),
	IAVF_PTT_UNUSED_ENTRY(772),
	IAVF_PTT_UNUSED_ENTRY(773),
	IAVF_PTT_UNUSED_ENTRY(774),
	IAVF_PTT_UNUSED_ENTRY(775),
	IAVF_PTT_UNUSED_ENTRY(776),
	IAVF_PTT_UNUSED_ENTRY(777),
	IAVF_PTT_UNUSED_ENTRY(778),
	IAVF_PTT_UNUSED_ENTRY(779),

	IAVF_PTT_UNUSED_ENTRY(780),
	IAVF_PTT_UNUSED_ENTRY(781),
	IAVF_PTT_UNUSED_ENTRY(782),
	IAVF_PTT_UNUSED_ENTRY(783),
	IAVF_PTT_UNUSED_ENTRY(784),
	IAVF_PTT_UNUSED_ENTRY(785),
	IAVF_PTT_UNUSED_ENTRY(786),
	IAVF_PTT_UNUSED_ENTRY(787),
	IAVF_PTT_UNUSED_ENTRY(788),
	IAVF_PTT_UNUSED_ENTRY(789),

	IAVF_PTT_UNUSED_ENTRY(790),
	IAVF_PTT_UNUSED_ENTRY(791),
	IAVF_PTT_UNUSED_ENTRY(792),
	IAVF_PTT_UNUSED_ENTRY(793),
	IAVF_PTT_UNUSED_ENTRY(794),
	IAVF_PTT_UNUSED_ENTRY(795),
	IAVF_PTT_UNUSED_ENTRY(796),
	IAVF_PTT_UNUSED_ENTRY(797),
	IAVF_PTT_UNUSED_ENTRY(798),
	IAVF_PTT_UNUSED_ENTRY(799),

	IAVF_PTT_UNUSED_ENTRY(800),
	IAVF_PTT_UNUSED_ENTRY(801),
	IAVF_PTT_UNUSED_ENTRY(802),
	IAVF_PTT_UNUSED_ENTRY(803),
	IAVF_PTT_UNUSED_ENTRY(804),
	IAVF_PTT_UNUSED_ENTRY(805),
	IAVF_PTT_UNUSED_ENTRY(806),
	IAVF_PTT_UNUSED_ENTRY(807),
	IAVF_PTT_UNUSED_ENTRY(808),
	IAVF_PTT_UNUSED_ENTRY(809),

	IAVF_PTT_UNUSED_ENTRY(810),
	IAVF_PTT_UNUSED_ENTRY(811),
	IAVF_PTT_UNUSED_ENTRY(812),
	IAVF_PTT_UNUSED_ENTRY(813),
	IAVF_PTT_UNUSED_ENTRY(814),
	IAVF_PTT_UNUSED_ENTRY(815),
	IAVF_PTT_UNUSED_ENTRY(816),
	IAVF_PTT_UNUSED_ENTRY(817),
	IAVF_PTT_UNUSED_ENTRY(818),
	IAVF_PTT_UNUSED_ENTRY(819),

	IAVF_PTT_UNUSED_ENTRY(820),
	IAVF_PTT_UNUSED_ENTRY(821),
	IAVF_PTT_UNUSED_ENTRY(822),
	IAVF_PTT_UNUSED_ENTRY(823),
	IAVF_PTT_UNUSED_ENTRY(824),
	IAVF_PTT_UNUSED_ENTRY(825),
	IAVF_PTT_UNUSED_ENTRY(826),
	IAVF_PTT_UNUSED_ENTRY(827),
	IAVF_PTT_UNUSED_ENTRY(828),
	IAVF_PTT_UNUSED_ENTRY(829),

	IAVF_PTT_UNUSED_ENTRY(830),
	IAVF_PTT_UNUSED_ENTRY(831),
	IAVF_PTT_UNUSED_ENTRY(832),
	IAVF_PTT_UNUSED_ENTRY(833),
	IAVF_PTT_UNUSED_ENTRY(834),
	IAVF_PTT_UNUSED_ENTRY(835),
	IAVF_PTT_UNUSED_ENTRY(836),
	IAVF_PTT_UNUSED_ENTRY(837),
	IAVF_PTT_UNUSED_ENTRY(838),
	IAVF_PTT_UNUSED_ENTRY(839),

	IAVF_PTT_UNUSED_ENTRY(840),
	IAVF_PTT_UNUSED_ENTRY(841),
	IAVF_PTT_UNUSED_ENTRY(842),
	IAVF_PTT_UNUSED_ENTRY(843),
	IAVF_PTT_UNUSED_ENTRY(844),
	IAVF_PTT_UNUSED_ENTRY(845),
	IAVF_PTT_UNUSED_ENTRY(846),
	IAVF_PTT_UNUSED_ENTRY(847),
	IAVF_PTT_UNUSED_ENTRY(848),
	IAVF_PTT_UNUSED_ENTRY(849),

	IAVF_PTT_UNUSED_ENTRY(850),
	IAVF_PTT_UNUSED_ENTRY(851),
	IAVF_PTT_UNUSED_ENTRY(852),
	IAVF_PTT_UNUSED_ENTRY(853),
	IAVF_PTT_UNUSED_ENTRY(854),
	IAVF_PTT_UNUSED_ENTRY(855),
	IAVF_PTT_UNUSED_ENTRY(856),
	IAVF_PTT_UNUSED_ENTRY(857),
	IAVF_PTT_UNUSED_ENTRY(858),
	IAVF_PTT_UNUSED_ENTRY(859),

	IAVF_PTT_UNUSED_ENTRY(860),
	IAVF_PTT_UNUSED_ENTRY(861),
	IAVF_PTT_UNUSED_ENTRY(862),
	IAVF_PTT_UNUSED_ENTRY(863),
	IAVF_PTT_UNUSED_ENTRY(864),
	IAVF_PTT_UNUSED_ENTRY(865),
	IAVF_PTT_UNUSED_ENTRY(866),
	IAVF_PTT_UNUSED_ENTRY(867),
	IAVF_PTT_UNUSED_ENTRY(868),
	IAVF_PTT_UNUSED_ENTRY(869),

	IAVF_PTT_UNUSED_ENTRY(870),
	IAVF_PTT_UNUSED_ENTRY(871),
	IAVF_PTT_UNUSED_ENTRY(872),
	IAVF_PTT_UNUSED_ENTRY(873),
	IAVF_PTT_UNUSED_ENTRY(874),
	IAVF_PTT_UNUSED_ENTRY(875),
	IAVF_PTT_UNUSED_ENTRY(876),
	IAVF_PTT_UNUSED_ENTRY(877),
	IAVF_PTT_UNUSED_ENTRY(878),
	IAVF_PTT_UNUSED_ENTRY(879),

	IAVF_PTT_UNUSED_ENTRY(880),
	IAVF_PTT_UNUSED_ENTRY(881),
	IAVF_PTT_UNUSED_ENTRY(882),
	IAVF_PTT_UNUSED_ENTRY(883),
	IAVF_PTT_UNUSED_ENTRY(884),
	IAVF_PTT_UNUSED_ENTRY(885),
	IAVF_PTT_UNUSED_ENTRY(886),
	IAVF_PTT_UNUSED_ENTRY(887),
	IAVF_PTT_UNUSED_ENTRY(888),
	IAVF_PTT_UNUSED_ENTRY(889),

	IAVF_PTT_UNUSED_ENTRY(890),
	IAVF_PTT_UNUSED_ENTRY(891),
	IAVF_PTT_UNUSED_ENTRY(892),
	IAVF_PTT_UNUSED_ENTRY(893),
	IAVF_PTT_UNUSED_ENTRY(894),
	IAVF_PTT_UNUSED_ENTRY(895),
	IAVF_PTT_UNUSED_ENTRY(896),
	IAVF_PTT_UNUSED_ENTRY(897),
	IAVF_PTT_UNUSED_ENTRY(898),
	IAVF_PTT_UNUSED_ENTRY(899),

	IAVF_PTT_UNUSED_ENTRY(900),
	IAVF_PTT_UNUSED_ENTRY(901),
	IAVF_PTT_UNUSED_ENTRY(902),
	IAVF_PTT_UNUSED_ENTRY(903),
	IAVF_PTT_UNUSED_ENTRY(904),
	IAVF_PTT_UNUSED_ENTRY(905),
	IAVF_PTT_UNUSED_ENTRY(906),
	IAVF_PTT_UNUSED_ENTRY(907),
	IAVF_PTT_UNUSED_ENTRY(908),
	IAVF_PTT_UNUSED_ENTRY(909),

	IAVF_PTT_UNUSED_ENTRY(910),
	IAVF_PTT_UNUSED_ENTRY(911),
	IAVF_PTT_UNUSED_ENTRY(912),
	IAVF_PTT_UNUSED_ENTRY(913),
	IAVF_PTT_UNUSED_ENTRY(914),
	IAVF_PTT_UNUSED_ENTRY(915),
	IAVF_PTT_UNUSED_ENTRY(916),
	IAVF_PTT_UNUSED_ENTRY(917),
	IAVF_PTT_UNUSED_ENTRY(918),
	IAVF_PTT_UNUSED_ENTRY(919),

	IAVF_PTT_UNUSED_ENTRY(920),
	IAVF_PTT_UNUSED_ENTRY(921),
	IAVF_PTT_UNUSED_ENTRY(922),
	IAVF_PTT_UNUSED_ENTRY(923),
	IAVF_PTT_UNUSED_ENTRY(924),
	IAVF_PTT_UNUSED_ENTRY(925),
	IAVF_PTT_UNUSED_ENTRY(926),
	IAVF_PTT_UNUSED_ENTRY(927),
	IAVF_PTT_UNUSED_ENTRY(928),
	IAVF_PTT_UNUSED_ENTRY(929),

	IAVF_PTT_UNUSED_ENTRY(930),
	IAVF_PTT_UNUSED_ENTRY(931),
	IAVF_PTT_UNUSED_ENTRY(932),
	IAVF_PTT_UNUSED_ENTRY(933),
	IAVF_PTT_UNUSED_ENTRY(934),
	IAVF_PTT_UNUSED_ENTRY(935),
	IAVF_PTT_UNUSED_ENTRY(936),
	IAVF_PTT_UNUSED_ENTRY(937),
	IAVF_PTT_UNUSED_ENTRY(938),
	IAVF_PTT_UNUSED_ENTRY(939),

	IAVF_PTT_UNUSED_ENTRY(940),
	IAVF_PTT_UNUSED_ENTRY(941),
	IAVF_PTT_UNUSED_ENTRY(942),
	IAVF_PTT_UNUSED_ENTRY(943),
	IAVF_PTT_UNUSED_ENTRY(944),
	IAVF_PTT_UNUSED_ENTRY(945),
	IAVF_PTT_UNUSED_ENTRY(946),
	IAVF_PTT_UNUSED_ENTRY(947),
	IAVF_PTT_UNUSED_ENTRY(948),
	IAVF_PTT_UNUSED_ENTRY(949),

	IAVF_PTT_UNUSED_ENTRY(950),
	IAVF_PTT_UNUSED_ENTRY(951),
	IAVF_PTT_UNUSED_ENTRY(952),
	IAVF_PTT_UNUSED_ENTRY(953),
	IAVF_PTT_UNUSED_ENTRY(954),
	IAVF_PTT_UNUSED_ENTRY(955),
	IAVF_PTT_UNUSED_ENTRY(956),
	IAVF_PTT_UNUSED_ENTRY(957),
	IAVF_PTT_UNUSED_ENTRY(958),
	IAVF_PTT_UNUSED_ENTRY(959),

	IAVF_PTT_UNUSED_ENTRY(960),
	IAVF_PTT_UNUSED_ENTRY(961),
	IAVF_PTT_UNUSED_ENTRY(962),
	IAVF_PTT_UNUSED_ENTRY(963),
	IAVF_PTT_UNUSED_ENTRY(964),
	IAVF_PTT_UNUSED_ENTRY(965),
	IAVF_PTT_UNUSED_ENTRY(966),
	IAVF_PTT_UNUSED_ENTRY(967),
	IAVF_PTT_UNUSED_ENTRY(968),
	IAVF_PTT_UNUSED_ENTRY(969),

	IAVF_PTT_UNUSED_ENTRY(970),
	IAVF_PTT_UNUSED_ENTRY(971),
	IAVF_PTT_UNUSED_ENTRY(972),
	IAVF_PTT_UNUSED_ENTRY(973),
	IAVF_PTT_UNUSED_ENTRY(974),
	IAVF_PTT_UNUSED_ENTRY(975),
	IAVF_PTT_UNUSED_ENTRY(976),
	IAVF_PTT_UNUSED_ENTRY(977),
	IAVF_PTT_UNUSED_ENTRY(978),
	IAVF_PTT_UNUSED_ENTRY(979),

	IAVF_PTT_UNUSED_ENTRY(980),
	IAVF_PTT_UNUSED_ENTRY(981),
	IAVF_PTT_UNUSED_ENTRY(982),
	IAVF_PTT_UNUSED_ENTRY(983),
	IAVF_PTT_UNUSED_ENTRY(984),
	IAVF_PTT_UNUSED_ENTRY(985),
	IAVF_PTT_UNUSED_ENTRY(986),
	IAVF_PTT_UNUSED_ENTRY(987),
	IAVF_PTT_UNUSED_ENTRY(988),
	IAVF_PTT_UNUSED_ENTRY(989),

	IAVF_PTT_UNUSED_ENTRY(990),
	IAVF_PTT_UNUSED_ENTRY(991),
	IAVF_PTT_UNUSED_ENTRY(992),
	IAVF_PTT_UNUSED_ENTRY(993),
	IAVF_PTT_UNUSED_ENTRY(994),
	IAVF_PTT_UNUSED_ENTRY(995),
	IAVF_PTT_UNUSED_ENTRY(996),
	IAVF_PTT_UNUSED_ENTRY(997),
	IAVF_PTT_UNUSED_ENTRY(998),
	IAVF_PTT_UNUSED_ENTRY(999),

	IAVF_PTT_UNUSED_ENTRY(1000),
	IAVF_PTT_UNUSED_ENTRY(1001),
	IAVF_PTT_UNUSED_ENTRY(1002),
	IAVF_PTT_UNUSED_ENTRY(1003),
	IAVF_PTT_UNUSED_ENTRY(1004),
	IAVF_PTT_UNUSED_ENTRY(1005),
	IAVF_PTT_UNUSED_ENTRY(1006),
	IAVF_PTT_UNUSED_ENTRY(1007),
	IAVF_PTT_UNUSED_ENTRY(1008),
	IAVF_PTT_UNUSED_ENTRY(1009),

	IAVF_PTT_UNUSED_ENTRY(1010),
	IAVF_PTT_UNUSED_ENTRY(1011),
	IAVF_PTT_UNUSED_ENTRY(1012),
	IAVF_PTT_UNUSED_ENTRY(1013),
	IAVF_PTT_UNUSED_ENTRY(1014),
	IAVF_PTT_UNUSED_ENTRY(1015),
	IAVF_PTT_UNUSED_ENTRY(1016),
	IAVF_PTT_UNUSED_ENTRY(1017),
	IAVF_PTT_UNUSED_ENTRY(1018),
	IAVF_PTT_UNUSED_ENTRY(1019),

	IAVF_PTT_UNUSED_ENTRY(1020),
	IAVF_PTT_UNUSED_ENTRY(1021),
	IAVF_PTT_UNUSED_ENTRY(1022),
	IAVF_PTT_UNUSED_ENTRY(1023),
};

/**
 * iavf_validate_mac_addr - Validate unicast MAC address
 * @mac_addr: pointer to MAC address
 *
 * Tests a MAC address to ensure it is a valid Individual Address
 **/
enum iavf_status iavf_validate_mac_addr(u8 *mac_addr)
{
	enum iavf_status status = IAVF_SUCCESS;

	DEBUGFUNC("iavf_validate_mac_addr");

	/* Broadcast addresses ARE multicast addresses
	 * Make sure it is not a multicast address
	 * Reject the zero address
	 */
	if (IAVF_IS_MULTICAST(mac_addr) ||
	    (mac_addr[0] == 0 && mac_addr[1] == 0 && mac_addr[2] == 0 &&
	      mac_addr[3] == 0 && mac_addr[4] == 0 && mac_addr[5] == 0))
		status = IAVF_ERR_INVALID_MAC_ADDR;

	return status;
}

/**
 * iavf_aq_send_msg_to_pf
 * @hw: pointer to the hardware structure
 * @v_opcode: opcodes for VF-PF communication
 * @v_retval: return error code
 * @msg: pointer to the msg buffer
 * @msglen: msg length
 * @cmd_details: pointer to command details
 *
 * Send message to PF driver using admin queue. By default, this message
 * is sent asynchronously, i.e. iavf_asq_send_command() does not wait for
 * completion before returning.
 **/
enum iavf_status iavf_aq_send_msg_to_pf(struct iavf_hw *hw,
				enum virtchnl_ops v_opcode,
				enum virtchnl_status_code v_retval,
				u8 *msg, u16 msglen,
				struct iavf_asq_cmd_details *cmd_details)
{
	struct iavf_aq_desc desc;
	struct iavf_asq_cmd_details details;
	enum iavf_status status;

	iavf_fill_default_direct_cmd_desc(&desc, iavf_aqc_opc_send_msg_to_pf);
	desc.flags |= CPU_TO_LE16((u16)IAVF_AQ_FLAG_SI);
	desc.cookie_high = CPU_TO_LE32(v_opcode);
	desc.cookie_low = CPU_TO_LE32(v_retval);
	if (msglen) {
		desc.flags |= CPU_TO_LE16((u16)(IAVF_AQ_FLAG_BUF
						| IAVF_AQ_FLAG_RD));
		if (msglen > IAVF_AQ_LARGE_BUF)
			desc.flags |= CPU_TO_LE16((u16)IAVF_AQ_FLAG_LB);
		desc.datalen = CPU_TO_LE16(msglen);
	}
	if (!cmd_details) {
		iavf_memset(&details, 0, sizeof(details), IAVF_NONDMA_MEM);
		details.async = true;
		cmd_details = &details;
	}
	status = iavf_asq_send_command(hw, (struct iavf_aq_desc *)&desc, msg,
				       msglen, cmd_details);
	return status;
}

/**
 * iavf_vf_parse_hw_config
 * @hw: pointer to the hardware structure
 * @msg: pointer to the virtual channel VF resource structure
 *
 * Given a VF resource message from the PF, populate the hw struct
 * with appropriate information.
 **/
void iavf_vf_parse_hw_config(struct iavf_hw *hw,
			     struct virtchnl_vf_resource *msg)
{
	struct virtchnl_vsi_resource *vsi_res;
	int i;

	vsi_res = &msg->vsi_res[0];

	hw->dev_caps.num_vsis = msg->num_vsis;
	hw->dev_caps.num_rx_qp = msg->num_queue_pairs;
	hw->dev_caps.num_tx_qp = msg->num_queue_pairs;
	hw->dev_caps.num_msix_vectors_vf = msg->max_vectors;
	hw->dev_caps.dcb = msg->vf_cap_flags &
			   VIRTCHNL_VF_OFFLOAD_L2;
	hw->dev_caps.max_mtu = msg->max_mtu;
	for (i = 0; i < msg->num_vsis; i++) {
		if (vsi_res->vsi_type == VIRTCHNL_VSI_SRIOV) {
			iavf_memcpy(hw->mac.perm_addr,
				    vsi_res->default_mac_addr,
				    ETH_ALEN,
				    IAVF_NONDMA_TO_NONDMA);
			iavf_memcpy(hw->mac.addr, vsi_res->default_mac_addr,
				    ETH_ALEN,
				    IAVF_NONDMA_TO_NONDMA);
		}
		vsi_res++;
	}
}

/**
 * iavf_vf_reset
 * @hw: pointer to the hardware structure
 *
 * Send a VF_RESET message to the PF. Does not wait for response from PF
 * as none will be forthcoming. Immediately after calling this function,
 * the admin queue should be shut down and (optionally) reinitialized.
 **/
enum iavf_status iavf_vf_reset(struct iavf_hw *hw)
{
	return iavf_aq_send_msg_to_pf(hw, VIRTCHNL_OP_RESET_VF,
				      VIRTCHNL_STATUS_SUCCESS, NULL, 0, NULL);
}

/**
* iavf_aq_clear_all_wol_filters
* @hw: pointer to the hw struct
* @cmd_details: pointer to command details structure or NULL
*
* Get information for the reason of a Wake Up event
**/
enum iavf_status iavf_aq_clear_all_wol_filters(struct iavf_hw *hw,
			struct iavf_asq_cmd_details *cmd_details)
{
	struct iavf_aq_desc desc;
	enum iavf_status status;

	iavf_fill_default_direct_cmd_desc(&desc,
					  iavf_aqc_opc_clear_all_wol_filters);

	status = iavf_asq_send_command(hw, &desc, NULL, 0, cmd_details);

	return status;
}
