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
 * @file iavf_txrx_legacy.c
 * @brief Legacy Tx/Rx hotpath functions
 *
 * Contains functions used to implement the Tx and Rx hotpaths of the legacy
 * driver implementation.
 */
#include "iavf_legacy.h"
#include "iavf_txrx_common.h"

#ifdef RSS
#include <net/rss_config.h>
#endif

/* Local Prototypes */
static void	iavf_rx_checksum(struct mbuf *, u32, u32, u8);
static void	iavf_refresh_mbufs(struct iavf_queue *, int);
static int      iavf_xmit(struct iavf_queue *, struct mbuf **);
static int	iavf_tx_setup_offload(struct iavf_queue *,
		    struct mbuf *, u32 *, u32 *);
static bool	iavf_tso_setup(struct iavf_queue *, struct mbuf *);
static void	iavf_queue_sw_irq(struct iavf_vsi *, int);

static inline void iavf_rx_discard(struct rx_ring *, int);
static inline void iavf_rx_input(struct rx_ring *, struct ifnet *,
		    struct mbuf *, u8);

static inline bool iavf_tso_detect_sparse(struct mbuf *mp);
static inline u32 iavf_get_tx_head(struct iavf_queue *que);

/**
 * iavf_rx_unrefreshed - Find number of unrefreshed Rx descriptors
 * @que: the Rx queue to check
 *
 * @returns the number of Rx descriptors in the queue that have not yet been
 * refreshed.
 */
static inline u16
iavf_rx_unrefreshed(struct iavf_queue *que)
{
        struct rx_ring	*rxr = &que->rxr;

	if (rxr->next_check > rxr->next_refresh)
		return (rxr->next_check - rxr->next_refresh - 1);
	else
		return ((que->num_rx_desc + rxr->next_check) -
		    rxr->next_refresh - 1);
}

/**
 * iavf_mq_start - Main Transmit entry function
 * @ifp: the ifnet structure
 * @m: packet mbuf to transmit
 *
 * Main entry point for transmitting a packet on a given ifnet device which
 * supports multiple queues.
 *
 * @returns zero on success, or an error code on failure.
 */
int
iavf_mq_start(struct ifnet *ifp, struct mbuf *m)
{
	struct iavf_vsi		*vsi = ifp->if_softc;
	struct iavf_queue	*que;
	struct tx_ring		*txr;
	int			err, i;
#ifdef RSS
	u32			bucket_id;
#endif

	/*
	 * Which queue to use:
	 *
	 * When doing RSS, map it to the same outbound
	 * queue as the incoming flow would be mapped to.
	 * If everything is setup correctly, it should be
	 * the same bucket that the current CPU we're on is.
	 */
	if (M_HASHTYPE_GET(m) != M_HASHTYPE_NONE) {
#ifdef  RSS
		if (rss_hash2bucket(m->m_pkthdr.flowid,
		    M_HASHTYPE_GET(m), &bucket_id) == 0) {
			i = bucket_id % vsi->num_queues;
                } else
#endif
                        i = m->m_pkthdr.flowid % vsi->num_queues;
        } else
		i = curcpu % vsi->num_queues;

	que = &vsi->queues[i];
	txr = &que->txr;

	/* Don't Tx frames that are too small */
	if (__predict_false(m->m_pkthdr.len < IAVF_MIN_FRAME)) {
		que->tx_pkt_too_small++;
		return (EINVAL);
	}

	err = drbr_enqueue(ifp, txr->br, m);
	if (err)
		return (err);
	if (IAVF_TX_TRYLOCK(txr)) {
		iavf_mq_start_locked(ifp, txr);
		IAVF_TX_UNLOCK(txr);
	} else
		taskqueue_enqueue(que->tq, &que->tx_task);

	return (0);
}

/**
 * iavf_mq_start_locked - Transmit a packet in the buf ring
 * @ifp: ifnet structure
 * @txr: the Tx ring to process
 *
 * Main function for processing the Tx bufring to transmit packets on a given
 * ring.
 *
 * @remark assumes the caller holds the Tx ring lock
 *
 * @returns zero on success, or an error code on failure
 */
int
iavf_mq_start_locked(struct ifnet *ifp, struct tx_ring *txr)
{
	struct iavf_queue	*que = txr->que;
	struct iavf_vsi		*vsi = que->vsi;
        struct mbuf		*next;
        int			err = 0;


	if (((ifp->if_drv_flags & IFF_DRV_RUNNING) == 0) ||
	    vsi->link_active == 0)
		return (ENETDOWN);

	/* Process the transmit queue */
	while ((next = drbr_peek(ifp, txr->br)) != NULL) {
		if ((err = iavf_xmit(que, &next)) != 0) {
			if (next == NULL)
				drbr_advance(ifp, txr->br);
			else
				drbr_putback(ifp, txr->br, next);
			break;
		}
		drbr_advance(ifp, txr->br);
		/* Send a copy of the frame to the BPF listener */
		ETHER_BPF_MTAP(ifp, next);
		if ((ifp->if_drv_flags & IFF_DRV_RUNNING) == 0)
			break;
	}

	if (txr->avail < IAVF_TX_CLEANUP_THRESHOLD)
		iavf_txeof(que);

	return (err);
}

/**
 * iavf_deferred_mq_start - Drain queued transmit packets
 * @arg: void pointer to the queue
 * @pending: unused parameter
 *
 * Called from the task queue to drain queued up transmit packets.
 */
void
iavf_deferred_mq_start(void *arg, int __unused pending)
{
	struct iavf_queue	*que = arg;
        struct tx_ring		*txr = &que->txr;
	struct iavf_vsi		*vsi = que->vsi;
        struct ifnet		*ifp = vsi->ifp;

	IAVF_TX_LOCK(txr);
	if (!drbr_empty(ifp, txr->br))
		iavf_mq_start_locked(ifp, txr);
	IAVF_TX_UNLOCK(txr);
}

/**
 * iavf_qflush - Flush all queue ring buffers
 * @ifp: ifnet structure
 *
 * Called to flush (drop) any outstanding packets in each Tx ring.
 */
void
iavf_qflush(struct ifnet *ifp)
{
	struct iavf_vsi	*vsi = ifp->if_softc;

        for (int i = 0; i < vsi->num_queues; i++) {
		struct iavf_queue *que = &vsi->queues[i];
		struct tx_ring	*txr = &que->txr;
		struct mbuf	*m;
		IAVF_TX_LOCK(txr);
		while ((m = buf_ring_dequeue_sc(txr->br)) != NULL)
			m_freem(m);
		IAVF_TX_UNLOCK(txr);
	}
	if_qflush(ifp);
}

/**
 * iavf_tso_detect_sparse - Detect of a TSO is not able to be transmitted
 * @mp: the mbuf cluster to check
 *
 * Check the mbufs associated with a packet to determine if it can properly
 * enable TSO for this hardware.
 *
 * XXX: this implementation is known to be problematic and should be replaced
 * with something more akin to the implementation in the iflib version.
 *
 * @returns true if the packet can be transmitted, false otherwise.
 */
static inline bool
iavf_tso_detect_sparse(struct mbuf *mp)
{
	struct mbuf	*m;
	int		num, mss;

	num = 0;
	mss = mp->m_pkthdr.tso_segsz;

	/* Exclude first mbuf; assume it contains all headers */
	for (m = mp->m_next; m != NULL; m = m->m_next) {
		num++;
		mss -= m->m_len % mp->m_pkthdr.tso_segsz;

		if (num > IAVF_SPARSE_CHAIN)
			return (true);
		if (mss < 1) {
			num = (mss == 0) ? 0 : 1;
			mss += mp->m_pkthdr.tso_segsz;
		}
	}

	return (false);
}


#define IAVF_TXD_CMD (IAVF_TX_DESC_CMD_EOP | IAVF_TX_DESC_CMD_RS)

/**
 * iavf_xmit - Main function to encapsulate and send packets
 * @que: pointer to the Tx queue
 * @m_headp: pointer to the head of the mbuf chain
 *
 * This routine maps the mbufs to tx descriptors, allowing the
 * TX engine to transmit the packets.
 *
 * @remark, the m_headp may be updated on return, for example of the packet
 * could not be transmitted, or if we had to defragment the mbuf chain.
 *
 * @returns 0 on success, positive on failure
 */
static int
iavf_xmit(struct iavf_queue *que, struct mbuf **m_headp)
{
	struct iavf_vsi		*vsi = que->vsi;
	struct iavf_hw		*hw = vsi->hw;
	struct tx_ring		*txr = &que->txr;
	struct iavf_tx_buf	*buf;
	struct iavf_tx_desc	*txd = NULL;
	struct mbuf		*m_head, *m;
	int			i, j, error, nsegs;
	int			first, last = 0;
	u16			vtag = 0;
	u32			cmd, off;
	bus_dmamap_t		map;
	bus_dma_tag_t		tag;
	bus_dma_segment_t	segs[IAVF_MAX_TSO_SEGS];

	cmd = off = 0;
	m_head = *m_headp;

        /*
         * Important to capture the first descriptor
         * used because it will contain the index of
         * the one we tell the hardware to report back
         */
        first = txr->next_avail;
	buf = &txr->buffers[first];
	map = buf->map;
	tag = txr->tx_tag;

	if (m_head->m_pkthdr.csum_flags & CSUM_TSO) {
		/* Use larger mapping for TSO */
		tag = txr->tso_tag;
		if (iavf_tso_detect_sparse(m_head)) {
			m = m_defrag(m_head, M_NOWAIT);
			if (m == NULL) {
				m_freem(*m_headp);
				*m_headp = NULL;
				return (ENOBUFS);
			}
			*m_headp = m;
		}
	}

	/*
	 * Map the packet for DMA.
	 */
	error = bus_dmamap_load_mbuf_sg(tag, map,
	    *m_headp, segs, &nsegs, BUS_DMA_NOWAIT);

	if (error == EFBIG) {
		struct mbuf *m;

		m = m_defrag(*m_headp, M_NOWAIT);
		if (m == NULL) {
			que->mbuf_defrag_failed++;
			m_freem(*m_headp);
			*m_headp = NULL;
			return (ENOBUFS);
		}
		*m_headp = m;

		/* Try it again */
		error = bus_dmamap_load_mbuf_sg(tag, map,
		    *m_headp, segs, &nsegs, BUS_DMA_NOWAIT);

		if (error != 0) {
			que->tx_dmamap_failed++;
			m_freem(*m_headp);
			*m_headp = NULL;
			return (error);
		}
	} else if (error != 0) {
		que->tx_dmamap_failed++;
		m_freem(*m_headp);
		*m_headp = NULL;
		return (error);
	}

	/* Make certain there are enough descriptors */
	if (nsegs > txr->avail - 2) {
		txr->no_desc++;
		error = ENOBUFS;
		goto xmit_fail;
	}
	m_head = *m_headp;

	/* Set up the TSO/CSUM offload */
	if (m_head->m_pkthdr.csum_flags & CSUM_OFFLOAD) {
		error = iavf_tx_setup_offload(que, m_head, &cmd, &off);
		if (error)
			goto xmit_fail;
	}

	cmd |= IAVF_TX_DESC_CMD_ICRC;
	/* Grab the VLAN tag */
	if (m_head->m_flags & M_VLANTAG) {
		cmd |= IAVF_TX_DESC_CMD_IL2TAG1;
		vtag = htole16(m_head->m_pkthdr.ether_vtag);
	}

	i = txr->next_avail;
	for (j = 0; j < nsegs; j++) {
		bus_size_t seglen;

		buf = &txr->buffers[i];
		buf->tag = tag; /* Keep track of the type tag */
		txd = &txr->base[i];
		seglen = segs[j].ds_len;

		txd->buffer_addr = htole64(segs[j].ds_addr);
		txd->cmd_type_offset_bsz =
		    htole64(IAVF_TX_DESC_DTYPE_DATA
		    | ((u64)cmd  << IAVF_TXD_QW1_CMD_SHIFT)
		    | ((u64)off << IAVF_TXD_QW1_OFFSET_SHIFT)
		    | ((u64)seglen  << IAVF_TXD_QW1_TX_BUF_SZ_SHIFT)
		    | ((u64)vtag  << IAVF_TXD_QW1_L2TAG1_SHIFT));

		last = i; /* descriptor that will get completion IRQ */

		if (++i == que->num_tx_desc)
			i = 0;

		buf->m_head = NULL;
		buf->eop_index = -1;
	}
	/* Set the last descriptor for report */
	txd->cmd_type_offset_bsz |=
	    htole64(((u64)IAVF_TXD_CMD << IAVF_TXD_QW1_CMD_SHIFT));
	txr->avail -= nsegs;
	txr->next_avail = i;

	buf->m_head = m_head;
	/* Swap the dma map between the first and last descriptor.
	 * The descriptor that gets checked on completion will now
	 * have the real map from the first descriptor.
	 */
	txr->buffers[first].map = buf->map;
	buf->map = map;
	bus_dmamap_sync(tag, map, BUS_DMASYNC_PREWRITE);

        /* Set the index of the descriptor that will be marked done */
        buf = &txr->buffers[first];
	buf->eop_index = last;

        bus_dmamap_sync(txr->dma.tag, txr->dma.map,
            BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	/*
	 * Advance the Transmit Descriptor Tail (Tdt), this tells the
	 * hardware that this frame is available to transmit.
	 */
	++txr->total_packets;
	wr32(hw, txr->tail, i);

	/* Mark outstanding work */
	atomic_store_rel_32(&txr->watchdog_timer, IAVF_WATCHDOG);
	return (0);

xmit_fail:
	bus_dmamap_unload(tag, buf->map);
	return (error);
}


/**
 * iavf_allocate_tx_data - Allocate Tx buffer structures
 * @que: queue pointer
 *
 * Allocate memory for tx_buffer structures. The tx_buffer stores all
 * the information needed to transmit a packet on the wire. This is
 * called only once at attach, setup is done every reset.
 *
 * @returns zero on success, or an error code on failure.
 */
int
iavf_allocate_tx_data(struct iavf_queue *que)
{
	struct tx_ring		*txr = &que->txr;
	struct iavf_vsi		*vsi = que->vsi;
	device_t		dev = vsi->dev;
	struct iavf_tx_buf	*buf;
	int			i, error = 0;

	/*
	 * Setup DMA descriptor areas.
	 */
	if ((error = bus_dma_tag_create(bus_get_dma_tag(dev), /* parent */
			       1, 0,			/* alignment, bounds */
			       BUS_SPACE_MAXADDR,	/* lowaddr */
			       BUS_SPACE_MAXADDR,	/* highaddr */
			       NULL, NULL,		/* filter, filterarg */
			       IAVF_TSO_SIZE,		/* maxsize */
			       IAVF_MAX_TX_SEGS,		/* nsegments */
			       IAVF_MAX_DMA_SEG_SIZE,	/* maxsegsize */
			       0,			/* flags */
			       NULL,			/* lockfunc */
			       NULL,			/* lockfuncarg */
			       &txr->tx_tag))) {
		device_printf(dev,"Unable to allocate TX DMA tag\n");
		return (error);
	}

	/* Make a special tag for TSO */
	if ((error = bus_dma_tag_create(bus_get_dma_tag(dev), /* parent */
			       1, 0,			/* alignment, bounds */
			       BUS_SPACE_MAXADDR,	/* lowaddr */
			       BUS_SPACE_MAXADDR,	/* highaddr */
			       NULL, NULL,		/* filter, filterarg */
			       IAVF_TSO_SIZE,		/* maxsize */
			       IAVF_MAX_TSO_SEGS,	/* nsegments */
			       IAVF_MAX_DMA_SEG_SIZE,	/* maxsegsize */
			       0,			/* flags */
			       NULL,			/* lockfunc */
			       NULL,			/* lockfuncarg */
			       &txr->tso_tag))) {
		device_printf(dev,"Unable to allocate TX TSO DMA tag\n");
		goto free_tx_dma;
	}

	if (!(txr->buffers =
	    (struct iavf_tx_buf *) malloc(sizeof(struct iavf_tx_buf) *
	    que->num_tx_desc, M_IAVF, M_NOWAIT | M_ZERO))) {
		device_printf(dev, "Unable to allocate tx_buffer memory\n");
		error = ENOMEM;
		goto free_tx_tso_dma;
	}

        /* Create the descriptor buffer default dma maps */
	buf = txr->buffers;
	for (i = 0; i < que->num_tx_desc; i++, buf++) {
		buf->tag = txr->tx_tag;
		error = bus_dmamap_create(buf->tag, 0, &buf->map);
		if (error != 0) {
			device_printf(dev, "Unable to create TX DMA map\n");
			goto free_buffers;
		}
	}

	return 0;

free_buffers:
	while (i--) {
		buf--;
		bus_dmamap_destroy(buf->tag, buf->map);
	}

	free(txr->buffers, M_IAVF);
	txr->buffers = NULL;
free_tx_tso_dma:
	bus_dma_tag_destroy(txr->tso_tag);
	txr->tso_tag = NULL;
free_tx_dma:
	bus_dma_tag_destroy(txr->tx_tag);
	txr->tx_tag = NULL;

	return (error);
}


/**
 * iavf_init_tx_ring - Initialize a queue transmit ring
 * @que: the queue to initialize
 *
 * (Re)Initialize a queue transmit ring. called by init, it clears the
 * descriptor ring, and frees any stale mbufs.
 */
void
iavf_init_tx_ring(struct iavf_queue *que)
{
	struct tx_ring		*txr = &que->txr;
	struct iavf_tx_buf	*buf;
	struct iavf_vsi *vsi = que->vsi;

	/* Clear the old ring contents */
	IAVF_TX_LOCK(txr);

	bzero((void *)txr->base,
	      (sizeof(struct iavf_tx_desc)) *
	      (que->num_tx_desc + (vsi->enable_head_writeback ? 1 : 0)));

	/* Reset indices */
	txr->next_avail = 0;
	txr->next_to_clean = 0;

	/* Reset watchdog status */
	txr->watchdog_timer = 0;

	/* Free any existing tx mbufs. */
        buf = txr->buffers;
	for (int i = 0; i < que->num_tx_desc; i++, buf++) {
		if (buf->m_head != NULL) {
			bus_dmamap_sync(buf->tag, buf->map,
			    BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(buf->tag, buf->map);
			m_freem(buf->m_head);
			buf->m_head = NULL;
		}
		/* Clear the EOP index */
		buf->eop_index = -1;
        }

	/* Set number of descriptors available */
	txr->avail = que->num_tx_desc;

	/* Reset tail register */
	wr32(vsi->hw, txr->tail, 0);

	bus_dmamap_sync(txr->dma.tag, txr->dma.map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	IAVF_TX_UNLOCK(txr);
}

/**
 * iavf_free_que_tx - Free transmit related data structures
 * @que: queue to free
 *
 * Frees resources allocated when setting up Tx queues.
 */
void
iavf_free_que_tx(struct iavf_queue *que)
{
	struct tx_ring *txr = &que->txr;
	struct iavf_tx_buf *buf;

	INIT_DBG_IF(que->vsi->ifp, "queue %d: begin", que->me);

	for (int i = 0; i < que->num_tx_desc; i++) {
		buf = &txr->buffers[i];
		if (buf->m_head != NULL) {
			bus_dmamap_sync(buf->tag, buf->map,
			    BUS_DMASYNC_POSTWRITE);
			m_freem(buf->m_head);
			buf->m_head = NULL;
		}
		bus_dmamap_unload(buf->tag, buf->map);
		bus_dmamap_destroy(buf->tag, buf->map);
	}
	if (txr->buffers != NULL) {
		free(txr->buffers, M_IAVF);
		txr->buffers = NULL;
	}
	if (txr->tx_tag != NULL) {
		bus_dma_tag_destroy(txr->tx_tag);
		txr->tx_tag = NULL;
	}
	if (txr->tso_tag != NULL) {
		bus_dma_tag_destroy(txr->tso_tag);
		txr->tso_tag = NULL;
	}

	INIT_DBG_IF(que->vsi->ifp, "queue %d: end", que->me);
	return;
}

/*********************************************************************
 *
 *  Setup descriptor for hw offloads 
 *
 **********************************************************************/

/**
 * iavf_tx_setup_offload - Setup hardware offload
 * @que: queue pointer
 * @mp: mbuf to process
 * @cmd: descriptor command value
 * @off: descriptor offset value
 *
 * Based on the packet type, and what offloads have been requested, updates
 * the Tx descriptor values to enable hardware offloads.
 *
 * @returns zero on success, or an error code on failure.
 */
static int
iavf_tx_setup_offload(struct iavf_queue *que,
    struct mbuf *mp, u32 *cmd, u32 *off)
{
	struct ether_vlan_header	*eh;
#ifdef INET
	struct ip			*ip = NULL;
#endif
	struct tcphdr			*th = NULL;
#ifdef INET6
	struct ip6_hdr			*ip6;
#endif
	int				elen, ip_hlen = 0, tcp_hlen;
	u16				etype;
	u8				ipproto = 0;
	bool				tso = FALSE;

	/* Set up the TSO context descriptor if required */
	if (mp->m_pkthdr.csum_flags & CSUM_TSO) {
		tso = iavf_tso_setup(que, mp);
		if (tso)
			++que->tso;
		else
			return (ENXIO);
	}

	/*
	 * Determine where frame payload starts.
	 * Jump over vlan headers if already present,
	 * helpful for QinQ too.
	 */
	eh = mtod(mp, struct ether_vlan_header *);
	if (eh->evl_encap_proto == htons(ETHERTYPE_VLAN)) {
		etype = ntohs(eh->evl_proto);
		elen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
	} else {
		etype = ntohs(eh->evl_encap_proto);
		elen = ETHER_HDR_LEN;
	}

	switch (etype) {
#ifdef INET
		case ETHERTYPE_IP:
			ip = (struct ip *)(mp->m_data + elen);
			ip_hlen = ip->ip_hl << 2;
			ipproto = ip->ip_p;
			th = (struct tcphdr *)((caddr_t)ip + ip_hlen);
			/* The IP checksum must be recalculated with TSO */
			if (tso)
				*cmd |= IAVF_TX_DESC_CMD_IIPT_IPV4_CSUM;
			else
				*cmd |= IAVF_TX_DESC_CMD_IIPT_IPV4;
			break;
#endif
#ifdef INET6
		case ETHERTYPE_IPV6:
			ip6 = (struct ip6_hdr *)(mp->m_data + elen);
			ip_hlen = sizeof(struct ip6_hdr);
			ipproto = ip6->ip6_nxt;
			th = (struct tcphdr *)((caddr_t)ip6 + ip_hlen);
			*cmd |= IAVF_TX_DESC_CMD_IIPT_IPV6;
			break;
#endif
		default:
			break;
	}

	*off |= (elen >> 1) << IAVF_TX_DESC_LENGTH_MACLEN_SHIFT;
	*off |= (ip_hlen >> 2) << IAVF_TX_DESC_LENGTH_IPLEN_SHIFT;

	switch (ipproto) {
		case IPPROTO_TCP:
			tcp_hlen = th->th_off << 2;
			if (mp->m_pkthdr.csum_flags & (CSUM_TCP|CSUM_TCP_IPV6)) {
				*cmd |= IAVF_TX_DESC_CMD_L4T_EOFT_TCP;
				*off |= (tcp_hlen >> 2) <<
				    IAVF_TX_DESC_LENGTH_L4_FC_LEN_SHIFT;
			}
			break;
		case IPPROTO_UDP:
			if (mp->m_pkthdr.csum_flags & (CSUM_UDP|CSUM_UDP_IPV6)) {
				*cmd |= IAVF_TX_DESC_CMD_L4T_EOFT_UDP;
				*off |= (sizeof(struct udphdr) >> 2) <<
				    IAVF_TX_DESC_LENGTH_L4_FC_LEN_SHIFT;
			}
			break;
		case IPPROTO_SCTP:
			if (mp->m_pkthdr.csum_flags & (CSUM_SCTP|CSUM_SCTP_IPV6)) {
				*cmd |= IAVF_TX_DESC_CMD_L4T_EOFT_SCTP;
				*off |= (sizeof(struct sctphdr) >> 2) <<
				    IAVF_TX_DESC_LENGTH_L4_FC_LEN_SHIFT;
			}
			/* Fall Thru */
		default:
			break;
	}

        return (0);
}

/**
 * iavf_tso_setup - Setup TSO context descriptor
 * @que: queue pointer
 * @mp: mbuf pointer to process
 *
 * Sets up a context descriptor to prepare the hardware for enabling
 * segmentation offload for the next data packet.
 *
 * @returns true if the TSO context was setup, and false otherwise.
 */
static bool
iavf_tso_setup(struct iavf_queue *que, struct mbuf *mp)
{
	struct tx_ring			*txr = &que->txr;
	struct iavf_tx_context_desc	*TXD;
	struct iavf_tx_buf		*buf;
	u32				cmd, mss, type, tsolen;
	u16				etype;
	int				idx, elen, ip_hlen, tcp_hlen;
	struct ether_vlan_header	*eh;
#ifdef INET
	struct ip			*ip;
#endif
#ifdef INET6
	struct ip6_hdr			*ip6;
#endif
#if defined(INET6) || defined(INET)
	struct tcphdr			*th;
#endif
	u64				type_cmd_tso_mss;

	/*
	 * Determine where frame payload starts.
	 * Jump over vlan headers if already present
	 */
	eh = mtod(mp, struct ether_vlan_header *);
	if (eh->evl_encap_proto == htons(ETHERTYPE_VLAN)) {
		elen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
		etype = eh->evl_proto;
	} else {
		elen = ETHER_HDR_LEN;
		etype = eh->evl_encap_proto;
	}

        switch (ntohs(etype)) {
#ifdef INET6
	case ETHERTYPE_IPV6:
		ip6 = (struct ip6_hdr *)(mp->m_data + elen);
		if (ip6->ip6_nxt != IPPROTO_TCP)
			return FALSE;
		ip_hlen = sizeof(struct ip6_hdr);
		th = (struct tcphdr *)((caddr_t)ip6 + ip_hlen);
		th->th_sum = in6_cksum_pseudo(ip6, 0, IPPROTO_TCP, 0);
		tcp_hlen = th->th_off << 2;
		/*
		 * The corresponding flag is set by the stack in the IPv4
		 * TSO case, but not in IPv6 (at least in FreeBSD 10.2).
		 * So, set it here because the rest of the flow requires it.
		 */
		mp->m_pkthdr.csum_flags |= CSUM_TCP_IPV6;
		break;
#endif
#ifdef INET
	case ETHERTYPE_IP:
		ip = (struct ip *)(mp->m_data + elen);
		if (ip->ip_p != IPPROTO_TCP)
			return FALSE;
		ip->ip_sum = 0;
		ip_hlen = ip->ip_hl << 2;
		th = (struct tcphdr *)((caddr_t)ip + ip_hlen);
		th->th_sum = in_pseudo(ip->ip_src.s_addr,
		    ip->ip_dst.s_addr, htons(IPPROTO_TCP));
		tcp_hlen = th->th_off << 2;
		break;
#endif
	default:
		printf("%s: CSUM_TSO but no supported IP version (0x%04x)",
		    __func__, ntohs(etype));
		return FALSE;
        }

        /* Ensure we have at least the IP+TCP header in the first mbuf. */
        if (mp->m_len < elen + ip_hlen + (int)sizeof(struct tcphdr))
		return FALSE;

	idx = txr->next_avail;
	buf = &txr->buffers[idx];
	TXD = (struct iavf_tx_context_desc *) &txr->base[idx];
	tsolen = mp->m_pkthdr.len - (elen + ip_hlen + tcp_hlen);

	type = IAVF_TX_DESC_DTYPE_CONTEXT;
	cmd = IAVF_TX_CTX_DESC_TSO;
	/* TSO MSS must not be less than 64 */
	if (mp->m_pkthdr.tso_segsz < IAVF_MIN_TSO_MSS) {
		que->mss_too_small++;
		mp->m_pkthdr.tso_segsz = IAVF_MIN_TSO_MSS;
	}
	mss = mp->m_pkthdr.tso_segsz;

	type_cmd_tso_mss = ((u64)type << IAVF_TXD_QW1_DTYPE_SHIFT) |
	    ((u64)cmd << IAVF_TXD_CTX_QW1_CMD_SHIFT) |
	    ((u64)tsolen << IAVF_TXD_CTX_QW1_TSO_LEN_SHIFT) |
	    ((u64)mss << IAVF_TXD_CTX_QW1_MSS_SHIFT);
	TXD->type_cmd_tso_mss = htole64(type_cmd_tso_mss);

	TXD->tunneling_params = htole32(0);
	buf->m_head = NULL;
	buf->eop_index = -1;

	if (++idx == que->num_tx_desc)
		idx = 0;

	txr->avail--;
	txr->next_avail = idx;

	return TRUE;
}

/**
 * iavf_get_tx_head - Retrieve head index from HW
 * @que: the queue to check
 *
 * @returns the head index in the hardware head register.
 */
static inline u32
iavf_get_tx_head(struct iavf_queue *que)
{
	struct tx_ring  *txr = &que->txr;
	void *head = &txr->base[que->num_tx_desc];
	return LE32_TO_CPU(*(volatile __le32 *)head);
}

/**
 * iavf_txeof_hwb - Clean descriptors after hardware is done
 * @que: the queue to clean
 *
 * Get index of last used descriptor/buffer from hardware, and clean
 * the descriptors/buffers up to that index.
 *
 * @remark this implementation is for the head write back mode.
 *
 * @returns true if there are still packets to clean, false otherwise.
 */
static bool
iavf_txeof_hwb(struct iavf_queue *que)
{
	struct tx_ring		*txr = &que->txr;
	u16			first, last, head, done;
	struct iavf_tx_buf	*buf;

	mtx_assert(&txr->mtx, MA_OWNED);

	/* These are not the descriptors you seek, move along :) */
	if (txr->avail == que->num_tx_desc) {
		atomic_store_rel_32(&txr->watchdog_timer, 0);
		return FALSE;
	}

	first = txr->next_to_clean;
	buf = &txr->buffers[first];
	last = buf->eop_index;
	if (last == (u16)-1)
		return FALSE;

	/* Sync DMA before reading head index from ring */
        bus_dmamap_sync(txr->dma.tag, txr->dma.map,
            BUS_DMASYNC_POSTREAD);

	/* Get the Head WB value */
	head = iavf_get_tx_head(que);

	/*
	** Get the index of the first descriptor
	** BEYOND the EOP and call that 'done'.
	** I do this so the comparison in the
	** inner while loop below can be simple
	*/
	if (++last == que->num_tx_desc) last = 0;
	done = last;

	/*
	** The HEAD index of the ring is written in a 
	** defined location, this rather than a done bit
	** is what is used to keep track of what must be
	** 'cleaned'.
	*/
	while (first != head) {
		/* We clean the range of the packet */
		while (first != done) {
			++txr->avail;

			if (buf->m_head) {
				txr->bytes += /* for ITR adjustment */
				    buf->m_head->m_pkthdr.len;
				txr->tx_bytes += /* for TX stats */
				    buf->m_head->m_pkthdr.len;
				bus_dmamap_sync(buf->tag,
				    buf->map,
				    BUS_DMASYNC_POSTWRITE);
				bus_dmamap_unload(buf->tag,
				    buf->map);
				m_freem(buf->m_head);
				buf->m_head = NULL;
			}
			buf->eop_index = -1;

			if (++first == que->num_tx_desc)
				first = 0;

			buf = &txr->buffers[first];
		}
		++txr->packets;
		/* If a packet was successfully cleaned, reset the watchdog timer */
		atomic_store_rel_32(&txr->watchdog_timer, IAVF_WATCHDOG);
		/* See if there is more work now */
		last = buf->eop_index;
		if (last != (u16) - 1) {
			/* Get next done point */
			if (++last == que->num_tx_desc)
				last = 0;
			done = last;
		} else
			break;
	}
	bus_dmamap_sync(txr->dma.tag, txr->dma.map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	txr->next_to_clean = first;

	/*
	 * If there are no pending descriptors, clear the timeout.
	 */
	if (txr->avail == que->num_tx_desc) {
		atomic_store_rel_32(&txr->watchdog_timer, 0);
		return FALSE;
	}

	return TRUE;
}

/**
 * iavf_txeof_dwb - Cleanup used Tx descriptors
 * @que: the queue pointer
 *
 * Use index kept by driver and the flag on each descriptor to find used
 * descriptor/buffers and clean them up for re-use.
 *
 * @remark this implementation is for the descriptor write back mode.
 *
 * @returns TRUE if there are more descriptors to be cleaned after this
 * function exits.
 */
static bool
iavf_txeof_dwb(struct iavf_queue *que)
{
	struct tx_ring		*txr = &que->txr;
	u16			first, last, done;
	u16			limit = 256;
	struct iavf_tx_buf	*buf;
	struct iavf_tx_desc	*eop_desc;

	mtx_assert(&txr->mtx, MA_OWNED);

	/* There are no descriptors to clean */
	if (txr->avail == que->num_tx_desc) {
		atomic_store_rel_32(&txr->watchdog_timer, 0);
		return FALSE;
	}

	/* Set starting index/descriptor/buffer */
	first = txr->next_to_clean;
	buf = &txr->buffers[first];

	/*
	 * This function operates per-packet -- identifies the start of the
	 * packet and gets the index of the last descriptor of the packet from
	 * it, from eop_index.
	 *
	 * If the last descriptor is marked "done" by the hardware, then all
	 * of the descriptors for the packet are cleaned.
	 */
	last = buf->eop_index;
	if (last == (u16)-1)
		return FALSE;
	eop_desc = &txr->base[last];

	/* Sync DMA before reading from ring */
        bus_dmamap_sync(txr->dma.tag, txr->dma.map, BUS_DMASYNC_POSTREAD);

	/*
	 * Get the index of the first descriptor beyond the EOP and call that
	 * 'done'. Simplifies the comparison for the inner loop below.
	 */
	if (++last == que->num_tx_desc)
		last = 0;
	done = last;

	/*
	 * We find the last completed descriptor by examining each
	 * descriptor's status bits to see if it's done.
	 */
	do {
		/* Break if last descriptor in packet isn't marked done */
		if ((eop_desc->cmd_type_offset_bsz & IAVF_TXD_QW1_DTYPE_MASK)
		    != IAVF_TX_DESC_DTYPE_DESC_DONE)
			break;

		/* Clean the descriptors that make up the processed packet */
		while (first != done) {
			/*
			 * If there was a buffer attached to this descriptor,
			 * prevent the adapter from accessing it, and add its
			 * length to the queue's TX stats.
			 */
			if (buf->m_head) {
				txr->bytes += buf->m_head->m_pkthdr.len;
				txr->tx_bytes += buf->m_head->m_pkthdr.len;
				bus_dmamap_sync(buf->tag, buf->map,
				    BUS_DMASYNC_POSTWRITE);
				bus_dmamap_unload(buf->tag, buf->map);
				m_freem(buf->m_head);
				buf->m_head = NULL;
			}
			buf->eop_index = -1;
			++txr->avail;

			if (++first == que->num_tx_desc)
				first = 0;
			buf = &txr->buffers[first];
		}
		++txr->packets;
		/* If a packet was successfully cleaned, reset the watchdog timer */
		atomic_store_rel_32(&txr->watchdog_timer, IAVF_WATCHDOG);

		/*
		 * Since buf is the first buffer after the one that was just
		 * cleaned, check if the packet it starts is done, too.
		 */
		last = buf->eop_index;
		if (last != (u16)-1) {
			eop_desc = &txr->base[last];
			/* Get next done point */
			if (++last == que->num_tx_desc) last = 0;
			done = last;
		} else
			break;
	} while (--limit);

	bus_dmamap_sync(txr->dma.tag, txr->dma.map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	txr->next_to_clean = first;

	/*
	 * If there are no pending descriptors, clear the watchdog timer.
	 */
	if (txr->avail == que->num_tx_desc) {
		atomic_store_rel_32(&txr->watchdog_timer, 0);
		return FALSE;
	}

	return TRUE;
}

/**
 * iavf_txeof - Cleanup used Tx descriptors
 * @que: queue pointer
 *
 * Calls the correct implementation of iavf_txeof based on whether the queue
 * is in head write back or descriptor write back mode.
 *
 * @returns true if there are more packets to clean, false otherwise.
 */
bool
iavf_txeof(struct iavf_queue *que)
{
	struct iavf_vsi *vsi = que->vsi;

	return (vsi->enable_head_writeback) ? iavf_txeof_hwb(que)
	    : iavf_txeof_dwb(que);
}


/**
 * iavf_refresh_mbufs - Refresh Rx descriptor ring buffers
 * @que: queue pointer
 * @limit: maximum number of buffers to refresh
 *
 * now keeps its own state so discards due to resource exhaustion are
 * unnecessary, if an mbuf cannot be obtained it just returns, keeping its
 * placeholder, thus it can simply be recalled to try again.
 */
static void
iavf_refresh_mbufs(struct iavf_queue *que, int limit)
{
	struct iavf_vsi		*vsi = que->vsi;
	struct rx_ring		*rxr = &que->rxr;
	bus_dma_segment_t	hseg[1];
	bus_dma_segment_t	pseg[1];
	struct iavf_rx_buf	*buf;
	struct mbuf		*mh, *mp;
	int			i, j, nsegs, error;
	bool			refreshed = FALSE;

	i = j = rxr->next_refresh;
	/* Control the loop with one beyond */
	if (++j == que->num_rx_desc)
		j = 0;

	while (j != limit) {
		buf = &rxr->buffers[i];
		if (rxr->hdr_split == FALSE)
			goto no_split;

		if (buf->m_head == NULL) {
			mh = m_gethdr(M_NOWAIT, MT_DATA);
			if (mh == NULL)
				goto update;
		} else
			mh = buf->m_head;

		mh->m_pkthdr.len = mh->m_len = MHLEN;
		mh->m_len = MHLEN;
		mh->m_flags |= M_PKTHDR;
		/* Get the memory mapping */
		error = bus_dmamap_load_mbuf_sg(rxr->htag,
		    buf->hmap, mh, hseg, &nsegs, BUS_DMA_NOWAIT);
		if (error != 0) {
			printf("Refresh mbufs: hdr dmamap load"
			    " failure - %d\n", error);
			m_free(mh);
			buf->m_head = NULL;
			goto update;
		}
		buf->m_head = mh;
		bus_dmamap_sync(rxr->htag, buf->hmap,
		    BUS_DMASYNC_PREREAD);
		rxr->base[i].read.hdr_addr =
		   htole64(hseg[0].ds_addr);

no_split:
		if (buf->m_pack == NULL) {
			mp = m_getjcl(M_NOWAIT, MT_DATA,
			    M_PKTHDR, rxr->mbuf_sz);
			if (mp == NULL)
				goto update;
		} else
			mp = buf->m_pack;

		mp->m_pkthdr.len = mp->m_len = rxr->mbuf_sz;
		/* Get the memory mapping */
		error = bus_dmamap_load_mbuf_sg(rxr->ptag,
		    buf->pmap, mp, pseg, &nsegs, BUS_DMA_NOWAIT);
		if (error != 0) {
			printf("Refresh mbufs: payload dmamap load"
			    " failure - %d\n", error);
			m_free(mp);
			buf->m_pack = NULL;
			goto update;
		}
		buf->m_pack = mp;
		bus_dmamap_sync(rxr->ptag, buf->pmap,
		    BUS_DMASYNC_PREREAD);
		rxr->base[i].read.pkt_addr =
		   htole64(pseg[0].ds_addr);
		/* Used only when doing header split */
		rxr->base[i].read.hdr_addr = 0;

		refreshed = TRUE;
		/* Next is precalculated */
		i = j;
		rxr->next_refresh = i;
		if (++j == que->num_rx_desc)
			j = 0;
	}
update:
	if (refreshed) /* Update hardware tail index */
		wr32(vsi->hw, rxr->tail, rxr->next_refresh);
	return;
}


/**
 * iavf_allocate_rx_data - Allocate Rx data for a queue
 * @que: queue to allocate
 *
 * Allocate memory for rx_buffer structures. Since we use one
 * rx_buffer per descriptor, the maximum number of rx_buffer's
 * that we'll need is equal to the number of receive descriptors
 * that we've defined.
 *
 * @returns zero on success, or an error code on failure.
 */
int
iavf_allocate_rx_data(struct iavf_queue *que)
{
	struct rx_ring		*rxr = &que->rxr;
	struct iavf_vsi		*vsi = que->vsi;
	device_t		dev = vsi->dev;
	struct iavf_rx_buf	*buf;
	int			i, bsize, error;

	if ((error = bus_dma_tag_create(bus_get_dma_tag(dev),	/* parent */
				   1, 0,		/* alignment, bounds */
				   BUS_SPACE_MAXADDR,	/* lowaddr */
				   BUS_SPACE_MAXADDR,	/* highaddr */
				   NULL, NULL,		/* filter, filterarg */
				   MSIZE,		/* maxsize */
				   1,			/* nsegments */
				   MSIZE,		/* maxsegsize */
				   0,			/* flags */
				   NULL,		/* lockfunc */
				   NULL,		/* lockfuncarg */
				   &rxr->htag))) {
		device_printf(dev, "Unable to create RX DMA htag\n");
		return (error);
	}

	if ((error = bus_dma_tag_create(bus_get_dma_tag(dev),	/* parent */
				   1, 0,		/* alignment, bounds */
				   BUS_SPACE_MAXADDR,	/* lowaddr */
				   BUS_SPACE_MAXADDR,	/* highaddr */
				   NULL, NULL,		/* filter, filterarg */
				   MJUM16BYTES,		/* maxsize */
				   1,			/* nsegments */
				   MJUM16BYTES,		/* maxsegsize */
				   0,			/* flags */
				   NULL,		/* lockfunc */
				   NULL,		/* lockfuncarg */
				   &rxr->ptag))) {
		device_printf(dev, "Unable to create RX DMA ptag\n");
		goto free_rx_htag;
	}

	bsize = sizeof(struct iavf_rx_buf) * que->num_rx_desc;
	if (!(rxr->buffers =
	    (struct iavf_rx_buf *) malloc(bsize,
	    M_IAVF, M_NOWAIT | M_ZERO))) {
		device_printf(dev, "Unable to allocate rx_buffer memory\n");
		error = ENOMEM;
		goto free_rx_ptag;
	}

	for (i = 0; i < que->num_rx_desc; i++) {
		buf = &rxr->buffers[i];
		error = bus_dmamap_create(rxr->htag,
		    BUS_DMA_NOWAIT, &buf->hmap);
		if (error) {
			device_printf(dev, "Unable to create RX head map\n");
			goto free_buffers;
		}
		error = bus_dmamap_create(rxr->ptag,
		    BUS_DMA_NOWAIT, &buf->pmap);
		if (error) {
			bus_dmamap_destroy(rxr->htag, buf->hmap);
			device_printf(dev, "Unable to create RX pkt map\n");
			goto free_buffers;
		}
	}

	return 0;
free_buffers:
	while (i--) {
		buf = &rxr->buffers[i];
		bus_dmamap_destroy(rxr->ptag, buf->pmap);
		bus_dmamap_destroy(rxr->htag, buf->hmap);
	}
	free(rxr->buffers, M_IAVF);
	rxr->buffers = NULL;
free_rx_ptag:
	bus_dma_tag_destroy(rxr->ptag);
	rxr->ptag = NULL;
free_rx_htag:
	bus_dma_tag_destroy(rxr->htag);
	rxr->htag = NULL;
	return (error);
}


/**
 * iavf_init_rx_ring - Initialize an Rx ring
 * @que: queue pointer
 *
 * (Re)Initialize the queue receive ring and its buffers.
 *
 * @returns zero on success, or an error code on failure.
 */
int
iavf_init_rx_ring(struct iavf_queue *que)
{
	struct rx_ring		*rxr = &que->rxr;
	struct iavf_vsi		*vsi = que->vsi;
#if defined(INET6) || defined(INET)
	struct ifnet		*ifp = vsi->ifp;
	struct lro_ctrl		*lro = &rxr->lro;
#endif
	struct iavf_rx_buf	*buf;
	bus_dma_segment_t	pseg[1], hseg[1];
	int			rsize, nsegs, error = 0;

	IAVF_RX_LOCK(rxr);
	/* Clear the ring contents */
	rsize = roundup2(que->num_rx_desc *
	    sizeof(union iavf_rx_desc), DBA_ALIGN);
	bzero((void *)rxr->base, rsize);
	/* Cleanup any existing buffers */
	for (int i = 0; i < que->num_rx_desc; i++) {
		buf = &rxr->buffers[i];
		if (buf->m_head != NULL) {
			bus_dmamap_sync(rxr->htag, buf->hmap,
			    BUS_DMASYNC_POSTREAD);
			bus_dmamap_unload(rxr->htag, buf->hmap);
			buf->m_head->m_flags |= M_PKTHDR;
			m_freem(buf->m_head);
		}
		if (buf->m_pack != NULL) {
			bus_dmamap_sync(rxr->ptag, buf->pmap,
			    BUS_DMASYNC_POSTREAD);
			bus_dmamap_unload(rxr->ptag, buf->pmap);
			buf->m_pack->m_flags |= M_PKTHDR;
			m_freem(buf->m_pack);
		}
		buf->m_head = NULL;
		buf->m_pack = NULL;
	}

	/* header split is off */
	rxr->hdr_split = FALSE;

	/* Now replenish the mbufs */
	for (int j = 0; j != que->num_rx_desc; ++j) {
		struct mbuf	*mh, *mp;

		buf = &rxr->buffers[j];
		/*
		** Don't allocate mbufs if not
		** doing header split, its wasteful
		*/
		if (rxr->hdr_split == FALSE)
			goto skip_head;

		/* First the header */
		buf->m_head = m_gethdr(M_NOWAIT, MT_DATA);
		if (buf->m_head == NULL) {
			error = ENOBUFS;
			goto fail;
		}
		m_adj(buf->m_head, ETHER_ALIGN);
		mh = buf->m_head;
		mh->m_len = mh->m_pkthdr.len = MHLEN;
		mh->m_flags |= M_PKTHDR;
		/* Get the memory mapping */
		error = bus_dmamap_load_mbuf_sg(rxr->htag,
		    buf->hmap, buf->m_head, hseg,
		    &nsegs, BUS_DMA_NOWAIT);
		if (error != 0) /* Nothing elegant to do here */
			goto fail;
		bus_dmamap_sync(rxr->htag,
		    buf->hmap, BUS_DMASYNC_PREREAD);
		/* Update descriptor */
		rxr->base[j].read.hdr_addr = htole64(hseg[0].ds_addr);

skip_head:
		/* Now the payload cluster */
		buf->m_pack = m_getjcl(M_NOWAIT, MT_DATA,
		    M_PKTHDR, rxr->mbuf_sz);
		if (buf->m_pack == NULL) {
			error = ENOBUFS;
                        goto fail;
		}
		mp = buf->m_pack;
		mp->m_pkthdr.len = mp->m_len = rxr->mbuf_sz;
		/* Get the memory mapping */
		error = bus_dmamap_load_mbuf_sg(rxr->ptag,
		    buf->pmap, mp, pseg,
		    &nsegs, BUS_DMA_NOWAIT);
		if (error != 0)
                        goto fail;
		bus_dmamap_sync(rxr->ptag,
		    buf->pmap, BUS_DMASYNC_PREREAD);
		/* Update descriptor */
		rxr->base[j].read.pkt_addr = htole64(pseg[0].ds_addr);
		rxr->base[j].read.hdr_addr = 0;
	}


	/* Setup our descriptor indices */
	rxr->next_check = 0;
	rxr->next_refresh = 0;
	rxr->lro_enabled = FALSE;
	rxr->split = 0;
	rxr->bytes = 0;
	rxr->discard = FALSE;

	wr32(vsi->hw, rxr->tail, que->num_rx_desc - 1);
	iavf_flush(vsi->hw);

#if defined(INET6) || defined(INET)
	/*
	** Now set up the LRO interface:
	*/
	if (ifp->if_capenable & IFCAP_LRO) {
		int err = tcp_lro_init(lro);
		if (err) {
			if_printf(ifp, "queue %d: LRO Initialization failed!\n", que->me);
			goto fail;
		}
		INIT_DBG_IF(ifp, "queue %d: RX Soft LRO Initialized", que->me);
		rxr->lro_enabled = TRUE;
		lro->ifp = vsi->ifp;
	}
#endif

	bus_dmamap_sync(rxr->dma.tag, rxr->dma.map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

fail:
	IAVF_RX_UNLOCK(rxr);
	return (error);
}

/**
 * iavf_free_que_rx - Free Rx queue resources
 * @que: the queue to free
 *
 * Free station receive ring data structures.
 */
void
iavf_free_que_rx(struct iavf_queue *que)
{
	struct rx_ring		*rxr = &que->rxr;
	struct iavf_rx_buf	*buf;

	/* Cleanup any existing buffers */
	if (rxr->buffers != NULL) {
		for (int i = 0; i < que->num_rx_desc; i++) {
			buf = &rxr->buffers[i];

			/* Free buffers and unload dma maps */
			iavf_rx_discard(rxr, i);

			bus_dmamap_destroy(rxr->htag, buf->hmap);
			bus_dmamap_destroy(rxr->ptag, buf->pmap);
		}
		free(rxr->buffers, M_IAVF);
		rxr->buffers = NULL;
	}

	if (rxr->htag != NULL) {
		bus_dma_tag_destroy(rxr->htag);
		rxr->htag = NULL;
	}
	if (rxr->ptag != NULL) {
		bus_dma_tag_destroy(rxr->ptag);
		rxr->ptag = NULL;
	}
}

/**
 * iavf_rx_input - Send Rx data up to the stack
 * @rxr: the Rx ring
 * @ifp: ifnet structure
 * @m: mbuf to send up
 * @ptype: packet type
 *
 * Determine if large receive offload is enabled, and call tcp_lro_rx if
 * necessary. Otherwise send a received packet mbuf up to the stack via
 * calling if_input.
 */
static inline void
iavf_rx_input(struct rx_ring *rxr, struct ifnet *ifp, struct mbuf *m,
	      u8 ptype __unused)
{

#if defined(INET6) || defined(INET)
        /*
         * ATM LRO is only for IPv4/TCP packets and TCP checksum of the packet
         * should be computed by hardware. Also it should not have VLAN tag in
         * ethernet header.
         */
        if (rxr->lro_enabled &&
            (ifp->if_capenable & IFCAP_VLAN_HWTAGGING) != 0 &&
            (m->m_pkthdr.csum_flags & (CSUM_DATA_VALID | CSUM_PSEUDO_HDR)) ==
            (CSUM_DATA_VALID | CSUM_PSEUDO_HDR)) {
                /*
                 * Send to the stack if:
                 **  - LRO not enabled, or
                 **  - no LRO resources, or
                 **  - lro enqueue fails
                 */
                if (rxr->lro.lro_cnt != 0)
                        if (tcp_lro_rx(&rxr->lro, m, 0) == 0)
                                return;
        }
#endif
	IAVF_RX_UNLOCK(rxr);
        (*ifp->if_input)(ifp, m);
	IAVF_RX_LOCK(rxr);
}

/**
 * iavf_rx_discard - Discard a received packet
 * @rxr: Rx ring pointer
 * @i: the index of the packet to discard
 *
 * Release mbufs associated with the current descriptor, essentially dropping
 * it instead of pushing the packet data up the stack.
 */
static inline void
iavf_rx_discard(struct rx_ring *rxr, int i)
{
	struct iavf_rx_buf	*rbuf;

	KASSERT(rxr != NULL, ("Receive ring pointer cannot be null"));
	KASSERT(i < rxr->que->num_rx_desc,
	    ("Descriptor index must be less than que->num_desc"));

	rbuf = &rxr->buffers[i];

	/* Free the mbufs in the current chain for the packet */
        if (rbuf->fmp != NULL) {
		bus_dmamap_sync(rxr->ptag, rbuf->pmap, BUS_DMASYNC_POSTREAD);
                m_freem(rbuf->fmp);
                rbuf->fmp = NULL;
	}

	/*
	 * Free the mbufs for the current descriptor; and let iavf_refresh_mbufs()
	 * assign new mbufs to these.
	 */
	if (rbuf->m_head) {
		bus_dmamap_sync(rxr->htag, rbuf->hmap, BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(rxr->htag, rbuf->hmap);
		m_free(rbuf->m_head);
		rbuf->m_head = NULL;
	}

	if (rbuf->m_pack) {
		bus_dmamap_sync(rxr->ptag, rbuf->pmap, BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(rxr->ptag, rbuf->pmap);
		m_free(rbuf->m_pack);
		rbuf->m_pack = NULL;
	}
}

/**
 * iavf_rxeof - Replenish Rx mbufs
 * @que: queue pointer
 * @count: maximum number to refresh
 *
 * This routine executes in interrupt context. It replenishes
 * the mbufs in the descriptor and sends data which has been
 * dma'ed into host memory to upper layer.
 *
 * We loop at most count times if count is > 0, or until done if
 * count < 0.
 *
 * @returns TRUE for more work, FALSE for all clean.
 */
bool
iavf_rxeof(struct iavf_queue *que, int count)
{
	struct iavf_vsi		*vsi = que->vsi;
	struct rx_ring		*rxr = &que->rxr;
	struct ifnet		*ifp = vsi->ifp;
#if defined(INET6) || defined(INET)
	struct lro_ctrl		*lro = &rxr->lro;
#endif
	int			i, nextp, processed = 0;
	union iavf_rx_desc	*cur;
	struct iavf_rx_buf	*rbuf, *nbuf;

	IAVF_RX_LOCK(rxr);

	for (i = rxr->next_check; count != 0;) {
		struct mbuf	*sendmp, *mh, *mp;
		u32		status, error;
		u16		hlen, plen, vtag;
		u64		qword;
		u8		ptype;
		bool		eop;

		/* Sync the ring. */
		bus_dmamap_sync(rxr->dma.tag, rxr->dma.map,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		cur = &rxr->base[i];
		qword = le64toh(cur->wb.qword1.status_error_len);
		status = (qword & IAVF_RXD_QW1_STATUS_MASK)
		    >> IAVF_RXD_QW1_STATUS_SHIFT;
		error = (qword & IAVF_RXD_QW1_ERROR_MASK)
		    >> IAVF_RXD_QW1_ERROR_SHIFT;
		plen = (qword & IAVF_RXD_QW1_LENGTH_PBUF_MASK)
		    >> IAVF_RXD_QW1_LENGTH_PBUF_SHIFT;
		hlen = (qword & IAVF_RXD_QW1_LENGTH_HBUF_MASK)
		    >> IAVF_RXD_QW1_LENGTH_HBUF_SHIFT;
		ptype = (qword & IAVF_RXD_QW1_PTYPE_MASK)
		    >> IAVF_RXD_QW1_PTYPE_SHIFT;

		if ((status & (1 << IAVF_RX_DESC_STATUS_DD_SHIFT)) == 0) {
			++rxr->not_done;
			break;
		}
		if ((ifp->if_drv_flags & IFF_DRV_RUNNING) == 0)
			break;

		count--;
		sendmp = NULL;
		nbuf = NULL;
		cur->wb.qword1.status_error_len = 0;
		rbuf = &rxr->buffers[i];
		mh = rbuf->m_head;
		mp = rbuf->m_pack;
		eop = (status & (1 << IAVF_RX_DESC_STATUS_EOF_SHIFT));
		if (status & (1 << IAVF_RX_DESC_STATUS_L2TAG1P_SHIFT))
			vtag = le16toh(cur->wb.qword0.lo_dword.l2tag1);
		else
			vtag = 0;

		/* Remove device access to the rx buffers. */
		if (rbuf->m_head != NULL) {
			bus_dmamap_sync(rxr->htag, rbuf->hmap,
			    BUS_DMASYNC_POSTREAD);
			bus_dmamap_unload(rxr->htag, rbuf->hmap);
		}
		if (rbuf->m_pack != NULL) {
			bus_dmamap_sync(rxr->ptag, rbuf->pmap,
			    BUS_DMASYNC_POSTREAD);
			bus_dmamap_unload(rxr->ptag, rbuf->pmap);
		}

		/*
		** Make sure bad packets are discarded,
		** note that only EOP descriptor has valid
		** error results.
		*/
                if (eop && (error & (1 << IAVF_RX_DESC_ERROR_RXE_SHIFT))) {
			rxr->desc_errs++;
			iavf_rx_discard(rxr, i);
			goto next_desc;
		}

		/* Prefetch the next buffer */
		if (!eop) {
			nextp = i + 1;
			if (nextp == que->num_rx_desc)
				nextp = 0;
			nbuf = &rxr->buffers[nextp];
			prefetch(nbuf);
		}

		/*
		** The header mbuf is ONLY used when header 
		** split is enabled, otherwise we get normal 
		** behavior, ie, both header and payload
		** are DMA'd into the payload buffer.
		**
		** Rather than using the fmp/lmp global pointers
		** we now keep the head of a packet chain in the
		** buffer struct and pass this along from one
		** descriptor to the next, until we get EOP.
		*/
		if (rxr->hdr_split && (rbuf->fmp == NULL)) {
			if (hlen > IAVF_RX_HDR)
				hlen = IAVF_RX_HDR;
			mh->m_len = hlen;
			mh->m_flags |= M_PKTHDR;
			mh->m_next = NULL;
			mh->m_pkthdr.len = mh->m_len;
			/* Null buf pointer so it is refreshed */
			rbuf->m_head = NULL;
			/*
			** Check the payload length, this
			** could be zero if its a small
			** packet.
			*/
			if (plen > 0) {
				mp->m_len = plen;
				mp->m_next = NULL;
				mp->m_flags &= ~M_PKTHDR;
				mh->m_next = mp;
				mh->m_pkthdr.len += mp->m_len;
				/* Null buf pointer so it is refreshed */
				rbuf->m_pack = NULL;
				rxr->split++;
			}
			/*
			** Now create the forward
			** chain so when complete 
			** we wont have to.
			*/
                        if (eop == 0) {
				/* stash the chain head */
                                nbuf->fmp = mh;
				/* Make forward chain */
                                if (plen)
                                        mp->m_next = nbuf->m_pack;
                                else
                                        mh->m_next = nbuf->m_pack;
                        } else {
				/* Singlet, prepare to send */
                                sendmp = mh;
                                if (vtag) {
                                        sendmp->m_pkthdr.ether_vtag = vtag;
                                        sendmp->m_flags |= M_VLANTAG;
                                }
                        }
		} else {
			/*
			** Either no header split, or a
			** secondary piece of a fragmented
			** split packet.
			*/
			mp->m_len = plen;
			/*
			** See if there is a stored head
			** that determines what we are
			*/
			sendmp = rbuf->fmp;
			rbuf->m_pack = rbuf->fmp = NULL;

			if (sendmp != NULL) /* secondary frag */
				sendmp->m_pkthdr.len += mp->m_len;
			else {
				/* first desc of a non-ps chain */
				sendmp = mp;
				sendmp->m_flags |= M_PKTHDR;
				sendmp->m_pkthdr.len = mp->m_len;
                        }
			/* Pass the head pointer on */
			if (eop == 0) {
				nbuf->fmp = sendmp;
				sendmp = NULL;
				mp->m_next = nbuf->m_pack;
			}
		}
		++processed;
		/* Sending this frame? */
		if (eop) {
			sendmp->m_pkthdr.rcvif = ifp;
			/* gather stats */
			rxr->rx_packets++;
			rxr->rx_bytes += sendmp->m_pkthdr.len;
			/* capture data for dynamic ITR adjustment */
			rxr->packets++;
			rxr->bytes += sendmp->m_pkthdr.len;
			/* Set VLAN tag (field only valid in eop desc) */
			if (vtag) {
				sendmp->m_pkthdr.ether_vtag = vtag;
				sendmp->m_flags |= M_VLANTAG;
			}
			if ((ifp->if_capenable & IFCAP_RXCSUM) != 0)
				iavf_rx_checksum(sendmp, status, error, ptype);
#ifdef RSS
			sendmp->m_pkthdr.flowid =
			    le32toh(cur->wb.qword0.hi_dword.rss);
			M_HASHTYPE_SET(sendmp, iavf_ptype_to_hash(ptype));
#else
			sendmp->m_pkthdr.flowid = que->msix;
			M_HASHTYPE_SET(sendmp, M_HASHTYPE_OPAQUE);
#endif
		}
next_desc:
		bus_dmamap_sync(rxr->dma.tag, rxr->dma.map,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		/* Advance our pointers to the next descriptor. */
		if (++i == que->num_rx_desc)
			i = 0;

		/* Now send to the stack or do LRO */
		if (sendmp != NULL) {
			rxr->next_check = i;
			iavf_rx_input(rxr, ifp, sendmp, ptype);
			/*
			 * Update index used in loop in case another
			 * iavf_rxeof() call executes when lock is released
			 */
			i = rxr->next_check;
		}

		/* Every 8 descriptors we go to refresh mbufs */
		if (processed == 8) {
			iavf_refresh_mbufs(que, i);
			processed = 0;
		}
	}

	/* Refresh any remaining buf structs */
	if (iavf_rx_unrefreshed(que))
		iavf_refresh_mbufs(que, i);

	rxr->next_check = i;

#if defined(INET6) || defined(INET)
#if __FreeBSD_version >= 1100105
	tcp_lro_flush_all(lro);
#else
	/*
	 * Flush any outstanding LRO work
	 */
	struct lro_entry *queued;
	while ((queued = SLIST_FIRST(&lro->lro_active)) != NULL) {
		SLIST_REMOVE_HEAD(&lro->lro_active, next);
		tcp_lro_flush(lro, queued);
	}
#endif
#endif /* defined(INET6) || defined(INET) */

	IAVF_RX_UNLOCK(rxr);
	return (FALSE);
}


/**
 * iavf_rx_checksum - Report hardware checksum status
 * @mp: mbuf chain
 * @status: the Rx descriptor status value
 * @error: the Rx descriptor error value
 * @ptype: the packet type
 *
 * Verify that the hardware indicated that the checksum is valid.
 * Inform the stack about the status of checksum so that stack
 * doesn't spend time verifying the checksum.
 */
static void
iavf_rx_checksum(struct mbuf * mp, u32 status, u32 error, u8 ptype)
{
	struct iavf_rx_ptype_decoded decoded;

	decoded = decode_rx_desc_ptype(ptype);

	/* Errors? */
	if (error & ((1 << IAVF_RX_DESC_ERROR_IPE_SHIFT) |
	    (1 << IAVF_RX_DESC_ERROR_L4E_SHIFT))) {
		mp->m_pkthdr.csum_flags = 0;
		return;
	}

	/* IPv6 with extension headers likely have bad csum */
	if (decoded.outer_ip == IAVF_RX_PTYPE_OUTER_IP &&
	    decoded.outer_ip_ver == IAVF_RX_PTYPE_OUTER_IPV6)
		if (status &
		    (1 << IAVF_RX_DESC_STATUS_IPV6EXADD_SHIFT)) {
			mp->m_pkthdr.csum_flags = 0;
			return;
		}


	/* IP Checksum Good */
	mp->m_pkthdr.csum_flags = CSUM_IP_CHECKED;
	mp->m_pkthdr.csum_flags |= CSUM_IP_VALID;

	if (status & (1 << IAVF_RX_DESC_STATUS_L3L4P_SHIFT)) {
		mp->m_pkthdr.csum_flags |=
		    (CSUM_DATA_VALID | CSUM_PSEUDO_HDR);
		mp->m_pkthdr.csum_data |= htons(0xffff);
	}
	return;
}

/**
 * iavf_queue_sq_irq - Program queue interrupt vector
 * @vsi: VSI structure
 * @qidx: the queue index to program
 *
 * Writes the dynamic interrupt control register for a given queue.
 */
static void
iavf_queue_sw_irq(struct iavf_vsi *vsi, int qidx)
{
	struct iavf_hw *hw = vsi->hw;
	u32 mask;

	mask = (IAVF_VFINT_DYN_CTLN1_INTENA_MASK |
		IAVF_VFINT_DYN_CTLN1_SWINT_TRIG_MASK |
		IAVF_VFINT_DYN_CTLN1_ITR_INDX_MASK);
	wr32(hw, IAVF_VFINT_DYN_CTLN1(qidx), mask);
}

/**
 * iavf_queue_hand_check - Check if a transmit queue has hung
 * @vsi: VSI structure
 *
 * @returns 0 if no queues are hung, or the number of hung queues detected.
 */
int
iavf_queue_hang_check(struct iavf_vsi *vsi)
{
	struct iavf_queue *que = vsi->queues;
	device_t dev = vsi->dev;
	struct tx_ring *txr;
	s32 timer, new_timer;
	int hung = 0;

	for (int i = 0; i < vsi->num_queues; i++, que++) {
		txr = &que->txr;
		/*
		 * If watchdog_timer is equal to defualt value set by iavf_txeof
		 * just substract hz and move on - the queue is most probably
		 * running. Otherwise check the value.
		 */
                if (atomic_cmpset_rel_32(&txr->watchdog_timer,
					IAVF_WATCHDOG, (IAVF_WATCHDOG) - hz) == 0) {
			timer = atomic_load_acq_32(&txr->watchdog_timer);
			/*
                         * Again - if the timer was reset to default value
			 * then queue is running. Otherwise check if watchdog
			 * expired and act accrdingly.
                         */

			if (timer > 0 && timer != IAVF_WATCHDOG) {
				new_timer = timer - hz;
				if (new_timer <= 0) {
					atomic_store_rel_32(&txr->watchdog_timer, -1);
					device_printf(dev, "WARNING: queue %d "
							"appears to be hung!\n", que->me);
					++hung;
					/* Try to unblock the queue with SW IRQ */
					iavf_queue_sw_irq(vsi, i);
				} else {
					/*
					 * If this fails, that means something in the TX path
					 * has updated the watchdog, so it means the TX path
					 * is still working and the watchdog doesn't need
					 * to countdown.
					 */
					atomic_cmpset_rel_32(&txr->watchdog_timer,
							timer, new_timer);
				}
			}
		}
	}

	return (hung);
}
