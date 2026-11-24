/*
 * Copyright (c) 2009-2019 Apple Inc. All rights reserved.
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

/*-
 * Copyright (C) 2003 WIDE Project.
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

/*-
 * Copyright 1998 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Theo de Raadt <deraadt@openbsd.org> came up with the idea of using
 * such a mathematical system to generate more random (yet non-repeating)
 * ids to solve the resolver/named problem.  But Niels designed the
 * actual system based on the constraints.
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
 *      This product includes software developed by Niels Provos.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

  
                               
                                  
                                     
                                            
  
                      
                                                             
                                  
                                     
                                                 
  
                                       
                                  
  
                                                                     
                                                                
                                                
   

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/random.h>
#include <sys/protosw.h>
#include <libkern/libkern.h>
#include <dev/random/randomdev.h>

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>

struct randomtab {
	const int       ru_bits;                     
	const long      ru_out;                                       
	const u_int32_t ru_max;                                             
	const u_int32_t ru_gen;                         
	const u_int32_t ru_n;                                                   
	const u_int32_t ru_agen;                                         
	const u_int32_t ru_m;                       
	const u_int32_t pfacts[4];                           

	u_int32_t ru_counter;
	u_int32_t ru_msb;

	u_int32_t ru_x;
	u_int32_t ru_seed, ru_seed2;
	u_int32_t ru_a, ru_b;
	u_int32_t ru_g;
	long ru_reseed;
};

static struct randomtab randomtab_32 = {
	.ru_bits = 32,                              
	.ru_out = 180,                                                
	.ru_max = 1000000000,                                               
	.ru_gen = 2,                                    
	.ru_n = 2147483629,                                    
	.ru_agen = 7,                                                   
	.ru_m = 1836660096,                                         
	.pfacts = { 2, 3, 59652323, 0 },                             
	.ru_counter = 0,
	.ru_msb = 0,
	.ru_x = 0,
	.ru_seed = 0,
	.ru_seed2 = 0,
	.ru_a = 0,
	.ru_b = 0,
	.ru_g = 0,
	.ru_reseed = 0
};

static u_int32_t pmod(u_int32_t, u_int32_t, u_int32_t);
static void initid(struct randomtab *);
static u_int32_t randomid(struct randomtab *);

  
                                                                    
                 
   
static u_int32_t
pmod(u_int32_t gen, u_int32_t expo, u_int32_t mod)
{
	u_int64_t s, t, u;

	s = 1;
	t = gen;
	u = expo;

	while (u) {
		if (u & 1) {
			s = (s * t) % mod;
		}
		u >>= 1;
		t = (t * t) % mod;
	}
	return (u_int32_t)s;
}

  
                                                                     
                                                              
                                                           
  
                                                             
                                               
   
static void
initid(struct randomtab *p)
{
	time_t curtime = (time_t)net_uptime();
	u_int32_t j, i;
	int noprime = 1;

	p->ru_x = RandomULong() % p->ru_m;

	                                    
	p->ru_seed = RandomULong() & (~0U >> (32 - p->ru_bits + 1));
	p->ru_seed2 = RandomULong() & (~0U >> (32 - p->ru_bits + 1));

	                              
	p->ru_b = (RandomULong() & (~0U >> (32 - p->ru_bits))) | 1;
	p->ru_a = pmod(p->ru_agen,
	    (RandomULong() & (~0U >> (32 - p->ru_bits))) & (~1U), p->ru_m);
	while (p->ru_b % 3 == 0) {
		p->ru_b += 2;
	}

	j = RandomULong() % p->ru_n;

	  
                                                       
                                                     
                     
    
	while (noprime) {
		for (i = 0; p->pfacts[i] > 0; i++) {
			if (j % p->pfacts[i] == 0) {
				break;
			}
		}

		if (p->pfacts[i] == 0) {
			noprime = 0;
		} else {
			j = (j + 1) % p->ru_n;
		}
	}

	p->ru_g = pmod(p->ru_gen, j, p->ru_n);
	p->ru_counter = 0;

	p->ru_reseed = curtime + p->ru_out;
	p->ru_msb = p->ru_msb ? 0 : (1U << (p->ru_bits - 1));
}

static u_int32_t
randomid(struct randomtab *p)
{
	time_t curtime = (time_t)net_uptime();
	int i, n;
	u_int32_t tmp;

	if (p->ru_counter >= p->ru_max || curtime > p->ru_reseed) {
		initid(p);
	}

	tmp = RandomULong();

	                                 
	n = tmp & 0x3; tmp = tmp >> 2;
	if (p->ru_counter + n >= p->ru_max) {
		initid(p);
	}

	for (i = 0; i <= n; i++) {
		                                   
		p->ru_x = ((u_int64_t)p->ru_a * p->ru_x + p->ru_b) % p->ru_m;
	}

	p->ru_counter += i;

	return (p->ru_seed ^ pmod(p->ru_g, p->ru_seed2 ^ p->ru_x, p->ru_n)) |
	       p->ru_msb;
}

u_int32_t
ip6_randomid(void)
{
	return randomid(&randomtab_32);
}

u_int32_t
ip6_randomflowlabel(void)
{
	return RandomULong() & IPV6_FLOWLABEL_MASK;
}
