/*
 * Copyright (c) 2000-2005 Apple Computer, Inc. All rights reserved.
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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

  
                                                                            
   

#include <sys/param.h>
#include <sys/utfconv.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <libkern/OSByteOrder.h>

#if defined(KERNEL) && !defined(VFS_UTF8_UNIT_TEST)
#include <kern/assert.h>
#else
#include <assert.h>
#endif

  
                                        
  
                                                                       
                                                                       
                                                               
  
                        
  
                                                                         
                                                                                
                                                     
                                                              
                                                                       
                                                                                
                                                                                
   


#define UNICODE_TO_UTF8_LEN(c)  \
	((c) < 0x0080 ? 1 : ((c) < 0x0800 ? 2 : (((c) & 0xf800) == 0xd800 ? 2 : 3)))

#define UCS_ALT_NULL    0x2400

                              
#define SP_HALF_SHIFT   10
#define SP_HALF_BASE    0x0010000u
#define SP_HALF_MASK    0x3FFu

#define SP_HIGH_FIRST   0xD800u
#define SP_HIGH_LAST    0xDBFFu
#define SP_LOW_FIRST    0xDC00u
#define SP_LOW_LAST             0xDFFFu


#include "vfs_utfconvdata.h"


  
                                  
  
                                                       
                                                           
   
int
unicode_combinable(u_int16_t character)
{
	const u_int8_t *bitmap = __CFUniCharCombiningBitmap;
	u_int8_t value;

	if (character < 0x0300) {
		return 0;
	}

	value = bitmap[(character >> 8) & 0xFF];

	if (value == 0xFF) {
		return 1;
	} else if (value) {
		bitmap = bitmap + ((value - 1) * 32) + 256;
		return bitmap[(character & 0xFF) / 8] & (1 << (character % 8)) ? 1 : 0;
	}
	return 0;
}

  
                                    
  
                                                 
   
int
unicode_decomposeable(u_int16_t character)
{
	const u_int8_t *bitmap = __CFUniCharDecomposableBitmap;
	u_int8_t value;

	if (character < 0x00C0) {
		return 0;
	}

	value = bitmap[(character >> 8) & 0xFF];

	if (value == 0xFF) {
		return 1;
	} else if (value) {
		bitmap = bitmap + ((value - 1) * 32) + 256;
		return bitmap[(character & 0xFF) / 8] & (1 << (character % 8)) ? 1 : 0;
	}
	return 0;
}


  
                         
  
                                                        
   
static inline u_int8_t
get_combining_class(u_int16_t character)
{
	const u_int8_t *bitmap = __CFUniCharCombiningPropertyBitmap;

	u_int8_t value = bitmap[(character >> 8)];

	if (value) {
		bitmap = bitmap + (value * 256);
		return bitmap[character % 256];
	}
	return 0;
}


static int unicode_decompose(u_int16_t character, u_int16_t *convertedChars);

static u_int16_t unicode_combine(u_int16_t base, u_int16_t combining);

static void prioritysort(u_int16_t* characters, int count);

static u_int16_t  ucs_to_sfm(u_int16_t ucs_ch, int lastchar);

static u_int16_t  sfm_to_ucs(u_int16_t ucs_ch);


char utf_extrabytes[32] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	-1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 1, 2, 2, 3, -1
};

const char hexdigits[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

  
                                                       
  
                                                                    
                                                                      
                                                                     
  
                                                                   
                                                     
  
        
                                                                         
  
                                                              
  
                                                                    
  
                                                       
  
                                                                        
  
         
          
   
size_t
utf8_encodelen(const u_int16_t * ucsp, size_t ucslen, u_int16_t altslash, int flags)
{
	u_int16_t ucs_ch;
	u_int16_t * chp = NULL;
	u_int16_t sequence[8];
	int extra = 0;
	size_t charcnt;
	int swapbytes = (flags & UTF_REVERSE_ENDIAN);
	int decompose = (flags & UTF_DECOMPOSED);
	size_t len;

	charcnt = ucslen / 2;
	len = 0;

	while (charcnt-- > 0) {
		if (extra > 0) {
			--extra;
			ucs_ch = *chp++;
		} else {
			ucs_ch = *ucsp++;
			if (swapbytes) {
				ucs_ch = OSSwapInt16(ucs_ch);
			}
			if (ucs_ch == '/') {
				ucs_ch = altslash ? altslash : '_';
			} else if (ucs_ch == '\0') {
				ucs_ch = UCS_ALT_NULL;
			} else if (decompose && unicode_decomposeable(ucs_ch)) {
				extra = unicode_decompose(ucs_ch, sequence) - 1;
				charcnt += extra;
				ucs_ch = sequence[0];
				chp = &sequence[1];
			}
		}
		len += UNICODE_TO_UTF8_LEN(ucs_ch);
	}

	return len;
}


  
                                                     
  
         
                                                    
  
                                                        
                                                      
  
               
                                                                       
  
                                                              
  
                                                                    
  
                                                       
  
                                                                   
  
          
                                                                   
  
                                                              
   
int
utf8_encodestr(const u_int16_t * ucsp, size_t ucslen, u_int8_t * utf8p,
    size_t * utf8len, size_t buflen, u_int16_t altslash, int flags)
{
	u_int8_t * bufstart;
	u_int8_t * bufend;
	u_int16_t ucs_ch;
	u_int16_t * chp = NULL;
	u_int16_t sequence[8];
	int extra = 0;
	size_t charcnt;
	int swapbytes = (flags & UTF_REVERSE_ENDIAN);
	int nullterm  = ((flags & UTF_NO_NULL_TERM) == 0);
	int decompose = (flags & UTF_DECOMPOSED);
	int sfmconv = (flags & UTF_SFM_CONVERSIONS);
	int result = 0;

	bufstart = utf8p;
	bufend = bufstart + buflen;
	if (nullterm) {
		--bufend;
	}
	charcnt = ucslen / 2;

	while (charcnt-- > 0) {
		if (extra > 0) {
			--extra;
			ucs_ch = *chp++;
		} else {
			ucs_ch = swapbytes ? OSSwapInt16(*ucsp++) : *ucsp++;

			if (decompose && unicode_decomposeable(ucs_ch)) {
				extra = unicode_decompose(ucs_ch, sequence) - 1;
				charcnt += extra;
				ucs_ch = sequence[0];
				chp = &sequence[1];
			}
		}

		                                      
		if (ucs_ch == '/') {
			if (altslash) {
				ucs_ch = altslash;
			} else {
				ucs_ch = '_';
				result = EINVAL;
			}
		} else if (ucs_ch == '\0') {
			ucs_ch = UCS_ALT_NULL;
		}

		if (ucs_ch < 0x0080) {
			if (utf8p >= bufend) {
				result = ENAMETOOLONG;
				break;
			}
			*utf8p++ = (u_int8_t)ucs_ch;
		} else if (ucs_ch < 0x800) {
			if ((utf8p + 1) >= bufend) {
				result = ENAMETOOLONG;
				break;
			}
			*utf8p++ = 0xc0 | (u_int8_t)(ucs_ch >> 6);
			*utf8p++ = 0x80 | (0x3f & ucs_ch);
		} else {
			                                      
			if (ucs_ch == 0xFFFE || ucs_ch == 0xFFFF) {
				result = EINVAL;
				break;
			}

			                                   
			if (ucs_ch >= SP_HIGH_FIRST && ucs_ch <= SP_HIGH_LAST
			    && charcnt > 0) {
				u_int16_t ch2;
				u_int32_t pair;

				ch2 = swapbytes ? OSSwapInt16(*ucsp) : *ucsp;
				if (ch2 >= SP_LOW_FIRST && ch2 <= SP_LOW_LAST) {
					pair = ((ucs_ch - SP_HIGH_FIRST) << SP_HALF_SHIFT)
					    + (ch2 - SP_LOW_FIRST) + SP_HALF_BASE;
					if ((utf8p + 3) >= bufend) {
						result = ENAMETOOLONG;
						break;
					}
					--charcnt;
					++ucsp;
					*utf8p++ = 0xf0 | (u_int8_t)(pair >> 18);
					*utf8p++ = 0x80 | (0x3f & (pair >> 12));
					*utf8p++ = 0x80 | (0x3f & (pair >> 6));
					*utf8p++ = 0x80 | (0x3f & pair);
					continue;
				}
			} else if (sfmconv) {
				ucs_ch = sfm_to_ucs(ucs_ch);
				if (ucs_ch < 0x0080) {
					if (utf8p >= bufend) {
						result = ENAMETOOLONG;
						break;
					}
					*utf8p++ = (u_int8_t)ucs_ch;
					continue;
				}
			}
			if ((utf8p + 2) >= bufend) {
				result = ENAMETOOLONG;
				break;
			}
			*utf8p++ = 0xe0 | (ucs_ch >> 12);
			*utf8p++ = 0x80 | (0x3f & (ucs_ch >> 6));
			*utf8p++ = 0x80 | (0x3f & ucs_ch);
		}
	}

	*utf8len = utf8p - bufstart;
	if (nullterm) {
		*utf8p++ = '\0';
	}

	return result;
}

                                                                     
static void
push(uint16_t ucs_ch, int *combcharcnt, uint16_t **ucsp)
{
	  
                                                         
    
	if (unicode_combinable(ucs_ch)) {
		++*combcharcnt;                                   
	} else if (*combcharcnt) {
		if (*combcharcnt > 1) {
			prioritysort(*ucsp - *combcharcnt, *combcharcnt);
		}
		*combcharcnt = 0;                       
	}

	*(*ucsp)++ = ucs_ch;
}

  
                                                          
  
         
                                                                
                        
  
                                                        
                                                      
  
               
                                                                     
  
                                                              
  
                                                                    
  
                                                             
  
                                                         
  
                                                                 
  
          
                                                                    
  
                                           
   
int
utf8_decodestr(const u_int8_t* utf8p, size_t utf8len, u_int16_t* ucsp,
    size_t *ucslen, size_t buflen, u_int16_t altslash, int flags)
{
	u_int16_t* bufstart;
	u_int16_t* bufend;
	unsigned int ucs_ch;
	unsigned int byte;
	int combcharcnt = 0;
	int result = 0;
	int decompose, precompose, escaping;
	int sfmconv;
	int extrabytes;

	decompose  = (flags & UTF_DECOMPOSED);
	precompose = (flags & UTF_PRECOMPOSED);
	escaping   = (flags & UTF_ESCAPE_ILLEGAL);
	sfmconv    = (flags & UTF_SFM_CONVERSIONS);

	bufstart = ucsp;
	bufend = (u_int16_t *)((u_int8_t *)ucsp + buflen);

	while (utf8len-- > 0 && (byte = *utf8p++) != '\0') {
		if (ucsp >= bufend) {
			goto toolong;
		}

		                     
		if (byte < 0x80) {
			ucs_ch = sfmconv ? ucs_to_sfm((u_int16_t)byte, utf8len == 0) : byte;
		} else {
			u_int32_t ch;

			extrabytes = utf_extrabytes[byte >> 3];
			if ((extrabytes < 0) || ((int)utf8len < extrabytes)) {
				goto escape;
			}
			utf8len -= extrabytes;

			switch (extrabytes) {
			case 1:
				ch = byte; ch <<= 6;                 
				byte = *utf8p++;                     
				if ((byte >> 6) != 2) {
					goto escape2;
				}
				ch += byte;
				ch -= 0x00003080UL;
				if (ch < 0x0080) {
					goto escape2;
				}
				ucs_ch = ch;
				break;
			case 2:
				ch = byte; ch <<= 6;                 
				byte = *utf8p++;                     
				if ((byte >> 6) != 2) {
					goto escape2;
				}
				ch += byte; ch <<= 6;
				byte = *utf8p++;                     
				if ((byte >> 6) != 2) {
					goto escape3;
				}
				ch += byte;
				ch -= 0x000E2080UL;
				if (ch < 0x0800) {
					goto escape3;
				}
				if (ch >= 0xD800) {
					if (ch <= 0xDFFF) {
						goto escape3;
					}
					if (ch == 0xFFFE || ch == 0xFFFF) {
						goto escape3;
					}
				}
				ucs_ch = ch;
				break;
			case 3:
				ch = byte; ch <<= 6;                 
				byte = *utf8p++;                     
				if ((byte >> 6) != 2) {
					goto escape2;
				}
				ch += byte; ch <<= 6;
				byte = *utf8p++;                     
				if ((byte >> 6) != 2) {
					goto escape3;
				}
				ch += byte; ch <<= 6;
				byte = *utf8p++;                     
				if ((byte >> 6) != 2) {
					goto escape4;
				}
				ch += byte;
				ch -= 0x03C82080UL + SP_HALF_BASE;
				ucs_ch = (ch >> SP_HALF_SHIFT) + SP_HIGH_FIRST;
				if (ucs_ch < SP_HIGH_FIRST || ucs_ch > SP_HIGH_LAST) {
					goto escape4;
				}
				push((uint16_t)ucs_ch, &combcharcnt, &ucsp);
				if (ucsp >= bufend) {
					goto toolong;
				}
				ucs_ch = (ch & SP_HALF_MASK) + SP_LOW_FIRST;
				if (ucs_ch < SP_LOW_FIRST || ucs_ch > SP_LOW_LAST) {
					--ucsp;
					goto escape4;
				}
				*ucsp++ = (u_int16_t)ucs_ch;
				continue;
			default:
				result = EINVAL;
				goto exit;
			}
			if (decompose) {
				if (unicode_decomposeable((u_int16_t)ucs_ch)) {
					u_int16_t sequence[8];
					int count, i;

					count = unicode_decompose((u_int16_t)ucs_ch, sequence);

					for (i = 0; i < count; ++i) {
						if (ucsp >= bufend) {
							goto toolong;
						}

						push(sequence[i], &combcharcnt, &ucsp);
					}

					continue;
				}
			} else if (precompose && (ucsp != bufstart)) {
				u_int16_t composite, base;

				if (unicode_combinable((u_int16_t)ucs_ch)) {
					base = ucsp[-1];
					composite = unicode_combine(base, (u_int16_t)ucs_ch);
					if (composite) {
						--ucsp;
						ucs_ch = composite;
					}
				}
			}
			if (ucs_ch == UCS_ALT_NULL) {
				ucs_ch = '\0';
			}
		}
		if (ucs_ch == altslash) {
			ucs_ch = '/';
		}

		push((u_int16_t)ucs_ch, &combcharcnt, &ucsp);
		continue;

		  
                                               
     
escape4:
		utf8p -= 3;
		goto escape;
escape3:
		utf8p -= 2;
		goto escape;
escape2:
		utf8p -= 1;
escape:
		if (!escaping) {
			result = EINVAL;
			goto exit;
		}
		if (extrabytes > 0) {
			utf8len += extrabytes;
		}
		byte = *(utf8p - 1);

		if ((ucsp + 2) >= bufend) {
			goto toolong;
		}

		                                                   
		if (combcharcnt > 1) {
			prioritysort(ucsp - combcharcnt, combcharcnt);
		}
		combcharcnt = 0;

		ucs_ch = '%';
		*ucsp++ = (u_int16_t)ucs_ch;
		ucs_ch =  hexdigits[byte >> 4];
		*ucsp++ = (u_int16_t)ucs_ch;
		ucs_ch =  hexdigits[byte & 0x0F];
		*ucsp++ = (u_int16_t)ucs_ch;
	}
	  
                                                
    
	if (combcharcnt > 1) {
		prioritysort(ucsp - combcharcnt, combcharcnt);
	}

	if (flags & UTF_REVERSE_ENDIAN) {
		uint16_t *p = bufstart;
		while (p < ucsp) {
			*p = OSSwapInt16(*p);
			++p;
		}
	}

exit:
	*ucslen = (u_int8_t*)ucsp - (u_int8_t*)bufstart;

	return result;

toolong:
	result = ENAMETOOLONG;
	goto exit;
}


  
                                                     
   
int
utf8_validatestr(const u_int8_t* utf8p, size_t utf8len)
{
	unsigned int byte;
	u_int32_t ch;
	unsigned int ucs_ch;
	size_t extrabytes;

	while (utf8len-- > 0 && (byte = *utf8p++) != '\0') {
		if (byte < 0x80) {
			continue;                   
		}
		extrabytes = utf_extrabytes[byte >> 3];

		if (utf8len < extrabytes) {
			goto invalid;
		}
		utf8len -= extrabytes;

		switch (extrabytes) {
		case 1:
			ch = byte; ch <<= 6;                 
			byte = *utf8p++;                     
			if ((byte >> 6) != 2) {
				goto invalid;
			}
			ch += byte;
			ch -= 0x00003080UL;
			if (ch < 0x0080) {
				goto invalid;
			}
			break;
		case 2:
			ch = byte; ch <<= 6;                 
			byte = *utf8p++;                     
			if ((byte >> 6) != 2) {
				goto invalid;
			}
			ch += byte; ch <<= 6;
			byte = *utf8p++;                     
			if ((byte >> 6) != 2) {
				goto invalid;
			}
			ch += byte;
			ch -= 0x000E2080UL;
			if (ch < 0x0800) {
				goto invalid;
			}
			if (ch >= 0xD800) {
				if (ch <= 0xDFFF) {
					goto invalid;
				}
				if (ch == 0xFFFE || ch == 0xFFFF) {
					goto invalid;
				}
			}
			break;
		case 3:
			ch = byte; ch <<= 6;                 
			byte = *utf8p++;                     
			if ((byte >> 6) != 2) {
				goto invalid;
			}
			ch += byte; ch <<= 6;
			byte = *utf8p++;                     
			if ((byte >> 6) != 2) {
				goto invalid;
			}
			ch += byte; ch <<= 6;
			byte = *utf8p++;                     
			if ((byte >> 6) != 2) {
				goto invalid;
			}
			ch += byte;
			ch -= 0x03C82080UL + SP_HALF_BASE;
			ucs_ch = (ch >> SP_HALF_SHIFT) + SP_HIGH_FIRST;
			if (ucs_ch < SP_HIGH_FIRST || ucs_ch > SP_HIGH_LAST) {
				goto invalid;
			}
			ucs_ch = (ch & SP_HALF_MASK) + SP_LOW_FIRST;
			if (ucs_ch < SP_LOW_FIRST || ucs_ch > SP_LOW_LAST) {
				goto invalid;
			}
			break;
		default:
			goto invalid;
		}
	}
	return 0;
invalid:
	return EINVAL;
}

  
                                                            
  
                                                                   
                                                                     
                                                                       
                                                                       
                                             
  
        
                                                                   
  
                                                               
  
                                                                     
  
                                                                 
  
         
                                                                          
  
                                                                  
   
int
utf8_normalizestr(const u_int8_t* instr, size_t inlen, u_int8_t* outstr,
    size_t *outlen, size_t buflen, int flags)
{
	u_int16_t unicodebuf[32];
	u_int16_t* unistr = NULL;
	size_t unicode_bytes;
	size_t uft8_bytes;
	size_t inbuflen;
	u_int8_t *outbufstart, *outbufend;
	const u_int8_t *inbufstart;
	unsigned int byte;
	int decompose, precompose;
	int result = 0;

	if (flags & ~(UTF_DECOMPOSED | UTF_PRECOMPOSED | UTF_NO_NULL_TERM | UTF_ESCAPE_ILLEGAL)) {
		return EINVAL;
	}
	decompose = (flags & UTF_DECOMPOSED);
	precompose = (flags & UTF_PRECOMPOSED);
	if ((decompose && precompose) || (!decompose && !precompose)) {
		return EINVAL;
	}
	outbufstart = outstr;
	outbufend = outbufstart + buflen;
	inbufstart = instr;
	inbuflen = inlen;

	while (inlen-- > 0 && (byte = *instr++) != '\0') {
		if (outstr >= outbufend) {
			result = ENAMETOOLONG;
			goto exit;
		}
		if (byte >= 0x80) {
			goto nonASCII;
		}
		                                  
		*outstr++ = (u_int8_t)byte;
	}
exit:
	*outlen = outstr - outbufstart;
	if (((flags & UTF_NO_NULL_TERM) == 0)) {
		if (outstr < outbufend) {
			*outstr++ = '\0';
		} else {
			result = ENAMETOOLONG;
		}
	}
	return result;


	  
                                                             
                                                            
                                                             
                                                         
    
nonASCII:

	                                             
	if (inbuflen > MAXPATHLEN) {
		result = ENAMETOOLONG;
		goto exit;
	}
	  
                                           
   
                                                              
                                                               
                                                                
                                      
    
	unicode_bytes = precompose ? (inbuflen * 2) : (inbuflen * 3);

	if (unicode_bytes <= sizeof(unicodebuf)) {
		unistr = &unicodebuf[0];
	} else {
		unistr = kheap_alloc(KHEAP_DATA_BUFFERS, unicode_bytes, Z_WAITOK);
	}

	                           
	result = utf8_decodestr(inbufstart, inbuflen, unistr, &unicode_bytes,
	    unicode_bytes, 0, flags & ~UTF_NO_NULL_TERM);
	if (result == 0) {
		                                  
		result = utf8_encodestr(unistr, unicode_bytes, outbufstart,
		    &uft8_bytes, buflen, 0, UTF_NO_NULL_TERM);
		outstr = outbufstart + uft8_bytes;
	}
	if (unistr && unistr != &unicodebuf[0]) {
		kheap_free(KHEAP_DATA_BUFFERS, unistr, unicode_bytes);
	}
	goto exit;
}


  
                                                                
   

typedef struct {
	u_int32_t _key;
	u_int32_t _value;
} unicode_mappings32;

static inline u_int32_t
getmappedvalue32(const unicode_mappings32 *theTable, u_int32_t numElem,
    u_int16_t character)
{
	const unicode_mappings32 *p, *q, *divider;

	if ((character < theTable[0]._key) || (character > theTable[numElem - 1]._key)) {
		return 0;
	}

	p = theTable;
	q = p + (numElem - 1);
	while (p <= q) {
		divider = p + ((q - p) >> 1);                    
		if (character < divider->_key) {
			q = divider - 1;
		} else if (character > divider->_key) {
			p = divider + 1;
		} else {
			return divider->_value;
		}
	}
	return 0;
}

#define RECURSIVE_DECOMPOSITION (1 << 15)
#define EXTRACT_COUNT(value)    (((value) >> 12) & 0x0007)

typedef struct {
	u_int16_t _key;
	u_int16_t _value;
} unicode_mappings16;

static inline u_int16_t
getmappedvalue16(const unicode_mappings16 *theTable, u_int32_t numElem,
    u_int16_t character)
{
	const unicode_mappings16 *p, *q, *divider;

	if ((character < theTable[0]._key) || (character > theTable[numElem - 1]._key)) {
		return 0;
	}

	p = theTable;
	q = p + (numElem - 1);
	while (p <= q) {
		divider = p + ((q - p) >> 1);                    
		if (character < divider->_key) {
			q = divider - 1;
		} else if (character > divider->_key) {
			p = divider + 1;
		} else {
			return divider->_value;
		}
	}
	return 0;
}


static u_int32_t
unicode_recursive_decompose(u_int16_t character, u_int16_t *convertedChars)
{
	u_int16_t value;
	u_int32_t length;
	u_int16_t firstChar;
	u_int16_t theChar;
	const u_int16_t *bmpMappings;
	u_int32_t usedLength;

	value = getmappedvalue16(
		(const unicode_mappings16 *)__CFUniCharDecompositionTable,
		__UniCharDecompositionTableLength, character);
	length = EXTRACT_COUNT(value);
	firstChar = value & 0x0FFF;
	theChar = firstChar;
	bmpMappings = (length == 1 ? &theChar : __CFUniCharMultipleDecompositionTable + firstChar);
	usedLength = 0;

	if (value & RECURSIVE_DECOMPOSITION) {
		usedLength = unicode_recursive_decompose((u_int16_t)*bmpMappings, convertedChars);

		--length;                                   
		if (!usedLength) {
			return 0;
		}
		++bmpMappings;
		convertedChars += usedLength;
	}

	usedLength += length;

	while (length--) {
		*(convertedChars++) = *(bmpMappings++);
	}

	return usedLength;
}

#define HANGUL_SBASE 0xAC00
#define HANGUL_LBASE 0x1100
#define HANGUL_VBASE 0x1161
#define HANGUL_TBASE 0x11A7

#define HANGUL_SCOUNT 11172
#define HANGUL_LCOUNT 19
#define HANGUL_VCOUNT 21
#define HANGUL_TCOUNT 28
#define HANGUL_NCOUNT (HANGUL_VCOUNT * HANGUL_TCOUNT)

  
                                                        
  
                                               
                                                 
                                                 
            
  
                                         
   
static int
unicode_decompose(u_int16_t character, u_int16_t *convertedChars)
{
	if ((character >= HANGUL_SBASE) &&
	    (character <= (HANGUL_SBASE + HANGUL_SCOUNT))) {
		u_int32_t length;

		character -= HANGUL_SBASE;
		length = (character % HANGUL_TCOUNT ? 3 : 2);

		*(convertedChars++) =
		    character / HANGUL_NCOUNT + HANGUL_LBASE;
		*(convertedChars++) =
		    (character % HANGUL_NCOUNT) / HANGUL_TCOUNT + HANGUL_VBASE;
		if (length > 2) {
			*convertedChars = (character % HANGUL_TCOUNT) + HANGUL_TBASE;
		}
		return length;
	} else {
		return unicode_recursive_decompose(character, convertedChars);
	}
}

  
                                                        
  
                                                              
                                                               
                                                            
                         
  
                                                             
                                       
   
static u_int16_t
unicode_combine(u_int16_t base, u_int16_t combining)
{
	u_int32_t value;

	                  
	if ((combining >= HANGUL_VBASE) && (combining < (HANGUL_TBASE + HANGUL_TCOUNT))) {
		                             
		if ((combining < (HANGUL_VBASE + HANGUL_VCOUNT)) &&
		    (base >= HANGUL_LBASE && base < (HANGUL_LBASE + HANGUL_LCOUNT))) {
			return HANGUL_SBASE +
			       ((base - HANGUL_LBASE) * (HANGUL_VCOUNT * HANGUL_TCOUNT)) +
			       ((combining  - HANGUL_VBASE) * HANGUL_TCOUNT);
		}

		                             
		if ((combining > HANGUL_TBASE) &&
		    (base >= HANGUL_SBASE && base < (HANGUL_SBASE + HANGUL_SCOUNT))) {
			if ((base - HANGUL_SBASE) % HANGUL_TCOUNT) {
				return 0;
			} else {
				return base + (combining - HANGUL_TBASE);
			}
		}
	}

	value = getmappedvalue32(
		(const unicode_mappings32 *)__CFUniCharPrecompSourceTable,
		__CFUniCharPrecompositionTableLength, combining);

	if (value) {
		value = getmappedvalue16(
			(const unicode_mappings16 *)
			((const u_int32_t *)__CFUniCharBMPPrecompDestinationTable + (value & 0xFFFF)),
			(value >> 16), base);
	}
	return (u_int16_t)value;
}


  
                                                            
  
                                   
   
static void
prioritysort(u_int16_t* characters, int count)
{
	u_int32_t p1, p2;
	u_int16_t *ch1, *ch2;
	u_int16_t *end;
	int changes = 0;

	end = characters + count;
	do {
		changes = 0;
		ch1 = characters;
		ch2 = characters + 1;
		p2 = get_combining_class(*ch1);
		while (ch2 < end) {
			p1 = p2;
			p2 = get_combining_class(*ch2);
			if (p1 > p2 && p2 != 0) {
				u_int16_t tmp;

				tmp = *ch1;
				*ch1 = *ch2;
				*ch2 = tmp;
				changes = 1;

				  
                                                             
                                                             
                                                                
                                                                
                                               
       
				p2 = p1;
			}
			++ch1;
			++ch2;
		}
	} while (changes);
}


  
                                                           
                                                               
  
                                                    
  
                                          
                                            
                                       
                                
                                
                                
                                
                                
                                
                                
                                
                                                                 
                                                                 
                                            
  
                                                       
   

#define MAX_SFM2MAC           0x29
#define SFMCODE_PREFIX_MASK   0xf000

  
                                                                             
                                                                     
                                                                              
                                                                               
                                                     
   
static u_int8_t
    sfm2mac[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,                
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,                
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,                
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,                
	0x22, 0x2a, 0x3a, 0x3c, 0x3e, 0x3f, 0x5c, 0x7c,                
	0x20, 0x2e                                                     
};
#define SFM2MAC_LEN     ((sizeof(sfm2mac))/sizeof(sfm2mac[0]))

static u_int8_t
    mac2sfm[] = {
	0x20, 0x21, 0x20, 0x23, 0x24, 0x25, 0x26, 0x27,                
	0x28, 0x29, 0x21, 0x2b, 0x2c, 0x2d, 0x2e, 0x22,                
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,                
	0x38, 0x39, 0x22, 0x3b, 0x23, 0x3d, 0x24, 0x25,                
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,                
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,                
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,                
	0x58, 0x59, 0x5a, 0x5b, 0x26, 0x5d, 0x5e, 0x5f,                
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,                
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,                
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,                
	0x78, 0x79, 0x7a, 0x7b, 0x27, 0x7d, 0x7e, 0x7f                 
};
#define MAC2SFM_LEN     ((sizeof(mac2sfm))/sizeof(mac2sfm[0]))


  
                                                                              
  
                                
   
static u_int16_t
ucs_to_sfm(u_int16_t ucs_ch, int lastchar)
{
	                                                                 
	if (lastchar) {
		if (ucs_ch == 0x20) {
			return 0xf028;
		} else if (ucs_ch == 0x2e) {
			return 0xf029;
		}
	}
	                                           
	if (ucs_ch <= 0x1f) {
		return ucs_ch | 0xf000;
	} else {                  
		u_int16_t lsb;

		assert((ucs_ch - 0x0020) < MAC2SFM_LEN);
		lsb = mac2sfm[ucs_ch - 0x0020];
		if (lsb != ucs_ch) {
			return 0xf000 | lsb;
		}
	}
	return ucs_ch;
}

  
                                            
   
static u_int16_t
sfm_to_ucs(u_int16_t ucs_ch)
{
	if (((ucs_ch & 0xffC0) == SFMCODE_PREFIX_MASK) &&
	    ((ucs_ch & 0x003f) <= MAX_SFM2MAC)) {
		assert((ucs_ch & 0x003f) < SFM2MAC_LEN);
		ucs_ch = sfm2mac[ucs_ch & 0x003f];
	}
	return ucs_ch;
}
