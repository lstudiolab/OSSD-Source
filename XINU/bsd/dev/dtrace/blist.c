/*
 * BLIST.C -	Bitmap allocator/deallocator, using a radix tree with hinting
 *
 *	(c)Copyright 1998, Matthew Dillon.  Terms for use and redistribution
 *	are covered by the BSD Copyright as found in /usr/src/COPYRIGHT.
 *
 *	This module implements a general bitmap allocator/deallocator.  The
 *	allocator eats around 2 bits per 'block'.  The module does not
 *	try to interpret the meaning of a 'block' other then to return
 *	SWAPBLK_NONE on an allocation failure.
 *
 *	A radix tree is used to maintain the bitmap.  Two radix constants are
 *	involved:  One for the bitmaps contained in the leaf nodes (typically
 *	32), and one for the meta nodes (typically 16).  Both meta and leaf
 *	nodes have a hint field.  This field gives us a hint as to the largest
 *	free contiguous range of blocks under the node.  It may contain a
 *	value that is too high, but will never contain a value that is too
 *	low.  When the radix tree is searched, allocation failures in subtrees
 *	update the hint.
 *
 *	The radix tree also implements two collapsed states for meta nodes:
 *	the ALL-ALLOCATED state and the ALL-FREE state.  If a meta node is
 *	in either of these two states, all information contained underneath
 *	the node is considered stale.  These states are used to optimize
 *	allocation and freeing operations.
 *
 *      The hinting greatly increases code efficiency for allocations while
 *	the general radix structure optimizes both allocations and frees.  The
 *	radix tree should be able to operate well no matter how much
 *	fragmentation there is and no matter how large a bitmap is used.
 *
 *	Unlike the rlist code, the blist code wires all necessary memory at
 *	creation time.  Neither allocations nor frees require interaction with
 *	the memory subsystem.  In contrast, the rlist code may allocate memory
 *	on an rlist_free() call.  The non-blocking features of the blist code
 *	are used to great advantage in the swap code (vm/nswap_pager.c).  The
 *	rlist code uses a little less overall memory then the blist code (but
 *	due to swap interleaving not all that much less), but the blist code
 *	scales much, much better.
 *
 *	LAYOUT: The radix tree is layed out recursively using a
 *	linear array.  Each meta node is immediately followed (layed out
 *	sequentially in memory) by BLIST_META_RADIX lower level nodes.  This
 *	is a recursive structure but one that can be easily scanned through
 *	a very simple 'skip' calculation.  In order to support large radixes,
 *	portions of the tree may reside outside our memory allocation.  We
 *	handle this with an early-termination optimization (when bighint is
 *	set to -1) on the scan.  The memory allocation is only large enough
 *	to cover the number of blocks requested at creation time even if it
 *	must be encompassed in larger root-node radix.
 *
 *	NOTE: the allocator cannot currently allocate more then
 *	BLIST_BMAP_RADIX blocks per call.  It will panic with 'allocation too
 *	large' if you try.  This is an area that could use improvement.  The
 *	radix is large enough that this restriction does not effect the swap
 *	system, though.  Currently only the allocation code is effected by
 *	this algorithmic unfeature.  The freeing code can handle arbitrary
 *	ranges.
 *
 *	This code can be compiled stand-alone for debugging.
 *
 * $FreeBSD: src/sys/kern/subr_blist.c,v 1.5.2.1 2000/03/17 10:47:29 ps Exp $
 */

typedef unsigned int u_daddr_t;

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lock.h>
#include <sys/kernel.h>
#include "blist.h"
#include <sys/malloc.h>

#if !defined(__APPLE__)
#define SWAPBLK_NONE ((daddr_t)-1)
#endif

#define malloc _MALLOC
#define free _FREE
#define M_SWAP M_TEMP

  
                           
   

static daddr_t blst_leaf_alloc(blmeta_t *scan, daddr_t blk, int count);
static daddr_t blst_meta_alloc(blmeta_t *scan, daddr_t blk,
    daddr_t count, daddr_t radix, int skip);
static void blst_leaf_free(blmeta_t *scan, daddr_t relblk, int count);
static void blst_meta_free(blmeta_t *scan, daddr_t freeBlk, daddr_t count,
    daddr_t radix, int skip, daddr_t blk);
static void blst_copy(blmeta_t *scan, daddr_t blk, daddr_t radix,
    daddr_t skip, blist_t dest, daddr_t count);
static daddr_t  blst_radix_init(blmeta_t *scan, daddr_t radix,
    int skip, daddr_t count);

  
                                                                          
                        
  
                                
  
                                                               
                                    
   

blist_t
blist_create(daddr_t blocks)
{
	blist_t bl;
	int radix;
	int skip = 0;

	  
                                                     
    
	radix = BLIST_BMAP_RADIX;

	while (radix < blocks) {
		radix <<= BLIST_META_RADIX_SHIFT;
		skip = (skip + 1) << BLIST_META_RADIX_SHIFT;
	}

	bl = malloc(sizeof(struct blist), M_SWAP, M_WAITOK);

	bzero(bl, sizeof(*bl));

	bl->bl_blocks = blocks;
	bl->bl_radix = radix;
	bl->bl_skip = skip;
	bl->bl_rootblks = 1 +
	    blst_radix_init(NULL, bl->bl_radix, bl->bl_skip, blocks);
	bl->bl_root = malloc(sizeof(blmeta_t) * bl->bl_rootblks, M_SWAP, M_WAITOK);

#if defined(BLIST_DEBUG)
	printf(
		"BLIST representing %d blocks (%d MB of swap)"
		", requiring %dK of ram\n",
		bl->bl_blocks,
		bl->bl_blocks * 4 / 1024,
		(bl->bl_rootblks * sizeof(blmeta_t) + 1023) / 1024
		);
	printf("BLIST raw radix tree contains %d records\n", bl->bl_rootblks);
#endif
	blst_radix_init(bl->bl_root, bl->bl_radix, bl->bl_skip, blocks);

	return bl;
}

void
blist_destroy(blist_t bl)
{
	free(bl->bl_root, M_SWAP);
	free(bl, M_SWAP);
}

  
                                                                      
                                                              
                          
   

daddr_t
blist_alloc(blist_t bl, daddr_t count)
{
	daddr_t blk = SWAPBLK_NONE;

	if (bl) {
		if (bl->bl_radix == BLIST_BMAP_RADIX) {
			blk = blst_leaf_alloc(bl->bl_root, 0, count);
		} else {
			blk = blst_meta_alloc(bl->bl_root, 0, count,
			    bl->bl_radix, bl->bl_skip);
		}
		if (blk != SWAPBLK_NONE) {
			bl->bl_free -= count;
		}
	}
	return blk;
}

  
                                                                     
                                                                 
           
   

void
blist_free(blist_t bl, daddr_t blkno, daddr_t count)
{
	if (bl) {
		if (bl->bl_radix == BLIST_BMAP_RADIX) {
			blst_leaf_free(bl->bl_root, blkno, count);
		} else {
			blst_meta_free(bl->bl_root, blkno, count,
			    bl->bl_radix, bl->bl_skip, 0);
		}
		bl->bl_free += count;
	}
}

  
                                                               
                                                      
                                                         
                                                          
                                                   
   

void
blist_resize(blist_t *pbl, daddr_t count, int freenew)
{
	blist_t newbl = blist_create(count);
	blist_t save = *pbl;

	*pbl = newbl;
	if (count > save->bl_blocks) {
		count = save->bl_blocks;
	}
	blst_copy(save->bl_root, 0, save->bl_radix, save->bl_skip, newbl, count);

	  
                                                             
    
	if (freenew && count < newbl->bl_blocks) {
		blist_free(newbl, count, newbl->bl_blocks - count);
	}
	blist_destroy(save);
}

#ifdef BLIST_DEBUG

  
                                     
   

void
blist_print(blist_t bl)
{
	printf("BLIST {\n");
	blst_radix_print(bl->bl_root, 0, bl->bl_radix, bl->bl_skip, 4);
	printf("}\n");
}

#endif

                                                                         
                                       
                                                                         
  
                                                                 
                                                                  
                                   
  
   

  
                                                                        
  
                                                                     
                                                                    
                                                                      
         
   

static daddr_t
blst_leaf_alloc(blmeta_t *scan, daddr_t blk, int count)
{
	u_daddr_t orig = scan->u.bmu_bitmap;

	if (orig == 0) {
		  
                                                         
                                                          
                                            
     
		scan->bm_bighint = 0;
		return SWAPBLK_NONE;
	}
	if (count == 1) {
		  
                                                         
     
		u_daddr_t mask;
		int j = BLIST_BMAP_RADIX / 2;
		int r = 0;

		mask = (u_daddr_t)-1 >> (BLIST_BMAP_RADIX / 2);

		while (j) {
			if ((orig & mask) == 0) {
				r += j;
				orig >>= j;
			}
			j >>= 1;
			mask >>= j;
		}
		scan->u.bmu_bitmap &= ~(1 << r);
		return blk + r;
	}
#if !defined(__APPLE__)
	if (count <= BLIST_BMAP_RADIX) {
#else
	if (count <= (int)BLIST_BMAP_RADIX) {
#endif                
		  
                                                             
                                                          
                                                              
                                                           
                                              
     
		int j;
		int n = BLIST_BMAP_RADIX - count;
		u_daddr_t mask;

		mask = (u_daddr_t)-1 >> n;

		for (j = 0; j <= n; ++j) {
			if ((orig & mask) == mask) {
				scan->u.bmu_bitmap &= ~mask;
				return blk + j;
			}
			mask = (mask << 1);
		}
	}
	  
                                                               
    
	scan->bm_bighint = count - 1;
	return SWAPBLK_NONE;
}

  
                                                             
  
                                                              
                                                                  
                                                                     
                                                     
   

static daddr_t
blst_meta_alloc(blmeta_t *scan, daddr_t blk, daddr_t count, daddr_t radix,
    int skip)
{
	int i;
	int next_skip = (skip >> BLIST_META_RADIX_SHIFT);

	if (scan->u.bmu_avail == 0) {
		  
                               
     
		scan->bm_bighint = count;
		return SWAPBLK_NONE;
	}

	if (scan->u.bmu_avail == radix) {
		radix >>= BLIST_META_RADIX_SHIFT;

		  
                                                   
              
     
		for (i = 1; i <= skip; i += next_skip) {
			if (scan[i].bm_bighint == (daddr_t)-1) {
				break;
			}
			if (next_skip == 1) {
				scan[i].u.bmu_bitmap = (u_daddr_t)-1;
				scan[i].bm_bighint = BLIST_BMAP_RADIX;
			} else {
				scan[i].bm_bighint = radix;
				scan[i].u.bmu_avail = radix;
			}
		}
	} else {
		radix >>= BLIST_META_RADIX_SHIFT;
	}

	for (i = 1; i <= skip; i += next_skip) {
		if (count <= scan[i].bm_bighint) {
			  
                          
      
			daddr_t r;
			if (next_skip == 1) {
				r = blst_leaf_alloc(&scan[i], blk, count);
			} else {
				r = blst_meta_alloc(&scan[i], blk, count,
				    radix, next_skip - 1);
			}
			if (r != SWAPBLK_NONE) {
				scan->u.bmu_avail -= count;
				if (scan->bm_bighint > scan->u.bmu_avail) {
					scan->bm_bighint = scan->u.bmu_avail;
				}
				return r;
			}
		} else if (scan[i].bm_bighint == (daddr_t)-1) {
			  
                
      
			break;
		} else if (count > radix) {
			  
                                                  
                    
      
			panic("blist_meta_alloc: allocation too large");
		}
		blk += radix;
	}

	  
                                                               
    
	if (scan->bm_bighint >= count) {
		scan->bm_bighint = count - 1;
	}
	return SWAPBLK_NONE;
}

  
                                                           
  
   

static void
blst_leaf_free(blmeta_t *scan, daddr_t blk, int count)
{
	  
                                 
   
        
                       
                            
               
    
	int n = blk & (BLIST_BMAP_RADIX - 1);
	u_daddr_t mask;

	mask = ((u_daddr_t)-1 << n) &
	    ((u_daddr_t)-1 >> (BLIST_BMAP_RADIX - count - n));

	if (scan->u.bmu_bitmap & mask) {
		panic("blst_radix_free: freeing free block");
	}
	scan->u.bmu_bitmap |= mask;

	  
                                                                    
                                                                
                                                               
                                                                    
    
	scan->bm_bighint = BLIST_BMAP_RADIX;
}

  
                                                                     
  
                                                                
                                                                
                                                                
                                                                
                                                                 
          
   

static void
blst_meta_free(blmeta_t *scan, daddr_t freeBlk, daddr_t count, daddr_t radix,
    int skip, daddr_t blk)
{
	int i;
	int next_skip = (skip >> BLIST_META_RADIX_SHIFT);

#if 0
	printf("FREE (%x,%d) FROM (%x,%d)\n",
	    freeBlk, count,
	    blk, radix
	    );
#endif

	if (scan->u.bmu_avail == 0) {
		  
                                              
                                       
     
		scan->u.bmu_avail = count;
		scan->bm_bighint = count;

		if (count != radix) {
			for (i = 1; i <= skip; i += next_skip) {
				if (scan[i].bm_bighint == (daddr_t)-1) {
					break;
				}
				scan[i].bm_bighint = 0;
				if (next_skip == 1) {
					scan[i].u.bmu_bitmap = 0;
				} else {
					scan[i].u.bmu_avail = 0;
				}
			}
			                  
		}
	} else {
		scan->u.bmu_avail += count;
		                               
	}

	  
                          
    

	if (scan->u.bmu_avail == radix) {
		return;
	}
	if (scan->u.bmu_avail > radix) {
		panic("blst_meta_free: freeing already free blocks (%d) %d/%d", count, scan->u.bmu_avail, radix);
	}

	  
                                           
    

	radix >>= BLIST_META_RADIX_SHIFT;

	i = (freeBlk - blk) / radix;
	blk += i * radix;
	i = i * next_skip + 1;

	while (i <= skip && blk < freeBlk + count) {
		daddr_t v;

		v = blk + radix - freeBlk;
		if (v > count) {
			v = count;
		}

		if (scan->bm_bighint == (daddr_t)-1) {
			panic("blst_meta_free: freeing unexpected range");
		}

		if (next_skip == 1) {
			blst_leaf_free(&scan[i], freeBlk, v);
		} else {
			blst_meta_free(&scan[i], freeBlk, v, radix,
			    next_skip - 1, blk);
		}
		if (scan->bm_bighint < scan[i].bm_bighint) {
			scan->bm_bighint = scan[i].bm_bighint;
		}
		count -= v;
		freeBlk += v;
		blk += radix;
		i += next_skip;
	}
}

  
                                                      
  
                                                                        
                                                               
   

static void
blst_copy(blmeta_t *scan, daddr_t blk, daddr_t radix,
    daddr_t skip, blist_t dest, daddr_t count)
{
	int next_skip;
	int i;

	  
             
    

	if (radix == BLIST_BMAP_RADIX) {
		u_daddr_t v = scan->u.bmu_bitmap;

		if (v == (u_daddr_t)-1) {
			blist_free(dest, blk, count);
		} else if (v != 0) {
#if !defined(__APPLE__)
			int i;

			for (i = 0; i < BLIST_BMAP_RADIX && i < count; ++i) {
				if (v & (1 << i)) {
					blist_free(dest, blk + i, 1);
				}
			}
#else
			int j;                              

			for (j = 0; j < (int)BLIST_BMAP_RADIX && j < count; ++j) {
				if (v & (1 << j)) {
					blist_free(dest, blk + j, 1);
				}
			}
#endif                
		}
		return;
	}

	  
             
    

	  
                                              
    
	if (scan->u.bmu_avail == 0) {
		return;
	}
	if (scan->u.bmu_avail == radix) {
		  
                                      
     
		if (count < radix) {
			blist_free(dest, blk, count);
		} else {
			blist_free(dest, blk, radix);
		}
		return;
	}

	radix >>= BLIST_META_RADIX_SHIFT;
	next_skip = (skip >> BLIST_META_RADIX_SHIFT);

	for (i = 1; count && i <= skip; i += next_skip) {
		if (scan[i].bm_bighint == (daddr_t)-1) {
			break;
		}

		if (count >= radix) {
			blst_copy(
				&scan[i],
				blk,
				radix,
				next_skip - 1,
				dest,
				radix
				);
			count -= radix;
		} else {
			if (count) {
				blst_copy(
					&scan[i],
					blk,
					radix,
					next_skip - 1,
					dest,
					count
					);
			}
			count = 0;
		}
		blk += radix;
	}
}

  
                                            
  
                                                                     
                                                                     
                                                                  
                       
   

static daddr_t
blst_radix_init(blmeta_t *scan, daddr_t radix, int skip, daddr_t count)
{
	int i;
	int next_skip;
	daddr_t memindex = 0;

	  
             
    

	if (radix == BLIST_BMAP_RADIX) {
		if (scan) {
			scan->bm_bighint = 0;
			scan->u.bmu_bitmap = 0;
		}
		return memindex;
	}

	  
                                                              
                                                            
                                                                   
    

	if (scan) {
		scan->bm_bighint = 0;
		scan->u.bmu_avail = 0;
	}

	radix >>= BLIST_META_RADIX_SHIFT;
	next_skip = (skip >> BLIST_META_RADIX_SHIFT);

	for (i = 1; i <= skip; i += next_skip) {
		if (count >= radix) {
			  
                                
      
			memindex = i + blst_radix_init(
				((scan) ? &scan[i] : NULL),
				radix,
				next_skip - 1,
				radix
				);
			count -= radix;
		} else if (count > 0) {
			  
                               
      
			memindex = i + blst_radix_init(
				((scan) ? &scan[i] : NULL),
				radix,
				next_skip - 1,
				count
				);
			count = 0;
		} else {
			  
                                  
      
			if (scan) {
				scan[i].bm_bighint = (daddr_t)-1;
			}
			break;
		}
	}
	if (memindex < i) {
		memindex = i;
	}
	return memindex;
}

#ifdef BLIST_DEBUG

static void
blst_radix_print(blmeta_t *scan, daddr_t blk, daddr_t radix, int skip, int tab)
{
	int i;
	int next_skip;
	int lastState = 0;

	if (radix == BLIST_BMAP_RADIX) {
		printf(
			"%*.*s(%04x,%d): bitmap %08x big=%d\n",
			tab, tab, "",
			blk, radix,
			scan->u.bmu_bitmap,
			scan->bm_bighint
			);
		return;
	}

	if (scan->u.bmu_avail == 0) {
		printf(
			"%*.*s(%04x,%d) ALL ALLOCATED\n",
			tab, tab, "",
			blk,
			radix
			);
		return;
	}
	if (scan->u.bmu_avail == radix) {
		printf(
			"%*.*s(%04x,%d) ALL FREE\n",
			tab, tab, "",
			blk,
			radix
			);
		return;
	}

	printf(
		"%*.*s(%04x,%d): subtree (%d/%d) big=%d {\n",
		tab, tab, "",
		blk, radix,
		scan->u.bmu_avail,
		radix,
		scan->bm_bighint
		);

	radix >>= BLIST_META_RADIX_SHIFT;
	next_skip = (skip >> BLIST_META_RADIX_SHIFT);
	tab += 4;

	for (i = 1; i <= skip; i += next_skip) {
		if (scan[i].bm_bighint == (daddr_t)-1) {
			printf(
				"%*.*s(%04x,%d): Terminator\n",
				tab, tab, "",
				blk, radix
				);
			lastState = 0;
			break;
		}
		blst_radix_print(
			&scan[i],
			blk,
			radix,
			next_skip - 1,
			tab
			);
		blk += radix;
	}
	tab -= 4;

	printf(
		"%*.*s}\n",
		tab, tab, ""
		);
}

#endif

#ifdef BLIST_DEBUG

int
main(int ac, char **av)
{
	int size = 1024;
	int i;
	blist_t bl;

	for (i = 1; i < ac; ++i) {
		const char *ptr = av[i];
		if (*ptr != '-') {
			size = strtol(ptr, NULL, 0);
			continue;
		}
		ptr += 2;
		fprintf(stderr, "Bad option: %s\n", ptr - 2);
		exit(1);
	}
	bl = blist_create(size);
	blist_free(bl, 0, size);

	for (;;) {
		char buf[1024];
		daddr_t da = 0;
		daddr_t count = 0;


		printf("%d/%d/%d> ", bl->bl_free, size, bl->bl_radix);
		fflush(stdout);
		if (fgets(buf, sizeof(buf), stdin) == NULL) {
			break;
		}
		switch (buf[0]) {
		case 'r':
			if (sscanf(buf + 1, "%d", &count) == 1) {
				blist_resize(&bl, count, 1);
			} else {
				printf("?\n");
			}
		case 'p':
			blist_print(bl);
			break;
		case 'a':
			if (sscanf(buf + 1, "%d", &count) == 1) {
				daddr_t blk = blist_alloc(bl, count);
				printf("    R=%04x\n", blk);
			} else {
				printf("?\n");
			}
			break;
		case 'f':
			if (sscanf(buf + 1, "%x %d", &da, &count) == 2) {
				blist_free(bl, da, count);
			} else {
				printf("?\n");
			}
			break;
		case '?':
		case 'h':
			puts(
				"p          -print\n"
				"a %d       -allocate\n"
				"f %x %d    -free\n"
				"r %d       -resize\n"
				"h/?        -help"
				);
			break;
		default:
			printf("?\n");
			break;
		}
	}
	return 0;
}

void
panic(const char *ctl, ...)
{
	va_list va;

	va_start(va, ctl);
	vfprintf(stderr, ctl, va);
	fprintf(stderr, "\n");
	va_end(va);
	exit(1);
}

#endif
