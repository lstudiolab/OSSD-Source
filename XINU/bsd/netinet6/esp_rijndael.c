/*
 * Copyright (c) 2008-2016 Apple Inc. All rights reserved.
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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

                                                                                       
                                                                  

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/syslog.h>
#include <sys/mbuf.h>
#include <sys/mcache.h>

#include <kern/locks.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet6/ipsec.h>
#include <netinet6/esp.h>
#include <netinet6/esp_rijndael.h>

#include <libkern/crypto/aes.h>

#include <netkey/key.h>

#include <net/net_osdep.h>

#define MAX_REALIGN_LEN 2000
#define AES_BLOCKLEN 16
#define ESP_GCM_SALT_LEN 4                        
#define ESP_GCM_IVLEN 8
#define ESP_GCM_ALIGN 16

extern lck_mtx_t *sadb_mutex;

typedef struct {
	ccgcm_ctx *decrypt;
	ccgcm_ctx *encrypt;
	ccgcm_ctx ctxt[0];
} aes_gcm_ctx;

size_t
esp_aes_schedlen(
	__unused const struct esp_algorithm *algo)
{
	return sizeof(aes_ctx);
}

int
esp_aes_schedule(
	__unused const struct esp_algorithm *algo,
	struct secasvar *sav)
{
	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);
	aes_ctx *ctx = (aes_ctx*)sav->sched;

	aes_decrypt_key((const unsigned char *) _KEYBUF(sav->key_enc), _KEYLEN(sav->key_enc), &ctx->decrypt);
	aes_encrypt_key((const unsigned char *) _KEYBUF(sav->key_enc), _KEYLEN(sav->key_enc), &ctx->encrypt);

	return 0;
}


                                                               
                                                                     
                     
                                                                   
                                                       
                                                                 
                                                                     
                                                                     
         
  
                 
                             
  
                         
                                                               
                  
                                                        
                                                 
                       
                                                                 
  
                       
                     
                   
                                                           
   


int
esp_cbc_decrypt_aes(
	struct mbuf *m,
	size_t off,
	struct secasvar *sav,
	const struct esp_algorithm *algo,
	int ivlen)
{
	struct mbuf *s;
	struct mbuf *d, *d0, *dp;
	int soff;                                                                
	int sn, dn;                                                    
	size_t ivoff, bodyoff;
	u_int8_t iv[AES_BLOCKLEN] __attribute__((aligned(4))), *dptr;
	u_int8_t sbuf[AES_BLOCKLEN] __attribute__((aligned(4))), *sp, *sp_unaligned, *sp_aligned = NULL;
	struct mbuf *scut;
	int scutoff;
	int     i, len;


	if (ivlen != AES_BLOCKLEN) {
		ipseclog((LOG_ERR, "esp_cbc_decrypt %s: "
		    "unsupported ivlen %d\n", algo->name, ivlen));
		m_freem(m);
		return EINVAL;
	}

	if (sav->flags & SADB_X_EXT_OLD) {
		              
		ivoff = off + sizeof(struct esp);
		bodyoff = off + sizeof(struct esp) + ivlen;
	} else {
		ivoff = off + sizeof(struct newesp);
		bodyoff = off + sizeof(struct newesp) + ivlen;
	}

	if (m->m_pkthdr.len < bodyoff) {
		ipseclog((LOG_ERR, "esp_cbc_decrypt %s: bad len %d/%u\n",
		    algo->name, m->m_pkthdr.len, (u_int32_t)bodyoff));
		m_freem(m);
		return EINVAL;
	}
	if ((m->m_pkthdr.len - bodyoff) % AES_BLOCKLEN) {
		ipseclog((LOG_ERR, "esp_cbc_decrypt %s: "
		    "payload length must be multiple of %d\n",
		    algo->name, AES_BLOCKLEN));
		m_freem(m);
		return EINVAL;
	}

	VERIFY(ivoff <= INT_MAX);

	             
	m_copydata(m, (int)ivoff, ivlen, (caddr_t) iv);

	s = m;
	soff = sn = dn = 0;
	d = d0 = dp = NULL;
	sp = dptr = NULL;

	                           
	while (soff < bodyoff) {
		if (soff + s->m_len > bodyoff) {
			sn = (int)(bodyoff - soff);
			break;
		}

		soff += s->m_len;
		s = s->m_next;
	}
	scut = s;
	scutoff = sn;

	                          
	while (s && s->m_len == 0) {
		s = s->m_next;
	}

	while (soff < m->m_pkthdr.len) {
		            
		if (sn + AES_BLOCKLEN <= s->m_len) {
			                        
			sp = mtod(s, u_int8_t *) + sn;
			len = s->m_len - sn;
			len -= len % AES_BLOCKLEN;                         
		} else {
			                            
			m_copydata(s, sn, AES_BLOCKLEN, (caddr_t) sbuf);
			sp = sbuf;
			len = AES_BLOCKLEN;                                            
		}

		                 
		if (!d || dn + AES_BLOCKLEN > d->m_len) {
			if (d) {
				dp = d;
			}
			MGET(d, M_DONTWAIT, MT_DATA);
			i = m->m_pkthdr.len - (soff + sn);
			if (d && i > MLEN) {
				MCLGET(d, M_DONTWAIT);
				if ((d->m_flags & M_EXT) == 0) {
					d = m_mbigget(d, M_DONTWAIT);
					if ((d->m_flags & M_EXT) == 0) {
						m_free(d);
						d = NULL;
					}
				}
			}
			if (!d) {
				m_freem(m);
				if (d0) {
					m_freem(d0);
				}
				return ENOBUFS;
			}
			if (!d0) {
				d0 = d;
			}
			if (dp) {
				dp->m_next = d;
			}

			                                
			if (!IPSEC_IS_P2ALIGNED(d->m_data)) {
				m_adj(d, IPSEC_GET_P2UNALIGNED_OFS(d->m_data));
			}

			d->m_len = (int)M_TRAILINGSPACE(d);
			d->m_len -= d->m_len % AES_BLOCKLEN;
			if (d->m_len > i) {
				d->m_len = i;
			}
			dptr = mtod(d, u_int8_t *);
			dn = 0;
		}

		                                                        
		if (len > d->m_len - dn) {
			len = d->m_len - dn;
		}

		             
		                                                                                                           
		if (IPSEC_IS_P2ALIGNED(sp)) {
			sp_unaligned = NULL;
		} else {
			sp_unaligned = sp;
			if (len > MAX_REALIGN_LEN) {
				m_freem(m);
				if (d0 != NULL) {
					m_freem(d0);
				}
				if (sp_aligned != NULL) {
					FREE(sp_aligned, M_SECA);
					sp_aligned = NULL;
				}
				return ENOBUFS;
			}
			if (sp_aligned == NULL) {
				sp_aligned = (u_int8_t *)_MALLOC(MAX_REALIGN_LEN, M_SECA, M_DONTWAIT);
				if (sp_aligned == NULL) {
					m_freem(m);
					if (d0 != NULL) {
						m_freem(d0);
					}
					return ENOMEM;
				}
			}
			sp = sp_aligned;
			memcpy(sp, sp_unaligned, len);
		}
		                                            
		aes_decrypt_cbc(sp, iv, len >> 4, dptr + dn,
		    (aes_decrypt_ctx*)(&(((aes_ctx*)sav->sched)->decrypt)));

		                            
		if (!IPSEC_IS_P2ALIGNED(sp_unaligned)) {
			sp = sp_unaligned;
		}

		                    
		sn += len;
		dn += len;

		          
		bcopy(sp + len - AES_BLOCKLEN, iv, AES_BLOCKLEN);

		                                
		while (s && sn >= s->m_len) {
			sn -= s->m_len;
			soff += s->m_len;
			s = s->m_next;
		}
	}

	                                                             
	m_freem(scut->m_next);
	scut->m_len = scutoff;
	scut->m_next = d0;

	              
	if (sp_aligned != NULL) {
		FREE(sp_aligned, M_SECA);
		sp_aligned = NULL;
	}

	                  
	bzero(iv, sizeof(iv));
	bzero(sbuf, sizeof(sbuf));

	return 0;
}

int
esp_cbc_encrypt_aes(
	struct mbuf *m,
	size_t off,
	__unused size_t plen,
	struct secasvar *sav,
	const struct esp_algorithm *algo,
	int ivlen)
{
	struct mbuf *s;
	struct mbuf *d, *d0, *dp;
	int soff;                                                                
	int sn, dn;                                                    
	size_t ivoff, bodyoff;
	u_int8_t *ivp, *dptr, *ivp_unaligned;
	u_int8_t sbuf[AES_BLOCKLEN] __attribute__((aligned(4))), *sp, *sp_unaligned, *sp_aligned = NULL;
	u_int8_t ivp_aligned_buf[AES_BLOCKLEN] __attribute__((aligned(4)));
	struct mbuf *scut;
	int scutoff;
	int i, len;

	if (ivlen != AES_BLOCKLEN) {
		ipseclog((LOG_ERR, "esp_cbc_encrypt %s: "
		    "unsupported ivlen %d\n", algo->name, ivlen));
		m_freem(m);
		return EINVAL;
	}

	if (sav->flags & SADB_X_EXT_OLD) {
		              
		ivoff = off + sizeof(struct esp);
		bodyoff = off + sizeof(struct esp) + ivlen;
	} else {
		ivoff = off + sizeof(struct newesp);
		bodyoff = off + sizeof(struct newesp) + ivlen;
	}

	VERIFY(ivoff <= INT_MAX);

	                            
	m_copyback(m, (int)ivoff, ivlen, sav->iv);
	ivp = (u_int8_t *) sav->iv;

	if (m->m_pkthdr.len < bodyoff) {
		ipseclog((LOG_ERR, "esp_cbc_encrypt %s: bad len %d/%u\n",
		    algo->name, m->m_pkthdr.len, (u_int32_t)bodyoff));
		m_freem(m);
		return EINVAL;
	}
	if ((m->m_pkthdr.len - bodyoff) % AES_BLOCKLEN) {
		ipseclog((LOG_ERR, "esp_cbc_encrypt %s: "
		    "payload length must be multiple of %d\n",
		    algo->name, AES_BLOCKLEN));
		m_freem(m);
		return EINVAL;
	}

	s = m;
	soff = sn = dn = 0;
	d = d0 = dp = NULL;
	sp = dptr = NULL;

	                     
	while (soff < bodyoff) {
		if (soff + s->m_len > bodyoff) {
			sn = (int)(bodyoff - soff);
			break;
		}

		soff += s->m_len;
		s = s->m_next;
	}
	scut = s;
	scutoff = sn;

	                          
	while (s && s->m_len == 0) {
		s = s->m_next;
	}

	while (soff < m->m_pkthdr.len) {
		            
		if (sn + AES_BLOCKLEN <= s->m_len) {
			                        
			sp = mtod(s, u_int8_t *) + sn;
			len = s->m_len - sn;
			len -= len % AES_BLOCKLEN;                         
		} else {
			                            
			m_copydata(s, sn, AES_BLOCKLEN, (caddr_t) sbuf);
			sp = sbuf;
			len = AES_BLOCKLEN;                                            
		}

		                 
		if (!d || dn + AES_BLOCKLEN > d->m_len) {
			if (d) {
				dp = d;
			}
			MGET(d, M_DONTWAIT, MT_DATA);
			i = m->m_pkthdr.len - (soff + sn);
			if (d && i > MLEN) {
				MCLGET(d, M_DONTWAIT);
				if ((d->m_flags & M_EXT) == 0) {
					d = m_mbigget(d, M_DONTWAIT);
					if ((d->m_flags & M_EXT) == 0) {
						m_free(d);
						d = NULL;
					}
				}
			}
			if (!d) {
				m_freem(m);
				if (d0) {
					m_freem(d0);
				}
				return ENOBUFS;
			}
			if (!d0) {
				d0 = d;
			}
			if (dp) {
				dp->m_next = d;
			}

			                                
			if (!IPSEC_IS_P2ALIGNED(d->m_data)) {
				m_adj(d, IPSEC_GET_P2UNALIGNED_OFS(d->m_data));
			}

			d->m_len = (int)M_TRAILINGSPACE(d);
			d->m_len -= d->m_len % AES_BLOCKLEN;
			if (d->m_len > i) {
				d->m_len = i;
			}
			dptr = mtod(d, u_int8_t *);
			dn = 0;
		}

		                                                
		if (len > d->m_len - dn) {
			len = d->m_len - dn;
		}

		             
		                                                                                                             
		if (IPSEC_IS_P2ALIGNED(sp)) {
			sp_unaligned = NULL;
		} else {
			sp_unaligned = sp;
			if (len > MAX_REALIGN_LEN) {
				m_freem(m);
				if (d0) {
					m_freem(d0);
				}
				if (sp_aligned != NULL) {
					FREE(sp_aligned, M_SECA);
					sp_aligned = NULL;
				}
				return ENOBUFS;
			}
			if (sp_aligned == NULL) {
				sp_aligned = (u_int8_t *)_MALLOC(MAX_REALIGN_LEN, M_SECA, M_DONTWAIT);
				if (sp_aligned == NULL) {
					m_freem(m);
					if (d0) {
						m_freem(d0);
					}
					return ENOMEM;
				}
			}
			sp = sp_aligned;
			memcpy(sp, sp_unaligned, len);
		}
		                                                                                                            
		if (IPSEC_IS_P2ALIGNED(ivp)) {
			ivp_unaligned = NULL;
		} else {
			ivp_unaligned = ivp;
			ivp = ivp_aligned_buf;
			memcpy(ivp, ivp_unaligned, AES_BLOCKLEN);
		}
		                                            
		aes_encrypt_cbc(sp, ivp, len >> 4, dptr + dn,
		    (aes_encrypt_ctx*)(&(((aes_ctx*)sav->sched)->encrypt)));

		                            
		if (!IPSEC_IS_P2ALIGNED(sp_unaligned)) {
			sp = sp_unaligned;
		}
		if (!IPSEC_IS_P2ALIGNED(ivp_unaligned)) {
			ivp = ivp_unaligned;
		}

		                    
		sn += len;
		dn += len;

		             
		ivp = dptr + dn - AES_BLOCKLEN;                        

		                                                     
		while (s && sn >= s->m_len) {
			sn -= s->m_len;
			soff += s->m_len;
			s = s->m_next;
		}
	}

	                                                             
	m_freem(scut->m_next);
	scut->m_len = scutoff;
	scut->m_next = d0;

	              
	if (sp_aligned != NULL) {
		FREE(sp_aligned, M_SECA);
		sp_aligned = NULL;
	}

	                  
	bzero(sbuf, sizeof(sbuf));
	key_sa_stir_iv(sav);

	return 0;
}

size_t
esp_gcm_schedlen(
	__unused const struct esp_algorithm *algo)
{
	return sizeof(aes_gcm_ctx) + aes_decrypt_get_ctx_size_gcm() + aes_encrypt_get_ctx_size_gcm() + ESP_GCM_ALIGN;
}

int
esp_gcm_schedule( __unused const struct esp_algorithm *algo,
    struct secasvar *sav)
{
	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);
	aes_gcm_ctx *ctx = (aes_gcm_ctx*)P2ROUNDUP(sav->sched, ESP_GCM_ALIGN);
	u_int ivlen = sav->ivlen;
	unsigned char nonce[ESP_GCM_SALT_LEN + ivlen];
	int rc;

	ctx->decrypt = &ctx->ctxt[0];
	ctx->encrypt = &ctx->ctxt[aes_decrypt_get_ctx_size_gcm() / sizeof(ccgcm_ctx)];

	rc = aes_decrypt_key_gcm((const unsigned char *) _KEYBUF(sav->key_enc), _KEYLEN(sav->key_enc) - ESP_GCM_SALT_LEN, ctx->decrypt);
	if (rc) {
		return rc;
	}

	bzero(nonce, ESP_GCM_SALT_LEN + ivlen);
	memcpy(nonce, _KEYBUF(sav->key_enc) + _KEYLEN(sav->key_enc) - ESP_GCM_SALT_LEN, ESP_GCM_SALT_LEN);
	memcpy(nonce + ESP_GCM_SALT_LEN, sav->iv, ivlen);

	rc = aes_encrypt_key_with_iv_gcm((const unsigned char *) _KEYBUF(sav->key_enc), _KEYLEN(sav->key_enc) - ESP_GCM_SALT_LEN, nonce, ctx->encrypt);
	if (rc) {
		return rc;
	}

	rc = aes_encrypt_reset_gcm(ctx->encrypt);
	if (rc) {
		return rc;
	}

	return rc;
}

int
esp_gcm_encrypt_finalize(struct secasvar *sav,
    unsigned char *tag, size_t tag_bytes)
{
	aes_gcm_ctx *ctx = (aes_gcm_ctx*)P2ROUNDUP(sav->sched, ESP_GCM_ALIGN);
	return aes_encrypt_finalize_gcm(tag, tag_bytes, ctx->encrypt);
}

int
esp_gcm_decrypt_finalize(struct secasvar *sav,
    unsigned char *tag, size_t tag_bytes)
{
	aes_gcm_ctx *ctx = (aes_gcm_ctx*)P2ROUNDUP(sav->sched, ESP_GCM_ALIGN);
	return aes_decrypt_finalize_gcm(tag, tag_bytes, ctx->decrypt);
}

int
esp_gcm_encrypt_aes(
	struct mbuf *m,
	size_t off,
	__unused size_t plen,
	struct secasvar *sav,
	const struct esp_algorithm *algo __unused,
	int ivlen)
{
	struct mbuf *s;
	struct mbuf *d, *d0, *dp;
	int soff;                                                                
	int sn, dn;                                                    
	size_t ivoff, bodyoff;
	u_int8_t *dptr, *sp, *sp_unaligned, *sp_aligned = NULL;
	aes_gcm_ctx *ctx;
	struct mbuf *scut;
	int scutoff;
	int i, len;
	unsigned char nonce[ESP_GCM_SALT_LEN + ivlen];

	if (ivlen != ESP_GCM_IVLEN) {
		ipseclog((LOG_ERR, "%s: unsupported ivlen %d\n", __FUNCTION__, ivlen));
		m_freem(m);
		return EINVAL;
	}

	if (sav->flags & SADB_X_EXT_OLD) {
		              
		ivoff = off + sizeof(struct esp);
		bodyoff = off + sizeof(struct esp) + ivlen;
	} else {
		ivoff = off + sizeof(struct newesp);
		bodyoff = off + sizeof(struct newesp) + ivlen;
	}

	bzero(nonce, ESP_GCM_SALT_LEN + ivlen);
	                     
	ctx = (aes_gcm_ctx *)P2ROUNDUP(sav->sched, ESP_GCM_ALIGN);

	if (aes_encrypt_reset_gcm(ctx->encrypt)) {
		ipseclog((LOG_ERR, "%s: gcm reset failure\n", __FUNCTION__));
		m_freem(m);
		return EINVAL;
	}

	if (aes_encrypt_inc_iv_gcm((unsigned char *)nonce, ctx->encrypt)) {
		ipseclog((LOG_ERR, "%s: iv generation failure\n", __FUNCTION__));
		m_freem(m);
		return EINVAL;
	}

	VERIFY(ivoff <= INT_MAX);

	  
                                                 
                                                      
                                                     
                                                    
                                                          
                                      
    
	memcpy(sav->iv, (nonce + ESP_GCM_SALT_LEN), ivlen);
	m_copyback(m, (int)ivoff, ivlen, sav->iv);
	bzero(nonce, ESP_GCM_SALT_LEN + ivlen);

	if (m->m_pkthdr.len < bodyoff) {
		ipseclog((LOG_ERR, "%s: bad len %d/%u\n", __FUNCTION__,
		    m->m_pkthdr.len, (u_int32_t)bodyoff));
		m_freem(m);
		return EINVAL;
	}

	VERIFY(off <= INT_MAX);

	                                        
	if (!(sav->flags & SADB_X_EXT_OLD)) {
		struct newesp esp;
		m_copydata(m, (int)off, sizeof(esp), (caddr_t) &esp);
		if (aes_encrypt_aad_gcm((unsigned char*)&esp, sizeof(esp), ctx->encrypt)) {
			ipseclog((LOG_ERR, "%s: packet decryption AAD failure\n", __FUNCTION__));
			m_freem(m);
			return EINVAL;
		}
	}

	s = m;
	soff = sn = dn = 0;
	d = d0 = dp = NULL;
	sp = dptr = NULL;

	                     
	while (soff < bodyoff) {
		if (soff + s->m_len > bodyoff) {
			sn = (int)(bodyoff - soff);
			break;
		}

		soff += s->m_len;
		s = s->m_next;
	}
	scut = s;
	scutoff = sn;

	                          
	while (s && s->m_len == 0) {
		s = s->m_next;
	}

	while (soff < m->m_pkthdr.len) {
		            
		sp = mtod(s, u_int8_t *) + sn;
		len = s->m_len - sn;

		                 
		if (!d || (dn + len > d->m_len)) {
			if (d) {
				dp = d;
			}
			MGET(d, M_DONTWAIT, MT_DATA);
			i = m->m_pkthdr.len - (soff + sn);
			if (d && i > MLEN) {
				MCLGET(d, M_DONTWAIT);
				if ((d->m_flags & M_EXT) == 0) {
					d = m_mbigget(d, M_DONTWAIT);
					if ((d->m_flags & M_EXT) == 0) {
						m_free(d);
						d = NULL;
					}
				}
			}
			if (!d) {
				m_freem(m);
				if (d0) {
					m_freem(d0);
				}
				return ENOBUFS;
			}
			if (!d0) {
				d0 = d;
			}
			if (dp) {
				dp->m_next = d;
			}

			                                
			if (!IPSEC_IS_P2ALIGNED(d->m_data)) {
				m_adj(d, IPSEC_GET_P2UNALIGNED_OFS(d->m_data));
			}

			d->m_len = (int)M_TRAILINGSPACE(d);

			if (d->m_len > i) {
				d->m_len = i;
			}

			dptr = mtod(d, u_int8_t *);
			dn = 0;
		}

		                                                
		if (len > d->m_len - dn) {
			len = d->m_len - dn;
		}

		             
		                                                                                                             
		if (IPSEC_IS_P2ALIGNED(sp)) {
			sp_unaligned = NULL;
		} else {
			sp_unaligned = sp;
			if (len > MAX_REALIGN_LEN) {
				m_freem(m);
				if (d0) {
					m_freem(d0);
				}
				if (sp_aligned != NULL) {
					FREE(sp_aligned, M_SECA);
					sp_aligned = NULL;
				}
				return ENOBUFS;
			}
			if (sp_aligned == NULL) {
				sp_aligned = (u_int8_t *)_MALLOC(MAX_REALIGN_LEN, M_SECA, M_DONTWAIT);
				if (sp_aligned == NULL) {
					m_freem(m);
					if (d0) {
						m_freem(d0);
					}
					return ENOMEM;
				}
			}
			sp = sp_aligned;
			memcpy(sp, sp_unaligned, len);
		}

		if (aes_encrypt_gcm(sp, len, dptr + dn, ctx->encrypt)) {
			ipseclog((LOG_ERR, "%s: failed to encrypt\n", __FUNCTION__));
			m_freem(m);
			return EINVAL;
		}

		                            
		if (!IPSEC_IS_P2ALIGNED(sp_unaligned)) {
			sp = sp_unaligned;
		}

		                    
		sn += len;
		dn += len;

		                                                     
		while (s && sn >= s->m_len) {
			sn -= s->m_len;
			soff += s->m_len;
			s = s->m_next;
		}
	}

	                                                             
	m_freem(scut->m_next);
	scut->m_len = scutoff;
	scut->m_next = d0;

	              
	if (sp_aligned != NULL) {
		FREE(sp_aligned, M_SECA);
		sp_aligned = NULL;
	}

	return 0;
}

int
esp_gcm_decrypt_aes(
	struct mbuf *m,
	size_t off,
	struct secasvar *sav,
	const struct esp_algorithm *algo __unused,
	int ivlen)
{
	struct mbuf *s;
	struct mbuf *d, *d0, *dp;
	int soff;                                                                
	int sn, dn;                                                    
	size_t ivoff, bodyoff;
	u_int8_t iv[ESP_GCM_IVLEN] __attribute__((aligned(4))), *dptr;
	u_int8_t *sp, *sp_unaligned, *sp_aligned = NULL;
	aes_gcm_ctx *ctx;
	struct mbuf *scut;
	int scutoff;
	int     i, len;
	unsigned char nonce[ESP_GCM_SALT_LEN + ivlen];

	if (ivlen != ESP_GCM_IVLEN) {
		ipseclog((LOG_ERR, "%s: unsupported ivlen %d\n", __FUNCTION__, ivlen));
		m_freem(m);
		return EINVAL;
	}

	if (sav->flags & SADB_X_EXT_OLD) {
		              
		ivoff = off + sizeof(struct esp);
		bodyoff = off + sizeof(struct esp) + ivlen;
	} else {
		ivoff = off + sizeof(struct newesp);
		bodyoff = off + sizeof(struct newesp) + ivlen;
	}

	if (m->m_pkthdr.len < bodyoff) {
		ipseclog((LOG_ERR, "%s: bad len %d/%u\n", __FUNCTION__,
		    m->m_pkthdr.len, (u_int32_t)bodyoff));
		m_freem(m);
		return EINVAL;
	}

	VERIFY(ivoff <= INT_MAX);

	             
	m_copydata(m, (int)ivoff, ivlen, (caddr_t) iv);

	            
	memcpy(nonce, _KEYBUF(sav->key_enc) + _KEYLEN(sav->key_enc) - ESP_GCM_SALT_LEN, ESP_GCM_SALT_LEN);
	memcpy(nonce + ESP_GCM_SALT_LEN, iv, ivlen);

	ctx = (aes_gcm_ctx *)P2ROUNDUP(sav->sched, ESP_GCM_ALIGN);
	if (aes_decrypt_set_iv_gcm(nonce, sizeof(nonce), ctx->decrypt)) {
		ipseclog((LOG_ERR, "%s: failed to set IV\n", __FUNCTION__));
		m_freem(m);
		bzero(nonce, sizeof(nonce));
		return EINVAL;
	}
	bzero(nonce, sizeof(nonce));

	VERIFY(off <= INT_MAX);

	                                        
	if (!(sav->flags & SADB_X_EXT_OLD)) {
		struct newesp esp;
		m_copydata(m, (int)off, sizeof(esp), (caddr_t) &esp);
		if (aes_decrypt_aad_gcm((unsigned char*)&esp, sizeof(esp), ctx->decrypt)) {
			ipseclog((LOG_ERR, "%s: packet decryption AAD failure\n", __FUNCTION__));
			return EINVAL;
		}
	}

	s = m;
	soff = sn = dn = 0;
	d = d0 = dp = NULL;
	sp = dptr = NULL;

	                           
	while (soff < bodyoff) {
		if (soff + s->m_len > bodyoff) {
			sn = (int)(bodyoff - soff);
			break;
		}

		soff += s->m_len;
		s = s->m_next;
	}
	scut = s;
	scutoff = sn;

	                          
	while (s && s->m_len == 0) {
		s = s->m_next;
	}

	while (soff < m->m_pkthdr.len) {
		            
		sp = mtod(s, u_int8_t *) + sn;
		len = s->m_len - sn;

		                 
		if (!d || (dn + len > d->m_len)) {
			if (d) {
				dp = d;
			}
			MGET(d, M_DONTWAIT, MT_DATA);
			i = m->m_pkthdr.len - (soff + sn);
			if (d && i > MLEN) {
				MCLGET(d, M_DONTWAIT);
				if ((d->m_flags & M_EXT) == 0) {
					d = m_mbigget(d, M_DONTWAIT);
					if ((d->m_flags & M_EXT) == 0) {
						m_free(d);
						d = NULL;
					}
				}
			}
			if (!d) {
				m_freem(m);
				if (d0) {
					m_freem(d0);
				}
				return ENOBUFS;
			}
			if (!d0) {
				d0 = d;
			}
			if (dp) {
				dp->m_next = d;
			}

			                                
			if (!IPSEC_IS_P2ALIGNED(d->m_data)) {
				m_adj(d, IPSEC_GET_P2UNALIGNED_OFS(d->m_data));
			}

			d->m_len = (int)M_TRAILINGSPACE(d);

			if (d->m_len > i) {
				d->m_len = i;
			}

			dptr = mtod(d, u_int8_t *);
			dn = 0;
		}

		                                                        
		if (len > d->m_len - dn) {
			len = d->m_len - dn;
		}

		             
		                                                                                                           
		if (IPSEC_IS_P2ALIGNED(sp)) {
			sp_unaligned = NULL;
		} else {
			sp_unaligned = sp;
			if (len > MAX_REALIGN_LEN) {
				m_freem(m);
				if (d0) {
					m_freem(d0);
				}
				if (sp_aligned != NULL) {
					FREE(sp_aligned, M_SECA);
					sp_aligned = NULL;
				}
				return ENOBUFS;
			}
			if (sp_aligned == NULL) {
				sp_aligned = (u_int8_t *)_MALLOC(MAX_REALIGN_LEN, M_SECA, M_DONTWAIT);
				if (sp_aligned == NULL) {
					m_freem(m);
					if (d0) {
						m_freem(d0);
					}
					return ENOMEM;
				}
			}
			sp = sp_aligned;
			memcpy(sp, sp_unaligned, len);
		}
		                                            

		if (aes_decrypt_gcm(sp, len, dptr + dn, ctx->decrypt)) {
			ipseclog((LOG_ERR, "%s: failed to decrypt\n", __FUNCTION__));
			m_freem(m);
			return EINVAL;
		}

		                            
		if (!IPSEC_IS_P2ALIGNED(sp_unaligned)) {
			sp = sp_unaligned;
		}

		                    
		sn += len;
		dn += len;

		                                
		while (s && sn >= s->m_len) {
			sn -= s->m_len;
			soff += s->m_len;
			s = s->m_next;
		}
	}

	                                                             
	m_freem(scut->m_next);
	scut->m_len = scutoff;
	scut->m_next = d0;

	              
	if (sp_aligned != NULL) {
		FREE(sp_aligned, M_SECA);
		sp_aligned = NULL;
	}

	                  
	bzero(iv, sizeof(iv));

	return 0;
}
