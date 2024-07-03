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
 * @file freebsd_compat_legacy.c
 * @brief FreeBSD kernel function backports
 *
 * Contains backports for functions defined in freebsd_compat_legacy.h, used
 * to aid in developing a legacy network driver that is compatible with
 * a variety of FreeBSD kernel versions.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include "freebsd_compat_legacy.h"

#if __FreeBSD_version < 1100084
/**
 * ffsll - Find first set bit in a long long value
 * @mask: the value to scan
 *
 * Returns the index of the first bit that is set in the long long. Note, this
 * function uses a 1 as the index of the first bit. 0 indicates that no bits
 * are set.
 */
int
ffsll(long long mask)
{
	int bit;

	if (mask == 0)
		return (0);
	for (bit = 1; !(mask & 1); bit++)
		mask = (unsigned long long)mask >> 1;
	return (bit);
}
#endif

#if __FreeBSD_version < 1100110
/**
 * _kc_sysctl_handle_bool - handle a boolean sysctl value
 * @oidp: sysctl oid structure
 * @arg1: pointer to memory, if not constant
 * @arg2: constant value used if arg1 is NULL
 * @req: sysctl request pointer
 *
 * Read a boolean value from a sysctl, and possibly update it. Similar to
 * sysctl_handle_8, except guarantees to normalize the value to 0 or 1.
 *
 * Note sysctl_handle_bool released with FreeBSD 11.0
 */
int
_kc_sysctl_handle_bool(SYSCTL_HANDLER_ARGS)
{
	uint8_t temp;
	int error;

	/*
	 * Attempt to get a coherent snapshot by making a copy of the data.
	 */
	if (arg1)
		temp = *(bool *)arg1 ? 1 : 0;
	else
		temp = arg2 ? 1 : 0;

	error = SYSCTL_OUT(req, &temp, sizeof(temp));
	if (error || !req->newptr)
		return (error);

	if (!arg1)
		error = EPERM;
	else {
		error = SYSCTL_IN(req, &temp, sizeof(temp));
		if (!error)
			*(bool *)arg1 = temp ? 1 : 0;
	}
	return (error);
}
#endif /* FreeBSD_version < 11 */

#if __FreeBSD_version < 1101515
/**
 * ifr_data_get_ptr - retrieve data pointer from ifreq pointer
 * @ifrp: pointer to struct ifreq to retrieve data from
 *
 * Originally added in r331797, this accessor function is intended
 * to provide compatibilty with ifconfig on 32-bit systems. For
 * our purposes, we define this function to just use the method
 * pre-11.2 kernels did to get the data from the ifreq pointer.
 */
void *
ifr_data_get_ptr(void *ifrp)
{
	struct ifreq *ifr = (struct ifreq *)ifrp;

	return (ifr->ifr_ifru.ifru_data);
}
#endif
