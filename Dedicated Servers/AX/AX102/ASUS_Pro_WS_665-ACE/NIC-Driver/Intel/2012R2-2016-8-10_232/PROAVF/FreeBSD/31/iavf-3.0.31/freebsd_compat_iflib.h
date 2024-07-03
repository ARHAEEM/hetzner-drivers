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
 * @file freebsd_compat_iflib.h
 * @brief FreeBSD kernel compatibility macros for iflib network drivers
 *
 * Contains macro definitions and function backports to aid in developing
 * an iflib network driver that compiles on a variety of FreeBSD kernels.
 *
 * For generic compatibility macros used by both iflib and legacy network
 * drivers, see freebsd_compat_common.[ch]
 */
#ifndef _FREEBSD_COMPAT_IFLIB_H_
#define _FREEBSD_COMPAT_IFLIB_H_

#include "freebsd_compat_common.h"

#include <net/ethernet.h>
#include <net/iflib.h>
#include "ifdi_if.h"

/*
 * Over the course of its development, iflib has made a few API changes, which
 * we need to work around here in order to allow the driver code to work on
 * different versions of the FreeBSD kernel.
 */

#if __FreeBSD_version < 1102000
/**
 * @typedef qidx_t
 * @brief queue index type
 *
 * New versions of iflib introduce the qidx_t type for representing an index
 * into a queue. Backport it as a uint16_t (the type it replaced) so that we
 * can use qidx_t everywhere.
 */
typedef uint16_t qidx_t;
#define QIDX_INVALID 0xFFFF
#else
#define HAVE_RXD_REFILL_USES_RXD_UPDATE_T
#define HAVE_TXRX_OPS_USE_QIDX_T
#define HAVE_IFDI_TX_QUEUE_INTR_ENABLE
#endif

#if __FreeBSD_version >= 1102000
#define HAVE_TXRX_OPS_IN_SOFTC_CTX
#define HAVE_CAPENABLE_IN_SOFTC_CTX
#endif

#if __FreeBSD_version >= 1102000
#define HAVE_IFLIB_SOFTIRQ_ALLOC_USES_IRQ
#endif

#if __FreeBSD_version >= 1200073
#define HAVE_TSO_MAX_IN_SHARED_CTX
#define HAVE_CAPABILITIES_IN_SOFTC_CTX
#endif

#if __FreeBSD_version < 1102000
#undef IFLIB_MAGIC
#define IFLIB_MAGIC ((int)0xCAFEF00D)
#endif

#if (__FreeBSD_version >= 1200085 || \
     (__FreeBSD_version < 1200000 && __FreeBSD_version >= 1103000))
#define HAVE_IFLIB_IN_DETACH
#endif

#if __FreeBSD_version < 1200000
#define IFLIB_CTX_LOCK_IS_MTX
#else
#define IFLIB_ATTACH_HOLDS_CTX_LOCK
#endif

/*
 * Older versions of iflib do not report the TCP header length when requesting
 * that a driver perform TCP checksum offloading. Unfortunately, ice hardware
 * requires the TCP length in order to correctly compute the checksum.
 */
#if __FreeBSD_version < 1102503
#define MISSING_TCP_HLEN_FOR_CSUM_IP_TCP
#endif

/*
 * Older versions of iflib do not correctly enable work arounds necessary for
 * TSO packets to behave properly on our hardware. Check if IFLIB_TSO_INIT_IP
 * exists, and if not, disable TSOs. (Note, this flag was backported to 11.2
 * in FreeBSD_version 1101515)
 */
#ifndef IFLIB_TSO_INIT_IP
#define IFLIB_TSO_INIT_IP (0) /* since we set this in the isc_flags */
#define MISSING_WORKAROUNDS_FOR_CSUM_IP_TSO
#endif

#if (__FreeBSD_version < 1102509 || \
     (__FreeBSD_version >= 1200000 && __FreeBSD_version < 1200085))
/**
 * iflib_request_reset - Request a device reset from iflib
 * @ctx: the iflib context structure
 *
 * Older versions of iflib lack the iflib_request_reset function. It's not
 * possible to reliably backport this function, as we need to directly access
 * the internals of the iflib context structure. Without the ability to
 * request that iflib reset, the system administrator must intervene by
 * bringing the device down and up again. Communicate this to the system
 * administrator by printing a message in the kernel message log.
 */
static inline void
iflib_request_reset(if_ctx_t ctx)
{
	device_t dev = iflib_get_dev(ctx);

	device_printf(dev, "Unable to request an iflib reset. Bring the device down and up to restore functionality.\n");
}
#endif

#if (__FreeBSD_version < 1102509 || \
     (__FreeBSD_version >= 1200000 && __FreeBSD_version < 1200507))
/**
 * iflib_get_rx_mbuf_sz - Determine the Rx buffer size that iflib is using
 * @ctx: the iflib context structure
 *
 * Older versions of iflib do not expose the Rx buffer size used when
 * allocating mbufs. This function mimics the behavior that iflib, by checking
 * the isc_max_frame_size and switching between MCLBYTES and MJUMPAGESIZE.
 */
static inline uint32_t
iflib_get_rx_mbuf_sz(if_ctx_t ctx)
{
	if_softc_ctx_t sctx = iflib_get_softc_ctx(ctx);

	/*
	 * XXX don't set the max_frame_size to larger
	 * than the hardware can handle
	 */
	if (sctx->isc_max_frame_size <= MCLBYTES)
		return MCLBYTES;
	else
		return MJUMPAGESIZE;
}
#endif

#if __FreeBSD_version < 1102509
/**
 * iflib_in_detach - Return true if iflib is detaching the driver
 * @ctx: the iflib context structure
 *
 * Pre-11.3 versions of iflib don't have iflib_in_detach(). The function is
 * used to determine whether iflib is in the process of unloading the driver
 * so that the driver doesn't do things that would lead to it accessing freed
 * memory or de-allocated structures.
 *
 * @returns 0 always, since there is no way to check the private flag the real
 * implementation uses.
 */
static inline uint8_t
iflib_in_detach(if_ctx_t __unused ctx)
{
	return (0);
}
#endif

#if (__FreeBSD_version < 1200516 || \
     (__FreeBSD_version >= 1300000 && __FreeBSD_version < 1300031))
/**
 * iflib_device_probe_vendor - Probe a device, returning BUS_PROBE_VENDOR
 * @dev: the device to probe
 *
 * Calls iflib_device_probe to determine if the given device is valid for this
 * driver. On success, instead of BUS_PROBE_DEFAULT, it will report
 * BUS_PROBE_VENDOR. This is used to indicate that the driver is a vendor
 * driver and should have higher priority than the in-kernel driver.
 *
 * @returns BUS_PROBE_VENDOR or an error indicating the device cannot load
 * with this driver.
 */
static inline int
iflib_device_probe_vendor(device_t dev)
{
	int probe;

	probe = iflib_device_probe(dev);
	if (probe == BUS_PROBE_DEFAULT)
		return (BUS_PROBE_VENDOR);
	else
		return (probe);
}
#endif

/*
 * Older versions of iflib do not support always running the admin task. We'll
 * just define this flag to zero if it doesn't exist so that it is a no-op
 * when bit-wise OR'ing it into .isc_flags.
 */
#ifndef IFLIB_ADMIN_ALWAYS_RUN
#define IFLIB_ADMIN_ALWAYS_RUN (0)
#endif

#ifndef IFLIB_PNP_INFO
#define IFLIB_PNP_DESCR "U32:vendor;U32:device;U32:subvendor;U32:subdevice;U32:revision;U32:class;D:human"
#define IFLIB_PNP_INFO(b, u, t) \
    MODULE_PNP_INFO(IFLIB_PNP_DESCR, b, u, t, sizeof(t[0]), nitems(t))
#endif /* IFLIB_PNP_INFO */

/*
 * The version of iflib pre-11.2 doesn't define these flags,
 * so it's defined here if it doesn't exist.
 * For the scratch, in order to work reliably, though, the driver may
 * actually need iflib to make the mbuf chain writable like it does in 11.2+.
 */
#ifndef IFLIB_NEED_SCRATCH
#define IFLIB_NEED_SCRATCH (0)
#endif
#ifndef IFLIB_INTR_RXTX
#define IFLIB_INTR_RXTX IFLIB_INTR_RX
#endif

/*
 * FreeBSD 12.0 introduced the isc_pause_frames variable to the softc context
 * structure for reporting whether the device saw pause frames since the last
 * timer tick. This was backported into FreeBSD 11.2.
 */
#if ((__FreeBSD_version >= 1101515 && __FreeBSD_version < 1200000) || \
     (__FreeBSD_version >= 1200027))
#define HAVE_SOFTC_CTX_ISC_PAUSE_FRAMES
#endif

#endif /* _FREEBSD_COMPAT_IFLIB_H_ */
