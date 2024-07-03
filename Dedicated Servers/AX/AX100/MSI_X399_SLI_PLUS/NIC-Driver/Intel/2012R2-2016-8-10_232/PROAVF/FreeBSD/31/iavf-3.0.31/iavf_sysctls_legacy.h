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
/**
 * @file iavf_sysctls_legacy.h
 * @brief global sysctls used by the legacy driver
 *
 * Contains sysctl definitions which are used by the legacy driver. Sysctls
 * which are unique to the legacy driver should be defined here.
 */
#ifndef _IAVF_SYSCTLS_LEGACY_H_
#define _IAVF_SYSCTLS_LEGACY_H_

#ifndef IAVF_NO_IFLIB
#error "Do not include iavf_legacy_sysctls.h in an iflib build"
#endif

#include "iavf_sysctls_common.h"

/**
 * @var iavf_tx_ring_size
 * @brief Number of descriptors per Tx ring
 *
 * @remark the Tx and Rx ring sizes are independent.
 */
static int iavf_tx_ring_size = IAVF_DEFAULT_RING;
SYSCTL_INT(_hw_iavf, OID_AUTO, tx_ring_size, CTLFLAG_RDTUN,
    &iavf_tx_ring_size, 0, "TX Descriptor Ring Size");

/**
 * @var iavf_rx_ring_size
 * @brief Number of descriptors per Rx ring
 *
 * @remark the Tx and Rx ring sizes are independent.
 */
static int iavf_rx_ring_size = IAVF_DEFAULT_RING;
SYSCTL_INT(_hw_iavf, OID_AUTO, rx_ring_size, CTLFLAG_RDTUN,
    &iavf_rx_ring_size, 0, "TX Descriptor Ring Size");

/**
 * @var iavf_max_queues
 * @brief Number of device queues to allocate
 *
 * If iavf_max_queues is zero, the driver will automatically calculate
 * a suitable value based on the number of CPUs in the system.
 */
int iavf_max_queues = 0;
SYSCTL_INT(_hw_iavf, OID_AUTO, max_queues, CTLFLAG_RDTUN,
    &iavf_max_queues, 0, "Number of Queues");

/**
 * @var iavf_txbrsz
 * @brief Number of entries in the Tx queue buf_ring
 *
 * Increasing this will reduce the number of errors when transmitting
 * fragmented UDP packets.
 */
static int iavf_txbrsz = IAVF_DEFAULT_TXBRSZ;
SYSCTL_INT(_hw_iavf, OID_AUTO, txbr_size, CTLFLAG_RDTUN,
    &iavf_txbrsz, 0, "TX Buf Ring Size");

/**
 * @var iavf_dynamic_rx_itr
 * @brief Control dynamic ITR calcuation support
 *
 * Set to true to enable the driver to dynamically adjust ITR at run time.
 */
int iavf_dynamic_rx_itr = 0;
SYSCTL_INT(_hw_iavf, OID_AUTO, dynamic_rx_itr, CTLFLAG_RDTUN,
    &iavf_dynamic_rx_itr, 0, "Dynamic RX Interrupt Rate");

/**
 * @var iavf_dynamic_tx_itr
 * @brief Control dynamic ITR calcuation support
 *
 * Set to true to enable the driver to dynamically adjust ITR at run time.
 */
int iavf_dynamic_tx_itr = 0;
SYSCTL_INT(_hw_iavf, OID_AUTO, dynamic_tx_itr, CTLFLAG_RDTUN,
    &iavf_dynamic_tx_itr, 0, "Dynamic TX Interrupt Rate");

#endif /* _IAVF_SYSCTLS_LEGACY_H_ */
