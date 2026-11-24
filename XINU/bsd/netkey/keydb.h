                                                           

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

#ifndef _NETKEY_KEYDB_H_
#define _NETKEY_KEYDB_H_
#include <sys/appleapiopts.h>

#ifdef BSD_KERNEL_PRIVATE

#include <netkey/key_var.h>

                                
                                            
struct secasindex {
	struct sockaddr_storage src;                               
	struct sockaddr_storage dst;                                    
	u_int16_t proto;                                               
	u_int8_t mode;                                                     
	u_int32_t reqid;                                                
	                                                                 
	u_int ipsec_ifindex;
};

#define SECURITY_ASSOCIATION_ANY          0x0000
#define SECURITY_ASSOCIATION_PFKEY        0x0001
#define SECURITY_ASSOCIATION_CUSTOM_IPSEC 0x0010

                                    
struct secashead {
	LIST_ENTRY(secashead) chain;

	struct secasindex saidx;

	ifnet_t ipsec_if;
	u_int outgoing_if;
	u_int8_t dir;                                                                
	u_int8_t state;                                      
	LIST_HEAD(_satree, secasvar) savtree[SADB_SASTATE_MAX + 1];
	              
	                                        

	struct route_in6 sa_route;                               

	uint16_t flags;
	u_int32_t use_count;
};

#define MAX_REPLAY_WINDOWS 4

                          
struct secasvar {
	LIST_ENTRY(secasvar) chain;
	LIST_ENTRY(secasvar) spihash;
	int refcnt;                                          
	u_int8_t state;                                                 

	u_int8_t alg_auth;                                                      
	u_int8_t alg_enc;                                                
	u_int32_t spi;                                                     
	u_int32_t flags;                                               
	u_int16_t flags2;                                                  

	struct sadb_key *key_auth;                                  
	struct sadb_key *key_enc;                               
	caddr_t iv;                                               
	u_int ivlen;                                      
	void *sched;                                                     
	size_t schedlen;

	struct secreplay *replay[MAX_REPLAY_WINDOWS];                        

	long created;                                     

	struct sadb_lifetime *lft_c;                                          
	struct sadb_lifetime *lft_h;                       
	struct sadb_lifetime *lft_s;                       

	struct socket *so;                        

	u_int32_t seq;                                       
	pid_t pid;                                         

	struct secashead *sah;                                             

	                                
	u_int64_t       natt_last_activity;
	u_int16_t       remote_ike_port;
	u_int16_t       natt_encapsulated_src_port;                             
	u_int16_t       natt_interval;                          
	u_int16_t       natt_offload_interval;                                           

	u_int8_t        always_expire;                                                 
};

                       
struct secreplay {
	u_int8_t wsize;                            
	u_int32_t count;
	u_int32_t seq;                              
	u_int32_t lastseq;                                   
	caddr_t bitmap;                               
	int overflow;                              
};

                                               
struct secreg {
	LIST_ENTRY(secreg) chain;

	struct socket *so;
};

#ifndef IPSEC_NONBLOCK_ACQUIRE
                           
struct secacq {
	LIST_ENTRY(secacq) chain;

	struct secasindex saidx;

	u_int32_t seq;                               
	long created;                             
	int count;                                
};
#endif

                                     
             

#define SADB_KILL_INTERVAL      600                      

struct key_cb {
	int key_count;
	int any_count;
};

               
extern struct secpolicy *keydb_newsecpolicy(void);
extern void keydb_delsecpolicy(struct secpolicy *);
               
extern struct secashead *keydb_newsecashead(void);
                                                                  
              
                                                                
                                                                
                                                                 
               
extern struct secreplay *keydb_newsecreplay(u_int8_t);
extern void keydb_delsecreplay(struct secreplay *);
            
                                                             
                                                             

#endif                         

#endif                       
