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
 * @file if_iavf_legacy.c
 * @brief main entry points for the legacy driver implementation
 *
 * Contains the main entry points used by the stack to communicate with the
 * driver. Implements the legacy driver interface.
 */
#include "iavf_legacy.h"
#include "iavf_vc_common.h"

#include "iavf_drv_info.h"
#include "iavf_sysctls_legacy.h"

/*********************************************************************
 *  Driver version
 *********************************************************************/
#define IAVF_DRIVER_VERSION_MAJOR	1
#define IAVF_DRIVER_VERSION_MINOR	6
#define IAVF_DRIVER_VERSION_BUILD	0

#define IAVF_DRIVER_VERSION_STRING			\
    __XSTRING(IAVF_DRIVER_VERSION_MAJOR) "."		\
    __XSTRING(IAVF_DRIVER_VERSION_MINOR) "."		\
    __XSTRING(IAVF_DRIVER_VERSION_BUILD)

/*********************************************************************
 *  Function prototypes
 *********************************************************************/
static int      iavf_probe(device_t);
static int      iavf_attach(device_t);
static int      iavf_detach(device_t);
static int      iavf_shutdown(device_t);
static void	iavf_init_locked(struct iavf_sc *);
static int	iavf_allocate_pci_resources(struct iavf_sc *);
static void	iavf_free_pci_resources(struct iavf_sc *);
static int	iavf_assign_msix(struct iavf_sc *);
static int	iavf_init_msix(struct iavf_sc *);
static int	iavf_setup_admin_tq(struct iavf_sc *);
static int	iavf_setup_queues(struct iavf_sc *);
static void	iavf_stop(struct iavf_sc *);
static void	iavf_free_queue(struct iavf_sc *sc, struct iavf_queue *que);
static void	iavf_free_queues(struct iavf_vsi *);
static int	iavf_setup_interface(device_t, struct iavf_sc *);
static void	iavf_teardown_adminq_msix(struct iavf_sc *);

static int	iavf_media_change(struct ifnet *);
static void	iavf_media_status(struct ifnet *, struct ifmediareq *);

static void	iavf_local_timer(void *);

static int	iavf_del_mac_filter(struct iavf_sc *sc, u8 *macaddr);

static void	iavf_msix_que(void *);
static void	iavf_msix_adminq(void *);
static void	iavf_do_adminq(void *, int);
static void	iavf_do_adminq_locked(struct iavf_sc *sc);
static void	iavf_handle_que(void *, int);
static void	iavf_set_queue_rx_itr(struct iavf_queue *);
static void	iavf_set_queue_tx_itr(struct iavf_queue *);
static void	iavf_configure_itr(struct iavf_sc *);

static void	iavf_enable_queue_irq(struct iavf_hw *, int);
static void	iavf_disable_queue_irq(struct iavf_hw *, int);

static void	iavf_register_vlan(void *, struct ifnet *, u16);
static void	iavf_unregister_vlan(void *, struct ifnet *, u16);

static void	iavf_cap_txcsum_tso(struct iavf_vsi *,
		    struct ifnet *, int);

static void	iavf_add_device_sysctls(struct iavf_sc *);
static void	iavf_vsi_setup_rings_size(struct iavf_vsi *vsi,
		    int tx_ring_size, int rx_ring_size);
static void	iavf_vsi_add_queues_stats(struct iavf_vsi *);

/*********************************************************************
 *  FreeBSD Device Interface Entry Points
 *********************************************************************/

static device_method_t iavf_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe, iavf_probe),
	DEVMETHOD(device_attach, iavf_attach),
	DEVMETHOD(device_detach, iavf_detach),
	DEVMETHOD(device_shutdown, iavf_shutdown),
	{0, 0}
};

static driver_t iavf_driver = {
	"iavf", iavf_methods, sizeof(struct iavf_sc),
};

devclass_t iavf_devclass;
DRIVER_MODULE(iavf, pci, iavf_driver, iavf_devclass, 0, 0);
LEGACY_PNP_INFO(pci, iavf, iavf_vendor_info_array);
MODULE_VERSION(iavf, 1);

MODULE_DEPEND(iavf, pci, 1, 1, 1);
MODULE_DEPEND(iavf, ether, 1, 1, 1);

/**
 * @var M_IAVF
 * @brief main iavf driver allocation type
 *
 * malloc(9) allocation type used by the majority of memory allocations in the
 * iavf legacy driver.
 */
MALLOC_DEFINE(M_IAVF, "iavf", "iavf driver allocations");

/**
 * iavf_probe - Device identification routine
 * @dev: stack device structure
 *
 * iavf_probe determines if the driver should be loaded on
 * the hardware based on PCI vendor/device id of the device.
 *
 * @returns BUS_PROBE_VENDOR on success, or ENXIO on failure.
 */
static int
iavf_probe(device_t dev)
{
	pci_vendor_info_t *ent;
	u16	pci_vendor_id, pci_device_id;
	u16	pci_subvendor_id, pci_subdevice_id;

	pci_vendor_id = pci_get_vendor(dev);
	if (pci_vendor_id != IAVF_INTEL_VENDOR_ID)
		return (ENXIO);

	pci_device_id = pci_get_device(dev);
	pci_subvendor_id = pci_get_subvendor(dev);
	pci_subdevice_id = pci_get_subdevice(dev);

	ent = iavf_vendor_info_array;
	while (ent->pvi_vendor_id != 0) {
		if ((pci_vendor_id == ent->pvi_vendor_id) &&
		    (pci_device_id == ent->pvi_device_id) &&

		    ((pci_subvendor_id == ent->pvi_subvendor_id) ||
		     (ent->pvi_subvendor_id == 0)) &&

		    ((pci_subdevice_id == ent->pvi_subdevice_id) ||
		     (ent->pvi_subdevice_id == 0))) {
			device_set_desc_copy(dev, ent->pvi_name);
			/*
			 * The in-kernel iavf drivers SHOULD return
			 * BUS_PROBE_DEFAULT instead of BUS_PROBE_VENDOR.
			 */
			return (BUS_PROBE_VENDOR);
		}
		ent++;
	}
	return (ENXIO);
}

/**
 * iavf_attach - Device initialization routine
 *
 * The attach entry point is called when the driver is being loaded.
 * This routine identifies the type of hardware, allocates all resources
 * and initializes the hardware.
 *
 * @return 0 on success, positive on failure
 */
static int
iavf_attach(device_t dev)
{
	struct iavf_sc	*sc;
	struct iavf_hw	*hw;
	struct iavf_vsi	*vsi;
	int error = 0;

	/* Setup pointers */
	sc = iavf_sc_from_dev(dev);

	vsi = &sc->vsi;
	vsi->back = sc;
	sc->dev = sc->osdep.dev = dev;
	hw = &sc->hw;

	vsi->dev = dev;
	vsi->hw = &sc->hw;
	vsi->num_vlans = 0;

	iavf_save_tunables(sc);

	/* Core Lock Init */
	mtx_init(&sc->mtx, device_get_nameunit(dev),
	    "IAVF SC Lock", MTX_DEF);

	/* Set up the timer callout */
	callout_init_mtx(&sc->timer, &sc->mtx, 0);

	/* Do PCI setup - map BAR0, etc */
	error = iavf_allocate_pci_resources(sc);
	if (error) {
		device_printf(dev, "%s: Allocation of PCI resources failed\n",
		    __func__);
		goto err_early;
	}

	iavf_dbg_init(sc, "Allocated PCI resources and MSIX vectors\n");

	error = iavf_set_mac_type(hw);
	if (error) {
		device_printf(dev, "%s: set_mac_type failed: %d\n",
		    __func__, error);
		goto err_pci_res;
	}

	error = iavf_reset_complete(hw);
	if (error) {
		device_printf(dev, "%s: Device is still being reset\n",
		    __func__);
		goto err_pci_res;
	}

	iavf_dbg_init(sc, "VF Device is ready for configuration\n");

	error = iavf_setup_vc(sc);
	if (error) {
		device_printf(dev, "%s: Error setting up PF comms, %d\n",
		    __func__, error);
		goto err_pci_res;
	}

	iavf_dbg_init(sc, "PF API version verified\n");

	/* Need API version before sending reset message */
	error = iavf_reset(sc);
	if (error) {
		device_printf(dev, "VF reset failed; reload the driver\n");
		goto err_aq;
	}

	iavf_dbg_init(sc, "VF reset complete\n");

	/* Ask for VF config from PF */
	error = iavf_vf_config(sc);
	if (error) {
		device_printf(dev, "Error getting configuration from PF: %d\n",
		    error);
		goto err_aq;
	}

	iavf_print_device_info(sc);

	error = iavf_get_vsi_res_from_vf_res(sc);
	if (error)
		goto err_res_buf;

	iavf_dbg_init(sc, "Resource Acquisition complete\n");

	iavf_set_mac_addresses(sc);

	/* Allocate filter lists */
	iavf_init_filters(sc);

	/* Now that the number of queues for this VF is known, set up interrupts */
	sc->msix = iavf_init_msix(sc);
	/* We fail without MSIX support */
	if (sc->msix == 0) {
		error = ENXIO;
		goto err_res_buf;
	}

	iavf_vsi_setup_rings_size(vsi, iavf_tx_ring_size, iavf_rx_ring_size);

	/* This allocates the memory and early settings */
	if (iavf_setup_queues(sc) != 0) {
		device_printf(dev, "%s: setup queues failed!\n",
		    __func__);
		error = EIO;
		goto out;
	}

	/* Do queue interrupt setup */
	if (iavf_assign_msix(sc) != 0) {
		device_printf(dev, "%s: allocating queue interrupts failed!\n",
		    __func__);
		error = ENXIO;
		goto out;
	}

	iavf_dbg_init(sc, "Queue memory and interrupts setup\n");

	/* Setup the stack interface */
	if (iavf_setup_interface(dev, sc) != 0) {
		device_printf(dev, "%s: setup interface failed!\n",
		    __func__);
		error = ENOMEM;
		goto out;
	}

	iavf_dbg_init(sc, "Interface setup complete\n");

	/* Start AdminQ taskqueue */
	iavf_setup_admin_tq(sc);

	/* We expect a link state message, so schedule the AdminQ task now */
	taskqueue_enqueue(sc->tq, &sc->aq_irq);

	/* Initialize stats */
	bzero(&sc->vsi.eth_stats, sizeof(struct iavf_eth_stats));
	iavf_add_device_sysctls(sc);

	/* Register for VLAN events */
	vsi->vlan_attach = EVENTHANDLER_REGISTER(vlan_config,
	    (void *)iavf_register_vlan, sc, EVENTHANDLER_PRI_FIRST);
	vsi->vlan_detach = EVENTHANDLER_REGISTER(vlan_unconfig,
	    (void *)iavf_unregister_vlan, sc, EVENTHANDLER_PRI_FIRST);

	/* Set things up to run init */
	atomic_store_rel_32(&sc->queues_enabled, 0);
	iavf_set_state(&sc->state, IAVF_STATE_INITIALIZED);

	/* We want AQ enabled early */
	iavf_enable_adminq_irq(hw);

	return (error);

out:
	iavf_free_queues(vsi);
	iavf_teardown_adminq_msix(sc);
err_res_buf:
	iavf_free_filters(sc);
	free(sc->vf_res, M_IAVF);
err_aq:
	iavf_shutdown_adminq(hw);
err_pci_res:
	iavf_free_pci_resources(sc);
err_early:
	mtx_destroy(&sc->mtx);
	return (error);
}

/**
 * iavf_detach - Device removal routine
 * @dev: stack device pointer
 *
 * The detach entry point is called when the driver is being removed.
 * This routine stops the adapter and deallocates all the resources
 * that were allocated for driver operation.
 *
 * @returns 0 on success, positive on failure
 */
static int
iavf_detach(device_t dev)
{
	struct iavf_sc *sc = iavf_sc_from_dev(dev);
	struct iavf_vsi	*vsi = &sc->vsi;
	struct iavf_hw *hw = &sc->hw;
	enum iavf_status status;

	iavf_dbg_init(sc, "begin\n");

	/* Make sure VLANS are not using driver */
	if (vsi->ifp->if_vlantrunk != NULL) {
		if_printf(vsi->ifp, "Vlan in use, detach first\n");
		return (EBUSY);
	}

	/* Remove all the media and link information */
	ifmedia_removeall(&sc->media);

	/* Stop driver */
	ether_ifdetach(vsi->ifp);
	if (vsi->ifp->if_drv_flags & IFF_DRV_RUNNING) {
		mtx_lock(&sc->mtx);
		iavf_stop(sc);
		mtx_unlock(&sc->mtx);
	}

	/* Unregister VLAN events */
	if (vsi->vlan_attach != NULL)
		EVENTHANDLER_DEREGISTER(vlan_config, vsi->vlan_attach);
	if (vsi->vlan_detach != NULL)
		EVENTHANDLER_DEREGISTER(vlan_unconfig, vsi->vlan_detach);

	iavf_disable_adminq_irq(hw);
	iavf_teardown_adminq_msix(sc);
	/* Drain admin queue taskqueue */
	taskqueue_free(sc->tq);
	status = iavf_shutdown_adminq(&sc->hw);
	if (status != IAVF_SUCCESS) {
		device_printf(dev,
		    "iavf_shutdown_adminq() failed with status %s\n",
		    iavf_stat_str(hw, status));
	}

	if_free(vsi->ifp);
	free(sc->vf_res, M_IAVF);
	iavf_free_queues(vsi);
	iavf_free_pci_resources(sc);
	iavf_free_filters(sc);

	mtx_destroy(&sc->mtx);
	iavf_dbg_init(sc, "end\n");
	return (0);
}

/**
 * iavf_shutdown - Shutdown entry point
 * @dev: stack device pointer
 *
 * Callback to shut down the device.
 *
 * @returns zero.
 */
static int
iavf_shutdown(device_t dev)
{
	struct iavf_sc *sc = iavf_sc_from_dev(dev);

	iavf_dbg_init(sc, "begin\n");

	mtx_lock(&sc->mtx);
	iavf_stop(sc);
	mtx_unlock(&sc->mtx);

	iavf_dbg_init(sc, "end\n");
	return (0);
}

/**
 * iavf_cap_txcsum_tso - Configure TXCSUM(IPV6) and TSO(4/6)
 * @vsi: main VSI pointer
 * @ifp: ifnet structure
 * @mask: checksum mask
 *
 * The hardware handles these together so we need to tweak them to make sure
 * they're always enabled together.
 */
static void
iavf_cap_txcsum_tso(struct iavf_vsi *vsi, struct ifnet *ifp, int mask)
{
	/* Enable/disable TXCSUM/TSO4 */
	if (!(ifp->if_capenable & IFCAP_TXCSUM)
	    && !(ifp->if_capenable & IFCAP_TSO4)) {
		if (mask & IFCAP_TXCSUM) {
			ifp->if_capenable |= IFCAP_TXCSUM;
			/* enable TXCSUM, restore TSO if previously enabled */
			if (vsi->flags & IAVF_FLAGS_KEEP_TSO4) {
				vsi->flags &= ~IAVF_FLAGS_KEEP_TSO4;
				ifp->if_capenable |= IFCAP_TSO4;
			}
		}
		else if (mask & IFCAP_TSO4) {
			ifp->if_capenable |= (IFCAP_TXCSUM | IFCAP_TSO4);
			vsi->flags &= ~IAVF_FLAGS_KEEP_TSO4;
			if_printf(ifp,
			    "TSO4 requires txcsum, enabling both...\n");
		}
	} else if((ifp->if_capenable & IFCAP_TXCSUM)
	    && !(ifp->if_capenable & IFCAP_TSO4)) {
		if (mask & IFCAP_TXCSUM)
			ifp->if_capenable &= ~IFCAP_TXCSUM;
		else if (mask & IFCAP_TSO4)
			ifp->if_capenable |= IFCAP_TSO4;
	} else if((ifp->if_capenable & IFCAP_TXCSUM)
	    && (ifp->if_capenable & IFCAP_TSO4)) {
		if (mask & IFCAP_TXCSUM) {
			vsi->flags |= IAVF_FLAGS_KEEP_TSO4;
			ifp->if_capenable &= ~(IFCAP_TXCSUM | IFCAP_TSO4);
			if_printf(ifp,
			    "TSO4 requires txcsum, disabling both...\n");
		} else if (mask & IFCAP_TSO4)
			ifp->if_capenable &= ~IFCAP_TSO4;
	}

	/* Enable/disable TXCSUM_IPV6/TSO6 */
	if (!(ifp->if_capenable & IFCAP_TXCSUM_IPV6)
	    && !(ifp->if_capenable & IFCAP_TSO6)) {
		if (mask & IFCAP_TXCSUM_IPV6) {
			ifp->if_capenable |= IFCAP_TXCSUM_IPV6;
			if (vsi->flags & IAVF_FLAGS_KEEP_TSO6) {
				vsi->flags &= ~IAVF_FLAGS_KEEP_TSO6;
				ifp->if_capenable |= IFCAP_TSO6;
			}
		} else if (mask & IFCAP_TSO6) {
			ifp->if_capenable |= (IFCAP_TXCSUM_IPV6 | IFCAP_TSO6);
			vsi->flags &= ~IAVF_FLAGS_KEEP_TSO6;
			if_printf(ifp,
			    "TSO6 requires txcsum6, enabling both...\n");
		}
	} else if((ifp->if_capenable & IFCAP_TXCSUM_IPV6)
	    && !(ifp->if_capenable & IFCAP_TSO6)) {
		if (mask & IFCAP_TXCSUM_IPV6)
			ifp->if_capenable &= ~IFCAP_TXCSUM_IPV6;
		else if (mask & IFCAP_TSO6)
			ifp->if_capenable |= IFCAP_TSO6;
	} else if ((ifp->if_capenable & IFCAP_TXCSUM_IPV6)
	    && (ifp->if_capenable & IFCAP_TSO6)) {
		if (mask & IFCAP_TXCSUM_IPV6) {
			vsi->flags |= IAVF_FLAGS_KEEP_TSO6;
			ifp->if_capenable &= ~(IFCAP_TXCSUM_IPV6 | IFCAP_TSO6);
			if_printf(ifp,
			    "TSO6 requires txcsum6, disabling both...\n");
		} else if (mask & IFCAP_TSO6)
			ifp->if_capenable &= ~IFCAP_TSO6;
	}
}

/**
 * iavf_ioctl - Ioctl entry point
 * @ifp: ifnet structure
 * @command: the ioctl command requested
 * @data: data for the ioctl
 *
 * iavf_ioctl is called when the user wants to configure the
 * interface.
 *
 * @return 0 on success, positive on failure
 */
static int
iavf_ioctl(struct ifnet *ifp, u_long command, caddr_t data)
{
	struct iavf_vsi		*vsi = (struct iavf_vsi *)ifp->if_softc;
	struct iavf_sc	*sc = vsi->back;
	struct ifreq		*ifr = (struct ifreq *)data;
#if defined(INET) || defined(INET6)
	struct ifaddr		*ifa = (struct ifaddr *)data;
	bool			avoid_reset = FALSE;
#endif
	int			error = 0;


	switch (command) {

        case SIOCSIFADDR:
#ifdef INET
		if (ifa->ifa_addr->sa_family == AF_INET)
			avoid_reset = TRUE;
#endif
#ifdef INET6
		if (ifa->ifa_addr->sa_family == AF_INET6)
			avoid_reset = TRUE;
#endif
#if defined(INET) || defined(INET6)
		/*
		** Calling init results in link renegotiation,
		** so we avoid doing it when possible.
		*/
		if (avoid_reset) {
			ifp->if_flags |= IFF_UP;
			if (!(ifp->if_drv_flags & IFF_DRV_RUNNING))
				iavf_init(vsi);
#ifdef INET
			if (!(ifp->if_flags & IFF_NOARP))
				arp_ifinit(ifp, ifa);
#endif
		} else
			error = ether_ioctl(ifp, command, data);
		break;
#endif
	case SIOCSIFMTU:
		IOCTL_DBG_IF2(ifp, "SIOCSIFMTU (Set Interface MTU)");
		mtx_lock(&sc->mtx);
		if (ifr->ifr_mtu < IAVF_MIN_MTU || ifr->ifr_mtu > IAVF_MAX_MTU) {
			error = EINVAL;
			device_printf(vsi->dev, "mtu %d is not in valid range [%d-%d]\n",
			    ifr->ifr_mtu, IAVF_MIN_MTU, IAVF_MAX_MTU);
		} else {
			IOCTL_DBG_IF2(ifp, "mtu: %lu -> %d", (u_long)ifp->if_mtu, ifr->ifr_mtu);
			// ERJ: Interestingly enough, these types don't match
			ifp->if_mtu = (u_long)ifr->ifr_mtu;
			vsi->max_frame_size =
			    ifp->if_mtu + ETHER_HDR_LEN + ETHER_CRC_LEN
			    + ETHER_VLAN_ENCAP_LEN;
			if (ifp->if_drv_flags & IFF_DRV_RUNNING)
				iavf_init_locked(sc);
		}
		mtx_unlock(&sc->mtx);
		break;
	case SIOCSIFFLAGS:
		IOCTL_DBG_IF2(ifp, "SIOCSIFFLAGS (Set Interface Flags)");
		mtx_lock(&sc->mtx);
		if (ifp->if_flags & IFF_UP) {
			if ((ifp->if_drv_flags & IFF_DRV_RUNNING)) {
				if ((ifp->if_flags ^ sc->if_flags) &
				    (IFF_PROMISC | IFF_ALLMULTI)) {
					iavf_config_promisc(sc, ifp->if_flags);
				}
			} else
				iavf_init_locked(sc);
		} else
			if (ifp->if_drv_flags & IFF_DRV_RUNNING)
				iavf_stop(sc);

		sc->if_flags = ifp->if_flags;
		mtx_unlock(&sc->mtx);
		break;
	case SIOCADDMULTI:
		IOCTL_DBG_IF2(ifp, "SIOCADDMULTI");
		if (ifp->if_drv_flags & IFF_DRV_RUNNING) {
			mtx_lock(&sc->mtx);
			iavf_disable_intr(vsi);
			iavf_multi_set(sc);
			iavf_enable_intr(vsi);
			mtx_unlock(&sc->mtx);
		}
		break;
	case SIOCDELMULTI:
		IOCTL_DBG_IF2(ifp, "SIOCDELMULTI");
		if (iavf_test_state(&sc->state, IAVF_STATE_RUNNING)) {
			mtx_lock(&sc->mtx);
			iavf_disable_intr(vsi);
			iavf_multi_set(sc);
			iavf_enable_intr(vsi);
			mtx_unlock(&sc->mtx);
		}
		break;
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		IOCTL_DBG_IF2(ifp, "SIOCxIFMEDIA (Get/Set Interface Media)");
		error = ifmedia_ioctl(ifp, ifr, &sc->media, command);
		break;
	case SIOCSIFCAP:
	{
		int mask = ifr->ifr_reqcap ^ ifp->if_capenable;
		IOCTL_DBG_IF2(ifp, "SIOCSIFCAP (Set Capabilities)");

		iavf_cap_txcsum_tso(vsi, ifp, mask);

		if (mask & IFCAP_RXCSUM)
			ifp->if_capenable ^= IFCAP_RXCSUM;
		if (mask & IFCAP_RXCSUM_IPV6)
			ifp->if_capenable ^= IFCAP_RXCSUM_IPV6;
		if (mask & IFCAP_LRO)
			ifp->if_capenable ^= IFCAP_LRO;
		if (mask & IFCAP_VLAN_HWTAGGING)
			ifp->if_capenable ^= IFCAP_VLAN_HWTAGGING;
		if (mask & IFCAP_VLAN_HWFILTER)
			ifp->if_capenable ^= IFCAP_VLAN_HWFILTER;
		if (mask & IFCAP_VLAN_HWTSO)
			ifp->if_capenable ^= IFCAP_VLAN_HWTSO;
		if (ifp->if_drv_flags & IFF_DRV_RUNNING) {
			iavf_init(vsi);
		}
		VLAN_CAPABILITIES(ifp);

		break;
	}

	default:
		IOCTL_DBG_IF2(ifp, "UNKNOWN (0x%X)", (int)command);
		error = ether_ioctl(ifp, command, data);
		break;
	}

	return (error);
}

/**
 * iavf_init_locked - Initialize the device
 * @sc: device softc
 *
 * Called to initialize the device, preparing it for transmit/receive.
 *
 * @pre this function assumes that sc->mtx is locked.
 */
static void
iavf_init_locked(struct iavf_sc *sc)
{
	struct iavf_hw		*hw = &sc->hw;
	struct iavf_vsi		*vsi = &sc->vsi;
	struct iavf_queue	*que = vsi->queues;
	struct ifnet		*ifp = vsi->ifp;
	int			 error = 0;
	u8 tmpaddr[ETHER_ADDR_LEN];

	INIT_DBG_IF(ifp, "begin");

	IAVF_CORE_LOCK_ASSERT(sc);

	/* Stop interface if it's already running */
	iavf_stop(sc);

	error = iavf_reset_complete(hw);
	if (error) {
		device_printf(sc->dev, "%s: VF reset failed\n",
		    __func__);
	}

	if (!iavf_check_asq_alive(hw)) {
		iavf_dbg_info(sc, "ASQ is not alive, re-initializing AQ\n");
		pci_enable_busmaster(sc->dev);
		iavf_shutdown_adminq(hw);
		iavf_init_adminq(hw);
	}

	ifp->if_hwassist = 0;
	if (ifp->if_capenable & IFCAP_TSO)
		ifp->if_hwassist |= CSUM_TSO;
	if (ifp->if_capenable & IFCAP_TXCSUM)
		ifp->if_hwassist |= (CSUM_OFFLOAD_IPV4 & ~CSUM_IP);
	if (ifp->if_capenable & IFCAP_TXCSUM_IPV6)
		ifp->if_hwassist |= CSUM_OFFLOAD_IPV6;

	bcopy(IF_LLADDR(ifp), tmpaddr, ETHER_ADDR_LEN);
	if (!cmp_etheraddr(hw->mac.addr, tmpaddr) &&
	    (iavf_validate_mac_addr(tmpaddr) == IAVF_SUCCESS)) {
		error = iavf_del_mac_filter(sc, hw->mac.addr);
		if (error == 0)
			iavf_send_vc_msg(sc, IAVF_FLAG_AQ_DEL_MAC_FILTER);

		bcopy(tmpaddr, hw->mac.addr, ETH_ALEN);
	}

	error = iavf_add_mac_filter(sc, hw->mac.addr, 0);
	if (!error || error == EEXIST)
		iavf_send_vc_msg(sc, IAVF_FLAG_AQ_ADD_MAC_FILTER);

	/* Prepare the queues for operation */
	for (int i = 0; i < vsi->num_queues; i++, que++) {
		struct  rx_ring	*rxr = &que->rxr;

		iavf_init_tx_ring(que);

		if (vsi->max_frame_size <= MCLBYTES)
			rxr->mbuf_sz = MCLBYTES;
		else
			rxr->mbuf_sz = MJUMPAGESIZE;
		iavf_init_rx_ring(que);
	}

	/* Set initial ITR values */
	iavf_configure_itr(sc);

	/* Configure queues */
	iavf_send_vc_msg(sc, IAVF_FLAG_AQ_CONFIGURE_QUEUES);

	/* Configure RSS capabilities */
	iavf_config_rss(sc);

	/* Map LAN queues and Admin queue to interrupts */
	iavf_send_vc_msg(sc, IAVF_FLAG_AQ_MAP_VECTORS);

	/* Configure promiscuous mode */
	iavf_config_promisc(sc, if_getflags(ifp));

	/* Enable LAN queues */
	iavf_send_vc_msg_sleep(sc, IAVF_FLAG_AQ_ENABLE_QUEUES);

	/* Start the local timer */
	callout_reset(&sc->timer, hz, iavf_local_timer, sc);

	iavf_set_state(&sc->state, IAVF_STATE_RUNNING);

	iavf_enable_intr(vsi);

	ifp->if_drv_flags |= IFF_DRV_RUNNING;

	INIT_DBG_IF(ifp, "end");
}

/**
 * iavf_init - Init entry point for the stack
 * @arg: void pointer to the main VSI
 *
 * Take the softc mutex and call iavf_init_locked.
 */
void
iavf_init(void *arg)
{
	struct iavf_vsi *vsi = (struct iavf_vsi *)arg;
	struct iavf_sc *sc = vsi->back;

	mtx_lock(&sc->mtx);
	iavf_init_locked(sc);
	mtx_unlock(&sc->mtx);
}

/**
 * iavf_init_msix - Allocate MSI/X vectors
 * @sc: device softc
 *
 * @post The AdminQ interrupt callback will also be initialized now, earlier
 * than the regular queue interrupts.
 *
 * @returns the number of vectors allocated, zero on failure.
 */
static int
iavf_init_msix(struct iavf_sc *sc)
{
	device_t dev = sc->dev;
	int rid, want, vectors, queues, available;
	int auto_max_queues;

	rid = PCIR_BAR(IAVF_MSIX_BAR);
	sc->msix_mem = bus_alloc_resource_any(dev,
	    SYS_RES_MEMORY, &rid, RF_ACTIVE);
	if (!sc->msix_mem) {
		/* May not be enabled */
		device_printf(sc->dev,
		    "Unable to map MSIX table\n");
		goto fail;
	}

	/* Update OS cache of MSIX control register values */
	iavf_update_msix_devinfo(dev);

	available = pci_msix_count(dev);
	if (available == 0) { /* system has msix disabled */
		bus_release_resource(dev, SYS_RES_MEMORY,
		    rid, sc->msix_mem);
		sc->msix_mem = NULL;
		goto fail;
	}

	/* Clamp queues to number of CPUs and # of MSI-X vectors available */
	auto_max_queues = min(mp_ncpus, available - 1);
	/* Clamp queues to # assigned to VF by PF */
	auto_max_queues = min(auto_max_queues, sc->vf_res->num_queue_pairs);

	/* Override with tunable value if tunable is less than autoconfig count */
	if ((iavf_max_queues > 0) && (iavf_max_queues <= auto_max_queues))
		queues = iavf_max_queues;
	/* Reject negative numbers */
	else if (iavf_max_queues < 0) {
		device_printf(dev, "iavf_max_queues (%d) is invalid, using "
		    "autoconfig amount (%d)...\n",
		    iavf_max_queues, auto_max_queues);
		queues = auto_max_queues;
	}
	/* Use autoconfig amount if that's lower */
	else if ((iavf_max_queues != 0) && (iavf_max_queues > auto_max_queues)) {
		device_printf(dev, "iavf_max_queues (%d) is too large, using "
		    "autoconfig amount (%d)...\n",
		    iavf_max_queues, auto_max_queues);
		queues = auto_max_queues;
	}
	/* Limit maximum auto-configured queues to 8 if no user value is set */
	else
		queues = min(auto_max_queues, 8);

#ifdef  RSS
	/* If we're doing RSS, clamp at the number of RSS buckets */
	if (queues > rss_getnumbuckets())
		queues = rss_getnumbuckets();
#endif

	/*
	** Want one vector (RX/TX pair) per queue
	** plus an additional for the admin queue.
	*/
	want = queues + 1;
	if (want <= available)	/* Have enough */
		vectors = want;
	else {
		device_printf(sc->dev,
		    "MSIX Configuration Problem, "
		    "%d vectors available but %d wanted!\n",
		    available, want);
		goto fail;
	}

#ifdef RSS
	/*
	* If we're doing RSS, the number of queues needs to
	* match the number of RSS buckets that are configured.
	*
	* + If there's more queues than RSS buckets, we'll end
	*   up with queues that get no traffic.
	*
	* + If there's more RSS buckets than queues, we'll end
	*   up having multiple RSS buckets map to the same queue,
	*   so there'll be some contention.
	*/
	if (queues != rss_getnumbuckets()) {
		device_printf(dev,
		    "%s: queues (%d) != RSS buckets (%d)"
		    "; performance will be impacted.\n",
		     __func__, queues, rss_getnumbuckets());
	}
#endif

	if (pci_alloc_msix(dev, &vectors) == 0) {
		device_printf(sc->dev,
		    "Using MSIX interrupts with %d vectors\n", vectors);
		sc->msix = vectors;
		sc->vsi.num_queues = queues;
	}

	/* Next we need to setup the vector for the Admin Queue */
	rid = 1;	/* zero vector + 1 */
	sc->res = bus_alloc_resource_any(dev, SYS_RES_IRQ,
	    &rid, RF_SHAREABLE | RF_ACTIVE);
	if (sc->res == NULL) {
		device_printf(dev, "Unable to allocate"
		    " bus resource: AQ interrupt \n");
		goto fail;
	}
	if (bus_setup_intr(dev, sc->res,
	    INTR_TYPE_NET | INTR_MPSAFE, NULL,
	    iavf_msix_adminq, sc, &sc->tag)) {
		sc->tag = NULL;
		device_printf(dev, "Failed to register AQ handler");
		goto fail;
	}
	bus_describe_intr(dev, sc->res, sc->tag, "adminq");

	return (vectors);

fail:
	/* The VF driver MUST use MSIX */
	return (0);
}

/**
 * iavf_allocate_pci_resources - Allocate PCI resources
 * @sc: device softc
 *
 * Allocate the PCI resources needed by the device, and call
 * pci_enable_busmaster.
 *
 * @returns zero on success, or an error code on failure.
 */
static int
iavf_allocate_pci_resources(struct iavf_sc *sc)
{
	device_t dev = sc->dev;
	int error = 0;

	error = iavf_allocate_pci_resources_common(sc);
	if (error)
		return (error);

	/* PCI busmaster must be set */
	pci_enable_busmaster(dev);

	/* Disable adminq interrupts (just in case) */
	iavf_disable_adminq_irq(&sc->hw);

	return (0);
}

/**
 * iavf_free_msix_resources - Free MSI-X related resources for a single queue
 * @sc: device softc
 * @que: the queue to release resources for
 *
 * Frees resources associated with a single queue.
 */
static void
iavf_free_msix_resources(struct iavf_sc *sc, struct iavf_queue *que)
{
	device_t dev = sc->dev;

	/*
	**  Release all msix queue resources:
	*/
	if (que->tag != NULL) {
		bus_teardown_intr(dev, que->res, que->tag);
		que->tag = NULL;
	}
	if (que->res != NULL) {
		int rid = que->msix + 1;
		bus_release_resource(dev, SYS_RES_IRQ, rid, que->res);
		que->res = NULL;
	}
	if (que->tq != NULL) {
		taskqueue_free(que->tq);
		que->tq = NULL;
	}
}

/**
 * iavf_free_pci_resources - Free PCI resources
 * @sc: device softc
 *
 * Releases PCI resources previously allocated for accessing the PCI registers
 * and the MSI-X table.
 */
static void
iavf_free_pci_resources(struct iavf_sc *sc)
{
	device_t dev = sc->dev;

	pci_release_msi(dev);

	if (sc->msix_mem != NULL)
		bus_release_resource(dev, SYS_RES_MEMORY,
		    PCIR_BAR(IAVF_MSIX_BAR), sc->msix_mem);

	if (sc->pci_mem != NULL)
		bus_release_resource(dev, SYS_RES_MEMORY,
		    PCIR_BAR(0), sc->pci_mem);
}

/**
 * iavf_setup_admin_tq - Setup task queues
 * @sc: device softc
 *
 * Create taskqueue and tasklet for Admin Queue interrupts.
 *
 * @returns zero on success, or an error code on failure.
 */
static int
iavf_setup_admin_tq(struct iavf_sc *sc)
{
	device_t dev = sc->dev;
	int error = 0;

	TASK_INIT(&sc->aq_irq, 0, iavf_do_adminq, sc);

	sc->tq = taskqueue_create_fast("iavf_aq", M_NOWAIT,
	    taskqueue_thread_enqueue, &sc->tq);
	if (!sc->tq) {
		device_printf(dev, "taskqueue_create_fast (for AQ) returned NULL!\n");
		return (ENOMEM);
	}
	error = taskqueue_start_threads(&sc->tq, 1, PI_NET, "%s aq",
	    device_get_nameunit(sc->dev));
	if (error) {
		device_printf(dev, "taskqueue_start_threads (for AQ) error: %d\n",
		    error);
		taskqueue_free(sc->tq);
		return (error);
	}

	return (error);
}

/**
 * iavf_assign_msix - Assign MSI-X vectors
 * @sc: device softc
 *
 * Setup MSI-X Interrupt resources and handlers for the VSI queues
 *
 * @returns zero on success, or an error code on failure.
 */
static int
iavf_assign_msix(struct iavf_sc *sc)
{
	device_t	dev = sc->dev;
	struct		iavf_vsi *vsi = &sc->vsi;
	struct		iavf_queue *que = vsi->queues;
	int		error, rid, vector = 1;
#ifdef	RSS
	cpuset_t	cpu_mask;
#endif

	for (int i = 0; i < vsi->num_queues; i++, vector++, que++) {
		int cpu_id = i;
		rid = vector + 1;
		que->res = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid,
		    RF_SHAREABLE | RF_ACTIVE);
		if (que->res == NULL) {
			device_printf(dev,"Unable to allocate"
			    " bus resource: que interrupt [%d]\n", vector);
			return (ENXIO);
		}
		/* Set the handler function */
		error = bus_setup_intr(dev, que->res,
		    INTR_TYPE_NET | INTR_MPSAFE, NULL,
		    iavf_msix_que, que, &que->tag);
		if (error) {
			que->tag = NULL;
			device_printf(dev, "Failed to register que handler");
			return (error);
		}
		bus_describe_intr(dev, que->res, que->tag, "que %d", i);
		/* Bind the vector to a CPU */
#ifdef RSS
		cpu_id = rss_getcpu(i % rss_getnumbuckets());
#endif
		bus_bind_intr(dev, que->res, cpu_id);
		que->msix = vector;
		TASK_INIT(&que->tx_task, 0, iavf_deferred_mq_start, que);
		TASK_INIT(&que->task, 0, iavf_handle_que, que);
		que->tq = taskqueue_create_fast("iavf_que", M_NOWAIT,
		    taskqueue_thread_enqueue, &que->tq);
#ifdef RSS
		CPU_SETOF(cpu_id, &cpu_mask);
		taskqueue_start_threads_cpuset(&que->tq, 1, PI_NET,
		    &cpu_mask, "%s (bucket %d)",
		    device_get_nameunit(dev), cpu_id);
#else
                taskqueue_start_threads(&que->tq, 1, PI_NET,
                    "%s que", device_get_nameunit(dev));
#endif
	}

	return (0);
}

/**
 * iavf_setup_interface - Setup stack ifnet structure
 * @dev: stack device pointer
 * @sc: device softc
 *
 * Setup networking device structure and register an interface.
 *
 * @returns zero on success, or an error code on failure.
 */
static int
iavf_setup_interface(device_t dev, struct iavf_sc *sc)
{
	struct iavf_vsi *vsi = &sc->vsi;
	struct iavf_queue *que = vsi->queues;
	struct ifnet *ifp;

	iavf_dbg_init(sc, "begin\n");

	ifp = vsi->ifp = if_alloc(IFT_ETHER);
	if (ifp == NULL) {
		device_printf(dev, "%s: could not allocate ifnet"
		    " structure!\n", __func__);
		return (ENOMEM);
	}

	if_initname(ifp, device_get_name(dev), device_get_unit(dev));

	ifp->if_mtu = ETHERMTU;
	ifp->if_init = iavf_init;
	ifp->if_softc = vsi;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = iavf_ioctl;

#if __FreeBSD_version >= 1100000
	if_setgetcounterfn(ifp, iavf_get_counter);
#endif

	ifp->if_transmit = iavf_mq_start;

	ifp->if_qflush = iavf_qflush;
	ifp->if_snd.ifq_maxlen = que->num_tx_desc - 2;

	ether_ifattach(ifp, sc->hw.mac.addr);

	vsi->max_frame_size =
	    ifp->if_mtu + ETHER_HDR_LEN + ETHER_CRC_LEN
	    + ETHER_VLAN_ENCAP_LEN;

	iavf_set_initial_baudrate(ifp);

	ifp->if_hw_tsomax = IP_MAXPACKET - (ETHER_HDR_LEN + ETHER_CRC_LEN);
	ifp->if_hw_tsomaxsegcount = IAVF_MAX_TSO_SEGS;
	ifp->if_hw_tsomaxsegsize = IAVF_MAX_DMA_SEG_SIZE;

	/*
	 * Tell the upper layer(s) we support long frames.
	 */
	ifp->if_hdrlen = sizeof(struct ether_vlan_header);

	ifp->if_capabilities |= IFCAP_HWCSUM;
	ifp->if_capabilities |= IFCAP_HWCSUM_IPV6;
	ifp->if_capabilities |= IFCAP_TSO;
	ifp->if_capabilities |= IFCAP_JUMBO_MTU;

	ifp->if_capabilities |= IFCAP_VLAN_HWTAGGING
			     |  IFCAP_VLAN_HWTSO
			     |  IFCAP_VLAN_MTU
			     |  IFCAP_VLAN_HWCSUM
			     |  IFCAP_LRO;
	ifp->if_capenable = ifp->if_capabilities;

	/*
	** Don't turn this on by default, if vlans are
	** created on another pseudo device (eg. lagg)
	** then vlan events are not passed thru, breaking
	** operation, but with HW FILTER off it works. If
	** using vlans directly on the iavf driver you can
	** enable this and get full hardware tag filtering.
	*/
	ifp->if_capabilities |= IFCAP_VLAN_HWFILTER;

	/*
	 * Specify the media types supported by this adapter and register
	 * callbacks to update media and link information
	 */
	ifmedia_init(&sc->media, IFM_IMASK, iavf_media_change,
		     iavf_media_status);

	ifmedia_add(&sc->media, IFM_ETHER | IFM_AUTO, 0, NULL);
	ifmedia_set(&sc->media, IFM_ETHER | IFM_AUTO);

	iavf_dbg_init(sc, "end\n");
	return (0);
}

/**
 * iavf_setup_queue - Allocate and setup a single queue
 * @sc: device softc
 * @que: pointer to que structure to initialize
 *
 * Allocate and setup a single queue for Tx and Rx.
 *
 * @returns zero on success, or an error code on failure.
 */
static int
iavf_setup_queue(struct iavf_sc *sc, struct iavf_queue *que)
{
	device_t		dev = sc->dev;
	struct tx_ring		*txr;
	struct rx_ring		*rxr;
	int			rsize, tsize;
	int			error = IAVF_SUCCESS;

	txr = &que->txr;
	txr->que = que;
	txr->tail = IAVF_QTX_TAIL1(que->me);
	/* Initialize the TX lock */
	snprintf(txr->mtx_name, sizeof(txr->mtx_name), "%s:tx(%d)",
	    device_get_nameunit(dev), que->me);
	mtx_init(&txr->mtx, txr->mtx_name, NULL, MTX_DEF);
	/*
	 * Create the TX descriptor ring
	 *
	 * In Head Writeback mode, the descriptor ring is one bigger
	 * than the number of descriptors for space for the HW to
	 * write back index of last completed descriptor.
	 */
	if (sc->vsi.enable_head_writeback) {
		tsize = roundup2((que->num_tx_desc *
		    sizeof(struct iavf_tx_desc)) +
		    sizeof(u32), DBA_ALIGN);
	} else {
		tsize = roundup2((que->num_tx_desc *
		    sizeof(struct iavf_tx_desc)), DBA_ALIGN);
	}
	if (iavf_allocate_dma_mem(&sc->hw,
	    &txr->dma, iavf_mem_reserved, tsize, DBA_ALIGN)) {
		device_printf(dev,
		    "Unable to allocate TX Descriptor memory\n");
		error = ENOMEM;
		goto err_destroy_tx_mtx;
	}
	txr->base = (struct iavf_tx_desc *)txr->dma.va;
	bzero((void *)txr->base, tsize);
	/* Now allocate transmit soft structs for the ring */
	if (iavf_allocate_tx_data(que)) {
		device_printf(dev,
		    "Critical Failure setting up TX structures\n");
		error = ENOMEM;
		goto err_free_tx_dma;
	}
	/* Allocate a buf ring */
	txr->br = buf_ring_alloc(iavf_txbrsz, M_IAVF,
	    M_WAITOK, &txr->mtx);
	if (txr->br == NULL) {
		device_printf(dev,
		    "Critical Failure setting up TX buf ring\n");
		error = ENOMEM;
		goto err_free_tx_data;
	}

	/*
	 * Next the RX queues...
	 */
	rsize = roundup2(que->num_rx_desc *
	    sizeof(union iavf_rx_desc), DBA_ALIGN);
	rxr = &que->rxr;
	rxr->que = que;
	rxr->tail = IAVF_QRX_TAIL1(que->me);

	/* Initialize the RX side lock */
	snprintf(rxr->mtx_name, sizeof(rxr->mtx_name), "%s:rx(%d)",
	    device_get_nameunit(dev), que->me);
	mtx_init(&rxr->mtx, rxr->mtx_name, NULL, MTX_DEF);

	if (iavf_allocate_dma_mem(&sc->hw,
	    &rxr->dma, iavf_mem_reserved, rsize, 4096)) { //JFV - should this be DBA?
		device_printf(dev,
		    "Unable to allocate RX Descriptor memory\n");
		error = ENOMEM;
		goto err_destroy_rx_mtx;
	}
	rxr->base = (union iavf_rx_desc *)rxr->dma.va;
	bzero((void *)rxr->base, rsize);

	/* Allocate receive soft structs for the ring */
	if (iavf_allocate_rx_data(que)) {
		device_printf(dev,
		    "Critical Failure setting up receive structs\n");
		error = ENOMEM;
		goto err_free_rx_dma;
	}

	return (0);

err_free_rx_dma:
	iavf_free_dma_mem(&sc->hw, &rxr->dma);
err_destroy_rx_mtx:
	mtx_destroy(&rxr->mtx);
	/* err_free_tx_buf_ring */
	buf_ring_free(txr->br, M_IAVF);
err_free_tx_data:
	iavf_free_que_tx(que);
err_free_tx_dma:
	iavf_free_dma_mem(&sc->hw, &txr->dma);
err_destroy_tx_mtx:
	mtx_destroy(&txr->mtx);

	return (error);
}

/**
 * iavf_setup_queues - Allocate and setup the interface queues
 * @sc: device softc
 *
 * Allocate memory for the array of queues, and call iavf_setup_queue for each
 * queue structure.
 *
 * @post also initializes the sysctl context for the main VSI.
 *
 * @returns zero on success, or an error code on failure.
 */
static int
iavf_setup_queues(struct iavf_sc *sc)
{
	device_t		dev = sc->dev;
	struct iavf_vsi		*vsi;
	struct iavf_queue	*que;
	int			i;
	int			error = IAVF_SUCCESS;

	vsi = &sc->vsi;
	vsi->back = sc;
	vsi->hw = &sc->hw;
	vsi->num_vlans = 0;

	/* Get memory for the station queues */
	if (!(vsi->queues =
		(struct iavf_queue *) malloc(sizeof(struct iavf_queue) *
		vsi->num_queues, M_IAVF, M_NOWAIT | M_ZERO))) {
			device_printf(dev, "Unable to allocate queue memory\n");
			return ENOMEM;
	}

	for (i = 0; i < vsi->num_queues; i++) {
		que = &vsi->queues[i];
		que->num_tx_desc = vsi->num_tx_desc;
		que->num_rx_desc = vsi->num_rx_desc;
		que->me = i;
		que->vsi = vsi;

		if (iavf_setup_queue(sc, que)) {
			error = ENOMEM;
			goto err_free_queues;
		}
	}
	sysctl_ctx_init(&vsi->sysctl_ctx);

	return (0);

err_free_queues:
	while (i--)
		iavf_free_queue(sc, &vsi->queues[i]);

	free(vsi->queues, M_IAVF);

	return (error);
}

/**
 * iavf_register_vlan - Register a VLAN filter
 * @arg: void pointer to the device softc
 * @ifp: ifnet structure
 * @vtag: the VLAN to add
 *
 * This routine is run via an vlan config EVENT, it enables us to use the HW
 * Filter table since we can get the vlan id. This just creates the entry in
 * the soft version of the VFTA, init will repopulate the real table.
 */
static void
iavf_register_vlan(void *arg, struct ifnet *ifp, u16 vtag)
{
	struct iavf_sc *sc = (struct iavf_sc *)arg;
	struct iavf_vsi *vsi = &sc->vsi;

	if (ifp->if_softc != vsi)   /* Not our event */
		return;

	if ((vtag == 0) || (vtag > 4095))	/* Invalid */
		return;

	mtx_lock(&sc->mtx);

	/* Add VLAN 0 to list, for untagged traffic */
	if (vsi->num_vlans == 0)
		iavf_add_vlan_filter(sc, 0);

	iavf_add_vlan_filter(sc, vtag);

	++vsi->num_vlans;

	iavf_send_vc_msg(sc, IAVF_FLAG_AQ_ADD_VLAN_FILTER);

	mtx_unlock(&sc->mtx);
}

/**
 * iavf_unregister_vlan - Remove a VLAN filter
 * @arg: void pointer to the device softc
 * @ifp: ifnet structure
 * @vtag: the VLAN to remove
 *
 * This routine is run via an vlan unconfig EVENT, remove our entry in the
 * soft vfta.
*/
static void
iavf_unregister_vlan(void *arg, struct ifnet *ifp, u16 vtag)
{
	struct iavf_sc *sc = (struct iavf_sc *)arg;
	struct iavf_vsi *vsi = &sc->vsi;
	int i = 0;

	if (ifp->if_softc != vsi)
		return;

	if ((vtag == 0) || (vtag > 4095))	/* Invalid */
		return;

	mtx_lock(&sc->mtx);

	if (vsi->num_vlans == 0) {
		mtx_unlock(&sc->mtx);
		return;
	}

	i = iavf_mark_del_vlan_filter(sc, vtag);
	vsi->num_vlans -= i;

	/* Remove VLAN filter 0 if the last VLAN is being removed */
	if (vsi->num_vlans == 0)
		i += iavf_mark_del_vlan_filter(sc, 0);

	if (i > 0)
		iavf_send_vc_msg(sc, IAVF_FLAG_AQ_DEL_VLAN_FILTER);

	mtx_unlock(&sc->mtx);
}

/**
 * iavf_teardown_adminq_msix - Release bus resources for the AdminQ vector
 * @sc: device softc
 *
 * Release the bus interrupt resources previously allocated to handle the
 * AdminQ interrupt vector.
 */
static void
iavf_teardown_adminq_msix(struct iavf_sc *sc)
{
	device_t dev = sc->dev;
	int error = 0;

	if (sc->tag != NULL) {
		error = bus_teardown_intr(dev, sc->res, sc->tag);
		if (error) {
			device_printf(dev, "bus_teardown_intr() for"
			    " interrupt 0 failed, error %d\n", error);
		}
		sc->tag = NULL;
	}
	if (sc->res != NULL) {
		error = bus_release_resource(dev, SYS_RES_IRQ, 1, sc->res);
		if (error) {
			device_printf(dev, "bus_release_resource() for"
			    " interrupt 0 failed, error %d\n", error);
		}
		sc->res = NULL;
	}
}

/**
 * iavf_msix_adminq - Admin Queue interrupt handler
 * @arg: void pointer to the device softc
 *
 * Interrupt handler for the AdminQ interrupt vector. Primarily used to
 * schedule the AdminQ task to handle the event outside of interrupt context.
 */
static void
iavf_msix_adminq(void *arg)
{
	struct iavf_sc	*sc = (struct iavf_sc *)arg;
	struct iavf_hw	*hw = &sc->hw;
	u32		reg;

	++sc->admin_irq;

	if (!iavf_test_state(&sc->state, IAVF_STATE_INITIALIZED))
		return;

        reg = rd32(hw, IAVF_VFINT_ICR01);
        rd32(hw, IAVF_VFINT_ICR0_ENA1);

        reg = rd32(hw, IAVF_VFINT_DYN_CTL01);
        reg |= IAVF_VFINT_DYN_CTL01_CLEARPBA_MASK;
        wr32(hw, IAVF_VFINT_DYN_CTL01, reg);

	/* schedule task */
	taskqueue_enqueue(sc->tq, &sc->aq_irq);
}

/**
 * iavf_enable_intr - Enable device interrupts
 * @vsi: main VSI
 *
 * Enable the AdminQ interrupt and the interrupts for each queue pair.
 */
void
iavf_enable_intr(struct iavf_vsi *vsi)
{
	struct iavf_hw		*hw = vsi->hw;
	struct iavf_queue	*que = vsi->queues;

	iavf_enable_adminq_irq(hw);
	for (int i = 0; i < vsi->num_queues; i++, que++)
		iavf_enable_queue_irq(hw, que->me);
}

/**
 * iavf_disable_intr - Disable device interrupts
 * @vsi: main VSI
 *
 * Disable queue pair interrupts.
 *
 * @post this function does not disable the AdminQ interrupt.
 */
void
iavf_disable_intr(struct iavf_vsi *vsi)
{
        struct iavf_hw          *hw = vsi->hw;
        struct iavf_queue       *que = vsi->queues;

	for (int i = 0; i < vsi->num_queues; i++, que++)
		iavf_disable_queue_irq(hw, que->me);
}

/**
 * iavf_enable_queue_irq - Program the DYN_CTL register for an interrupt
 * @hw: device hardware structure
 * @id: interrupt vector id to program
 *
 * Enable the given interrupt vector by programming the IAVF_VFINT_DYN_CTLN1
 * register for that interrupt.
 */
static void
iavf_enable_queue_irq(struct iavf_hw *hw, int id)
{
	u32		reg;

	reg = IAVF_VFINT_DYN_CTLN1_INTENA_MASK |
            IAVF_VFINT_DYN_CTLN1_CLEARPBA_MASK |
	    IAVF_VFINT_DYN_CTLN1_ITR_INDX_MASK;
	wr32(hw, IAVF_VFINT_DYN_CTLN1(id), reg);
}

/**
 * iavf_disable_queue_irq - Disable the DYN_CTL register for an interrupt
 * @hw: device hardware structure
 * @id: the vector to disable
 *
 * Disable the given interrupt vector by programming the IAVF_VFINT_DYN_CTLN1
 * register.
 */
static void
iavf_disable_queue_irq(struct iavf_hw *hw, int id)
{
	wr32(hw, IAVF_VFINT_DYN_CTLN1(id),
	    IAVF_VFINT_DYN_CTLN1_ITR_INDX_MASK);
	rd32(hw, IAVF_VFGEN_RSTAT);
	return;
}

/**
 * iavf_configure_itr - Get initial ITR values from tunable values.
 * @sc: device softc
 *
 * Setup the initial ITR values from the driver tunables.
 */
static void
iavf_configure_itr(struct iavf_sc *sc)
{
	struct iavf_hw		*hw = &sc->hw;
	struct iavf_vsi		*vsi = &sc->vsi;
	struct iavf_queue	*que = vsi->queues;

	vsi->rx_itr_setting = sc->rx_itr;
	vsi->tx_itr_setting = sc->tx_itr;

	for (int i = 0; i < vsi->num_queues; i++, que++) {
		struct tx_ring	*txr = &que->txr;
		struct rx_ring	*rxr = &que->rxr;

		wr32(hw, IAVF_VFINT_ITRN1(IAVF_RX_ITR, i),
		    vsi->rx_itr_setting);
		rxr->itr = vsi->rx_itr_setting;
		rxr->latency = IAVF_AVE_LATENCY;

		wr32(hw, IAVF_VFINT_ITRN1(IAVF_TX_ITR, i),
		    vsi->tx_itr_setting);
		txr->itr = vsi->tx_itr_setting;
		txr->latency = IAVF_AVE_LATENCY;
	}
}

/**
 * iavf_set_queue_rx_itr - Update dynamic Rx ITR
 * @que: queue to program
 *
 * Provide a update to the queue RX interrupt moderation value.
 */
static void
iavf_set_queue_rx_itr(struct iavf_queue *que)
{
	struct iavf_vsi	*vsi = que->vsi;
	struct iavf_hw	*hw = vsi->hw;
	struct rx_ring	*rxr = &que->rxr;
	u16		rx_itr;
	u16		rx_latency = 0;
	int		rx_bytes;


	/* Idle, do nothing */
	if (rxr->bytes == 0)
		return;

	if (iavf_dynamic_rx_itr) {
		rx_bytes = rxr->bytes/rxr->itr;
		rx_itr = rxr->itr;

		/* Adjust latency range */
		switch (rxr->latency) {
		case IAVF_LOW_LATENCY:
			if (rx_bytes > 10) {
				rx_latency = IAVF_AVE_LATENCY;
				rx_itr = IAVF_ITR_20K;
			}
			break;
		case IAVF_AVE_LATENCY:
			if (rx_bytes > 20) {
				rx_latency = IAVF_BULK_LATENCY;
				rx_itr = IAVF_ITR_8K;
			} else if (rx_bytes <= 10) {
				rx_latency = IAVF_LOW_LATENCY;
				rx_itr = IAVF_ITR_100K;
			}
			break;
		case IAVF_BULK_LATENCY:
			if (rx_bytes <= 20) {
				rx_latency = IAVF_AVE_LATENCY;
				rx_itr = IAVF_ITR_20K;
			}
			break;
		 }

		rxr->latency = rx_latency;

		if (rx_itr != rxr->itr) {
			/* do an exponential smoothing */
			rx_itr = (10 * rx_itr * rxr->itr) /
			    ((9 * rx_itr) + rxr->itr);
			rxr->itr = min(rx_itr, IAVF_MAX_ITR);
			wr32(hw, IAVF_VFINT_ITRN1(IAVF_RX_ITR,
			    que->me), rxr->itr);
		}
	} else { /* We may have have toggled to non-dynamic */
		if (vsi->rx_itr_setting & IAVF_ITR_DYNAMIC)
			vsi->rx_itr_setting = iavf_rx_itr;
		/* Update the hardware if needed */
		if (rxr->itr != vsi->rx_itr_setting) {
			rxr->itr = vsi->rx_itr_setting;
			wr32(hw, IAVF_VFINT_ITRN1(IAVF_RX_ITR,
			    que->me), rxr->itr);
		}
	}
	rxr->bytes = 0;
	rxr->packets = 0;
	return;
}


/**
 * iavf_set_queue_tx_itr - Update dynamic Tx ITR
 * @que: the queue to program
 *
 * Provide a update to the queue TX interrupt moderation value.
 */
static void
iavf_set_queue_tx_itr(struct iavf_queue *que)
{
	struct iavf_vsi	*vsi = que->vsi;
	struct iavf_hw	*hw = vsi->hw;
	struct tx_ring	*txr = &que->txr;
	u16		tx_itr;
	u16		tx_latency = 0;
	int		tx_bytes;


	/* Idle, do nothing */
	if (txr->bytes == 0)
		return;

	if (iavf_dynamic_tx_itr) {
		tx_bytes = txr->bytes/txr->itr;
		tx_itr = txr->itr;

		switch (txr->latency) {
		case IAVF_LOW_LATENCY:
			if (tx_bytes > 10) {
				tx_latency = IAVF_AVE_LATENCY;
				tx_itr = IAVF_ITR_20K;
			}
			break;
		case IAVF_AVE_LATENCY:
			if (tx_bytes > 20) {
				tx_latency = IAVF_BULK_LATENCY;
				tx_itr = IAVF_ITR_8K;
			} else if (tx_bytes <= 10) {
				tx_latency = IAVF_LOW_LATENCY;
				tx_itr = IAVF_ITR_100K;
			}
			break;
		case IAVF_BULK_LATENCY:
			if (tx_bytes <= 20) {
				tx_latency = IAVF_AVE_LATENCY;
				tx_itr = IAVF_ITR_20K;
			}
			break;
		}

		txr->latency = tx_latency;

		if (tx_itr != txr->itr) {
			/* do an exponential smoothing */
			tx_itr = (10 * tx_itr * txr->itr) /
			    ((9 * tx_itr) + txr->itr);
			txr->itr = min(tx_itr, IAVF_MAX_ITR);
			wr32(hw, IAVF_VFINT_ITRN1(IAVF_TX_ITR,
			    que->me), txr->itr);
		}

	} else { /* We may have have toggled to non-dynamic */
		if (vsi->tx_itr_setting & IAVF_ITR_DYNAMIC)
			vsi->tx_itr_setting = iavf_tx_itr;
		/* Update the hardware if needed */
		if (txr->itr != vsi->tx_itr_setting) {
			txr->itr = vsi->tx_itr_setting;
			wr32(hw, IAVF_VFINT_ITRN1(IAVF_TX_ITR,
			    que->me), txr->itr);
		}
	}
	txr->bytes = 0;
	txr->packets = 0;
	return;
}


/**
 * iavf_handle_que - Taskqueue task for processing a device queue
 * @context: void pointer to the queue
 * @pending: unused parameter, required by taskqueue API
 *
 * Task to handle queue traffic outside of interrupt context. Will perform
 * a limited amount of work. If more work remains, reschedule the task.
 * Otherwise, re-enable the MSI-X interrupt.
 */
static void
iavf_handle_que(void *context, int pending __unused)
{
	struct iavf_queue *que = (struct iavf_queue *)context;
	struct iavf_vsi *vsi = que->vsi;
	struct iavf_hw *hw = vsi->hw;
	struct tx_ring *txr = &que->txr;
	struct ifnet *ifp = vsi->ifp;
	bool more;

	if (ifp->if_drv_flags & IFF_DRV_RUNNING) {
		more = iavf_rxeof(que, IAVF_RX_LIMIT);
		mtx_lock(&txr->mtx);
		iavf_txeof(que);
		if (!drbr_empty(ifp, txr->br))
			iavf_mq_start_locked(ifp, txr);
		mtx_unlock(&txr->mtx);
		if (more) {
			taskqueue_enqueue(que->tq, &que->task);
			return;
		}
	}

	/* Reenable this interrupt - hmmm */
	iavf_enable_queue_irq(hw, que->me);
	return;
}

/**
 * iavf_msix_que - MSI-X interrupt handler for queue events
 * @arg: void pointer to the queue structure
 *
 * MSIX Queue Interrupt Service routine used to handle interrupts for a given
 * device queue.
 *
 * Will perform a limited amount of Tx and Rx cleanup. If there is still more
 * work to do, schedule the queue task to handle it outside of interrupt
 * context. Otherwise, re-enable the interrupt to wait for more traffic.
 */
static void
iavf_msix_que(void *arg)
{
	struct iavf_queue *que = (struct iavf_queue *)arg;
	struct iavf_vsi	*vsi = que->vsi;
	struct iavf_hw *hw = vsi->hw;
	struct tx_ring *txr = &que->txr;
	bool more_tx, more_rx;

	/* Spurious interrupts are ignored */
	if (!(vsi->ifp->if_drv_flags & IFF_DRV_RUNNING))
		return;

	/* There are drivers which disable auto-masking of interrupts,
	 * which is a global setting for all ports. We have to make sure
	 * to mask it to not lose IRQs */
	iavf_disable_queue_irq(hw, que->me);

	++que->irqs;

	more_rx = iavf_rxeof(que, IAVF_RX_LIMIT);

	mtx_lock(&txr->mtx);
	more_tx = iavf_txeof(que);
	/*
	 * Make certain that if the stack 
	 * has anything queued the task gets
	 * scheduled to handle it.
	 */
	if (!drbr_empty(vsi->ifp, txr->br))
		more_tx = 1;
	mtx_unlock(&txr->mtx);

	iavf_set_queue_rx_itr(que);
	iavf_set_queue_tx_itr(que);

	if (more_tx || more_rx)
		taskqueue_enqueue(que->tq, &que->task);
	else
		iavf_enable_queue_irq(hw, que->me);
}

/**
 * iavf_media_status - Media Ioctl callback
 * @ifp: ifnet structure
 * @ifmr: structure to store media status
 *
 * This routine is called whenever the user queries the status of
 * the interface using ifconfig.
 */
static void
iavf_media_status(struct ifnet * ifp, struct ifmediareq * ifmr)
{
	struct iavf_vsi *vsi = (struct iavf_vsi *)ifp->if_softc;
	struct iavf_sc *sc = vsi->back;

	mtx_lock(&sc->mtx);

	iavf_media_status_common(sc, ifmr);

	mtx_unlock(&sc->mtx);
}

/**
 * iavf_media_change - Media Ioctl callback
 * @ifp: ifnet structure
 *
 * This routine is called when the user changes speed/duplex using
 * media/mediopt option with ifconfig.
 *
 * @returns zero on success, or an error code on failure.
 */
static int
iavf_media_change(struct ifnet *ifp)
{
	return iavf_media_change_common(ifp);
}

/**
 * iavf_local_timer - Timer routine
 * @arg: void pointer to the device softc
 *
 * This routine checks for link status, updates statistics,
 * and runs the watchdog check.
 */
static void
iavf_local_timer(void *arg)
{
	struct iavf_sc *sc = (struct iavf_sc *)arg;
	struct iavf_hw *hw = &sc->hw;
	struct iavf_vsi	*vsi = &sc->vsi;
	u32 val;

	IAVF_CORE_LOCK_ASSERT(sc);

	/* If Reset is in progress just bail */
	if (iavf_test_state(&sc->state, IAVF_STATE_RESET_PENDING))
		return;

	/* Check for when PF triggers a VF reset */
	val = rd32(hw, IAVF_VFGEN_RSTAT) &
	    IAVF_VFGEN_RSTAT_VFR_STATE_MASK;

	if (val != VIRTCHNL_VFR_VFACTIVE
	    && val != VIRTCHNL_VFR_COMPLETED) {
		iavf_dbg_info(sc, "%s: reset in progress! (%d)",
		    __func__, val);
		return;
	}

	iavf_request_stats(sc);

	/* clean and process any events */
	taskqueue_enqueue(sc->tq, &sc->aq_irq);

	if (iavf_queue_hang_check(vsi))
		sc->watchdog_events++;

	callout_reset(&sc->timer, hz, iavf_local_timer, sc);
}

/**
 * iavf_update_link_status - Update OS of the link state
 * @sc: device softc
 *
 * This routine updates the OS on the link state. The real check of the
 * hardware only happens with a link interrupt.
 */
void
iavf_update_link_status(struct iavf_sc *sc)
{
	struct iavf_vsi		*vsi = &sc->vsi;
	struct ifnet		*ifp = vsi->ifp;

	if (sc->link_up) {
		if (vsi->link_active == FALSE) {
			if (bootverbose)
				if_printf(ifp, "Link is Up, %s\n",
				    iavf_vc_speed_to_string(sc->link_speed));
			vsi->link_active = TRUE;
			if_link_state_change(ifp, LINK_STATE_UP);
		}
	} else { /* Link down */
		if (vsi->link_active == TRUE) {
			if (bootverbose)
				if_printf(ifp, "Link is Down\n");
			if_link_state_change(ifp, LINK_STATE_DOWN);
			vsi->link_active = FALSE;
		}
	}
}

/**
 * iavf_stop - Stop the device
 * @sc: device softc
 *
 * This routine disables all traffic on the adapter by issuing a
 * global reset on the MAC, then it deallocates Tx/Rx buffers.
 */
static void
iavf_stop(struct iavf_sc *sc)
{
	struct iavf_vsi *vsi = &sc->vsi;
	struct ifnet *ifp = vsi->ifp;

	INIT_DBG_IF(ifp, "begin");

	IAVF_CORE_LOCK_ASSERT(sc);

	ifp->if_drv_flags &= ~IFF_DRV_RUNNING;

	iavf_clear_state(&sc->state, IAVF_STATE_RUNNING);

	iavf_disable_intr(vsi);

	iavf_disable_queues_with_retries(sc);

	/* Stop the local timer */
	callout_stop(&sc->timer);

	INIT_DBG_IF(ifp, "end");
}

/**
 * iavf_free_queue - Free a single queue struct
 * @sc: device softc
 * @que: the queue to free
 *
 * Release resources connected to a single queue pair.
 */
static void
iavf_free_queue(struct iavf_sc *sc, struct iavf_queue *que)
{
	struct tx_ring *txr = &que->txr;
	struct rx_ring *rxr = &que->rxr;

	if (!mtx_initialized(&txr->mtx)) /* uninitialized */
		return;
	IAVF_TX_LOCK(txr);
	if (txr->br)
		buf_ring_free(txr->br, M_IAVF);
	iavf_free_que_tx(que);
	if (txr->base)
		iavf_free_dma_mem(&sc->hw, &txr->dma);
	IAVF_TX_UNLOCK(txr);
	IAVF_TX_LOCK_DESTROY(txr);

	if (!mtx_initialized(&rxr->mtx)) /* uninitialized */
		return;
	IAVF_RX_LOCK(rxr);
	iavf_free_que_rx(que);
	if (rxr->base)
		iavf_free_dma_mem(&sc->hw, &rxr->dma);
	IAVF_RX_UNLOCK(rxr);
	IAVF_RX_LOCK_DESTROY(rxr);
}

/**
 * iavf_free_queues - Release all queue resources
 * @vsi: the main VSI
 *
 * Free all station queue structs by calling ice_free_queue on each queue
 * pair. Then release the sysctl context for the per-queue sysctls, and
 * release the queue array memory.
 */
static void
iavf_free_queues(struct iavf_vsi *vsi)
{
	struct iavf_sc	*sc = (struct iavf_sc *)vsi->back;
	struct iavf_queue	*que = vsi->queues;

	for (int i = 0; i < vsi->num_queues; i++, que++) {
		/* First, free the MSI-X resources */
		iavf_free_msix_resources(sc, que);
		/* Then free other queue data */
		iavf_free_queue(sc, que);
	}

	sysctl_ctx_free(&vsi->sysctl_ctx);
	free(vsi->queues, M_IAVF);
}

/**
 * iavf_del_mac_filter - Marks a MAC filter for deletion.
 * @sc: device softc
 * @macaddr: the MAC address filter to remove
 *
 * Prepare to remove a MAC address filter by marking it for removal. The
 * actual delete will be handled later.
 *
 * @returns zero on success, or ENOENT if the filter doesn't exist.
 */
static int
iavf_del_mac_filter(struct iavf_sc *sc, u8 *macaddr)
{
	struct iavf_mac_filter	*f;

	f = iavf_find_mac_filter(sc, macaddr);
	if (f == NULL)
		return (ENOENT);

	f->flags |= IAVF_FILTER_DEL;
	return (0);
}

/**
 * iavf_do_adminq - Tasklet handler for MSIX Adminq interrupts
 * @context: void pointer to the device softc
 * @pending: unused parameter, required by taskqueue API
 *
 * @remark done outside interrupt context since it might sleep
 *
 * Takes the sc->mtx and calls iavf_do_adminq_locked.
*/
static void
iavf_do_adminq(void *context, int pending __unused)
{
	struct iavf_sc *sc = (struct iavf_sc *)context;

	mtx_lock(&sc->mtx);
	iavf_do_adminq_locked(sc);
	mtx_unlock(&sc->mtx);
}

/**
 * iavf_do_adminq_locked - Handle AdminQ messages
 * @sc: device softc
 *
 * @pre assumes the sc->mtx lock is held.
 *
 * Check the AdminQ for any messages from the PF and handle them
 * appropriately.
 */
static void
iavf_do_adminq_locked(struct iavf_sc *sc)
{
	struct iavf_hw			*hw = &sc->hw;
	struct iavf_arq_event_info	event;
	struct virtchnl_msg	*v_msg;
	device_t			dev = sc->dev;
	u16				result = 0;
	u32				reg, oldreg;
	enum iavf_status		ret;
	bool				aq_error = false;

	IAVF_CORE_LOCK_ASSERT(sc);

	event.buf_len = IAVF_AQ_BUF_SZ;
        event.msg_buf = sc->aq_buffer;
	v_msg = (struct virtchnl_msg *)&event.desc;

	do {
		ret = iavf_clean_arq_element(hw, &event, &result);
		if (ret)
			break;
		iavf_vc_completion(sc, v_msg->v_opcode,
		    v_msg->v_retval, event.msg_buf, event.msg_len);
		if (result != 0)
			bzero(event.msg_buf, IAVF_AQ_BUF_SZ);
	} while (result);

	/* check for Admin queue errors */
	oldreg = reg = rd32(hw, hw->aq.arq.len);
	if (reg & IAVF_VF_ARQLEN1_ARQVFE_MASK) {
		device_printf(dev, "ARQ VF Error detected\n");
		reg &= ~IAVF_VF_ARQLEN1_ARQVFE_MASK;
		aq_error = true;
	}
	if (reg & IAVF_VF_ARQLEN1_ARQOVFL_MASK) {
		device_printf(dev, "ARQ Overflow Error detected\n");
		reg &= ~IAVF_VF_ARQLEN1_ARQOVFL_MASK;
		aq_error = true;
	}
	if (reg & IAVF_VF_ARQLEN1_ARQCRIT_MASK) {
		device_printf(dev, "ARQ Critical Error detected\n");
		reg &= ~IAVF_VF_ARQLEN1_ARQCRIT_MASK;
		aq_error = true;
	}
	if (oldreg != reg)
		wr32(hw, hw->aq.arq.len, reg);

	oldreg = reg = rd32(hw, hw->aq.asq.len);
	if (reg & IAVF_VF_ATQLEN1_ATQVFE_MASK) {
		device_printf(dev, "ASQ VF Error detected\n");
		reg &= ~IAVF_VF_ATQLEN1_ATQVFE_MASK;
		aq_error = true;
	}
	if (reg & IAVF_VF_ATQLEN1_ATQOVFL_MASK) {
		device_printf(dev, "ASQ Overflow Error detected\n");
		reg &= ~IAVF_VF_ATQLEN1_ATQOVFL_MASK;
		aq_error = true;
	}
	if (reg & IAVF_VF_ATQLEN1_ATQCRIT_MASK) {
		device_printf(dev, "ASQ Critical Error detected\n");
		reg &= ~IAVF_VF_ATQLEN1_ATQCRIT_MASK;
		aq_error = true;
	}
	if (oldreg != reg)
		wr32(hw, hw->aq.asq.len, reg);

	if (aq_error) {
		/* Need to reset adapter */
		device_printf(dev, "WARNING: Resetting!\n");
		iavf_set_state(&sc->state, IAVF_STATE_RESET_REQUIRED);
		iavf_init_locked(sc);
	}
	iavf_enable_adminq_irq(hw);
}

/**
 * iavf_add_device_sysctls - Add sysctls to configure the device
 * @sc: device softc
 *
 * Add sysctls for reporting device status and configuring device properties.
 */
static void
iavf_add_device_sysctls(struct iavf_sc *sc)
{
	device_t dev = sc->dev;
	struct iavf_vsi *vsi = &sc->vsi;

	struct sysctl_ctx_list *ctx = device_get_sysctl_ctx(dev);
	struct sysctl_oid_list *ctx_list =
	    SYSCTL_CHILDREN(device_get_sysctl_tree(dev));
	struct sysctl_oid_list *debug_list;

	iavf_add_device_sysctls_common(sc);

	SYSCTL_ADD_UQUAD(ctx, ctx_list, OID_AUTO, "watchdog_events",
			CTLFLAG_RD, &sc->watchdog_events,
			"Watchdog timeouts");

	SYSCTL_ADD_INT(ctx, ctx_list, OID_AUTO, "tx_ring_size",
			CTLFLAG_RD, &vsi->num_tx_desc, 0,
			"TX ring size");
	SYSCTL_ADD_INT(ctx, ctx_list, OID_AUTO, "rx_ring_size",
			CTLFLAG_RD, &vsi->num_rx_desc, 0,
			"RX ring size");

	debug_list = iavf_create_debug_sysctl_tree(sc);

	iavf_add_debug_sysctls_common(sc, debug_list);

	/* VSI statistics sysctls */
	iavf_add_vsi_sysctls(dev, vsi, ctx, "vsi");

	iavf_vsi_add_queues_stats(vsi);
}

/**
 * iavf_vsi_setup_rings_size - Adjust the Tx and Rx ring sizes
 * @vsi: main VSI
 * @tx_ring_size: requested Tx ring size
 * @rx_ring_size: requested Rx ring size
 *
 * Set TX and RX ring size adjusting value to supported range
 */
static void
iavf_vsi_setup_rings_size(struct iavf_vsi *vsi, int tx_ring_size, int rx_ring_size)
{
	device_t dev = vsi->dev;

	if (tx_ring_size < IAVF_MIN_RING
	     || tx_ring_size > IAVF_MAX_RING
	     || tx_ring_size % IAVF_RING_INCREMENT != 0) {
		device_printf(dev, "Invalid tx_ring_size value of %d set!\n",
		    tx_ring_size);
		device_printf(dev, "tx_ring_size must be between %d and %d, "
		    "inclusive, and must be a multiple of %d\n",
		    IAVF_MIN_RING, IAVF_MAX_RING, IAVF_RING_INCREMENT);
		device_printf(dev, "Using default value of %d instead\n",
		    IAVF_DEFAULT_RING);
		vsi->num_tx_desc = IAVF_DEFAULT_RING;
	} else
		vsi->num_tx_desc = tx_ring_size;

	if (rx_ring_size < IAVF_MIN_RING
	     || rx_ring_size > IAVF_MAX_RING
	     || rx_ring_size % IAVF_RING_INCREMENT != 0) {
		device_printf(dev, "Invalid rx_ring_size value of %d set!\n",
		    rx_ring_size);
		device_printf(dev, "rx_ring_size must be between %d and %d, "
		    "inclusive, and must be a multiple of %d\n",
		    IAVF_MIN_RING, IAVF_MAX_RING, IAVF_RING_INCREMENT);
		device_printf(dev, "Using default value of %d instead\n",
		    IAVF_DEFAULT_RING);
		vsi->num_rx_desc = IAVF_DEFAULT_RING;
	} else
		vsi->num_rx_desc = rx_ring_size;

	device_printf(dev, "Using %d tx descriptors and %d rx descriptors\n",
		vsi->num_tx_desc, vsi->num_rx_desc);
}

/**
 * iavf_vsi_add_queues_stats - Add statistics sysctls for each queue
 * @vsi: main VSI
 *
 * Create sysctls to report the per-queue statistics captured by the device.
 */
static void
iavf_vsi_add_queues_stats(struct iavf_vsi * vsi)
{
	char queue_namebuf[IAVF_QUEUE_NAME_LEN];
	struct sysctl_oid_list	*vsi_list, *queue_list;
	struct iavf_queue	*queues = vsi->queues;
	struct sysctl_oid	*queue_node;
	struct sysctl_ctx_list	*ctx;
	struct tx_ring		*txr;
	struct rx_ring		*rxr;

	vsi_list = SYSCTL_CHILDREN(vsi->vsi_node);
	ctx = &vsi->sysctl_ctx;

	/* Queue statistics */
	for (int q = 0; q < vsi->num_queues; q++) {
		snprintf(queue_namebuf, IAVF_QUEUE_NAME_LEN, "que%d", q);
		queue_node = SYSCTL_ADD_NODE(ctx, vsi_list,
		    OID_AUTO, queue_namebuf, CTLFLAG_RD, NULL, "Queue #");
		queue_list = SYSCTL_CHILDREN(queue_node);

		txr = &(queues[q].txr);
		rxr = &(queues[q].rxr);

		SYSCTL_ADD_UQUAD(ctx, queue_list, OID_AUTO, "mbuf_defrag_failed",
				CTLFLAG_RD, &(queues[q].mbuf_defrag_failed),
				"m_defrag() failed");
		SYSCTL_ADD_UQUAD(ctx, queue_list, OID_AUTO, "irqs",
				CTLFLAG_RD, &(queues[q].irqs),
				"irqs on this queue");
		SYSCTL_ADD_UQUAD(ctx, queue_list, OID_AUTO, "tso_tx",
				CTLFLAG_RD, &(queues[q].tso),
				"TSO");
		SYSCTL_ADD_UQUAD(ctx, queue_list, OID_AUTO, "tx_dmamap_failed",
				CTLFLAG_RD, &(queues[q].tx_dmamap_failed),
				"Driver tx dma failure in xmit");
		SYSCTL_ADD_UQUAD(ctx, queue_list, OID_AUTO, "mss_too_small",
				CTLFLAG_RD, &(queues[q].mss_too_small),
				"TSO sends with an MSS less than 64");
		SYSCTL_ADD_UQUAD(ctx, queue_list, OID_AUTO, "no_desc_avail",
				CTLFLAG_RD, &(txr->no_desc),
				"Queue No Descriptor Available");
		SYSCTL_ADD_UQUAD(ctx, queue_list, OID_AUTO, "tx_packets",
				CTLFLAG_RD, &(txr->total_packets),
				"Queue Packets Transmitted");
		SYSCTL_ADD_UQUAD(ctx, queue_list, OID_AUTO, "tx_bytes",
				CTLFLAG_RD, &(txr->tx_bytes),
				"Queue Bytes Transmitted");
		SYSCTL_ADD_UQUAD(ctx, queue_list, OID_AUTO, "rx_packets",
				CTLFLAG_RD, &(rxr->rx_packets),
				"Queue Packets Received");
		SYSCTL_ADD_UQUAD(ctx, queue_list, OID_AUTO, "rx_bytes",
				CTLFLAG_RD, &(rxr->rx_bytes),
				"Queue Bytes Received");
		SYSCTL_ADD_UQUAD(ctx, queue_list, OID_AUTO, "rx_desc_err",
				CTLFLAG_RD, &(rxr->desc_errs),
				"Queue Rx Descriptor Errors");
		SYSCTL_ADD_UINT(ctx, queue_list, OID_AUTO, "rx_itr",
				CTLFLAG_RD, &(rxr->itr), 0,
				"Queue Rx ITR Interval");
		SYSCTL_ADD_UINT(ctx, queue_list, OID_AUTO, "tx_itr",
				CTLFLAG_RD, &(txr->itr), 0,
				"Queue Tx ITR Interval");
#ifdef IAVF_DEBUG
		SYSCTL_ADD_INT(ctx, queue_list, OID_AUTO, "txr_watchdog",
				CTLFLAG_RD, &(txr->watchdog_timer), 0,
				"Ticks before watchdog timer causes interface reinit");
		SYSCTL_ADD_U16(ctx, queue_list, OID_AUTO, "tx_next_avail",
				CTLFLAG_RD, &(txr->next_avail), 0,
				"Next TX descriptor to be used");
		SYSCTL_ADD_U16(ctx, queue_list, OID_AUTO, "tx_next_to_clean",
				CTLFLAG_RD, &(txr->next_to_clean), 0,
				"Next TX descriptor to be cleaned");
		SYSCTL_ADD_UQUAD(ctx, queue_list, OID_AUTO, "rx_not_done",
				CTLFLAG_RD, &(rxr->not_done),
				"Queue Rx Descriptors not Done");
		SYSCTL_ADD_UINT(ctx, queue_list, OID_AUTO, "rx_next_refresh",
				CTLFLAG_RD, &(rxr->next_refresh), 0,
				"Queue Rx Descriptors not Done");
		SYSCTL_ADD_UINT(ctx, queue_list, OID_AUTO, "rx_next_check",
				CTLFLAG_RD, &(rxr->next_check), 0,
				"Queue Rx Descriptors not Done");
#endif
	}

}

/**
 * iavf_driver_is_detaching - Check if the driver is detaching/unloading
 * @sc: device private softc
 *
 * @returns true if the driver is detaching, false otherwise.
 */
bool
iavf_driver_is_detaching(struct iavf_sc *sc)
{
	return !iavf_test_state(&sc->state, IAVF_STATE_INITIALIZED);
}

/**
 * iavf_get_counter - Get ifnet counter values
 * @ifp: ifnet structure
 * @cnt: the ifnet counter to read
 *
 * @returns the uint64_t value of the requested ifnet counter.
 */
uint64_t
iavf_get_counter(if_t ifp, ift_counter cnt)
{
	struct iavf_vsi *vsi;

	vsi = (struct iavf_vsi *)if_getsoftc(ifp);

	switch (cnt) {
	case IFCOUNTER_IPACKETS:
		return (vsi->ipackets);
	case IFCOUNTER_IERRORS:
		return (vsi->ierrors);
	case IFCOUNTER_OPACKETS:
		return (vsi->opackets);
	case IFCOUNTER_OERRORS:
		return (vsi->oerrors);
	case IFCOUNTER_COLLISIONS:
		/* Collisions are by standard impossible in 40G/10G Ethernet */
		return (0);
	case IFCOUNTER_IBYTES:
		return (vsi->ibytes);
	case IFCOUNTER_OBYTES:
		return (vsi->obytes);
	case IFCOUNTER_IMCASTS:
		return (vsi->imcasts);
	case IFCOUNTER_OMCASTS:
		return (vsi->omcasts);
	case IFCOUNTER_IQDROPS:
		return (vsi->iqdrops);
	case IFCOUNTER_OQDROPS:
		return (vsi->oqdrops);
	case IFCOUNTER_NOPROTO:
		return (vsi->noproto);
	default:
		return (if_get_counter_default(ifp, cnt));
	}
}

/**
 * iavf_send_vc_msg_sleep - Send a virtchnl message and wait for a response
 * @sc: device softc
 * @op: the virtchnl op to send
 *
 * Send a virtchnl message to the PF, and sleep waiting for a response.
 *
 * @returns zero on success, or an error code on failure.
 */
int
iavf_send_vc_msg_sleep(struct iavf_sc *sc, u32 op)
{
	device_t dev = sc->dev;
	int error = 0;

	error = iavf_vc_send_cmd(sc, op);
	if (error != 0) {
		iavf_dbg_vc(sc, "Error sending %b: %d\n", op, IAVF_FLAGS, error);
		return (error);
	}

	iavf_dbg_vc(sc, "Sleeping for op %b\n", op, IAVF_FLAGS);
	error = mtx_sleep(iavf_vc_get_op_chan(sc, op),
	    &sc->mtx, PRI_MAX, "iavf_vc", IAVF_AQ_TIMEOUT);

	if (error == EWOULDBLOCK)
		device_printf(dev, "%b timed out\n", op, IAVF_FLAGS);

	return (error);
}

/**
 * iavf_send_vc_msg - Send a virtchnl message to the PF
 * @sc: device softc
 * @op: the virtchnl op to send
 *
 * Send a virtchnl message to the PF. Do not wait for a response.
 *
 * @returns zero on success, or an error code on failure.
 */
int
iavf_send_vc_msg(struct iavf_sc *sc, u32 op)
{
	int error = 0;

	error = iavf_vc_send_cmd(sc, op);
	if (error != 0)
		iavf_dbg_vc(sc, "Error sending %b: %d\n", op, IAVF_FLAGS, error);

	return (error);
}
