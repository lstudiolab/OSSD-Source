/*
 * Copyright (c) 2008-2016, 2020 Apple Inc. All rights reserved.
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
/* inftrees.c -- generate Huffman trees for efficient decoding
 * Copyright (C) 1995-2005 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "zutil.h"
#include "inftrees.h"

#define MAXBITS 15

const char inflate_copyright[] =
   " inflate 1.2.3 Copyright 1995-2005 Mark Adler ";
/*
  If you use the zlib library in a product, an acknowledgment is welcome
  in the documentation of your product. If for some reason you cannot
  include such an acknowledgment, I would appreciate that you keep this
  copyright string in the executable of your product.
 */

  
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
                                                        
   
int
inflate_table(codetype type, unsigned short FAR *lens, unsigned codes,
	      code FAR * FAR *table, unsigned FAR *bits,
	      unsigned short FAR *work)
{
    unsigned len;                                            
    unsigned sym;                                          
    unsigned min, max;                                                
    unsigned root;                                                       
    unsigned curr;                                                          
    unsigned drop;                                                   
    int left;                                                         
    unsigned used;                                              
    unsigned huff;                                
    unsigned incr;                                                
    unsigned fill;                                                 
    unsigned low;                                                    
    unsigned mask;                                          
    code this;                                                   
    code FAR *next;                                                
    const unsigned short FAR *base;                                  
    const unsigned short FAR *extra;                                 
    unsigned match;                                                         
    unsigned short count[MAXBITS+1];                                        
    unsigned short offs[MAXBITS+1];                                           
    static const unsigned short lbase[31] = {                                 
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
    static const unsigned short lext[31] = {                                  
        16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18,
        19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 16, 201, 196};
    static const unsigned short dbase[32] = {                                
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577, 0, 0};
    static const unsigned short dext[32] = {                                 
        16, 16, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
        23, 23, 24, 24, 25, 25, 26, 26, 27, 27,
        28, 28, 29, 29, 64, 64};

      
                                                                             
                                                                         
                                                                              
                                                                           
                                                                             
                                                                           
                                                                           
                                                                          
                                                                          
                                                                           
                                 

                                                                           
                                                                        
                                                                           
                                          

                                                                          
                                                                            
                                                                         
                                                                            
                  

                                                                          
                                                                         
                                                                           
                                                                      
                       
       

                                                                         
    for (len = 0; len <= MAXBITS; len++)
        count[len] = 0;
    for (sym = 0; sym < codes; sym++)
        count[lens[sym]]++;

                                                                  
    root = *bits;
    for (max = MAXBITS; max >= 1; max--)
        if (count[max] != 0) break;
    if (root > max) root = max;
    if (max == 0) {                                                    
        this.op = (unsigned char)64;                             
        this.bits = (unsigned char)1;
        this.val = (unsigned short)0;
        *(*table)++ = this;                                                 
        *(*table)++ = this;
        *bits = 1;
        return 0;                                                            
    }
    for (min = 1; min <= MAXBITS; min++)
        if (count[min] != 0) break;
    if (root < min) root = min;

                                                                   
    left = 1;
    for (len = 1; len <= MAXBITS; len++) {
        left <<= 1;
        left -= count[len];
        if (left < 0) return -1;                             
    }
    if (left > 0 && (type == CODES || max != 1))
        return -1;                                          

                                                                        
    offs[1] = 0;
    for (len = 1; len < MAXBITS; len++)
        offs[len + 1] = offs[len] + count[len];

                                                                    
    for (sym = 0; sym < codes; sym++)
        if (lens[sym] != 0) work[offs[lens[sym]]++] = (unsigned short)sym;

      
                                                                         
                                                                              
                                                                            
                                                                             
                                                                             
                                              

                                                                             
                                                                              
                                                                              
                                                                            
                                                                       

                                                                           
                                                                           
                                                                           
                             

                                                                              
                                                                            
                                                                             
                                                                             
                                                                               
                                                      

                                                                       
                                                                          
                                                                             
                                                                    
       

                              
    switch (type) {
    case CODES:
        base = extra = work;                               
        match = 20;
        break;
    case LENS:
        base = lbase;
        extra = lext;
        match = 257;
        break;
    default:                       
        base = dbase;
        extra = dext;
        match = 0;
    }

                                   
    huff = 0;                                      
    sym = 0;                                              
    len = min;                                            
    next = *table;                                            
    curr = root;                                              
    drop = 0;                                                                 
    low = (unsigned)(-1);                                                  
    used = 1U << root;                                      
    mask = used - 1;                                        

                                     
    if (type == LENS && used >= ENOUGH - MAXD)
        return 1;

                                                  
    for (;;) {
                                
        this.bits = (unsigned char)(len - drop);
        if (work[sym] + 1 < match) {
            this.op = (unsigned char)0;
            this.val = work[sym];
        }
        else if (work[sym] >= match) {
            this.op = (unsigned char)(extra[work[sym] - match]);
            this.val = base[work[sym] - match];
        }
        else {
            this.op = (unsigned char)(32 + 64);                           
            this.val = 0;
        }

                                                                         
        incr = 1U << (len - drop);
        fill = 1U << curr;
        min = fill;                                                
        do {
            fill -= incr;
            next[(huff >> drop) + fill] = this;
        } while (fill != 0);

                                                       
        incr = 1U << (len - 1);
        while (huff & incr)
            incr >>= 1;
        if (incr != 0) {
            huff &= incr - 1;
            huff += incr;
        }
        else
            huff = 0;

                                                  
        sym++;
        if (--(count[len]) == 0) {
            if (len == max) break;
            len = lens[work[sym]];
        }

                                            
        if (len > root && (huff & mask) != low) {
                                                         
            if (drop == 0)
                drop = root;

                                           
            next += min;                                       

                                                
            curr = len - drop;
            left = (int)(1 << curr);
            while (curr + drop < max) {
                left -= count[curr + drop];
                if (left <= 0) break;
                curr++;
                left <<= 1;
            }

                                        
            used += 1U << curr;
            if (type == LENS && used >= ENOUGH - MAXD)
                return 1;

                                                        
            low = huff & mask;
            (*table)[low].op = (unsigned char)curr;
            (*table)[low].bits = (unsigned char)root;
            (*table)[low].val = (unsigned short)(next - *table);
        }
    }

      
                                                                               
                                                                             
                                                                           
                                                                               
                                                                           
       
    this.op = (unsigned char)64;                                         
    this.bits = (unsigned char)(len - drop);
    this.val = (unsigned short)0;
    while (huff != 0) {
                                                               
        if (drop != 0 && (huff & mask) != low) {
            drop = 0;
            len = root;
            next = *table;
            this.bits = (unsigned char)len;
        }

                                              
        next[huff >> drop] = this;

                                                       
        incr = 1U << (len - 1);
        while (huff & incr)
            incr >>= 1;
        if (incr != 0) {
            huff &= incr - 1;
            huff += incr;
        }
        else
            huff = 0;
    }

                               
    *table += used;
    *bits = root;
    return 0;
}
