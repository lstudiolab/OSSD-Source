/*
 * Copyright (c) 2003-2020 Apple Inc. All rights reserved.
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
 *
 */

/*
 * Copyright (c) 1982, 1986, 1991, 1993
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
 *	@(#)in_pcb.c	8.2 (Berkeley) 1/4/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sockio.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/priv.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/ntstat.h>
#include <net/restricted_in_port.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip6.h>
#include <netinet/ip_var.h>

#include <netinet6/ip6_var.h>
#include <netinet6/nd6.h>
#include <netinet/in_pcb.h>
#include <netinet6/in6_pcb.h>

#include <net/if_types.h>
#include <net/if_var.h>

#include <kern/kern_types.h>
#include <kern/zalloc.h>

#if IPSEC
#include <netinet6/ipsec.h>
#include <netinet6/ipsec6.h>
#include <netinet6/ah.h>
#include <netinet6/ah6.h>
#include <netkey/key.h>
#endif            

#if NECP
#include <net/necp.h>
#endif           

  
                                                  
                                                      
                                                    
                                                   
                                                  
                                                  
                  
   
static struct inpcb *
in6_pcblookup_local_and_cleanup(struct inpcbinfo *pcbinfo,
    struct in6_addr *laddr, u_int lport_arg, int wild_okay)
{
	struct inpcb *inp;

	                           
	inp = in6_pcblookup_local(pcbinfo, laddr, lport_arg, wild_okay);

	                                                               
	if (inp != NULL && inp->inp_wantcnt == WNT_STOPUSING) {
		struct socket *so = inp->inp_socket;

		socket_lock(so, 0);

		if (so->so_usecount == 0) {
			if (inp->inp_state != INPCB_STATE_DEAD) {
				in6_pcbdetach(inp);
			}
			in_pcbdispose(inp);                                
			inp = NULL;
		} else {
			socket_unlock(so, 0);
		}
	}

	return inp;
}

  
                                                                          
                                           
   
int
in6_pcbbind(struct inpcb *inp, struct sockaddr *nam, struct proc *p)
{
	struct socket *so = inp->inp_socket;
	struct inpcbinfo *pcbinfo = inp->inp_pcbinfo;
	u_short lport = 0;
	int wild = 0, reuseport = (so->so_options & SO_REUSEPORT);
	struct ifnet *outif = NULL;
	struct sockaddr_in6 sin6;
#if XNU_TARGET_OS_OSX
	int error;
	kauth_cred_t cred;
#endif                        

	if (TAILQ_EMPTY(&in6_ifaddrhead)) {                  
		return EADDRNOTAVAIL;
	}
	if (!(so->so_options & (SO_REUSEADDR | SO_REUSEPORT))) {
		wild = 1;
	}

	socket_unlock(so, 0);                     
	lck_rw_lock_exclusive(pcbinfo->ipi_lock);
	if (inp->inp_lport || !IN6_IS_ADDR_UNSPECIFIED(&inp->in6p_laddr)) {
		                                       
		lck_rw_done(pcbinfo->ipi_lock);
		socket_lock(so, 0);
		return EINVAL;
	}

	bzero(&sin6, sizeof(sin6));
	if (nam != NULL) {
		if (nam->sa_len != sizeof(struct sockaddr_in6)) {
			lck_rw_done(pcbinfo->ipi_lock);
			socket_lock(so, 0);
			return EINVAL;
		}
		  
                  
     
		if (nam->sa_family != AF_INET6) {
			lck_rw_done(pcbinfo->ipi_lock);
			socket_lock(so, 0);
			return EAFNOSUPPORT;
		}
		lport = SIN6(nam)->sin6_port;

		*(&sin6) = *SIN6(nam);

		                              
		if (in6_embedscope(&sin6.sin6_addr, &sin6, inp, NULL,
		    NULL) != 0) {
			lck_rw_done(pcbinfo->ipi_lock);
			socket_lock(so, 0);
			return EINVAL;
		}

		                                              
		sin6.sin6_flowinfo = 0;
		sin6.sin6_scope_id = 0;
		sin6.sin6_port = 0;

		if (IN6_IS_ADDR_MULTICAST(&sin6.sin6_addr)) {
			  
                                                       
                                              
                                                    
                                              
                                 
      
			if (so->so_options & SO_REUSEADDR) {
				reuseport = SO_REUSEADDR | SO_REUSEPORT;
			}
		} else if (!IN6_IS_ADDR_UNSPECIFIED(&sin6.sin6_addr)) {
			struct ifaddr *ifa;

			ifa = ifa_ifwithaddr(SA(&sin6));
			if (ifa == NULL) {
				lck_rw_done(pcbinfo->ipi_lock);
				socket_lock(so, 0);
				return EADDRNOTAVAIL;
			} else {
				  
                                            
                                               
                                               
                                             
                                      
       
				IFA_LOCK_SPIN(ifa);
				if (((struct in6_ifaddr *)ifa)->ia6_flags &
				    (IN6_IFF_ANYCAST | IN6_IFF_NOTREADY |
				    IN6_IFF_DETACHED | IN6_IFF_CLAT46)) {
					IFA_UNLOCK(ifa);
					IFA_REMREF(ifa);
					lck_rw_done(pcbinfo->ipi_lock);
					socket_lock(so, 0);
					return EADDRNOTAVAIL;
				}
				  
                                               
                                               
                                           
                                             
                                             
                                            
       
				outif = ifa->ifa_ifp;
				IFA_UNLOCK(ifa);
				IFA_REMREF(ifa);
			}
		}


		if (lport != 0) {
			struct inpcb *t;
			uid_t u;

#if XNU_TARGET_OS_OSX
			if (ntohs(lport) < IPV6PORT_RESERVED &&
			    !IN6_IS_ADDR_UNSPECIFIED(&sin6.sin6_addr) &&
			    !(inp->inp_flags2 & INP2_EXTERNAL_PORT)) {
				cred = kauth_cred_proc_ref(p);
				error = priv_check_cred(cred,
				    PRIV_NETINET_RESERVEDPORT, 0);
				kauth_cred_unref(&cred);
				if (error != 0) {
					lck_rw_done(pcbinfo->ipi_lock);
					socket_lock(so, 0);
					return EACCES;
				}
			}
#endif                        
			  
                                                                      
      
			if (!current_task_can_use_restricted_in_port(lport,
			    (uint8_t)SOCK_PROTO(so), PORT_FLAGS_BSD)) {
				lck_rw_done(pcbinfo->ipi_lock);
				socket_lock(so, 0);
				return EADDRINUSE;
			}

			if (!IN6_IS_ADDR_MULTICAST(&sin6.sin6_addr) &&
			    (u = kauth_cred_getuid(so->so_cred)) != 0) {
				t = in6_pcblookup_local_and_cleanup(pcbinfo,
				    &sin6.sin6_addr, lport,
				    INPLOOKUP_WILDCARD);
				if (t != NULL &&
				    (!IN6_IS_ADDR_UNSPECIFIED(&sin6.sin6_addr) ||
				    !IN6_IS_ADDR_UNSPECIFIED(&t->in6p_laddr) ||
				    !(t->inp_socket->so_options & SO_REUSEPORT)) &&
				    (u != kauth_cred_getuid(t->inp_socket->so_cred)) &&
				    !(t->inp_socket->so_flags & SOF_REUSESHAREUID) &&
				    (!(t->inp_flags2 & INP2_EXTERNAL_PORT) ||
				    !(inp->inp_flags2 & INP2_EXTERNAL_PORT) ||
				    uuid_compare(t->necp_client_uuid, inp->necp_client_uuid) != 0)) {
					lck_rw_done(pcbinfo->ipi_lock);
					socket_lock(so, 0);
					return EADDRINUSE;
				}
				if (!(inp->inp_flags & IN6P_IPV6_V6ONLY) &&
				    IN6_IS_ADDR_UNSPECIFIED(&sin6.sin6_addr)) {
					struct sockaddr_in sin;

					in6_sin6_2_sin(&sin, &sin6);
					t = in_pcblookup_local_and_cleanup(
						pcbinfo, sin.sin_addr, lport,
						INPLOOKUP_WILDCARD);
					if (t != NULL &&
					    !(t->inp_socket->so_options & SO_REUSEPORT) &&
					    (kauth_cred_getuid(so->so_cred) !=
					    kauth_cred_getuid(t->inp_socket->so_cred)) &&
					    (t->inp_laddr.s_addr != INADDR_ANY ||
					    SOCK_DOM(so) == SOCK_DOM(t->inp_socket)) &&
					    (!(t->inp_flags2 & INP2_EXTERNAL_PORT) ||
					    !(inp->inp_flags2 & INP2_EXTERNAL_PORT) ||
					    uuid_compare(t->necp_client_uuid, inp->necp_client_uuid) != 0)) {
						lck_rw_done(pcbinfo->ipi_lock);
						socket_lock(so, 0);
						return EADDRINUSE;
					}

				}
			}
			t = in6_pcblookup_local_and_cleanup(pcbinfo,
			    &sin6.sin6_addr, lport, wild);
			if (t != NULL &&
			    (reuseport & t->inp_socket->so_options) == 0 &&
			    (!(t->inp_flags2 & INP2_EXTERNAL_PORT) ||
			    !(inp->inp_flags2 & INP2_EXTERNAL_PORT) ||
			    uuid_compare(t->necp_client_uuid, inp->necp_client_uuid) != 0)) {
				lck_rw_done(pcbinfo->ipi_lock);
				socket_lock(so, 0);
				return EADDRINUSE;
			}
			if (!(inp->inp_flags & IN6P_IPV6_V6ONLY) &&
			    IN6_IS_ADDR_UNSPECIFIED(&sin6.sin6_addr)) {
				struct sockaddr_in sin;

				in6_sin6_2_sin(&sin, &sin6);
				t = in_pcblookup_local_and_cleanup(pcbinfo,
				    sin.sin_addr, lport, wild);
				if (t != NULL && (reuseport &
				    t->inp_socket->so_options) == 0 &&
				    (t->inp_laddr.s_addr != INADDR_ANY ||
				    SOCK_DOM(so) == SOCK_DOM(t->inp_socket)) &&
				    (!(t->inp_flags2 & INP2_EXTERNAL_PORT) ||
				    !(inp->inp_flags2 & INP2_EXTERNAL_PORT) ||
				    uuid_compare(t->necp_client_uuid, inp->necp_client_uuid) != 0)) {
					lck_rw_done(pcbinfo->ipi_lock);
					socket_lock(so, 0);
					return EADDRINUSE;
				}
			}
		}
	}

	socket_lock(so, 0);
	  
                                                       
                                                 
                                        
    
	if (inp->inp_state == INPCB_STATE_DEAD) {
		lck_rw_done(pcbinfo->ipi_lock);
		return ECONNABORTED;
	}

	                                                              
	if (inp->inp_lport || !IN6_IS_ADDR_UNSPECIFIED(&inp->in6p_laddr)) {
		lck_rw_done(pcbinfo->ipi_lock);
		return EINVAL;
	}

	if (!IN6_IS_ADDR_UNSPECIFIED(&sin6.sin6_addr)) {
		inp->in6p_laddr = sin6.sin6_addr;
		inp->in6p_last_outifp = outif;
	}

	if (lport == 0) {
		int e;
		if ((e = in6_pcbsetport(&inp->in6p_laddr, inp, p, 1)) != 0) {
			                                       
			inp->in6p_laddr = in6addr_any;
			inp->in6p_last_outifp = NULL;
			lck_rw_done(pcbinfo->ipi_lock);
			return e;
		}
	} else {
		inp->inp_lport = lport;
		if (in_pcbinshash(inp, 1) != 0) {
			inp->in6p_laddr = in6addr_any;
			inp->inp_lport = 0;
			inp->in6p_last_outifp = NULL;
			lck_rw_done(pcbinfo->ipi_lock);
			return EAGAIN;
		}
	}
	lck_rw_done(pcbinfo->ipi_lock);
	sflt_notify(so, sock_evt_bound, NULL);
	return 0;
}

  
                                                                  
                                                                    
                                                                
                                                
  
                                                                   
  
                                                                       
                                                                    
                                           
   
int
in6_pcbladdr(struct inpcb *inp, struct sockaddr *nam,
    struct in6_addr *plocal_addr6, struct ifnet **outif)
{
	struct in6_addr *addr6 = NULL;
	struct in6_addr src_storage;
	int error = 0;
	unsigned int ifscope;

	if (outif != NULL) {
		*outif = NULL;
	}
	if (nam->sa_len != sizeof(struct sockaddr_in6)) {
		return EINVAL;
	}
	if (SIN6(nam)->sin6_family != AF_INET6) {
		return EAFNOSUPPORT;
	}
	if (SIN6(nam)->sin6_port == 0) {
		return EADDRNOTAVAIL;
	}

	                              
	if (in6_embedscope(&SIN6(nam)->sin6_addr, SIN6(nam), inp, NULL, NULL) != 0) {
		return EINVAL;
	}

	if (!TAILQ_EMPTY(&in6_ifaddrhead)) {
		  
                                                    
                                    
     
		if (IN6_IS_ADDR_UNSPECIFIED(&SIN6(nam)->sin6_addr)) {
			SIN6(nam)->sin6_addr = in6addr_loopback;
		}
	}

	ifscope = (inp->inp_flags & INP_BOUND_IF) ?
	    inp->inp_boundifp->if_index : IFSCOPE_NONE;

	  
                                                            
                                                           
                                
   
                                                              
                                                             
                
    
	addr6 = in6_selectsrc(SIN6(nam), inp->in6p_outputopts, inp,
	    &inp->in6p_route, outif, &src_storage, ifscope, &error);

	if (outif != NULL) {
		struct rtentry *rt = inp->in6p_route.ro_rt;
		  
                                                         
                                                         
                                                          
                                                         
                                            
     
		if (rt != NULL && rt->rt_ifp != *outif) {
			ifnet_reference(rt->rt_ifp);                    
			if (*outif != NULL) {
				ifnet_release(*outif);
			}
			*outif = rt->rt_ifp;
		}
	}

	if (addr6 == NULL) {
		if (outif != NULL && (*outif) != NULL &&
		    inp_restricted_send(inp, *outif)) {
			soevent(inp->inp_socket,
			    (SO_FILT_HINT_LOCKED | SO_FILT_HINT_IFDENIED));
			error = EHOSTUNREACH;
		}
		if (error == 0) {
			error = EADDRNOTAVAIL;
		}
		return error;
	}

	*plocal_addr6 = *addr6;
	  
                                                     
                                                             
    
	return 0;
}

  
                    
                                                
                                                           
                                                     
                 
   
int
in6_pcbconnect(struct inpcb *inp, struct sockaddr *nam, struct proc *p)
{
	struct in6_addr addr6;
	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)(void *)nam;
	struct inpcb *pcb;
	int error = 0;
	struct ifnet *outif = NULL;
	struct socket *so = inp->inp_socket;

#if CONTENT_FILTER
	so->so_state_change_cnt++;
#endif

	if (so->so_proto->pr_protocol == IPPROTO_UDP &&
	    sin6->sin6_port == htons(53) && !(so->so_flags1 & SOF1_DNS_COUNTED)) {
		so->so_flags1 |= SOF1_DNS_COUNTED;
		INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_inet_dgram_dns);
	}

	  
                                                          
                                                           
   
                                                              
                                                           
                           
    
	if ((error = in6_pcbladdr(inp, nam, &addr6, &outif)) != 0) {
		if (outif != NULL && inp_restricted_send(inp, outif)) {
			soevent(so,
			    (SO_FILT_HINT_LOCKED | SO_FILT_HINT_IFDENIED));
		}
		goto done;
	}
	socket_unlock(so, 0);
	pcb = in6_pcblookup_hash(inp->inp_pcbinfo, &sin6->sin6_addr,
	    sin6->sin6_port, IN6_IS_ADDR_UNSPECIFIED(&inp->in6p_laddr) ?
	    &addr6 : &inp->in6p_laddr, inp->inp_lport, 0, NULL);
	socket_lock(so, 0);
	if (pcb != NULL) {
		in_pcb_checkstate(pcb, WNT_RELEASE, pcb == inp ? 1 : 0);
		error = EADDRINUSE;
		goto done;
	}
	if (IN6_IS_ADDR_UNSPECIFIED(&inp->in6p_laddr)) {
		if (inp->inp_lport == 0) {
			error = in6_pcbbind(inp, NULL, p);
			if (error) {
				goto done;
			}
		}
		inp->in6p_laddr = addr6;
		inp->in6p_last_outifp = outif;                           
		inp->in6p_flags |= INP_IN6ADDR_ANY;
	}
	if (!lck_rw_try_lock_exclusive(inp->inp_pcbinfo->ipi_lock)) {
		                                                             
		socket_unlock(so, 0);
		lck_rw_lock_exclusive(inp->inp_pcbinfo->ipi_lock);
		socket_lock(so, 0);
	}
	inp->in6p_faddr = sin6->sin6_addr;
	inp->inp_fport = sin6->sin6_port;
	if (nstat_collect && SOCK_PROTO(so) == IPPROTO_UDP) {
		nstat_pcb_invalidate_cache(inp);
	}
	in_pcbrehash(inp);
	lck_rw_done(inp->inp_pcbinfo->ipi_lock);

done:
	if (outif != NULL) {
		ifnet_release(outif);
	}

	return error;
}

void
in6_pcbdisconnect(struct inpcb *inp)
{
	struct socket *so = inp->inp_socket;

#if CONTENT_FILTER
	if (so) {
		so->so_state_change_cnt++;
	}
#endif

	if (!lck_rw_try_lock_exclusive(inp->inp_pcbinfo->ipi_lock)) {
		                                                             
		socket_unlock(so, 0);
		lck_rw_lock_exclusive(inp->inp_pcbinfo->ipi_lock);
		socket_lock(so, 0);
	}
	if (nstat_collect && SOCK_PROTO(so) == IPPROTO_UDP) {
		nstat_pcb_cache(inp);
	}
	bzero((caddr_t)&inp->in6p_faddr, sizeof(inp->in6p_faddr));
	inp->inp_fport = 0;
	                               
	inp->inp_flow &= ~IPV6_FLOWLABEL_MASK;
	in_pcbrehash(inp);
	lck_rw_done(inp->inp_pcbinfo->ipi_lock);
	  
                                                                        
                                                                     
                                                                        
    
	if (!(so->so_flags & SOF_MP_SUBFLOW) && (so->so_state & SS_NOFDREF)) {
		in6_pcbdetach(inp);
	}
}

void
in6_pcbdetach(struct inpcb *inp)
{
	struct socket *so = inp->inp_socket;

	if (so->so_pcb == NULL) {
		                           
		panic("%s: inp=%p so=%p proto=%d so_pcb is null!\n", __func__,
		    inp, so, SOCK_PROTO(so));
		                
	}

#if IPSEC
	if (inp->in6p_sp != NULL) {
		(void) ipsec6_delete_pcbpolicy(inp);
	}
#endif            

	if (inp->inp_stat != NULL && SOCK_PROTO(so) == IPPROTO_UDP) {
		if (inp->inp_stat->rxpackets == 0 && inp->inp_stat->txpackets == 0) {
			INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_inet6_dgram_no_data);
		}
	}

	  
                                                     
                        
    
	if (nstat_collect &&
	    (SOCK_PROTO(so) == IPPROTO_TCP || SOCK_PROTO(so) == IPPROTO_UDP)) {
		nstat_pcb_detach(inp);
	}
	                               
	if (in_pcb_checkstate(inp, WNT_STOPUSING, 1) != WNT_STOPUSING) {
		panic("%s: so=%p proto=%d couldn't set to STOPUSING\n",
		    __func__, so, SOCK_PROTO(so));
		                
	}

	if (!(so->so_flags & SOF_PCBCLEARING)) {
		struct ip_moptions *imo;
		struct ip6_moptions *im6o;

		inp->inp_vflag = 0;
		if (inp->in6p_options != NULL) {
			m_freem(inp->in6p_options);
			inp->in6p_options = NULL;
		}
		ip6_freepcbopts(inp->in6p_outputopts);
		inp->in6p_outputopts = NULL;
		ROUTE_RELEASE(&inp->in6p_route);
		                                                        
		if (inp->inp_options != NULL) {
			(void) m_free(inp->inp_options);
			inp->inp_options = NULL;
		}
		im6o = inp->in6p_moptions;
		inp->in6p_moptions = NULL;

		imo = inp->inp_moptions;
		inp->inp_moptions = NULL;

		sofreelastref(so, 0);
		inp->inp_state = INPCB_STATE_DEAD;
		                                                     
		so->so_flags |= SOF_PCBCLEARING;

		inpcb_gc_sched(inp->inp_pcbinfo, INPCB_TIMER_FAST);

		  
                                                   
     
		if (im6o != NULL || imo != NULL) {
			socket_unlock(so, 0);
			if (im6o != NULL) {
				IM6O_REMREF(im6o);
			}
			if (imo != NULL) {
				IMO_REMREF(imo);
			}
			socket_lock(so, 0);
		}
	}
}

struct sockaddr *
in6_sockaddr(in_port_t port, struct in6_addr *addr_p)
{
	struct sockaddr_in6 *sin6;

	MALLOC(sin6, struct sockaddr_in6 *, sizeof(*sin6), M_SONAME, M_WAITOK);
	if (sin6 == NULL) {
		return NULL;
	}
	bzero(sin6, sizeof(*sin6));
	sin6->sin6_family = AF_INET6;
	sin6->sin6_len = sizeof(*sin6);
	sin6->sin6_port = port;
	sin6->sin6_addr = *addr_p;

	                                                                  
	if (IN6_IS_SCOPE_LINKLOCAL(&sin6->sin6_addr)) {
		sin6->sin6_scope_id = ntohs(sin6->sin6_addr.s6_addr16[1]);
	} else {
		sin6->sin6_scope_id = 0;                 
	}
	if (IN6_IS_SCOPE_LINKLOCAL(&sin6->sin6_addr)) {
		sin6->sin6_addr.s6_addr16[1] = 0;
	}

	return (struct sockaddr *)sin6;
}

void
in6_sockaddr_s(in_port_t port, struct in6_addr *addr_p,
    struct sockaddr_in6 *sin6)
{
	bzero(sin6, sizeof(*sin6));
	sin6->sin6_family = AF_INET6;
	sin6->sin6_len = sizeof(*sin6);
	sin6->sin6_port = port;
	sin6->sin6_addr = *addr_p;

	                                                                  
	if (IN6_IS_SCOPE_LINKLOCAL(&sin6->sin6_addr)) {
		sin6->sin6_scope_id = ntohs(sin6->sin6_addr.s6_addr16[1]);
	} else {
		sin6->sin6_scope_id = 0;                 
	}
	if (IN6_IS_SCOPE_LINKLOCAL(&sin6->sin6_addr)) {
		sin6->sin6_addr.s6_addr16[1] = 0;
	}
}

  
                                                                        
                                                                       
                                                                           
                                           
   
int
in6_getsockaddr(struct socket *so, struct sockaddr **nam)
{
	struct inpcb *inp;
	struct in6_addr addr;
	in_port_t port;

	if ((inp = sotoinpcb(so)) == NULL) {
		return EINVAL;
	}

	port = inp->inp_lport;
	addr = inp->in6p_laddr;

	*nam = in6_sockaddr(port, &addr);
	if (*nam == NULL) {
		return ENOBUFS;
	}
	return 0;
}

int
in6_getsockaddr_s(struct socket *so, struct sockaddr_in6 *ss)
{
	struct inpcb *inp;
	struct in6_addr addr;
	in_port_t port;

	VERIFY(ss != NULL);
	bzero(ss, sizeof(*ss));

	if ((inp = sotoinpcb(so)) == NULL) {
		return EINVAL;
	}

	port = inp->inp_lport;
	addr = inp->in6p_laddr;

	in6_sockaddr_s(port, &addr, ss);
	return 0;
}

int
in6_getpeeraddr(struct socket *so, struct sockaddr **nam)
{
	struct inpcb *inp;
	struct in6_addr addr;
	in_port_t port;

	if ((inp = sotoinpcb(so)) == NULL) {
		return EINVAL;
	}

	port = inp->inp_fport;
	addr = inp->in6p_faddr;

	*nam = in6_sockaddr(port, &addr);
	if (*nam == NULL) {
		return ENOBUFS;
	}
	return 0;
}

int
in6_mapped_sockaddr(struct socket *so, struct sockaddr **nam)
{
	struct  inpcb *inp = sotoinpcb(so);
	int     error;

	if (inp == NULL) {
		return EINVAL;
	}
	if (inp->inp_vflag & INP_IPV4) {
		error = in_getsockaddr(so, nam);
		if (error == 0) {
			error = in6_sin_2_v4mapsin6_in_sock(nam);
		}
	} else {
		                                                        
		error = in6_getsockaddr(so, nam);
	}
	return error;
}

int
in6_mapped_peeraddr(struct socket *so, struct sockaddr **nam)
{
	struct  inpcb *inp = sotoinpcb(so);
	int     error;

	if (inp == NULL) {
		return EINVAL;
	}
	if (inp->inp_vflag & INP_IPV4) {
		error = in_getpeeraddr(so, nam);
		if (error == 0) {
			error = in6_sin_2_v4mapsin6_in_sock(nam);
		}
	} else {
		                                                        
		error = in6_getpeeraddr(so, nam);
	}
	return error;
}

  
                                                          
                                                                      
                                                                    
                                                                    
                                                           
                                                        
                                       
   
void
in6_pcbnotify(struct inpcbinfo *pcbinfo, struct sockaddr *dst, u_int fport_arg,
    const struct sockaddr *src, u_int lport_arg, int cmd, void *cmdarg,
    void (*notify)(struct inpcb *, int))
{
	struct inpcbhead *head = pcbinfo->ipi_listhead;
	struct inpcb *inp, *ninp;
	struct sockaddr_in6 sa6_src, *sa6_dst;
	uint16_t fport = (uint16_t)fport_arg, lport = (uint16_t)lport_arg;
	u_int32_t flowinfo;
	int errno;

	if ((unsigned)cmd >= PRC_NCMDS || dst->sa_family != AF_INET6) {
		return;
	}

	sa6_dst = (struct sockaddr_in6 *)(void *)dst;
	if (IN6_IS_ADDR_UNSPECIFIED(&sa6_dst->sin6_addr)) {
		return;
	}

	  
                                                                        
    
	sa6_src = (src == NULL) ?
	    sa6_any : *(struct sockaddr_in6 *)(uintptr_t)(size_t)src;
	flowinfo = sa6_src.sin6_flowinfo;

	  
                                                      
                                                       
                                                              
                                                        
                                                                  
                                
    
	if (PRC_IS_REDIRECT(cmd) || cmd == PRC_HOSTDEAD) {
		fport = 0;
		lport = 0;
		bzero((caddr_t)&sa6_src.sin6_addr, sizeof(sa6_src.sin6_addr));

		if (cmd != PRC_HOSTDEAD) {
			notify = in6_rtchange;
		}
	}
	errno = inet6ctlerrmap[cmd];
	lck_rw_lock_shared(pcbinfo->ipi_lock);
	for (inp = LIST_FIRST(head); inp != NULL; inp = ninp) {
		ninp = LIST_NEXT(inp, inp_list);

		if (!(inp->inp_vflag & INP_IPV6)) {
			continue;
		}

		  
                                                             
                                                                
                                                        
                                                          
                                                               
                          
                                                             
     
		if (cmd == PRC_MSGSIZE && cmdarg != NULL) {
			socket_lock(inp->inp_socket, 1);
			ip6_notify_pmtu(inp, (struct sockaddr_in6 *)(void *)dst,
			    (u_int32_t *)cmdarg);
			socket_unlock(inp->inp_socket, 1);
		}

		  
                                                           
                                                              
                                                            
                                                         
                                                        
              
     
		if (lport == 0 && fport == 0 && flowinfo &&
		    inp->inp_socket != NULL &&
		    flowinfo == (inp->inp_flow & IPV6_FLOWLABEL_MASK) &&
		    IN6_ARE_ADDR_EQUAL(&inp->in6p_laddr, &sa6_src.sin6_addr)) {
			goto do_notify;
		} else if (!IN6_ARE_ADDR_EQUAL(&inp->in6p_faddr,
		    &sa6_dst->sin6_addr) || inp->inp_socket == NULL ||
		    (lport && inp->inp_lport != lport) ||
		    (!IN6_IS_ADDR_UNSPECIFIED(&sa6_src.sin6_addr) &&
		    !IN6_ARE_ADDR_EQUAL(&inp->in6p_laddr,
		    &sa6_src.sin6_addr)) || (fport && inp->inp_fport != fport)) {
			continue;
		}

do_notify:
		if (notify) {
			if (in_pcb_checkstate(inp, WNT_ACQUIRE, 0) ==
			    WNT_STOPUSING) {
				continue;
			}
			socket_lock(inp->inp_socket, 1);
			(*notify)(inp, errno);
			(void) in_pcb_checkstate(inp, WNT_RELEASE, 1);
			socket_unlock(inp->inp_socket, 1);
		}
	}
	lck_rw_done(pcbinfo->ipi_lock);
}

  
                                                    
   
struct inpcb *
in6_pcblookup_local(struct inpcbinfo *pcbinfo, struct in6_addr *laddr,
    u_int lport_arg, int wild_okay)
{
	struct inpcb *inp;
	int matchwild = 3, wildcard;
	uint16_t lport = (uint16_t)lport_arg;
	struct inpcbporthead *porthash;
	struct inpcb *match = NULL;
	struct inpcbport *phd;

	if (!wild_okay) {
		struct inpcbhead *head;
		  
                                                             
                                                          
     
		head = &pcbinfo->ipi_hashbase[INP_PCBHASH(INADDR_ANY, lport, 0,
		    pcbinfo->ipi_hashmask)];
		LIST_FOREACH(inp, head, inp_hash) {
			if (!(inp->inp_vflag & INP_IPV6)) {
				continue;
			}
			if (IN6_IS_ADDR_UNSPECIFIED(&inp->in6p_faddr) &&
			    IN6_ARE_ADDR_EQUAL(&inp->in6p_laddr, laddr) &&
			    inp->inp_lport == lport) {
				  
             
       
				return inp;
			}
		}
		  
               
     
		return NULL;
	}
	  
                        
   
                                                            
                   
    
	porthash = &pcbinfo->ipi_porthashbase[INP_PCBPORTHASH(lport,
	    pcbinfo->ipi_porthashmask)];
	LIST_FOREACH(phd, porthash, phd_hash) {
		if (phd->phd_port == lport) {
			break;
		}
	}
	if (phd != NULL) {
		  
                                                      
         
     
		LIST_FOREACH(inp, &phd->phd_pcblist, inp_portlist) {
			wildcard = 0;
			if (!(inp->inp_vflag & INP_IPV6)) {
				continue;
			}
			if (!IN6_IS_ADDR_UNSPECIFIED(&inp->in6p_faddr)) {
				wildcard++;
			}
			if (!IN6_IS_ADDR_UNSPECIFIED(&inp->in6p_laddr)) {
				if (IN6_IS_ADDR_UNSPECIFIED(laddr)) {
					wildcard++;
				} else if (!IN6_ARE_ADDR_EQUAL(
					    &inp->in6p_laddr, laddr)) {
					continue;
				}
			} else {
				if (!IN6_IS_ADDR_UNSPECIFIED(laddr)) {
					wildcard++;
				}
			}
			if (wildcard < matchwild) {
				match = inp;
				matchwild = wildcard;
				if (matchwild == 0) {
					break;
				}
			}
		}
	}
	return match;
}

  
                                                     
                                                      
                                                             
                                                        
   
void
in6_losing(struct inpcb *in6p)
{
	struct rtentry *rt;

	if ((rt = in6p->in6p_route.ro_rt) != NULL) {
		RT_LOCK(rt);
		if (rt->rt_flags & RTF_DYNAMIC) {
			  
                                                   
                                                   
                                                 
      
			rt->rt_flags |= RTF_CONDEMNED;
			RT_UNLOCK(rt);
			(void) rtrequest(RTM_DELETE, rt_key(rt),
			    rt->rt_gateway, rt_mask(rt), rt->rt_flags, NULL);
		} else {
			RT_UNLOCK(rt);
		}
		  
                                 
                                       
     
	}
	ROUTE_RELEASE(&in6p->in6p_route);
}

  
                                            
                                         
   
void
in6_rtchange(struct inpcb *inp, int errno)
{
#pragma unused(errno)
	  
                                              
                        
    
	ROUTE_RELEASE(&inp->in6p_route);
}

  
                                                                    
   
int
in6_pcblookup_hash_exists(struct inpcbinfo *pcbinfo, struct in6_addr *faddr,
    u_int fport_arg, struct in6_addr *laddr, u_int lport_arg, int wildcard,
    uid_t *uid, gid_t *gid, struct ifnet *ifp)
{
	struct inpcbhead *head;
	struct inpcb *inp;
	uint16_t fport = (uint16_t)fport_arg, lport = (uint16_t)lport_arg;
	int found;

	*uid = UID_MAX;
	*gid = GID_MAX;

	lck_rw_lock_shared(pcbinfo->ipi_lock);

	  
                                  
    
	head = &pcbinfo->ipi_hashbase[INP_PCBHASH(faddr->s6_addr32[3]          ,
	    lport, fport, pcbinfo->ipi_hashmask)];
	LIST_FOREACH(inp, head, inp_hash) {
		if (!(inp->inp_vflag & INP_IPV6)) {
			continue;
		}

		if (inp_restricted_recv(inp, ifp)) {
			continue;
		}

#if NECP
		if (!necp_socket_is_allowed_to_recv_on_interface(inp, ifp)) {
			continue;
		}
#endif           

		if (IN6_ARE_ADDR_EQUAL(&inp->in6p_faddr, faddr) &&
		    IN6_ARE_ADDR_EQUAL(&inp->in6p_laddr, laddr) &&
		    inp->inp_fport == fport &&
		    inp->inp_lport == lport) {
			if ((found = (inp->inp_socket != NULL))) {
				  
                                         
       
				*uid = kauth_cred_getuid(
					inp->inp_socket->so_cred);
				*gid = kauth_cred_getgid(
					inp->inp_socket->so_cred);
			}
			lck_rw_done(pcbinfo->ipi_lock);
			return found;
		}
	}
	if (wildcard) {
		struct inpcb *local_wild = NULL;

		head = &pcbinfo->ipi_hashbase[INP_PCBHASH(INADDR_ANY, lport, 0,
		    pcbinfo->ipi_hashmask)];
		LIST_FOREACH(inp, head, inp_hash) {
			if (!(inp->inp_vflag & INP_IPV6)) {
				continue;
			}

			if (inp_restricted_recv(inp, ifp)) {
				continue;
			}

#if NECP
			if (!necp_socket_is_allowed_to_recv_on_interface(inp, ifp)) {
				continue;
			}
#endif           

			if (IN6_IS_ADDR_UNSPECIFIED(&inp->in6p_faddr) &&
			    inp->inp_lport == lport) {
				if (IN6_ARE_ADDR_EQUAL(&inp->in6p_laddr,
				    laddr)) {
					found = (inp->inp_socket != NULL);
					if (found) {
						*uid = kauth_cred_getuid(
							inp->inp_socket->so_cred);
						*gid = kauth_cred_getgid(
							inp->inp_socket->so_cred);
					}
					lck_rw_done(pcbinfo->ipi_lock);
					return found;
				} else if (IN6_IS_ADDR_UNSPECIFIED(
					    &inp->in6p_laddr)) {
					local_wild = inp;
				}
			}
		}
		if (local_wild) {
			if ((found = (local_wild->inp_socket != NULL))) {
				*uid = kauth_cred_getuid(
					local_wild->inp_socket->so_cred);
				*gid = kauth_cred_getgid(
					local_wild->inp_socket->so_cred);
			}
			lck_rw_done(pcbinfo->ipi_lock);
			return found;
		}
	}

	  
              
    
	lck_rw_done(pcbinfo->ipi_lock);
	return 0;
}

  
                           
   
struct inpcb *
in6_pcblookup_hash(struct inpcbinfo *pcbinfo, struct in6_addr *faddr,
    u_int fport_arg, struct in6_addr *laddr, u_int lport_arg, int wildcard,
    struct ifnet *ifp)
{
	struct inpcbhead *head;
	struct inpcb *inp;
	uint16_t fport = (uint16_t)fport_arg, lport = (uint16_t)lport_arg;

	lck_rw_lock_shared(pcbinfo->ipi_lock);

	  
                                  
    
	head = &pcbinfo->ipi_hashbase[INP_PCBHASH(faddr->s6_addr32[3]          ,
	    lport, fport, pcbinfo->ipi_hashmask)];
	LIST_FOREACH(inp, head, inp_hash) {
		if (!(inp->inp_vflag & INP_IPV6)) {
			continue;
		}

		if (inp_restricted_recv(inp, ifp)) {
			continue;
		}

#if NECP
		if (!necp_socket_is_allowed_to_recv_on_interface(inp, ifp)) {
			continue;
		}
#endif           

		if (IN6_ARE_ADDR_EQUAL(&inp->in6p_faddr, faddr) &&
		    IN6_ARE_ADDR_EQUAL(&inp->in6p_laddr, laddr) &&
		    inp->inp_fport == fport &&
		    inp->inp_lport == lport) {
			  
                                        
      
			if (in_pcb_checkstate(inp, WNT_ACQUIRE, 0) !=
			    WNT_STOPUSING) {
				lck_rw_done(pcbinfo->ipi_lock);
				return inp;
			} else {
				                                             
				lck_rw_done(pcbinfo->ipi_lock);
				return NULL;
			}
		}
	}
	if (wildcard) {
		struct inpcb *local_wild = NULL;

		head = &pcbinfo->ipi_hashbase[INP_PCBHASH(INADDR_ANY, lport, 0,
		    pcbinfo->ipi_hashmask)];
		LIST_FOREACH(inp, head, inp_hash) {
			if (!(inp->inp_vflag & INP_IPV6)) {
				continue;
			}

			if (inp_restricted_recv(inp, ifp)) {
				continue;
			}

#if NECP
			if (!necp_socket_is_allowed_to_recv_on_interface(inp, ifp)) {
				continue;
			}
#endif           

			if (IN6_IS_ADDR_UNSPECIFIED(&inp->in6p_faddr) &&
			    inp->inp_lport == lport) {
				if (IN6_ARE_ADDR_EQUAL(&inp->in6p_laddr,
				    laddr)) {
					if (in_pcb_checkstate(inp, WNT_ACQUIRE,
					    0) != WNT_STOPUSING) {
						lck_rw_done(pcbinfo->ipi_lock);
						return inp;
					} else {
						                              
						lck_rw_done(pcbinfo->ipi_lock);
						return NULL;
					}
				} else if (IN6_IS_ADDR_UNSPECIFIED(
					    &inp->in6p_laddr)) {
					local_wild = inp;
				}
			}
		}
		if (local_wild && in_pcb_checkstate(local_wild,
		    WNT_ACQUIRE, 0) != WNT_STOPUSING) {
			lck_rw_done(pcbinfo->ipi_lock);
			return local_wild;
		} else {
			lck_rw_done(pcbinfo->ipi_lock);
			return NULL;
		}
	}

	  
              
    
	lck_rw_done(pcbinfo->ipi_lock);
	return NULL;
}

void
init_sin6(struct sockaddr_in6 *sin6, struct mbuf *m)
{
	struct ip6_hdr *ip;

	ip = mtod(m, struct ip6_hdr *);
	bzero(sin6, sizeof(*sin6));
	sin6->sin6_len = sizeof(*sin6);
	sin6->sin6_family = AF_INET6;
	sin6->sin6_addr = ip->ip6_src;
	if (IN6_IS_SCOPE_LINKLOCAL(&sin6->sin6_addr)) {
		sin6->sin6_addr.s6_addr16[1] = 0;
		if ((m->m_pkthdr.pkt_flags & (PKTF_LOOP | PKTF_IFAINFO)) ==
		    (PKTF_LOOP | PKTF_IFAINFO)) {
			sin6->sin6_scope_id = m->m_pkthdr.src_ifindex;
		} else if (m->m_pkthdr.rcvif != NULL) {
			sin6->sin6_scope_id = m->m_pkthdr.rcvif->if_index;
		}
	}
}

  
                                                
  
                                                                           
                                                                           
                                                                          
                                                                           
                                                                          
                                                                           
                                                   
  
                                                                           
                                                                              
                                                                          
                                                                          
                                                            
  
                                                                          
                                                                             
                                                                             
                                                                            
                                                                             
                                                                           
                                                                           
                                  
   
void
in6p_route_copyout(struct inpcb *inp, struct route_in6 *dst)
{
	struct route_in6 *src = &inp->in6p_route;

	socket_lock_assert_owned(inp->inp_socket);

	                        
	if (src->ro_rt != NULL && rt_key(src->ro_rt)->sa_family != AF_INET6) {
		panic("%s: wrong or corrupted route: %p", __func__, src);
	}

	route_copyout((struct route *)dst, (struct route *)src, sizeof(*dst));
}

void
in6p_route_copyin(struct inpcb *inp, struct route_in6 *src)
{
	struct route_in6 *dst = &inp->in6p_route;

	socket_lock_assert_owned(inp->inp_socket);

	                        
	if (src->ro_rt != NULL && rt_key(src->ro_rt)->sa_family != AF_INET6) {
		panic("%s: wrong or corrupted route: %p", __func__, src);
	}

	route_copyin((struct route *)src, (struct route *)dst, sizeof(*src));
}
