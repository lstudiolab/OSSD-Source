/*
 * Copyright (C) 2016-2020 Apple, Inc. All rights reserved.
 * Some portions covered by other copyrights, listed below.
 *---
 * Copyright (C) 2016 and later: Unicode, Inc. and others.
 * License & terms of use: http://www.unicode.org/copyright.html
 *---
 * Copyright (C) 1999-2015, International Business Machines
 * Corporation and others.  All Rights Reserved.
 *
 * add APPLE_OSREFERENCE_LICENSE_HEADER stuff...
 */

#include <libkern/libkern.h>
#include <sys/errno.h>
#include <sys/unicode.h>
#include "vfs_unicode_data.h"
#define STATIC_UNLESS_TEST static

enum {
	                                                                                    
	kMaxUTF8BytesPerChar = 4
};

                                                                                       
STATIC_UNLESS_TEST
int32_t utf8ToU32Code(int32_t u32char, const char** srcPtr, const char* srcLimit);
STATIC_UNLESS_TEST
int32_t normalizeOptCaseFoldU32Char(int32_t u32char, bool case_sens,
    int32_t u32NormFoldBuf[kNFCSingleCharDecompMax],
    uint8_t combClass[kNFCSingleCharDecompMax]);
                                                                                     
static int nextBaseAndAnyMarks(const char** strP, const char *strLimit, bool case_sens,
    int32_t* unorm, uint8_t* unormcc, int32_t* unormlenP, int32_t* unormstartP,
    int32_t* buf, uint8_t* bufcc, int32_t* buflenP,
    bool* needReorderP, bool* startP);
void doReorder(int32_t* buf, uint8_t* bufcc, int32_t buflen);
int32_t u32CharToUTF8Bytes(uint32_t u32char, uint8_t utf8Bytes[kMaxUTF8BytesPerChar]);

  
                                       
  
                                                                        
                                                                        
                                                                        
                                                                                     
                                                                            
                                               
   
void
utf8_normalizeOptCaseFoldGetUVersion(unsigned char version[4])
{
	version[0] = 13;
	version[1] = 0;
	version[2] = 0;
	version[3] = 0;
	return;
}

  
                                   
  
                                                               
                                                                              
                                                                                     
                                                                       
                                                                        
                                                                       
                                                                       
                            
                                                
  
                            
                                                                            
                                                                              
                                                                                
                                                                             
   

int
utf8_normalizeOptCaseFoldAndHash(const char *str,
    size_t      str_len,
    bool        case_sens,
    void      (*hash_func)(void *buf, size_t buf_len, void *ctx),
    void       *hash_ctx)
{
	const char *strLimit = str + str_len;

	                                                         
                                                                
                                                                    
	int32_t unorm[kNFCSingleCharDecompMax];
	uint8_t unormcc[kNFCSingleCharDecompMax];
	int32_t unormlen = 0;
	int32_t unormstart = 0;

	bool start = true;

	             
                                                                                   
                                                                               
                                                                                   
                                                                              
                              
   
                                                                                   
               
   
                                                        
                                                                                     
                                                                                      
                                                                                      
                                                                                 
                                   
                                                                                   
                                            
                                                                                  
                                                                                   
              
                                                                                    
                                                                                    
                                                                               
                                                                                
                                                                                   
                                                                     
                                                                    
   
    
	do {
		                                                    
		int32_t buf[kNCFStreamSafeBufMax];
		uint8_t bufcc[kNCFStreamSafeBufMax];
		int32_t buflen = 0;
		bool needReorder = false;
		int err;

		err = nextBaseAndAnyMarks(&str, strLimit, case_sens, unorm, unormcc, &unormlen, &unormstart,
		    buf, bufcc, &buflen, &needReorder, &start);
		if (err != 0) {
			return err;
		}

		if (buflen > 0) {
			                                                                              
                                                                                 
                                                  
			                                               
			if (needReorder) {
				doReorder(buf, bufcc, buflen);
			}
			                            
			hash_func(buf, buflen * sizeof(buf[0]), hash_ctx);
		}
		                                                                    
	} while (str < strLimit || unormlen > 0);
	return 0;
}

  
                                      
  
                                                                      
                                                                  
                                                                               
                                                                  
                                                                                     
                                                                            
                                                                               
                              
  
                            
                                                                                 
                                                                                      
                                                                                   
                                                                             
                                                                                 
                                                                                  
                                                             
   
enum { kNFCSingleCharDecompMaxPlusPushback = kNFCSingleCharDecompMax + 4 };                                

int
utf8_normalizeOptCaseFoldAndCompare(const char *strA,
    size_t      strA_len,
    const char *strB,
    size_t      strB_len,
    bool        case_sens,
    bool       *are_equal)
{
	const char *strALimit = strA + strA_len;
	const char *strBLimit = strB + strB_len;

	                                                               
                                                                 
                                                                     
	int32_t unormA[kNFCSingleCharDecompMaxPlusPushback], unormB[kNFCSingleCharDecompMaxPlusPushback];
	uint8_t unormAcc[kNFCSingleCharDecompMaxPlusPushback], unormBcc[kNFCSingleCharDecompMaxPlusPushback];
	int32_t unormAlen = 0, unormBlen = 0;
	int32_t unormAstart = 0, unormBstart = 0;

	bool startA = true, startB = true;

	             
                                                                                       
                                         
                                                                     
                                                        
                                                                              
                                                                 
                                                                              
                                                                            
    
	do {
		                                                         
		int32_t bufA[kNCFStreamSafeBufMax], bufB[kNCFStreamSafeBufMax];
		uint8_t bufAcc[kNCFStreamSafeBufMax], bufBcc[kNCFStreamSafeBufMax];
		int32_t bufAlen = 0, bufBlen = 0;
		bool needReorderA = false, needReorderB = false;
		int err;

		err = nextBaseAndAnyMarks(&strA, strALimit, case_sens, unormA, unormAcc, &unormAlen, &unormAstart,
		    bufA, bufAcc, &bufAlen, &needReorderA, &startA);
		if (err != 0) {
			return err;
		}
		err = nextBaseAndAnyMarks(&strB, strBLimit, case_sens, unormB, unormBcc, &unormBlen, &unormBstart,
		    bufB, bufBcc, &bufBlen, &needReorderB, &startB);
		if (err != 0) {
			return err;
		}

		if (bufAlen > 0 || bufBlen > 0) {
			                                                                                   
                                                                                 
                                                  
			                                               
			if (needReorderA) {
				doReorder(bufA, bufAcc, bufAlen);
			}
			if (needReorderB) {
				doReorder(bufB, bufBcc, bufBlen);
			}
			                          
			int32_t idx;
			if (!case_sens) {
				if (bufAlen > 1 && bufA[bufAlen - 1] == 0x03B9 && unormAstart == 0) {
					int32_t tailCount = 0;
					while (tailCount < kNFCSingleCharDecompMaxPlusPushback - unormAlen && bufAlen > 1 && bufA[bufAlen - 1] == 0x03B9) {
						tailCount++;
						bufAlen--;
					}
					for (idx = unormAlen; idx > 0; idx--) {
						unormA[idx - 1 + tailCount] = unormA[idx - 1];
						unormAcc[idx - 1 + tailCount] = unormAcc[idx - 1];
					}
					for (idx = 0; idx < tailCount; idx++) {
						unormA[idx] = 0x03B9;
						unormAcc[idx] = 0;
					}
					unormAlen += tailCount;
				}
				if (bufBlen > 1 && bufB[bufBlen - 1] == 0x03B9 && unormBstart == 0) {
					int32_t tailCount = 0;
					while (tailCount < kNFCSingleCharDecompMaxPlusPushback - unormBlen && bufBlen > 1 && bufB[bufBlen - 1] == 0x03B9) {
						tailCount++;
						bufBlen--;
					}
					for (idx = unormBlen; idx > 0; idx--) {
						unormB[idx - 1 + tailCount] = unormB[idx - 1];
						unormBcc[idx - 1 + tailCount] = unormBcc[idx - 1];
					}
					for (idx = 0; idx < tailCount; idx++) {
						unormB[idx] = 0x03B9;
						unormBcc[idx] = 0;
					}
					unormBlen += tailCount;
				}
			}
			                              
			if (bufAlen != bufBlen || memcmp(bufA, bufB, bufAlen * sizeof(bufA[0])) != 0) {
				*are_equal = false;
				return 0;
			}
		}
		                                                                    
	} while ((strA < strALimit || unormAlen > 0) && (strB < strBLimit || unormBlen > 0));

	*are_equal = (strA == strALimit && unormAlen == 0 && strB == strBLimit && unormBlen == 0);
	return 0;
}

  
                            
  
                                                               
                                                                              
                                                                                     
                                                                       
                                                                    
                                                    
                                                                              
                                                           
  
                            
                                                                            
                                                                              
                                                                                
                                                               
                                                                               
                                                                    
   
int
utf8_normalizeOptCaseFold(const char *str,
    size_t      str_len,
    bool        case_sens,
    int32_t    *ustr,
    int32_t     ustr_size,
    int32_t    *ustr_len)
{
	const char *strLimit = str + str_len;
	int32_t *ustrCur = ustr;
	const int32_t *ustrLimit = ustr + ustr_size;

	                                                         
                                                                   
	int32_t unorm[kNFCSingleCharDecompMax];
	uint8_t unormcc[kNFCSingleCharDecompMax];
	int32_t unormlen = 0;
	int32_t unormstart = 0;

	bool start = true;

	*ustr_len = 0;
	do {
		                                                    
		int32_t buf[kNCFStreamSafeBufMax];
		uint8_t bufcc[kNCFStreamSafeBufMax];
		int32_t buflen = 0;
		bool needReorder = false;
		int err;

		err = nextBaseAndAnyMarks(&str, strLimit, case_sens, unorm, unormcc, &unormlen, &unormstart,
		    buf, bufcc, &buflen, &needReorder, &start);
		if (err != 0) {
			return err;
		}

		if (buflen > 0) {
			if (needReorder) {
				doReorder(buf, bufcc, buflen);
			}
			                               
			int32_t idx;
			if (ustrCur + buflen > ustrLimit) {
				return ENOMEM;
			}
			for (idx = 0; idx < buflen; idx++) {
				*ustrCur++ = buf[idx];
			}
		}
		                                                                    
	} while (str < strLimit || unormlen > 0);
	*ustr_len = (uint32_t)(ustrCur - ustr);                                                                                 
	return 0;
}

  
                                  
                                                                                   
                                                                           
  
                                                               
                                                                              
                                                                                     
                                                                       
                                                                   
                                             
                                                                              
                                                    
  
                            
                                                                            
                                                                              
                                                                                
                                                               
                                                                               
                                                                    
   
int
utf8_normalizeOptCaseFoldToUTF8(const char *str,
    size_t      str_len,
    bool        case_sens,
    char       *ustr,
    size_t      ustr_size,
    size_t     *ustr_len)
{
	const char *strLimit = str + str_len;
	char *ustrCur = ustr;
	const char *ustrLimit = ustr + ustr_size;

	                                                         
                                                                   
	int32_t unorm[kNFCSingleCharDecompMax];
	uint8_t unormcc[kNFCSingleCharDecompMax];
	int32_t unormlen = 0;
	int32_t unormstart = 0;

	bool start = true;

	*ustr_len = 0;
	do {
		                                                    
		int32_t buf[kNCFStreamSafeBufMax];
		uint8_t bufcc[kNCFStreamSafeBufMax];
		int32_t buflen = 0;
		bool needReorder = false;
		int err;

		err = nextBaseAndAnyMarks(&str, strLimit, case_sens, unorm, unormcc, &unormlen, &unormstart,
		    buf, bufcc, &buflen, &needReorder, &start);
		if (err != 0) {
			return err;
		}

		if (buflen > 0) {
			uint8_t utf8Bytes[kMaxUTF8BytesPerChar];
			int32_t *bufPtr = buf;
			if (needReorder) {
				doReorder(buf, bufcc, buflen);
			}
			                               
			while (buflen-- > 0) {
				int32_t idx, utf8Len = u32CharToUTF8Bytes((uint32_t)*bufPtr++, utf8Bytes);
				if (ustrCur + utf8Len > ustrLimit) {
					return ENOMEM;
				}
				for (idx = 0; idx < utf8Len; idx++) {
					*ustrCur++ = (char)utf8Bytes[idx];
				}
			}
		}
		                                                                    
	} while (str < strLimit || unormlen > 0);
	*ustr_len = ustrCur - ustr;
	return 0;
}

  
                                             
  
                                                                                  
                                           
                                                                  
                                                                                     
                                                                                       
                                                                                     
                           
                                                                               
                                                                                     
                                                                            
                                                                                  
                                                                 
                              
                                                                                      
                                                                    
  
                            
                                                                                     
                                                                                      
                                                                                    
                              
                                                                                      
                                                                                       
                                             
   
int
utf8_normalizeOptCaseFoldAndMatchSubstring(const char    *strA,
    size_t         strA_len,
    const int32_t *ustrB,
    int32_t        ustrB_len,
    bool           case_sens,
    void          *buf,
    size_t         buf_size,
    bool          *has_match)
{
	  
                                                                                  
                                                                                  
                                                                               
                           
                                    
                                                                              
                                                                                 
                                                                               
                                                                                 
                                                                                     
                                                                               
                                                                              
                                    
                                                                                
                                                                               
                                                                           
    
	if (ustrB_len == 0) {                     
		*has_match = true;
		return 0;
	}
	*has_match = false;                              
	if (ustrB_len > 2 * strA_len) {
		                                                                               
                                                                         
                                                                                  
                                                                            
                                      
		return 0;
	}

	const char *strALimit = strA + strA_len;
	int32_t *ustrA = (int32_t *)buf;
	const int32_t *ustrALimit = ustrA + (buf_size / sizeof(int32_t));
	int32_t *ustrANormEnd = ustrA;                                                  

	                                                               
                                                                 
                                                                     
	int32_t unormA[kNFCSingleCharDecompMax];
	uint8_t unormAcc[kNFCSingleCharDecompMax];
	int32_t unormAlen = 0;
	int32_t unormAstart = 0;

	bool startA = true;

	while (true) {
		                                                                                  
		if (ustrANormEnd - ustrA < ustrB_len) {
			do {
				                                                         
				int32_t bufA[kNCFStreamSafeBufMax];
				uint8_t bufAcc[kNCFStreamSafeBufMax];
				int32_t bufAlen = 0;
				bool needReorderA = false;
				int err;

				err = nextBaseAndAnyMarks(&strA, strALimit, case_sens, unormA, unormAcc, &unormAlen, &unormAstart,
				    bufA, bufAcc, &bufAlen, &needReorderA, &startA);
				if (err != 0) {
					return err;
				}

				if (bufAlen > 0) {
					                                                                                   
                                                                                   
                                                    
					                                                                                 
                                                                                      
					if (needReorderA) {
						doReorder(bufA, bufAcc, bufAlen);
					}
					                                
					int32_t idx;
					if (ustrANormEnd + bufAlen > ustrALimit) {
						return ENOMEM;
					}
					for (idx = 0; idx < bufAlen; idx++) {
						*ustrANormEnd++ = bufA[idx];
					}
				}
				                                                                    
			} while ((ustrANormEnd - ustrA < ustrB_len) && (strA < strALimit || unormAlen > 0));
		}

		if (ustrANormEnd - ustrA < ustrB_len) {
			return 0;                                        
		}
		                                   
		if (memcmp(ustrA, ustrB, ustrB_len * sizeof(ustrB[0])) == 0) {
			*has_match = true;
			return 0;
		}
		ustrA++;                             
	}
}

                       
                                                                                 
                                    
                                                                                  
             
                                                                                    
                                                                                     
                                                                                     
                                                                                
                                  
                                                                                  
                                           
                                                                                 
                                                                                  
             
                                                                                   
                                                                                   
   

static int
nextBaseAndAnyMarks(const char** strP, const char *strLimit, bool case_sens,
    int32_t* unorm, uint8_t* unormcc, int32_t* unormlenP, int32_t* unormstartP,
    int32_t* buf, uint8_t* bufcc, int32_t* buflenP,
    bool* needReorderP, bool* startP)
{
	                            
	if (*unormlenP > 0 && *unormstartP < *unormlenP) {
		                                                       
		*needReorderP = false;
		for (*buflenP = 0; true;) {
			if (*buflenP > 0 && unormcc[*unormstartP] > 0 && unormcc[*unormstartP] < bufcc[(*buflenP) - 1]) {
				*needReorderP = true;
			}
			buf[*buflenP] = unorm[*unormstartP];
			bufcc[(*buflenP)++] = unormcc[(*unormstartP)++];
			if (*unormstartP >= *unormlenP || unormcc[*unormstartP] == 0) {
				break;
			}
		}
	}
	if (*unormstartP >= *unormlenP) {
		*unormstartP = *unormlenP = 0;
		while (*strP < strLimit) {
			int32_t idx;
			uint32_t bytevalue = (uint8_t)*(*strP)++;
			                                                                            
                                          
			if (bytevalue == 0 || bytevalue == 0x2F        ) {
				return EILSEQ;
			}
			if (bytevalue < 0x80) {
				unorm[0] = (!case_sens && bytevalue >= 'A' && bytevalue <= 'Z')? bytevalue += 0x20: bytevalue;
				*unormlenP = 1;
				unormcc[0] = 0;
				*startP = false;
				break;
			} else {
				int32_t u32char = utf8ToU32Code(bytevalue, strP, strLimit);
				if (u32char <= 0) {
					return EILSEQ;
				}
				*unormlenP = normalizeOptCaseFoldU32Char(u32char, case_sens, unorm, unormcc);
				if (*unormlenP <= 0) {
					return EILSEQ;
				}
				if (unormcc[0] == 0 || *startP) {
					*startP = false;
					break;
				}
			}
			                                                                                      
			if (*buflenP + *unormlenP > kNCFStreamSafeBufMax) {
				return EILSEQ;
			}
			for (idx = 0; idx < *unormlenP; idx++, (*buflenP)++) {
				if (*buflenP > 0 && unormcc[idx] > 0 && unormcc[idx] < bufcc[(*buflenP) - 1]) {
					*needReorderP = true;
				}
				buf[*buflenP] = unorm[idx];
				bufcc[*buflenP] = unormcc[idx];
			}
			*unormlenP = 0;
		}
	}
	return 0;
}

                                                       
static void swapBufCharCCWithPrevious(int32_t jdx, int32_t buf[], uint8_t bufcc[]);
static int32_t adjustCase(bool case_sens, int32_t uSeqLen,
    int32_t u32NormFoldBuf[kNFCSingleCharDecompMax]);
static uint8_t getCombClassU32Char(int32_t u32char);
static int32_t decomposeHangul(int32_t u32char, int32_t u32NormFoldBuf[kNFCSingleCharDecompMax]);

                                                                             
                                                                                 
void
doReorder(int32_t* buf, uint8_t* bufcc, int32_t buflen)
{
	int32_t idx, jdx;
	for (idx = 0; idx < buflen - 1; idx++) {
		for (jdx = buflen - 1; jdx > idx; jdx--) {
			if (bufcc[jdx] < bufcc[jdx - 1]) {
				swapBufCharCCWithPrevious(jdx, buf, bufcc);
			}
		}
	}
}
                                   
static void
swapBufCharCCWithPrevious(int32_t jdx, int32_t buf[], uint8_t bufcc[])
{
	int32_t bufchar = buf[jdx];
	uint8_t bufccval = bufcc[jdx];
	buf[jdx] = buf[jdx - 1];
	bufcc[jdx] = bufcc[jdx - 1];
	buf[jdx - 1] = bufchar;
	bufcc[jdx - 1] = bufccval;
}

  
                                                                                           
                                        
  
                                                    
   
int32_t
u32CharToUTF8Bytes(uint32_t u32char, uint8_t utf8Bytes[kMaxUTF8BytesPerChar])
{
	int32_t idx = 0;
	if (u32char <= 0x7F) {
		utf8Bytes[idx++] = (uint8_t)u32char;
	} else {
		if (u32char <= 0x7FF) {
			utf8Bytes[idx++] = (uint8_t)((u32char >> 6) | 0xC0);
		} else {
			if (u32char <= 0xFFFF) {
				utf8Bytes[idx++] = (uint8_t)((u32char >> 12) | 0xE0);
			} else {
				utf8Bytes[idx++] = (uint8_t)((u32char >> 18) | 0xF0);
				utf8Bytes[idx++] = (uint8_t)(((u32char >> 12) & 0x3F) | 0x80);
			}
			utf8Bytes[idx++] = (uint8_t)(((u32char >> 6) & 0x3F) | 0x80);
		}
		utf8Bytes[idx++] = (uint8_t)((u32char & 0x3F) | 0x80);
	}
	return idx;
}

                                          
#define U8_COUNT_TRAIL_BYTES_LOC(leadByte) \
((uint8_t)(leadByte)<0XF0 ? \
((uint8_t)(leadByte)>=0XC0)+((uint8_t)(leadByte)>=0XE0) : \
(uint8_t)(leadByte)<0XFE ? 3+((uint8_t)(leadByte)>=0XF8)+((uint8_t)(leadByte)>=0XFC) : 0)

#define U8_MASK_LEAD_BYTE_LOC(leadByte, countTrailBytes) ((leadByte)&=(1<<(6-(countTrailBytes)))-1)

                                         
static const int32_t utf8_minLegal[4] = { 0, 0X80, 0x800, 0x10000 };

  
                                                                                               
  
                                                                                           
                                                                                      
  
                         
                                                                                            
                                                
                                                                                            
                                                   
                                                                                         
  
                                                                                       
                                                                                    
                                           
                                                                                        
                                                                                        
                                                                      
  
                                         
                   
                    
  
                                                
   
STATIC_UNLESS_TEST
int32_t
utf8ToU32Code(int32_t u32char, const char** srcPtr, const char* srcLimit)
{
	const char* src = *srcPtr;
	uint8_t pt1, pt2;
	if (0xE0 < u32char && u32char <= 0xEC && src + 1 < srcLimit && (pt1 = (uint8_t)(src[0] - 0x80)) <= 0x3F && (pt2 = (uint8_t)(src[1] - 0x80)) <= 0x3F) {
		                           
		                                                                                                         
		u32char = (uint16_t)((u32char << 12) | (pt1 << 6) | pt2);
		src += 2;
	} else if (u32char < 0xE0 && u32char >= 0xC2 && src < srcLimit && (pt1 = (uint8_t)(src[0] - 0x80)) <= 0x3F) {
		                           
		u32char = ((u32char & 0x1F) << 6) | pt1;
		src++;
	} else {
		                                                                               
		uint8_t count = U8_COUNT_TRAIL_BYTES_LOC(u32char);
		if (src + count <= srcLimit) {
			uint8_t trail;

			U8_MASK_LEAD_BYTE_LOC(u32char, count);
			switch (count) {
			                                                
			case 0:                                                                                             
			case 5:
			case 4:                                                                                         
				break;
			case 3:
				trail = *src++ - 0X80;
				u32char = (u32char << 6) | trail;
				                                                                         
				if (u32char >= 0x110 || trail > 0X3F) {
					break;
				}
			case 2:
				trail = *src++ - 0X80;
				u32char = (u32char << 6) | trail;
				  
                                       
                                                                    
       
				if (((u32char & 0xFFE0) == 0x360) || trail > 0X3F) {
					break;
				}
			case 1:
				trail = *src++ - 0X80;
				u32char = (u32char << 6) | trail;
				if (trail > 0X3F) {
					break;
				}
				                                                            
				if (u32char >= utf8_minLegal[count]) {
					*srcPtr = src;
					return u32char;
				}
				                                                                      
			}
		}
		u32char = -1;
	}
	*srcPtr = src;
	return u32char;
}

  
                                                                                   
                                                                                
  
                                                                                   
                                    
  
                                   
                                                                                         
                                                                                             
                                            
                                                                                        
                                                                                     
                                                                                       
  
                                                                                            
                                                                                  
                                                                   
  
                                                
  
                                                                                      
                                                          
   
STATIC_UNLESS_TEST
int32_t
normalizeOptCaseFoldU32Char(int32_t u32char, bool case_sens,
    int32_t u32NormFoldBuf[kNFCSingleCharDecompMax],
    uint8_t combClass[kNFCSingleCharDecompMax])
{
	combClass[0] = 0;
	                                                         
	if (u32char >= kU32HiPUAStart) {
		if ((u32char & 0xFFFE) == 0xFFFE) {
			return -1;
		}
		u32NormFoldBuf[0] = u32char;
		return 1;
	}
	                                                                                       
	                                                               
	int32_t u32charLookup = u32char;
	if (u32charLookup >= kU32LowRangeLimit) {
		u32charLookup -= (kU32HiRangeStart - kU32LowRangeLimit);
		if (u32charLookup < kU32LowRangeLimit || u32charLookup >= (kU32LowRangeLimit + kU32HiRangeLen)) {
			return -1;                                                             
		}
	}
	                                                                              
                                                                                
                                                                          
                                                
	uint16_t trieValue;
	             
	trieValue = nfTrieHi[u32charLookup >> kNFTrieHiShift];
	if (trieValue == kInvalidCodeFlag) {
		return -1;
	}
	if (trieValue == 0 || (trieValue & kFlagTestMask) == kCombClassFlag) {                    
		u32NormFoldBuf[0] = u32char;
		combClass[0] = trieValue & kFlagValueMask;
		return 1;
	}
	if (trieValue == kHangulMask) {
		combClass[1] = combClass[2] = 0;
		return decomposeHangul(u32char, u32NormFoldBuf);
	}
	              
	trieValue = nfTrieMid[trieValue & kNextIndexValueMask][(u32charLookup >> kNFTrieMidShift) & kNFTrieMidMask];
	if (trieValue == kInvalidCodeFlag) {
		return -1;
	}
	if (trieValue == 0 || (trieValue & kFlagTestMask) == kCombClassFlag) {
		u32NormFoldBuf[0] = u32char;
		combClass[0] = trieValue & kFlagValueMask;
		return adjustCase(case_sens, 1, u32NormFoldBuf);
	}
	if ((trieValue & kFlagTestMask) == kInvMaskFlag) {
		uint16_t invalidMask = nfU16InvMasks[trieValue & kFlagValueMask];
		uint16_t testBit = (uint16_t)(1 << (u32charLookup & kNFTrieLoMask));
		if (testBit & invalidMask) {
			             
			return -1;
		} else {
			                                     
			u32NormFoldBuf[0] = u32char;
			return adjustCase(case_sens, 1, u32NormFoldBuf);;
		}
	}
	if (trieValue == kHangulMask) {
		combClass[1] = combClass[2] = 0;
		return decomposeHangul(u32char, u32NormFoldBuf);
	}
	             
	trieValue = nfTrieLo[trieValue & kNextIndexValueMask][u32charLookup & kNFTrieLoMask];
	if (trieValue == kInvalidCodeFlag) {
		return -1;
	}
	if (trieValue == kHangulMask) {
		combClass[1] = combClass[2] = 0;
		return decomposeHangul(u32char, u32NormFoldBuf);
	}
	if (trieValue < kToU16Seq2Mask || trieValue > kSpecialsEnd) {
		if (trieValue == 0 || (trieValue & kFlagTestMask) == kCombClassFlag) {
			u32NormFoldBuf[0] = u32char;
			combClass[0] = trieValue & kFlagValueMask;
		} else {
			u32NormFoldBuf[0] = trieValue;
		}
		return adjustCase(case_sens, 1, u32NormFoldBuf);;
	}
	const uint16_t* u16SeqPtr = NULL;
	const int32_t*  u32SeqPtr = NULL;
	int32_t         uSeqLen = 0;
	switch (trieValue & kSpecialsMask) {
	case kToU16Seq2Mask:
		if (case_sens && (trieValue & kToSeqCaseFoldMask)) {
			                                                        
			u32NormFoldBuf[0] = u32char;
			                                   
			return 1;
		}
		u16SeqPtr = nfU16Seq2[trieValue & kToSeqIndexMask];
		uSeqLen = 2;
		break;
	case kToU16Seq3Mask:
		if (case_sens && (trieValue & kToSeqCaseFoldMask)) {
			                                                        
			u32NormFoldBuf[0] = u32char;
			                                   
			return 1;
		}
		u16SeqPtr = nfU16Seq3[trieValue & kToSeqIndexMask];
		uSeqLen = 3;
		break;
	case kToU16SeqMiscMask:
		u16SeqPtr = &nfU16SeqMisc[trieValue & kToSeqMiscIndexMask];
		uSeqLen = *u16SeqPtr & kToSeqMiscLenMask;
		combClass[0] = (uint8_t)(*u16SeqPtr++ >> kToSeqMiscCCShift);
		break;
	case kToU32CharMask:
		if (case_sens && (trieValue & kToSeqCaseFoldMask)) {
			                                                        
			u32NormFoldBuf[0] = u32char;
			                                   
			return 1;
		}
		u32SeqPtr = &nfU32Char[trieValue & kToSeqIndexMask];
		uSeqLen = 1;
		break;
	case kToU32SeqMiscMask:
		u32SeqPtr = &nfU32SeqMisc[trieValue & kToSeqMiscIndexMask];
		uSeqLen = *u32SeqPtr & kToSeqMiscLenMask;
		combClass[0] = (uint8_t)(*u32SeqPtr++ >> kToSeqMiscCCShift);
		break;
	default:
		return -1;
	}
	if (kNFCSingleCharDecompMax < uSeqLen) {
		return 0;
	}
	int32_t idx;
	for (idx = 0; idx < uSeqLen; idx++) {
		u32NormFoldBuf[idx] = (u16SeqPtr)? *u16SeqPtr++: *u32SeqPtr++;
		if (idx > 0) {
			combClass[idx] = getCombClassU32Char(u32NormFoldBuf[idx]);
		}
	}
	return adjustCase(case_sens, uSeqLen, u32NormFoldBuf);
}

  
                                                                                
  
                                                                                         
                                                                   
                                                                                      
                                   
  
                                                                                                 
                                                                                  
                                                                   
  
                                                                           
   

static int32_t
adjustCase(bool case_sens, int32_t uSeqLen,
    int32_t u32NormFoldBuf[kNFCSingleCharDecompMax])
{
	if (!case_sens && uSeqLen > 0) {
		if (u32NormFoldBuf[0] < kSimpleCaseFoldLimit) {
			u32NormFoldBuf[0] = nfBasicCF[u32NormFoldBuf[0]];
			                                                                               
                                                                                 
                                                                           
                                                     
      
		}
		                                                                             
                                                                                 
                                              
     
		if (uSeqLen > 1 && u32NormFoldBuf[uSeqLen - 1] == 0x0345) {
			u32NormFoldBuf[uSeqLen - 1] = 0x03B9;
		}
	}
	return uSeqLen;
}

  
                                                                                      
  
                                                                                      
                            
  
                                            
  
                                                                                      
                                                                                      
                                                                                       
                                                                                     
  
                                                                           
   
static uint8_t
getCombClassU32Char(int32_t u32char)
{
	if (u32char >= kU32HiPUAStart) {
		return 0;
	}
	if (u32char == 0x03B9) {
		return 240;
	}
	                                                                                       
	                                                               
	int32_t u32charLookup = u32char;
	if (u32charLookup >= kU32LowRangeLimit) {
		u32charLookup -= (kU32HiRangeStart - kU32LowRangeLimit);
	}
	                                                                              
                                                                                
                                                                          
                                                
	uint16_t trieValue;
	             
	trieValue = nfTrieHi[u32charLookup >> kNFTrieHiShift];
	if (trieValue == 0 || (trieValue & kFlagTestMask) == kCombClassFlag) {
		return trieValue & kFlagValueMask;
	}
	              
	trieValue = nfTrieMid[trieValue & kNextIndexValueMask][(u32charLookup >> kNFTrieMidShift) & kNFTrieMidMask];
	if (trieValue == 0 || (trieValue & kFlagTestMask) == kCombClassFlag) {                    
		return trieValue & kFlagValueMask;
	}
	if ((trieValue & kFlagTestMask) == kInvMaskFlag) {
		return 0;
	}
	             
	trieValue = nfTrieLo[trieValue & kNextIndexValueMask][u32charLookup & kNFTrieMidMask];
	return ((trieValue & kFlagTestMask) == kCombClassFlag)? (trieValue & kFlagValueMask): 0;
}

  
                                                                       
                                                   
  
                                                         
  
                                   
                                                                                             
                                            
  
                                                      
  
                                                         
  
   

enum {
	HANGUL_BASE=0xAC00,
	JAMO_L_BASE=0x1100,                      
	JAMO_V_BASE=0x1161,                       
	JAMO_T_BASE=0x11A7,                       
	JAMO_L_COUNT=19,
	JAMO_V_COUNT=21,
	JAMO_T_COUNT=28,
};

static int32_t
decomposeHangul(int32_t u32char, int32_t u32NormFoldBuf[kNFCSingleCharDecompMax])
{
	u32char -= HANGUL_BASE;
	int32_t tIndex = u32char % JAMO_T_COUNT;
	u32char /= JAMO_T_COUNT;
	u32NormFoldBuf[0] = (uint16_t)(JAMO_L_BASE + u32char / JAMO_V_COUNT);
	u32NormFoldBuf[1] = (uint16_t)(JAMO_V_BASE + u32char % JAMO_V_COUNT);
	if (tIndex == 0) {
		return 2;
	}
	u32NormFoldBuf[2] = (uint16_t)(JAMO_T_BASE + tIndex);
	return 3;
}
