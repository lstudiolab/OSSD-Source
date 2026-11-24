/*
 * Copyright (c) 2012 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <libkern/crypto/crypto_internal.h>
#include <libkern/libkern.h>
#include <libkern/crypto/aesxts.h>
#include <corecrypto/ccmode.h>
#include <corecrypto/ccpad.h>
#include <kern/debug.h>

  
                                                        
   

uint32_t
xts_start(uint32_t cipher __unused,                                               
    const uint8_t *IV __unused,                         
    const uint8_t *key1, int keylen,
    const uint8_t *key2, int tweaklen __unused,                                                     
    uint32_t num_rounds __unused,                         
    uint32_t options __unused,                            
    symmetric_xts *xts)
{
	const struct ccmode_xts *enc, *dec;

	if (!g_crypto_funcs) {
		panic("%s: corecrypto not registered!\n", __FUNCTION__);
	}

	enc = g_crypto_funcs->ccaes_xts_encrypt;
	dec = g_crypto_funcs->ccaes_xts_decrypt;

	if (!enc || !dec) {
		panic("%s: xts mode not registered? enc=%p, dec=%p\n", __FUNCTION__, enc, dec);
	}

	                                                                     
	if ((enc->size > sizeof(xts->enc)) || (dec->size > sizeof(xts->dec))) {
		panic("%s: inconsistent size for AES-XTS context", __FUNCTION__);
	}

	int rc = enc->init(enc, xts->enc, keylen, key1, key2);
	rc |= dec->init(dec, xts->dec, keylen, key1, key2);

	return rc;
}

int
xts_encrypt(const uint8_t *pt, unsigned long ptlen,
    uint8_t *ct,
    const uint8_t *iv,                                                                         
    symmetric_xts *xts)
{
	const struct ccmode_xts *xtsenc = g_crypto_funcs->ccaes_xts_encrypt;
	ccxts_tweak_decl(xtsenc->tweak_size, tweak);

	if (ptlen % 16) {
		panic("xts encrypt not a multiple of block size\n");
	}

	int rc = xtsenc->set_tweak(xts->enc, tweak, iv);
	if (rc) {
		return rc;
	}

	xtsenc->xts(xts->enc, tweak, ptlen / 16, pt, ct);
	return 0;
}

int
xts_decrypt(const uint8_t *ct, unsigned long ptlen,
    uint8_t *pt,
    const uint8_t *iv,                                                                                 
    symmetric_xts *xts)
{
	const struct ccmode_xts *xtsdec = g_crypto_funcs->ccaes_xts_decrypt;
	ccxts_tweak_decl(xtsdec->tweak_size, tweak);

	if (ptlen % 16) {
		panic("xts decrypt not a multiple of block size\n");
	}

	int rc = xtsdec->set_tweak(xts->dec, tweak, iv);
	if (rc) {
		return rc;
	}

	xtsdec->xts(xts->dec, tweak, ptlen / 16, ct, pt);
	return 0;
}

void
xts_done(symmetric_xts *xts __unused)
{
	cc_clear(sizeof(xts->enc), xts->enc);
	cc_clear(sizeof(xts->dec), xts->dec);
}
