/*
 * Copyright (c) 2000-2016 Apple Inc. All rights reserved.
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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1994 Gordon Ross, Adam Glass
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 *	California, Lawrence Berkeley Laboratory and its contributors.
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

#include <nfs/nfs_conf.h>
#if CONFIG_NFS_CLIENT

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/kpi_mbuf.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/systm.h>
#include <sys/reboot.h>

#include <net/if.h>
#include <netinet/in.h>

#include <nfs/rpcv2.h>
#include <nfs/krpc.h>

  
                             
  
                                                                   
  
                                                          
                                        
   

  
                      
   

struct auth_info {
	u_int32_t       rp_atype;                      
	u_int32_t       rp_alen;                         
};

struct rpc_call {
	u_int32_t       rp_xid;                                     
	int32_t         rp_direction;                           
	u_int32_t       rp_rpcvers;                          
	u_int32_t       rp_prog;                     
	u_int32_t       rp_vers;                     
	u_int32_t       rp_proc;                       
	struct  auth_info rp_auth;
	struct  auth_info rp_verf;
};

struct rpc_reply {
	u_int32_t rp_xid;                                           
	int32_t  rp_direction;                                  
	int32_t  rp_astatus;                                             
	union {
		u_int32_t rpu_errno;
		struct {
			struct auth_info rp_auth;
			u_int32_t       rp_rstatus;
		} rpu_ok;
	} rp_u;
};

#define MIN_REPLY_HDR 16                                    
#define REPLY_SIZE 24                                        

  
                                                                
                                                             
                                                              
                                                            
   
#define MAX_RESEND_DELAY 5                   

                                                                            
#define IP_FORMAT       "%d.%d.%d.%d"
#define IP_CH(ip)       ((u_char *)ip)
#define IP_LIST(ip)     IP_CH(ip)[0],IP_CH(ip)[1],IP_CH(ip)[2],IP_CH(ip)[3]


  
                                                                    
                                     
   
int
krpc_portmap(
	struct sockaddr_in *sin,                            
	u_int prog, u_int vers, u_int proto,                    
	u_int16_t *portp)                                  
{
	struct sdata {
		u_int32_t prog;                           
		u_int32_t vers;                           
		u_int32_t proto;                           
		u_int32_t port;                                 
	} *sdata;
	struct rdata {
		u_int16_t pad;
		u_int16_t port;
	} *rdata;
	mbuf_t m;
	int error;

	                                   
	if (prog == PMAPPROG) {
		*portp = htons(PMAPPORT);
		return 0;
	}

	error = mbuf_gethdr(MBUF_WAITOK, MBUF_TYPE_DATA, &m);
	if (error) {
		return error;
	}
	mbuf_setlen(m, sizeof(*sdata));
	mbuf_pkthdr_setlen(m, sizeof(*sdata));
	sdata = mbuf_data(m);

	                           
	sdata->prog = htonl(prog);
	sdata->vers = htonl(vers);
	sdata->proto = htonl(proto);
	sdata->port = 0;

	sin->sin_port = htons(PMAPPORT);
	error = krpc_call(sin, SOCK_DGRAM, PMAPPROG, PMAPVERS, PMAPPROC_GETPORT, &m, NULL);
	if (error) {
		return error;
	}

	rdata = mbuf_data(m);

	if (mbuf_len(m) >= sizeof(*rdata)) {
		*portp = rdata->port;
	}

	if (mbuf_len(m) < sizeof(*rdata) || !rdata->port) {
		error = EPROGUNAVAIL;
	}

	mbuf_freem(m);
	return error;
}

  
                                                           
                                                          
                                                            
   
int
krpc_call(
	struct sockaddr_in *sa,
	u_int sotype, u_int prog, u_int vers, u_int func,
	mbuf_t *data,                                     
	struct sockaddr_in *from_p)                 
{
	socket_t so;
	struct sockaddr_in *sin;
	mbuf_t m, nam, mhead;
	struct rpc_call *call;
	struct rpc_reply *reply;
	int error, timo, secs;
	size_t len;
	static u_int32_t xid = ~0xFF;
	u_int16_t tport;
	size_t maxpacket = 1 << 16;

	  
                            
                                   
    
	if (sa->sin_family != AF_INET) {
		return EAFNOSUPPORT;
	}

	                              
	nam = mhead = NULL;

	  
                                              
    
	if ((error = sock_socket(AF_INET, sotype, 0, 0, 0, &so))) {
		goto out1;
	}

	{
		struct timeval tv;

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		if ((error = sock_setsockopt(so, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)))) {
			goto out;
		}
	}

	  
                                  
    

	if (from_p && (sotype == SOCK_DGRAM)) {
		int on = 1;
		if ((error = sock_setsockopt(so, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)))) {
			goto out;
		}
	}

	  
                                               
                                                 
                                        
    
	if ((error = mbuf_get(MBUF_WAITOK, MBUF_TYPE_SONAME, &m))) {
		goto out;
	}
	sin = mbuf_data(m);
	bzero(sin, sizeof(*sin));
	mbuf_setlen(m, sizeof(*sin));
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	tport = IPPORT_RESERVED;
	do {
		tport--;
		sin->sin_port = htons(tport);
		error = sock_bind(so, (struct sockaddr*)sin);
	} while (error == EADDRINUSE &&
	    tport > IPPORT_RESERVED / 2);
	mbuf_freem(m);
	m = NULL;
	if (error) {
		printf("bind failed\n");
		goto out;
	}

	  
                                        
    
	if ((error = mbuf_get(MBUF_WAITOK, MBUF_TYPE_SONAME, &nam))) {
		goto out;
	}
	sin = mbuf_data(nam);
	mbuf_setlen(nam, sa->sin_len);
	bcopy((caddr_t)sa, (caddr_t)sin, sa->sin_len);

	if (sotype == SOCK_STREAM) {
		struct timeval tv;
		tv.tv_sec = 60;
		tv.tv_usec = 0;
		error = sock_connect(so, mbuf_data(nam), MSG_DONTWAIT);
		if (error && (error != EINPROGRESS)) {
			goto out;
		}
		error = sock_connectwait(so, &tv);
		if (error) {
			if (error == EINPROGRESS) {
				error = ETIMEDOUT;
			}
			printf("krpc_call: error waiting for TCP socket connect: %d\n", error);
			goto out;
		}
	}

	  
                               
    
	m = *data;
	*data = NULL;
#if     DIAGNOSTIC
	if ((mbuf_flags(m) & MBUF_PKTHDR) == 0) {
		panic("krpc_call: send data w/o pkthdr");
	}
	if (mbuf_pkthdr_len(m) < mbuf_len(m)) {
		panic("krpc_call: pkthdr.len not set");
	}
#endif
	len = sizeof(*call);
	if (sotype == SOCK_STREAM) {
		len += 4;                                     
	}
	mhead = m;
	if ((error = mbuf_prepend(&mhead, len, MBUF_WAITOK))) {
		goto out;
	}
	if ((error = mbuf_pkthdr_setrcvif(mhead, NULL))) {
		goto out;
	}

	  
                          
    
	if (sotype == SOCK_STREAM) {
		                                      
		u_int32_t *recmark = mbuf_data(mhead);
		size_t pkthdr_len = mbuf_pkthdr_len(mhead);
		assert(pkthdr_len <= UINT32_MAX);
		*recmark = htonl(0x80000000 | (uint32_t)(pkthdr_len - 4));
		call = (struct rpc_call *)(recmark + 1);
	} else {
		call = mbuf_data(mhead);
	}
	bzero((caddr_t)call, sizeof(*call));
	xid++;
	call->rp_xid = htonl(xid);
	                             
	call->rp_rpcvers = htonl(2);
	call->rp_prog = htonl(prog);
	call->rp_vers = htonl(vers);
	call->rp_proc = htonl(func);
	                        
	                        

	  
                                                   
                                                   
                                                     
    
	timo = 0;
	for (;;) {
		struct msghdr msg;

		                                    
		if ((error = mbuf_copym(mhead, 0, MBUF_COPYALL, MBUF_WAITOK, &m))) {
			goto out;
		}
		bzero(&msg, sizeof(msg));
		if (sotype == SOCK_STREAM) {
			msg.msg_name = NULL;
			msg.msg_namelen = 0;
		} else {
			msg.msg_name = mbuf_data(nam);
			if (mbuf_len(nam) > UINT_MAX) {
				printf("krpc_call: mbuf_len is too long: EINVAL\n");
				error = EINVAL;
				goto out;
			}
			msg.msg_namelen = (uint32_t)mbuf_len(nam);
		}
		error = sock_sendmbuf(so, &msg, m, 0, 0);
		if (error) {
			printf("krpc_call: sosend: %d\n", error);
			goto out;
		}
		m = NULL;

		                            
		if (timo < MAX_RESEND_DELAY) {
			timo++;
		} else {
			printf("RPC timeout for server " IP_FORMAT "\n",
			    IP_LIST(&(sin->sin_addr.s_addr)));
		}

		  
                                             
                                                    
     
		secs = timo;
		while (secs > 0) {
			size_t readlen;

			if (m) {
				mbuf_freem(m);
				m = NULL;
			}
			if (sotype == SOCK_STREAM) {
				int maxretries = 60;
				struct iovec aio;
				aio.iov_base = &len;
				aio.iov_len = sizeof(u_int32_t);
				bzero(&msg, sizeof(msg));
				msg.msg_iov = &aio;
				msg.msg_iovlen = 1;
				do {
					error = sock_receive(so, &msg, MSG_WAITALL, &readlen);
					if ((error == EWOULDBLOCK) && (--maxretries <= 0)) {
						error = ETIMEDOUT;
					}
				} while (error == EWOULDBLOCK);
				if (!error && readlen < aio.iov_len) {
					                                                 
					if (readlen != 0) {
						printf("short receive (%ld/%ld) from server " IP_FORMAT "\n",
						    readlen, sizeof(u_int32_t), IP_LIST(&(sin->sin_addr.s_addr)));
					}
					error = EPIPE;
				}
				if (error) {
					goto out;
				}
				len = ntohll(len) & ~0x80000000;
				  
                                                          
                                                          
       
				if (len > maxpacket) {
					printf("impossible packet length (%ld) from server " IP_FORMAT "\n",
					    len, IP_LIST(&(sin->sin_addr.s_addr)));
					error = EFBIG;
					goto out;
				}

				do {
					readlen = len;
					error = sock_receivembuf(so, NULL, &m, MSG_WAITALL, &readlen);
				} while (error == EWOULDBLOCK);

				if (!error && (len > readlen)) {
					printf("short receive (%ld/%ld) from server " IP_FORMAT "\n",
					    readlen, len, IP_LIST(&(sin->sin_addr.s_addr)));
					error = EPIPE;
				}
			} else {
				len = maxpacket;
				readlen = len;
				bzero(&msg, sizeof(msg));
				msg.msg_name = from_p;
				msg.msg_namelen = (from_p == NULL) ? 0 : sizeof(*from_p);
				error = sock_receivembuf(so, &msg, &m, 0, &readlen);
			}

			if (error == EWOULDBLOCK) {
				secs--;
				continue;
			}
			if (error) {
				goto out;
			}
			len = readlen;

			                                               
			if (len < MIN_REPLY_HDR) {
				continue;
			}
			if (mbuf_len(m) < MIN_REPLY_HDR) {
				continue;
			}
			reply = mbuf_data(m);

			                            
			if (reply->rp_direction != htonl(RPC_REPLY)) {
				continue;
			}

			if (reply->rp_xid != htonl(xid)) {
				continue;
			}

			                                          
			if (reply->rp_astatus != 0) {
				error = ntohl(reply->rp_u.rpu_errno);
				printf("rpc denied, error=%d\n", error);
				                                
				switch (error) {
				case RPC_MISMATCH:
					error = ERPCMISMATCH;
					break;
				case RPC_AUTHERR:
					error = EAUTH;
					break;
				}
				goto out;
			}


			if (mbuf_len(m) < REPLY_SIZE) {
				error = RPC_SYSTEM_ERR;
			} else {
				error = ntohl(reply->rp_u.rpu_ok.rp_rstatus);
			}

			                           
			if (error != 0) {
				printf("rpc status=%d\n", error);
				                                
				switch (error) {
				case RPC_PROGUNAVAIL:
					error = EPROGUNAVAIL;
					break;
				case RPC_PROGMISMATCH:
					error = EPROGMISMATCH;
					break;
				case RPC_PROCUNAVAIL:
					error = EPROCUNAVAIL;
					break;
				case RPC_GARBAGE:
					error = EINVAL;
					break;
				case RPC_SYSTEM_ERR:
					error = EIO;
					break;
				}
				goto out;
			}

			goto gotreply;                        
		}                 
	}                           

	error = ETIMEDOUT;
	goto out;

gotreply:

	  
                                                   
                                                      
                                                       
                                                    
                                          
    
#if     DIAGNOSTIC
	if ((mbuf_flags(m) & MBUF_PKTHDR) == 0) {
		panic("krpc_call: received pkt w/o header?");
	}
#endif
	len = mbuf_pkthdr_len(m);
	if (sotype == SOCK_STREAM) {
		len -= 4;                                                 
	}
	if (mbuf_len(m) < len) {
		if ((error = mbuf_pullup(&m, len))) {
			goto out;
		}
		reply = mbuf_data(m);
	}

	  
                    
    
	len = sizeof(*reply);
	if (reply->rp_u.rpu_ok.rp_auth.rp_atype != 0) {
		len += ntohl(reply->rp_u.rpu_ok.rp_auth.rp_alen);
		len = (len + 3) & ~3;           
	}

	if (len > INT_MAX) {
		error = EINVAL;
		goto out;
	}

	mbuf_adj(m, (int)len);

	            
	*data = m;
out:
	sock_close(so);
out1:
	if (nam) {
		mbuf_freem(nam);
	}
	if (mhead) {
		mbuf_freem(mhead);
	}
	return error;
}

#endif                        
