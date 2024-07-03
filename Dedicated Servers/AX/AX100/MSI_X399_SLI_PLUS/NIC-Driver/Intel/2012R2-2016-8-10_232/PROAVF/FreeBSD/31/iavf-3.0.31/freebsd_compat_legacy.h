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
 * @file freebsd_compat_legacy.h
 * @brief FreeBSD kernel compatibility definitions for legacy network drivers
 *
 * Contains macros definitions, structure definitions, and function backports
 * used to aid in creating legacy network drivers that compile on a variety of
 * FreeBSD kernel versions.
 * Contains various structure definitions which are used to provide
 * compatibility across kernel versions. It also contains some structures
 * which originated from the iflib implementation such as if_pkt_into and
 * pci_vendor_info.
 */
#ifndef _FREEBSD_COMPAT_LEGACY_H_
#define _FREEBSD_COMPAT_LEGACY_H_

#include <sys/bus.h>
#include <sys/bus_dma.h>

#include "freebsd_compat_common.h"

#if __FreeBSD_version < 1100110
/**
 * @typedef if_t
 * @brief opaque ifnet structure pointer
 *
 * the if_t typedef wasn't introduced until FreeBSD 11, so we backport it to
 * allow the main driver code to use if_t where possible.
 */
typedef struct ifnet * if_t;
#endif

/* ffsll wasn't introduced until FreeBSD 11 */
#if __FreeBSD_version < 1100084
int ffsll(long long);
#endif

/**
 * @typedef_struct pci_vendor_info
 * @brief compatibility shim for PCI vendor info
 *
 * Originally used by the iflib probing code. Since this probing code is
 * rather generic, we re-use the structure for the legacy driver probing
 * instead of implementing our own.
 *
 * We can stop carrying our own copy if the structure is ever moved to a more
 * generic header file.
 */
typedef struct pci_vendor_info {
	uint32_t	pvi_vendor_id;
	uint32_t	pvi_device_id;
	uint32_t	pvi_subvendor_id;
	uint32_t	pvi_subdevice_id;
	uint32_t	pvi_rev_id;
	uint32_t	pvi_class_mask;
	c_caddr_t	pvi_name;
} pci_vendor_info_t;

/**
 * @typedef_struct if_pkt_info
 * @brief compatibility shim representing packet info
 *
 * This structure is used by iflib to extract and store various information
 * from an mbuf. We re-implement this structure here to allow easier sharing
 * of common setup functions in legacy and iflib driver implementations.
 *
 * Note: only values which are actually shared between the iflib and legacy
 * implementations are implemented here. This structure is not intended to be
 * binary compatible with the iflib implementation, only source compatible.
 */
typedef struct if_pkt_info {
	bus_dma_segment_t	*ipi_segs;	/* physical addresses */
	uint32_t		ipi_len;	/* packet length */
	uint16_t		ipi_nsegs;	/* number of segments */
	uint16_t		ipi_pidx;	/* start pidx for encap */

	/* offload handling */
	uint8_t			ipi_ehdrlen;	/* ether header length */
	uint8_t			ipi_ip_hlen;	/* ip header length */
	uint8_t			ipi_tcp_hlen;	/* tcp header length */
	uint8_t			ipi_ipproto;	/* ip protocol */

	uint32_t		ipi_csum_flags;	/* packet checksum flags */
	uint16_t		ipi_tso_segsz;	/* tso segment size */
	uint16_t		ipi_vtag;	/* VLAN tag */
	uint16_t		ipi_etype;	/* ether header type */
	uint8_t			ipi_mflags;	/* packet mbuf flags */
} *if_pkt_info_t;

#undef PVID
#define PVID(vendor, devid, name) {vendor, devid, 0, 0, 0, 0, name}

#undef PVID_OEM
#define PVID_OEM(vendor, devid, svid, sdevid, revid, name) {vendor, devid, svid, sdevid, revid, 0, name}

#undef PVID_END
#define PVID_END {0, 0, 0, 0, 0, 0, NULL}

#define LEGACY_PNP_DESCR "U32:vendor;U32:device;U32:subvendor;U32:subdevice;U32:revision;U32:class;D:human"

/*
 * The number of arguments to MODULE_PNP_INFO() was shortened from 6 to 5
 * between 11 and 12-RELEASE. (r338948)
 */
#if __FreeBSD_version < 1200085
#define LEGACY_PNP_INFO(b, u, t) \
    MODULE_PNP_INFO(LEGACY_PNP_DESCR, b, u, t, sizeof(t[0]), nitems(t))
#else
#define LEGACY_PNP_INFO(b, u, t) \
    MODULE_PNP_INFO(LEGACY_PNP_DESCR, b, u, t, nitems(t))
#endif

#if __FreeBSD_version < 1100110
#include <sys/sysctl.h>

/* SYSCTL_BOOL wasn't implemented until FreeBSD 11 */
int
_kc_sysctl_handle_bool(SYSCTL_HANDLER_ARGS);

/* Oid for a bool.  If ptr is NULL, val is returned. */
#define	SYSCTL_NULL_BOOL_PTR ((bool *)NULL)
#define	SYSCTL_BOOL(parent, nbr, name, access, ptr, val, descr)	\
	SYSCTL_OID(parent, nbr, name,				\
	    CTLTYPE_U64 | CTLFLAG_MPSAFE | (access),		\
	    ptr, val, _kc_sysctl_handle_bool, "CU", descr);	\
	CTASSERT(((access) & CTLTYPE) == 0 &&			\
	    sizeof(bool) == sizeof(*(ptr)))

#define	SYSCTL_ADD_BOOL(ctx, parent, nbr, name, access, ptr, val, descr) \
({									\
	bool *__ptr = (ptr);						\
	CTASSERT(((access) & CTLTYPE) == 0);				\
	sysctl_add_oid(ctx, parent, nbr, name,				\
	    CTLTYPE_U64 | CTLFLAG_MPSAFE | (access),			\
	    __ptr, val, _kc_sysctl_handle_bool, "CU", __DESCR(descr));	\
})

/* Support for U64 types is available in FreeBSD 10, but SYSCTL_ADD_U64 wasn't
 * added until FreeBSD 10
 */
#define	SYSCTL_ADD_U64(ctx, parent, nbr, name, access, ptr, val, descr)	\
({									\
	uint64_t *__ptr = (ptr);					\
	CTASSERT(((access) & CTLTYPE) == 0 ||				\
	    ((access) & SYSCTL_CT_ASSERT_MASK) == CTLTYPE_U64);		\
	sysctl_add_oid(ctx, parent, nbr, name,				\
	    CTLTYPE_U64 | CTLFLAG_MPSAFE | (access),			\
	    __ptr, val, sysctl_handle_64, "QU", __DESCR(descr));	\
})

#endif /* FreeBSD_version < 11 */

#if __FreeBSD_version < 1101515
void *ifr_data_get_ptr(void *ifrp);
#endif

#endif /* _FREEBSD_COMPAT_LEGACY_H_ */
