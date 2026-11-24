/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
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
/*
 * Copyright (c) 1982, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>

#include <pexpert/pexpert.h>

#define etherbroadcastaddr      fugly
#include <net/if.h>
#include <net/route.h>
#include <net/if_llc.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_ether.h>
#include <net/if_gif.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>                 
#include <net/kpi_interface.h>
#include <net/kpi_protocol.h>
#undef etherbroadcastaddr

  
         
                        
                            
  
                              
                        
       
   
#include <net/ether_if_module.h>
#include <sys/socketvar.h>
#include <net/if_vlan_var.h>
#include <net/if_6lowpan_var.h>
#if BOND
#include <net/if_bond_internal.h>
#endif           
#if IF_BRIDGE
#include <net/if_bridgevar.h>
#endif                
#if IF_FAKE
#include <net/if_fake_var.h>
#endif              
#if IF_HEADLESS
extern void if_headless_init(void);
#endif                  

#include <net/dlil.h>

SYSCTL_DECL(_net_link);
SYSCTL_NODE(_net_link, IFT_ETHER, ether, CTLFLAG_RW | CTLFLAG_LOCKED, 0,
    "Ethernet");

struct en_desc {
	u_int16_t type;                                                      
	u_int32_t protocol_family;                           
	u_int32_t data[2];                                 
};

                                                                
#if !XNU_TARGET_OS_OSX
#define ETHER_DESC_BLK_SIZE (2)              
#else                        
#define ETHER_DESC_BLK_SIZE (10)
#endif                        

  
                                                                  
   
struct ether_desc_blk_str {
	u_int32_t  n_max_used;
	u_int32_t       n_count;
	u_int32_t       n_used;
	struct en_desc  block_ptr[1];
};

                                                                 
#define ETHER_DESC_HEADER_SIZE  \
	((size_t) offsetof(struct ether_desc_blk_str, block_ptr))

__private_extern__ u_char etherbroadcastaddr[ETHER_ADDR_LEN] =
{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

  
                                                                                
                                                                             
                      
   
int
ether_del_proto(ifnet_t ifp, protocol_family_t protocol_family)
{
	struct ether_desc_blk_str *desc_blk =
	    (struct ether_desc_blk_str *)ifp->if_family_cookie;
	u_int32_t current = 0;
	int found = 0;

	if (desc_blk == NULL) {
		return 0;
	}

	for (current = desc_blk->n_max_used; current > 0; current--) {
		if (desc_blk->block_ptr[current - 1].protocol_family ==
		    protocol_family) {
			found = 1;
			desc_blk->block_ptr[current - 1].type = 0;
			desc_blk->n_used--;
		}
	}

	if (desc_blk->n_used == 0) {
		FREE(ifp->if_family_cookie, M_IFADDR);
		ifp->if_family_cookie = 0;
	} else {
		                          
		for (; desc_blk->n_max_used > 0 &&
		    desc_blk->block_ptr[desc_blk->n_max_used - 1].type == 0;
		    desc_blk->n_max_used--) {
			;
		}
	}

	return 0;
}

static int
ether_add_proto_internal(struct ifnet *ifp, protocol_family_t protocol,
    const struct ifnet_demux_desc *demux)
{
	struct en_desc *ed;
	struct ether_desc_blk_str *desc_blk =
	    (struct ether_desc_blk_str *)ifp->if_family_cookie;
	u_int32_t i;

	switch (demux->type) {
	                               
	                             
	case DLIL_DESC_ETYPE2:
		if (demux->datalen != 2) {
			return EINVAL;
		}
		break;

	case DLIL_DESC_SAP:
		if (demux->datalen != 3) {
			return EINVAL;
		}
		break;

	case DLIL_DESC_SNAP:
		if (demux->datalen != 5) {
			return EINVAL;
		}
		break;

	default:
		return ENOTSUP;
	}

	                                                 
	if (desc_blk != NULL) {
		switch (demux->type) {
		case DLIL_DESC_ETYPE2:
			for (i = 0; i < desc_blk->n_max_used; i++) {
				if (desc_blk->block_ptr[i].type ==
				    DLIL_DESC_ETYPE2 &&
				    desc_blk->block_ptr[i].data[0] ==
				    *(u_int16_t*)demux->data) {
					return EADDRINUSE;
				}
			}
			break;
		case DLIL_DESC_SAP:
		case DLIL_DESC_SNAP:
			for (i = 0; i < desc_blk->n_max_used; i++) {
				if (desc_blk->block_ptr[i].type ==
				    demux->type &&
				    bcmp(desc_blk->block_ptr[i].data,
				    demux->data, demux->datalen) == 0) {
					return EADDRINUSE;
				}
			}
			break;
		}
	}

	                                                                  
	if (desc_blk == NULL || desc_blk->n_used == desc_blk->n_count) {
		struct ether_desc_blk_str *tmp;
		u_int32_t new_count = ETHER_DESC_BLK_SIZE;
		u_int32_t new_size;
		u_int32_t old_size = 0;

		i = 0;

		if (desc_blk) {
			new_count += desc_blk->n_count;
			old_size = desc_blk->n_count * sizeof(struct en_desc) +
			    ETHER_DESC_HEADER_SIZE;
			i = desc_blk->n_used;
		}

		new_size = new_count * sizeof(struct en_desc) +
		    ETHER_DESC_HEADER_SIZE;

		tmp = _MALLOC(new_size, M_IFADDR, M_WAITOK);
		if (tmp == NULL) {
			  
                                                      
      
			return ENOMEM;
		}

		bzero(((char *)tmp) + old_size, new_size - old_size);
		if (desc_blk) {
			bcopy(desc_blk, tmp, old_size);
			FREE(desc_blk, M_IFADDR);
		}
		desc_blk = tmp;
		ifp->if_family_cookie = (uintptr_t)desc_blk;
		desc_blk->n_count = new_count;
	} else {
		                       
		for (i = 0; i < desc_blk->n_count; i++) {
			if (desc_blk->block_ptr[i].type == 0) {
				break;
			}
		}
	}

	                                    
	if (i + 1 > desc_blk->n_max_used) {
		desc_blk->n_max_used = i + 1;
	}

	ed = &desc_blk->block_ptr[i];
	ed->protocol_family = protocol;
	ed->data[0] = 0;
	ed->data[1] = 0;

	switch (demux->type) {
	case DLIL_DESC_ETYPE2:
		                                                         
		                                           
		ed->type = DLIL_DESC_ETYPE2;
		ed->data[0] = *(u_int16_t*)demux->data;
		break;

	case DLIL_DESC_SAP:
		ed->type = DLIL_DESC_SAP;
		bcopy(demux->data, &ed->data[0], 3);
		break;

	case DLIL_DESC_SNAP: {
		u_int8_t*       pDest = ((u_int8_t*)&ed->data[0]) + 3;
		ed->type = DLIL_DESC_SNAP;
		bcopy(demux->data, pDest, 5);
		break;
	}
	}

	desc_blk->n_used++;

	return 0;
}

int
ether_add_proto(ifnet_t  ifp, protocol_family_t protocol,
    const struct ifnet_demux_desc *demux_list, u_int32_t demux_count)
{
	int error = 0;
	u_int32_t i;

	for (i = 0; i < demux_count; i++) {
		error = ether_add_proto_internal(ifp, protocol, &demux_list[i]);
		if (error) {
			ether_del_proto(ifp, protocol);
			break;
		}
	}

	return error;
}

int
ether_demux(ifnet_t ifp, mbuf_t m, char *frame_header,
    protocol_family_t *protocol_family)
{
	struct ether_header *eh = (struct ether_header *)(void *)frame_header;
	u_short  ether_type = eh->ether_type;
	u_int16_t type;
	u_int8_t *data;
	u_int32_t i = 0;
	struct ether_desc_blk_str *desc_blk =
	    (struct ether_desc_blk_str *)ifp->if_family_cookie;
	u_int32_t maxd = desc_blk ? desc_blk->n_max_used : 0;
	struct en_desc  *ed = desc_blk ? desc_blk->block_ptr : NULL;
	u_int32_t extProto1 = 0;
	u_int32_t extProto2 = 0;

	if (eh->ether_dhost[0] & 1) {
		                         
		if (_ether_cmp(etherbroadcastaddr, eh->ether_dhost) == 0) {
			m->m_flags |= M_BCAST;
		} else {
			m->m_flags |= M_MCAST;
		}
	}

	if (m->m_flags & M_HASFCS) {
		  
                                                              
                                                              
                  
     
		m_adj(m, -ETHER_CRC_LEN);
		m->m_flags &= ~M_HASFCS;
	}

	if ((eh->ether_dhost[0] & 1) == 0) {
		  
                                                                
                                                             
                                                                
                                                               
             
     
		if (_ether_cmp(eh->ether_dhost, IF_LLADDR(ifp))) {
			m->m_flags |= M_PROMISC;
		}
	}

	                             
	if (ether_type == htons(ETHERTYPE_IEEE802154)) {
		*protocol_family = PF_802154;
		return 0;
	}

	                    
	if ((m->m_pkthdr.csum_flags & CSUM_VLAN_TAG_VALID) != 0) {
		if (EVL_VLANOFTAG(m->m_pkthdr.vlan_tag) != 0) {
			*protocol_family = PF_VLAN;
			return 0;
		}
		                                                       
		m->m_pkthdr.csum_flags &= ~CSUM_VLAN_TAG_VALID;
	} else if (ether_type == htons(ETHERTYPE_VLAN)) {
		struct ether_vlan_header *      evl;

		evl = (struct ether_vlan_header *)(void *)frame_header;
		if (m->m_len < ETHER_VLAN_ENCAP_LEN ||
		    ntohs(evl->evl_proto) == ETHERTYPE_VLAN ||
		    EVL_VLANOFTAG(ntohs(evl->evl_tag)) != 0) {
			*protocol_family = PF_VLAN;
			return 0;
		}
		                                        

		                                                          
		ether_type = evl->evl_encap_proto = evl->evl_proto;

		                                     
		m->m_len -= ETHER_VLAN_ENCAP_LEN;
		m->m_data += ETHER_VLAN_ENCAP_LEN;
		m->m_pkthdr.len -= ETHER_VLAN_ENCAP_LEN;
		m->m_pkthdr.csum_flags = 0;                                    
	} else if (ether_type == htons(ETHERTYPE_ARP)) {
		m->m_pkthdr.pkt_flags |= PKTF_INET_RESOLVE;                 
	}
	data = mtod(m, u_int8_t*);

	  
                                                                    
                             
    
	if (ntohs(ether_type) <= 1500) {
		bcopy(data, &extProto1, sizeof(u_int32_t));

		                 
		if ((extProto1 & htonl(0xFFFFFF00)) == htonl(0xAAAA0300)) {
			          
			type = DLIL_DESC_SNAP;
			bcopy(data + sizeof(u_int32_t), &extProto2,
			    sizeof(u_int32_t));
			extProto1 &= htonl(0x000000FF);
		} else {
			type = DLIL_DESC_SAP;
			extProto1 &= htonl(0xFFFFFF00);
		}
	} else {
		type = DLIL_DESC_ETYPE2;
	}

	  
                                                       
    
	switch (type) {
	case DLIL_DESC_ETYPE2:
		for (i = 0; i < maxd; i++) {
			if ((ed[i].type == type) &&
			    (ed[i].data[0] == ether_type)) {
				*protocol_family = ed[i].protocol_family;
				return 0;
			}
		}
		break;

	case DLIL_DESC_SAP:
		for (i = 0; i < maxd; i++) {
			if ((ed[i].type == type) &&
			    (ed[i].data[0] == extProto1)) {
				*protocol_family = ed[i].protocol_family;
				return 0;
			}
		}
		break;

	case DLIL_DESC_SNAP:
		for (i = 0; i < maxd; i++) {
			if ((ed[i].type == type) &&
			    (ed[i].data[0] == extProto1) &&
			    (ed[i].data[1] == extProto2)) {
				*protocol_family = ed[i].protocol_family;
				return 0;
			}
		}
		break;
	}

	return ENOENT;
}

  
                                                                     
                                                                         
                                                                         
                               
   
#if KPI_INTERFACE_EMBEDDED
int
ether_frameout(struct ifnet *ifp, struct mbuf **m,
    const struct sockaddr *ndest, const char *edst,
    const char *ether_type, u_int32_t *prepend_len, u_int32_t *postpend_len)
#else                              
int
ether_frameout(struct ifnet *ifp, struct mbuf **m,
    const struct sockaddr *ndest, const char *edst,
    const char *ether_type)
#endif                             
{
#if KPI_INTERFACE_EMBEDDED
	return ether_frameout_extended(ifp, m, ndest, edst, ether_type,
	           prepend_len, postpend_len);
#else                              
	return ether_frameout_extended(ifp, m, ndest, edst, ether_type,
	           NULL, NULL);
#endif                              
}

  
                           
                                                         
                                                              
                                                              
   
int
ether_frameout_extended(struct ifnet *ifp, struct mbuf **m,
    const struct sockaddr *ndest, const char *edst,
    const char *ether_type, u_int32_t *prepend_len, u_int32_t *postpend_len)
{
	struct ether_header *eh;
	int hlen;                                     

	hlen = ETHER_HDR_LEN;

	  
                                                               
                                                             
                                                             
                                                            
                                                               
                                                             
                                                         
    
	if ((ifp->if_flags & IFF_SIMPLEX) &&
	    ((*m)->m_flags & M_LOOP) && lo_ifp != NULL) {
		if ((*m)->m_flags & M_BCAST) {
			struct mbuf *n = m_copy(*m, 0, (int)M_COPYALL);
			if (n != NULL) {
				dlil_output(lo_ifp, ndest->sa_family,
				    n, NULL, ndest, 0, NULL);
			}
		} else if (_ether_cmp(edst, IF_LLADDR(ifp)) == 0) {
			dlil_output(lo_ifp, ndest->sa_family, *m,
			    NULL, ndest, 0, NULL);
			return EJUSTRETURN;
		}
	}

	  
                                                     
                     
    
	M_PREPEND(*m, sizeof(struct ether_header), M_DONTWAIT, 0);
	if (*m == NULL) {
		return EJUSTRETURN;
	}

	if (prepend_len != NULL) {
		*prepend_len = sizeof(struct ether_header);
	}
	if (postpend_len != NULL) {
		*postpend_len = 0;
	}

	eh = mtod(*m, struct ether_header *);
	(void) memcpy(&eh->ether_type, ether_type, sizeof(eh->ether_type));
	(void) memcpy(eh->ether_dhost, edst, ETHER_ADDR_LEN);
	ifnet_lladdr_copy_bytes(ifp, eh->ether_shost, ETHER_ADDR_LEN);

	return 0;
}

errno_t
ether_check_multi(ifnet_t ifp, const struct sockaddr *proto_addr)
{
#pragma unused(ifp)
	errno_t result = EAFNOSUPPORT;
	const u_char *e_addr;

	  
                                                        
                                                       
    
	switch (proto_addr->sa_family) {
	case AF_UNSPEC:
		e_addr = (const u_char*)&proto_addr->sa_data[0];
		if ((e_addr[0] & 0x01) != 0x01) {
			result = EADDRNOTAVAIL;
		} else {
			result = 0;
		}
		break;

	case AF_LINK:
		e_addr = CONST_LLADDR((const struct sockaddr_dl*)
		    (uintptr_t)(size_t)proto_addr);
		if ((e_addr[0] & 0x01) != 0x01) {
			result = EADDRNOTAVAIL;
		} else {
			result = 0;
		}
		break;
	}

	return result;
}

int
ether_ioctl(ifnet_t ifp, u_int32_t command, void *data)
{
#pragma unused(ifp, command, data)
	return EOPNOTSUPP;
}

__private_extern__ int
ether_family_init(void)
{
	errno_t error = 0;

	                                              
	if ((error = proto_register_plumber(PF_INET, APPLE_IF_FAM_ETHERNET,
	    ether_attach_inet, ether_detach_inet)) != 0) {
		printf("proto_register_plumber failed for PF_INET error=%d\n",
		    error);
		goto done;
	}
	if ((error = proto_register_plumber(PF_INET6, APPLE_IF_FAM_ETHERNET,
	    ether_attach_inet6, ether_detach_inet6)) != 0) {
		printf("proto_register_plumber failed for PF_INET6 error=%d\n",
		    error);
		goto done;
	}
#if VLAN
	vlan_family_init();
#endif           
#if BOND
	bond_family_init();
#endif           
#if IF_BRIDGE
	bridgeattach(0);
#endif                
#if IF_FAKE
	if_fake_init();
#endif              
#if IF_HEADLESS
	if_headless_init();
#endif                  
#if SIXLOWPAN
	sixlowpan_family_init();
#endif           
done:

	return error;
}
