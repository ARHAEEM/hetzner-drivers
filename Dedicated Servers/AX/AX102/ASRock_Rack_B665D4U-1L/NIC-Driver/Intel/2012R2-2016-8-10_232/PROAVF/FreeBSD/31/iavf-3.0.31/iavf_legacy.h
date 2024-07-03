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
 * @file iavf_legacy.h
 * @brief main header for the legacy driver
 *
 * Contains definitions for structures and functions used throughout the
 * driver code. This header contains the implementation for the legacy driver.
 */
#ifndef _IAVF_LEGACY_H_
#define _IAVF_LEGACY_H_

#ifndef IAVF_NO_IFLIB
#error "Do not include iavf_legacy.h in an iflib build"
#endif

#include "iavf_opts.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf_ring.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/sockio.h>
#include <sys/eventhandler.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/if_var.h>
#include <net/if_arp.h>
#include <net/bpf.h>
#include <net/ethernet.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#include <net/bpf.h>
#include <net/if_types.h>
#include <net/if_vlan_var.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/tcp_lro.h>
#include <netinet/udp.h>
#include <netinet/sctp.h>

#include <machine/in_cksum.h>

#include <sys/bus.h>
#include <sys/pciio.h>
#include <machine/bus.h>
#include <sys/rman.h>
#include <machine/resource.h>
#include <vm/vm.h>
#include <vm/pmap.h>
#include <machine/clock.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <sys/proc.h>
#include <sys/endian.h>
#include <sys/taskqueue.h>
#include <sys/pcpu.h>
#include <sys/smp.h>
#include <sys/sbuf.h>
#include <machine/smp.h>
#include <machine/stdarg.h>

#ifdef RSS
#include <net/rss_config.h>
#endif

#include "iavf_lib.h"
#include "freebsd_compat_legacy.h"

/*
** Default number of entries in Tx queue buf_ring.
*/
#define IAVF_DEFAULT_TXBRSZ	4096

/*
 * This is the max watchdog interval, ie. the time that can
 * pass between any two TX clean operations, such only happening
 * when the TX hardware is functioning.
 *
 * XXX: Watchdog currently counts down in units of (hz)
 * Set this to just (hz) if you want queues to hang under a little bit of stress
 */
#define IAVF_WATCHDOG		(10 * hz)

/*
 * This parameter controls when the driver calls the routine to reclaim
 * transmit descriptors (txeof).
 */
#define IAVF_TX_CLEANUP_THRESHOLD	(que->num_tx_desc / 8)

#define IAVF_CORE_LOCK_ASSERT(sc)	 mtx_assert(&(sc)->mtx, MA_OWNED)
#define IAVF_TX_LOCK(_sc)                mtx_lock(&(_sc)->mtx)
#define IAVF_TX_UNLOCK(_sc)              mtx_unlock(&(_sc)->mtx)
#define IAVF_TX_LOCK_DESTROY(_sc)        mtx_destroy(&(_sc)->mtx)
#define IAVF_TX_TRYLOCK(_sc)             mtx_trylock(&(_sc)->mtx)
#define IAVF_TX_LOCK_ASSERT(_sc)         mtx_assert(&(_sc)->mtx, MA_OWNED)

#define IAVF_RX_LOCK(_sc)                mtx_lock(&(_sc)->mtx)
#define IAVF_RX_UNLOCK(_sc)              mtx_unlock(&(_sc)->mtx)
#define IAVF_RX_LOCK_DESTROY(_sc)        mtx_destroy(&(_sc)->mtx)

#if defined(__amd64__) || defined(i386)
static __inline
void prefetch(void *x)
{
	__asm volatile("prefetcht0 %0" :: "m" (*(unsigned long *)x));
}
#else
#define	prefetch(x)
#endif

#define iavf_sc_from_dev(_dev) \
    ((struct iavf_sc *)device_get_softc(_dev))


/**
 * @struct iavf_tx_buf
 * @brief Tx buffer data
 *
 * Data representing a single Tx buffer entry in a Tx ring.
 */
struct iavf_tx_buf {
	u32		eop_index;
	struct mbuf	*m_head;
	bus_dmamap_t	map;
	bus_dma_tag_t	tag;
};

/**
 * @struct iavf_rx_buf
 * @brief Rx buffer data
 *
 * Data representing a single Rx buffer entry in an Rx ring.
 */
struct iavf_rx_buf {
	struct mbuf	*m_head;
	struct mbuf	*m_pack;
	struct mbuf	*fmp;
	bus_dmamap_t	hmap;
	bus_dmamap_t	pmap;
};

/**
 * @struct tx_ring
 * @brief Transmit ring control struct
 *
 * Structure used to track the hardware Tx ring data.
 */
struct tx_ring {
        struct iavf_queue	*que;
	struct mtx		mtx;
	u32			tail;
	struct iavf_tx_desc	*base;
	struct iavf_dma_mem	dma;
	u16			next_avail;
	u16			next_to_clean;
	u32			itr;
	u32			latency;
	struct iavf_tx_buf	*buffers;
	volatile u16		avail;
	bus_dma_tag_t		tx_tag;
	bus_dma_tag_t		tso_tag;
	char			mtx_name[16];
	struct buf_ring		*br;
	s32			watchdog_timer;

	/* Used for Dynamic ITR calculation */
	u32			packets;
	u32			bytes;

	/* Soft Stats */
	u64			tx_bytes;
	u64			no_desc;
	u64			total_packets;
};

/**
 * @struct rx_ring
 * @brief Receive ring control struct
 *
 * Structure used to track the hardware Rx ring data.
 */
struct rx_ring {
        struct iavf_queue	*que;
	struct mtx		mtx;
	union iavf_rx_desc	*base;
	struct iavf_dma_mem	dma;
	struct lro_ctrl		lro;
	bool			lro_enabled;
	bool			hdr_split;
	bool			discard;
        u32			next_refresh;
        u32			next_check;
	u32			itr;
	u32			latency;
	char			mtx_name[16];
	struct iavf_rx_buf	*buffers;
	u32			mbuf_sz;
	u32			tail;
	bus_dma_tag_t		htag;
	bus_dma_tag_t		ptag;

	/* Used for Dynamic ITR calculation */
	u32			packets;
	u32			bytes;

	/* Soft stats */
	u64			split;
	u64			rx_packets;
	u64			rx_bytes;
	u64			desc_errs;
	u64			not_done;
};

/**
 * @struct iavf_queue
 * @brief Driver queue structure
 *
 * Driver queue structure that acts as an interrupt container for an
 * associated Tx and Rx ring pair.
 */
struct iavf_queue {
	struct iavf_vsi		*vsi;
	u32			me;
	u32			msix;           /* This queue's MSIX vector */
	struct resource		*res;
	void			*tag;
	u16			num_tx_desc;
	u16			num_rx_desc;
	struct tx_ring		txr;
	struct rx_ring		rxr;
	struct task		task;
	struct task		tx_task;
	struct taskqueue	*tq;

	/* Queue stats */
	u64			irqs;
	u64			tso;
	u64			mbuf_defrag_failed;
	u64			mbuf_hdr_failed;
	u64			mbuf_pkt_failed;
	u64			tx_dmamap_failed;
	u64			dropped_pkts;
	u64			mss_too_small;
	u32			tx_pkt_too_small;
};

/**
 * @struct iavf_vsi
 * @brief Virtual Station Interface
 *
 * Data tracking a VSI for an iavf device.
 */
struct iavf_vsi {
	struct iavf_sc		*back;
	struct ifnet		*ifp;
	device_t		dev;
	struct iavf_hw		*hw;
	int			id;
	u16			num_queues;
	int			num_tx_desc;
	int			num_rx_desc;
	u32			rx_itr_setting;
	u32			tx_itr_setting;
	u16			max_frame_size;
	bool			enable_head_writeback;

	struct iavf_queue	*queues;	/* head of queues */

	bool			link_active;

	eventhandler_tag	vlan_attach;
	eventhandler_tag	vlan_detach;
	u16			num_vlans;

	/* Per-VSI stats from hardware */
	struct iavf_eth_stats	eth_stats;
	struct iavf_eth_stats	eth_stats_offsets;
	bool			stat_offsets_loaded;
	/* VSI stat counters */
	u64			ipackets;
	u64			ierrors;
	u64			opackets;
	u64			oerrors;
	u64			ibytes;
	u64			obytes;
	u64			imcasts;
	u64			omcasts;
	u64			iqdrops;
	u64			oqdrops;
	u64			noproto;

	/* Misc. */
	u64			flags;
	struct sysctl_oid	*vsi_node;
	struct sysctl_ctx_list  sysctl_ctx;
};

/**
 * @struct iavf_mac_filter
 * @brief MAC Address filter data
 *
 * Entry in the MAC filter list describing a MAC address filter used to
 * program hardware to filter a specific MAC address.
 */
struct iavf_mac_filter {
	SLIST_ENTRY(iavf_mac_filter)  next;
	u8      macaddr[ETHER_ADDR_LEN];
	u16     flags;
};

/**
 * @struct mac_list
 * @brief MAC filter list head
 *
 * List head type for a singly-linked list of MAC address filters.
 */
SLIST_HEAD(mac_list, iavf_mac_filter);

/**
 * @struct iavf_vlan_filter
 * @brief VLAN filter data
 *
 * Entry in the VLAN filter list describing a VLAN filter used to
 * program hardware to filter traffic on a specific VLAN.
 */
struct iavf_vlan_filter {
	SLIST_ENTRY(iavf_vlan_filter)  next;
	u16     vlan;
	u16     flags;
};

/**
 * @struct vlan_list
 * @brief VLAN filter list head
 *
 * List head type for a singly-linked list of VLAN filters.
 */
SLIST_HEAD(vlan_list, iavf_vlan_filter);

/**
 * @struct iavf_sc
 * @brief Main context structure for the iavf driver
 *
 * Software context structure used to store information about a single device
 * that is loaded by the iavf driver.
 */
struct iavf_sc {
	struct iavf_hw		hw;
	struct iavf_osdep	osdep;
	device_t		dev;

	struct resource		*pci_mem;
	struct resource		*msix_mem;

	/* driver state flags, only access using atomic functions */
	u32			state;

	/*
	 * Interrupt resources
	 */
	void			*tag;
	struct resource		*res; /* For the AQ */

	struct ifmedia		media;
	struct callout		timer;
	int			msix;
	struct virtchnl_version_info version;
	int			if_flags;
	enum iavf_dbg_mask	dbg_mask;

	bool			link_up;
	union {
		enum virtchnl_link_speed link_speed;
		u32		link_speed_adv;
	};

	struct mtx		mtx;

	u32			admvec;
	struct timeout_task	timeout;
	struct task		aq_irq;
	struct task		aq_sched;
	struct taskqueue	*tq;

	struct iavf_vsi		vsi;

	/* Filter lists */
	struct mac_list		*mac_filters;
	struct vlan_list	*vlan_filters;

	/* Promiscuous mode */
	u32			promiscuous_flags; /* Remove */
	u16			promisc_flags;

	/* Admin queue task flags */
	u32			aq_wait_count;

	/* Tunable settings */
	int			tx_itr;
	int			rx_itr;

	/* Virtual comm channel */
	struct virtchnl_vf_resource *vf_res;
	struct virtchnl_vsi_resource *vsi_res;

	/* Misc stats maintained by the driver */
	u64			watchdog_events;
	u64			admin_irq;

	/* Buffer used for reading AQ responses */
	u8			aq_buffer[IAVF_AQ_BUF_SZ];

	/* State flag used in init/stop */
	u32			queues_enabled;
	u8			enable_queues_chan;
	u8			disable_queues_chan;
};

/*
 * Tx/Rx function prototypes
 */
int	iavf_allocate_tx_data(struct iavf_queue *);
int	iavf_allocate_rx_data(struct iavf_queue *);
void	iavf_init_tx_ring(struct iavf_queue *);
int	iavf_init_rx_ring(struct iavf_queue *);
bool	iavf_rxeof(struct iavf_queue *, int);
bool	iavf_txeof(struct iavf_queue *);
void	iavf_free_que_tx(struct iavf_queue *);
void	iavf_free_que_rx(struct iavf_queue *);

int	iavf_mq_start(struct ifnet *, struct mbuf *);
int	iavf_mq_start_locked(struct ifnet *, struct tx_ring *);

void	iavf_deferred_mq_start(void *, int);

int	iavf_queue_hang_check(struct iavf_vsi *);

void	iavf_qflush(struct ifnet *);

#if __FreeBSD_version >= 1100000
uint64_t iavf_get_counter(if_t ifp, ift_counter cnt);
#endif
void	iavf_get_default_rss_key(u32 *);
const char *	iavf_vc_stat_str(struct iavf_hw *hw,
    enum virtchnl_status_code stat_err);

/*
 * Other function prototypes
 */
void	iavf_init(void *);
void	iavf_enable_intr(struct iavf_vsi *);
void	iavf_disable_intr(struct iavf_vsi *);

#endif /* _IAVF_LEGACY_H_ */
