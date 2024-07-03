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
#ifndef _IAVF_REGISTER_H_
#define _IAVF_REGISTER_H_

#define IAVF_VF_ARQBAH1              0x00006000 /* Reset: EMPR */
#define IAVF_VF_ARQBAL1              0x00006C00 /* Reset: EMPR */
#define IAVF_VF_ARQH1            0x00007400 /* Reset: EMPR */
#define IAVF_VF_ARQH1_ARQH_SHIFT 0
#define IAVF_VF_ARQH1_ARQH_MASK  IAVF_MASK(0x3FF, IAVF_VF_ARQH1_ARQH_SHIFT)
#define IAVF_VF_ARQLEN1                 0x00008000 /* Reset: EMPR */
#define IAVF_VF_ARQLEN1_ARQVFE_SHIFT    28
#define IAVF_VF_ARQLEN1_ARQVFE_MASK     IAVF_MASK(1UL, IAVF_VF_ARQLEN1_ARQVFE_SHIFT)
#define IAVF_VF_ARQLEN1_ARQOVFL_SHIFT   29
#define IAVF_VF_ARQLEN1_ARQOVFL_MASK    IAVF_MASK(1UL, IAVF_VF_ARQLEN1_ARQOVFL_SHIFT)
#define IAVF_VF_ARQLEN1_ARQCRIT_SHIFT   30
#define IAVF_VF_ARQLEN1_ARQCRIT_MASK    IAVF_MASK(1UL, IAVF_VF_ARQLEN1_ARQCRIT_SHIFT)
#define IAVF_VF_ARQLEN1_ARQENABLE_SHIFT 31
#define IAVF_VF_ARQLEN1_ARQENABLE_MASK  IAVF_MASK(1UL, IAVF_VF_ARQLEN1_ARQENABLE_SHIFT)
#define IAVF_VF_ARQT1            0x00007000 /* Reset: EMPR */
#define IAVF_VF_ATQBAH1              0x00007800 /* Reset: EMPR */
#define IAVF_VF_ATQBAL1              0x00007C00 /* Reset: EMPR */
#define IAVF_VF_ATQH1            0x00006400 /* Reset: EMPR */
#define IAVF_VF_ATQLEN1                 0x00006800 /* Reset: EMPR */
#define IAVF_VF_ATQLEN1_ATQVFE_SHIFT    28
#define IAVF_VF_ATQLEN1_ATQVFE_MASK     IAVF_MASK(1UL, IAVF_VF_ATQLEN1_ATQVFE_SHIFT)
#define IAVF_VF_ATQLEN1_ATQOVFL_SHIFT   29
#define IAVF_VF_ATQLEN1_ATQOVFL_MASK    IAVF_MASK(1UL, IAVF_VF_ATQLEN1_ATQOVFL_SHIFT)
#define IAVF_VF_ATQLEN1_ATQCRIT_SHIFT   30
#define IAVF_VF_ATQLEN1_ATQCRIT_MASK    IAVF_MASK(1UL, IAVF_VF_ATQLEN1_ATQCRIT_SHIFT)
#define IAVF_VF_ATQLEN1_ATQENABLE_SHIFT 31
#define IAVF_VF_ATQLEN1_ATQENABLE_MASK  IAVF_MASK(1UL, IAVF_VF_ATQLEN1_ATQENABLE_SHIFT)
#define IAVF_VF_ATQT1            0x00008400 /* Reset: EMPR */
#define IAVF_VFGEN_RSTAT                 0x00008800 /* Reset: VFR */
#define IAVF_VFGEN_RSTAT_VFR_STATE_SHIFT 0
#define IAVF_VFGEN_RSTAT_VFR_STATE_MASK  IAVF_MASK(0x3, IAVF_VFGEN_RSTAT_VFR_STATE_SHIFT)
#define IAVF_VFINT_DYN_CTL01                       0x00005C00 /* Reset: VFR */
#define IAVF_VFINT_DYN_CTL01_INTENA_SHIFT          0
#define IAVF_VFINT_DYN_CTL01_INTENA_MASK           IAVF_MASK(1UL, IAVF_VFINT_DYN_CTL01_INTENA_SHIFT)
#define IAVF_VFINT_DYN_CTL01_CLEARPBA_SHIFT        1
#define IAVF_VFINT_DYN_CTL01_CLEARPBA_MASK         IAVF_MASK(1UL, IAVF_VFINT_DYN_CTL01_CLEARPBA_SHIFT)
#define IAVF_VFINT_DYN_CTL01_SWINT_TRIG_SHIFT      2
#define IAVF_VFINT_DYN_CTL01_SWINT_TRIG_MASK       IAVF_MASK(1UL, IAVF_VFINT_DYN_CTL01_SWINT_TRIG_SHIFT)
#define IAVF_VFINT_DYN_CTL01_ITR_INDX_SHIFT        3
#define IAVF_VFINT_DYN_CTL01_ITR_INDX_MASK         IAVF_MASK(0x3, IAVF_VFINT_DYN_CTL01_ITR_INDX_SHIFT)
#define IAVF_VFINT_DYN_CTL01_INTERVAL_SHIFT        5
#define IAVF_VFINT_DYN_CTL01_INTERVAL_MASK         IAVF_MASK(0xFFF, IAVF_VFINT_DYN_CTL01_INTERVAL_SHIFT)
#define IAVF_VFINT_DYN_CTL01_SW_ITR_INDX_ENA_SHIFT 24
#define IAVF_VFINT_DYN_CTL01_SW_ITR_INDX_ENA_MASK  IAVF_MASK(1UL, IAVF_VFINT_DYN_CTL01_SW_ITR_INDX_ENA_SHIFT)
#define IAVF_VFINT_DYN_CTL01_SW_ITR_INDX_SHIFT     25
#define IAVF_VFINT_DYN_CTL01_SW_ITR_INDX_MASK      IAVF_MASK(0x3, IAVF_VFINT_DYN_CTL01_SW_ITR_INDX_SHIFT)
#define IAVF_VFINT_DYN_CTLN1(_INTVF)               (0x00003800 + ((_INTVF) * 4)) /* _i=0...15 */ /* Reset: VFR */
#define IAVF_VFINT_DYN_CTLN1_INTENA_SHIFT          0
#define IAVF_VFINT_DYN_CTLN1_INTENA_MASK           IAVF_MASK(1UL, IAVF_VFINT_DYN_CTLN1_INTENA_SHIFT)
#define IAVF_VFINT_DYN_CTLN1_CLEARPBA_SHIFT        1
#define IAVF_VFINT_DYN_CTLN1_CLEARPBA_MASK         IAVF_MASK(1UL, IAVF_VFINT_DYN_CTLN1_CLEARPBA_SHIFT)
#define IAVF_VFINT_DYN_CTLN1_SWINT_TRIG_SHIFT      2
#define IAVF_VFINT_DYN_CTLN1_SWINT_TRIG_MASK       IAVF_MASK(1UL, IAVF_VFINT_DYN_CTLN1_SWINT_TRIG_SHIFT)
#define IAVF_VFINT_DYN_CTLN1_ITR_INDX_SHIFT        3
#define IAVF_VFINT_DYN_CTLN1_ITR_INDX_MASK         IAVF_MASK(0x3, IAVF_VFINT_DYN_CTLN1_ITR_INDX_SHIFT)
#define IAVF_VFINT_DYN_CTLN1_INTERVAL_SHIFT        5
#define IAVF_VFINT_DYN_CTLN1_INTERVAL_MASK         IAVF_MASK(0xFFF, IAVF_VFINT_DYN_CTLN1_INTERVAL_SHIFT)
#define IAVF_VFINT_DYN_CTLN1_SW_ITR_INDX_ENA_SHIFT 24
#define IAVF_VFINT_DYN_CTLN1_SW_ITR_INDX_ENA_MASK  IAVF_MASK(1UL, IAVF_VFINT_DYN_CTLN1_SW_ITR_INDX_ENA_SHIFT)
#define IAVF_VFINT_DYN_CTLN1_SW_ITR_INDX_SHIFT     25
#define IAVF_VFINT_DYN_CTLN1_SW_ITR_INDX_MASK      IAVF_MASK(0x3, IAVF_VFINT_DYN_CTLN1_SW_ITR_INDX_SHIFT)
#define IAVF_VFINT_ICR0_ENA1                        0x00005000 /* Reset: CORER */
#define IAVF_VFINT_ICR0_ENA1_ADMINQ_SHIFT           30
#define IAVF_VFINT_ICR0_ENA1_ADMINQ_MASK            IAVF_MASK(1UL, IAVF_VFINT_ICR0_ENA1_ADMINQ_SHIFT)
#define IAVF_VFINT_ICR0_ENA1_RSVD_SHIFT             31
#define IAVF_VFINT_ICR01                        0x00004800 /* Reset: CORER */
#define IAVF_VFINT_ICR01_QUEUE_0_SHIFT          1
#define IAVF_VFINT_ICR01_QUEUE_0_MASK           IAVF_MASK(1UL, IAVF_VFINT_ICR01_QUEUE_0_SHIFT)
#define IAVF_VFINT_ICR01_LINK_STAT_CHANGE_SHIFT 25
#define IAVF_VFINT_ICR01_LINK_STAT_CHANGE_MASK  IAVF_MASK(1UL, IAVF_VFINT_ICR01_LINK_STAT_CHANGE_SHIFT)
#define IAVF_VFINT_ICR01_ADMINQ_SHIFT           30
#define IAVF_VFINT_ICR01_ADMINQ_MASK            IAVF_MASK(1UL, IAVF_VFINT_ICR01_ADMINQ_SHIFT)
#define IAVF_VFINT_ITR01(_i)            (0x00004C00 + ((_i) * 4)) /* _i=0...2 */ /* Reset: VFR */
#define IAVF_VFINT_ITRN1(_i, _INTVF)     (0x00002800 + ((_i) * 64 + (_INTVF) * 4)) /* _i=0...2, _INTVF=0...15 */ /* Reset: VFR */
#define IAVF_VFINT_STAT_CTL01                      0x00005400 /* Reset: CORER */
#define IAVF_QRX_TAIL1(_Q)        (0x00002000 + ((_Q) * 4)) /* _i=0...15 */ /* Reset: CORER */
#define IAVF_QTX_TAIL1(_Q)        (0x00000000 + ((_Q) * 4)) /* _i=0...15 */ /* Reset: PFR */
#define IAVF_VFQF_HENA(_i)             (0x0000C400 + ((_i) * 4)) /* _i=0...1 */ /* Reset: CORER */
#define IAVF_VFQF_HKEY(_i)         (0x0000CC00 + ((_i) * 4)) /* _i=0...12 */ /* Reset: CORER */
#define IAVF_VFQF_HKEY_MAX_INDEX   12
#define IAVF_VFQF_HLUT(_i)        (0x0000D000 + ((_i) * 4)) /* _i=0...15 */ /* Reset: CORER */
#define IAVF_VFQF_HLUT_MAX_INDEX  15
#define IAVF_VFINT_DYN_CTLN1_WB_ON_ITR_SHIFT       30
#define IAVF_VFINT_DYN_CTLN1_WB_ON_ITR_MASK        IAVF_MASK(1UL, IAVF_VFINT_DYN_CTLN1_WB_ON_ITR_SHIFT)

#define INT_DYN_CTL0(hw)					\
	((hw)->device_id == IAVF_DEV_ID_VDEV ?	 		\
	VDEV_INT_DYN_CTL01 : IAVF_VFINT_DYN_CTL01)
#define INT_DYN_CTL(hw, INTVF)					\
	((hw)->device_id == IAVF_DEV_ID_VDEV ?			\
	VDEV_INT_DYN_CTL(INTVF) : IAVF_VFINT_DYN_CTLN1(INTVF))
#define INT_ITRN1(hw, _i, _INTVF)				\
	((hw)->device_id == IAVF_DEV_ID_VDEV ?			\
	((_i == IAVF_RX_ITR) ? VDEV_INT_ITR_0(_INTVF) : VDEV_INT_ITR_1(_INTVF)) : IAVF_VFINT_ITRN1(_i, _INTVF))
#define MBX_ATQT(hw)	((hw)->device_id == IAVF_DEV_ID_VDEV ? VDEV_MBX_ATQT : IAVF_VF_ATQT1)
#define MBX_ATQH(hw)	((hw)->device_id == IAVF_DEV_ID_VDEV ? VDEV_MBX_ATQH : IAVF_VF_ATQH1)
#define MBX_ATQLEN(hw)	((hw)->device_id == IAVF_DEV_ID_VDEV ? VDEV_MBX_ATQLEN : IAVF_VF_ATQLEN1)
#define MBX_ATQBAL(hw)	((hw)->device_id == IAVF_DEV_ID_VDEV ? VDEV_MBX_ATQBAL : IAVF_VF_ATQBAL1)
#define MBX_ATQBAH(hw)	((hw)->device_id == IAVF_DEV_ID_VDEV ? VDEV_MBX_ATQBAH : IAVF_VF_ATQBAH1)
#define MBX_ARQT(hw)	((hw)->device_id == IAVF_DEV_ID_VDEV ? VDEV_MBX_ARQT : IAVF_VF_ARQT1)
#define MBX_ARQH(hw)	((hw)->device_id == IAVF_DEV_ID_VDEV ? VDEV_MBX_ARQH : IAVF_VF_ARQH1)
#define MBX_ARQLEN(hw)	((hw)->device_id == IAVF_DEV_ID_VDEV ? VDEV_MBX_ARQLEN : IAVF_VF_ARQLEN1)
#define MBX_ARQBAL(hw)	((hw)->device_id == IAVF_DEV_ID_VDEV ? VDEV_MBX_ARQBAL : IAVF_VF_ARQBAL1)
#define MBX_ARQBAH(hw)	((hw)->device_id == IAVF_DEV_ID_VDEV ? VDEV_MBX_ARQBAH : IAVF_VF_ARQBAH1)
#define QTX_TAIL(hw, _Q)	((hw)->device_id == IAVF_DEV_ID_VDEV ? VDEV_QTX_TAIL(_Q) : IAVF_QTX_TAIL1(_Q))
#define QRX_TAIL(hw, _Q)	((hw)->device_id == IAVF_DEV_ID_VDEV ? VDEV_QRX_TAIL(_Q) : IAVF_QRX_TAIL1(_Q))
#endif /* _IAVF_REGISTER_H_ */
