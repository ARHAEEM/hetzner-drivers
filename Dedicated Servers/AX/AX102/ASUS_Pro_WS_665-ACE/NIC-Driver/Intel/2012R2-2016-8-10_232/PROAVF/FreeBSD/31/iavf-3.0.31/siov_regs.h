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
#ifndef _SIOV_REGS_H_
#define _SIOV_REGS_H_
#define VDEV_MBX_START			0x20000 /* Begin at 128KB */
#define VDEV_MBX_ATQBAL			(VDEV_MBX_START + 0x0000)
#define VDEV_MBX_ATQBAH			(VDEV_MBX_START + 0x0004)
#define VDEV_MBX_ATQLEN			(VDEV_MBX_START + 0x0008)
#define VDEV_MBX_ATQH			(VDEV_MBX_START + 0x000C)
#define VDEV_MBX_ATQT			(VDEV_MBX_START + 0x0010)
#define VDEV_MBX_ARQBAL			(VDEV_MBX_START + 0x0014)
#define VDEV_MBX_ARQBAH			(VDEV_MBX_START + 0x0018)
#define VDEV_MBX_ARQLEN			(VDEV_MBX_START + 0x001C)
#define VDEV_MBX_ARQH			(VDEV_MBX_START + 0x0020)
#define VDEV_MBX_ARQT			(VDEV_MBX_START + 0x0024)
#define VDEV_GET_RSTAT			0x21000 /* 132KB for RSTAT */

/* Begin at offset after 1MB (after 256 4k pages) */
#define VDEV_QRX_TAIL_START		0x100000
#define VDEV_QRX_TAIL(_i)		(VDEV_QRX_TAIL_START + ((_i) * 0x1000)) /* 2k Rx queues */

#define VDEV_QRX_BUFQ_TAIL_START	0x900000 /* Begin at offset of 9MB for  Rx buffer queue tail register pages */
#define VDEV_QRX_BUFQ_TAIL(_i)		(VDEV_QRX_BUFQ_TAIL_START + ((_i) * 0x1000)) /* 2k Rx buffer queues */

#define VDEV_QTX_TAIL_START		0x1100000 /* Begin at offset of 17MB for 2k Tx queues */
#define VDEV_QTX_TAIL(_i)		(VDEV_QTX_TAIL_START + ((_i) * 0x1000)) /* 2k Tx queues */

#define VDEV_QTX_COMPL_TAIL_START	0x1900000 /* Begin at offset of 25MB for 2k Tx completion queues */
#define VDEV_QTX_COMPL_TAIL(_i)		(VDEV_QTX_COMPL_TAIL_START + ((_i) * 0x1000)) /* 2k Tx completion queues */

#define VDEV_INT_DYN_CTL01		0x2100000 /* Begin at offset 33MB */

#define VDEV_INT_DYN_START		(VDEV_INT_DYN_CTL01 + 0x1000) /* Begin at offset of 33MB + 4k to accomdate CTL01 register */
#define VDEV_INT_DYN_CTL(_i)		(VDEV_INT_DYN_START + ((_i) * 0x1000))
#define VDEV_INT_ITR_0(_i)		(VDEV_INT_DYN_START + ((_i) * 0x1000) + 0x04)
#define VDEV_INT_ITR_1(_i)		(VDEV_INT_DYN_START + ((_i) * 0x1000) + 0x08)
#define VDEV_INT_ITR_2(_i)		(VDEV_INT_DYN_START + ((_i) * 0x1000) + 0x0C)

/* Next offset to begin at 42MB (0x2A00000) */
#endif /* _SIOV_REGS_H_ */
