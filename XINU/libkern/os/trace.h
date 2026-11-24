/*
 * Copyright (c) 2013-2015 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#ifndef __OS_TRACE_H__
#define __OS_TRACE_H__

#include <Availability.h>
#include <os/base.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#if __has_include(<xpc/xpc.h>)
#include <xpc/xpc.h>
#else
typedef void *xpc_object_t;
#endif

#if !__GNUC__
#error "must be GNU C compatible"
#endif

__BEGIN_DECLS

extern void *__dso_handle;

OS_ALWAYS_INLINE
static inline void
_os_trace_verify_printf(const char *msg, ...)
__attribute__((format(printf, 1, 2)))
{
#pragma unused(msg)
}

#if !defined OS_COUNT_ARGS
#define OS_COUNT_ARGS(...) OS_COUNT_ARGS1(, ##__VA_ARGS__, _8, _7, _6, _5, _4, _3, _2, _1, _0)
#define OS_COUNT_ARGS1(z, a, b, c, d, e, f, g, h, cnt, ...) cnt
#endif

                                                                  
#if (defined(__IPHONE_OS_VERSION_MIN_REQUIRED) && __IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_10_0) \
        || (defined(__WATCH_OS_VERSION_MIN_REQUIRED) && __WATCH_OS_VERSION_MIN_REQUIRED < __WATCHOS_3_0) \
        || (defined(__TV_OS_VERSION_MIN_REQUIRED) && __TV_OS_VERSION_MIN_REQUIRED < __TVOS_10_0) \
        || (defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && __MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_10_12)

#define _os_trace_0(_l, _m, _t) __extension__({ \
	_os_trace_verify_printf(_l); \
	_os_trace_with_buffer(&__dso_handle, _m, _t, NULL, 0, NULL); \
	__asm__("");                      \
})

#define _os_trace_1(_l, _m, _t, _1) __extension__({ \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wpacked\"") \
	const __typeof__(_1) _c1 = _1; \
	_os_trace_verify_printf(_l, _c1); \
	const struct __attribute__((packed)) { \
	        __typeof__(_c1) _f1; \
	        uint8_t _s[1]; \
	        uint8_t _cnt; \
	} _buf = { \
	        ._f1 = _c1, ._s[0] = sizeof(_c1), \
	        ._cnt = 1, \
	}; \
	_os_trace_with_buffer(&__dso_handle, _m, _t, &_buf, sizeof(_buf), NULL); \
	__asm__("");                      \
	_Pragma("clang diagnostic pop") \
})

#define _os_trace_2(_l, _m, _t, _1, _2) __extension__({ \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wpacked\"") \
	const __typeof__(_1) _c1 = _1; \
	const __typeof__(_2) _c2 = _2; \
	_os_trace_verify_printf(_l, _c1, _c2); \
	const struct __attribute__((packed)) { \
	        __typeof__(_c1) _f1; \
	        __typeof__(_c2) _f2; \
	        uint8_t _s[2]; \
	        uint8_t _cnt; \
	} _buf = { \
	        ._f1 = _c1, ._s[0] = sizeof(_c1), \
	        ._f2 = _c2, ._s[1] = sizeof(_c2), \
	        ._cnt = 2, \
	}; \
	_os_trace_with_buffer(&__dso_handle, _m, _t, &_buf, sizeof(_buf), NULL); \
	__asm__("");                      \
	_Pragma("clang diagnostic pop") \
})

#define _os_trace_3(_l, _m, _t, _1, _2, _3) __extension__({ \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wpacked\"") \
	const __typeof__(_1) _c1 = _1; \
	const __typeof__(_2) _c2 = _2; \
	const __typeof__(_3) _c3 = _3; \
	_os_trace_verify_printf(_l, _c1, _c2, _c3); \
	const struct __attribute__((packed)) { \
	        __typeof__(_c1) _f1; \
	        __typeof__(_c2) _f2; \
	        __typeof__(_c3) _f3; \
	        uint8_t _s[3]; \
	        uint8_t _cnt; \
	} _buf = { \
	        ._f1 = _c1, ._s[0] = sizeof(_c1), \
	        ._f2 = _c2, ._s[1] = sizeof(_c2), \
	        ._f3 = _c3, ._s[2] = sizeof(_c3), \
	        ._cnt = 3, \
	}; \
	_os_trace_with_buffer(&__dso_handle, _m, _t, &_buf, sizeof(_buf), NULL); \
	__asm__("");                      \
	_Pragma("clang diagnostic pop") \
})

#define _os_trace_4(_l, _m, _t, _1, _2, _3, _4) __extension__({ \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wpacked\"") \
	const __typeof__(_1) _c1 = _1; \
	const __typeof__(_2) _c2 = _2; \
	const __typeof__(_3) _c3 = _3; \
	const __typeof__(_4) _c4 = _4; \
	_os_trace_verify_printf(_l, _c1, _c2, _c3, _c4); \
	const struct __attribute__((packed)) { \
	        __typeof__(_c1) _f1; \
	        __typeof__(_c2) _f2; \
	        __typeof__(_c3) _f3; \
	        __typeof__(_c4) _f4; \
	        uint8_t _s[4]; \
	        uint8_t _cnt; \
	} _buf = { \
	        ._f1 = _c1, ._s[0] = sizeof(_c1), \
	        ._f2 = _c2, ._s[1] = sizeof(_c2), \
	        ._f3 = _c3, ._s[2] = sizeof(_c3), \
	        ._f4 = _c4, ._s[3] = sizeof(_c4), \
	        ._cnt = 4, \
	}; \
	_os_trace_with_buffer(&__dso_handle, _m, _t, &_buf, sizeof(_buf), NULL); \
	__asm__("");                      \
	_Pragma("clang diagnostic pop") \
})

#define _os_trace_5(_l, _m, _t, _1, _2, _3, _4, _5) __extension__({ \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wpacked\"") \
	const __typeof__(_1) _c1 = _1; \
	const __typeof__(_2) _c2 = _2; \
	const __typeof__(_3) _c3 = _3; \
	const __typeof__(_4) _c4 = _4; \
	const __typeof__(_5) _c5 = _5; \
	_os_trace_verify_printf(_l, _c1, _c2, _c3, _c4, _c5); \
	const struct __attribute__((packed)) { \
	        __typeof__(_c1) _f1; \
	        __typeof__(_c2) _f2; \
	        __typeof__(_c3) _f3; \
	        __typeof__(_c4) _f4; \
	        __typeof__(_c5) _f5; \
	        uint8_t _s[5]; \
	        uint8_t _cnt; \
	} _buf = { \
	        ._f1 = _c1, ._s[0] = sizeof(_c1), \
	        ._f2 = _c2, ._s[1] = sizeof(_c2), \
	        ._f3 = _c3, ._s[2] = sizeof(_c3), \
	        ._f4 = _c4, ._s[3] = sizeof(_c4), \
	        ._f5 = _c5, ._s[4] = sizeof(_c5), \
	        ._cnt = 5, \
	}; \
	_os_trace_with_buffer(&__dso_handle, _m, _t, &_buf, sizeof(_buf), NULL); \
	__asm__("");                      \
	_Pragma("clang diagnostic pop") \
})

#define _os_trace_6(_l, _m, _t, _1, _2, _3, _4, _5, _6) __extension__({ \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wpacked\"") \
	const __typeof__(_1) _c1 = _1; \
	const __typeof__(_2) _c2 = _2; \
	const __typeof__(_3) _c3 = _3; \
	const __typeof__(_4) _c4 = _4; \
	const __typeof__(_5) _c5 = _5; \
	const __typeof__(_6) _c6 = _6; \
	_os_trace_verify_printf(_l, _c1, _c2, _c3, _c4, _c5, _c6); \
	const struct __attribute__((packed)) { \
	        __typeof__(_c1) _f1; \
	        __typeof__(_c2) _f2; \
	        __typeof__(_c3) _f3; \
	        __typeof__(_c4) _f4; \
	        __typeof__(_c5) _f5; \
	        __typeof__(_c6) _f6; \
	        uint8_t _s[6]; \
	        uint8_t _cnt; \
	} _buf = { \
	        ._f1 = _c1, ._s[0] = sizeof(_c1), \
	        ._f2 = _c2, ._s[1] = sizeof(_c2), \
	        ._f3 = _c3, ._s[2] = sizeof(_c3), \
	        ._f4 = _c4, ._s[3] = sizeof(_c4), \
	        ._f5 = _c5, ._s[4] = sizeof(_c5), \
	        ._f6 = _c6, ._s[5] = sizeof(_c6), \
	        ._cnt = 6, \
	}; \
	_os_trace_with_buffer(&__dso_handle, _m, _t, &_buf, sizeof(_buf), NULL); \
	__asm__("");                      \
	_Pragma("clang diagnostic pop") \
})

#define _os_trace_7(_l, _m, _t, _1, _2, _3, _4, _5, _6, _7) __extension__({ \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wpacked\"") \
	const __typeof__(_1) _c1 = _1; \
	const __typeof__(_2) _c2 = _2; \
	const __typeof__(_3) _c3 = _3; \
	const __typeof__(_4) _c4 = _4; \
	const __typeof__(_5) _c5 = _5; \
	const __typeof__(_6) _c6 = _6; \
	const __typeof__(_7) _c7 = _7; \
	_os_trace_verify_printf(_l, _c1, _c2, _c3, _c4, _c5, _c6, _c7); \
	const struct __attribute__((packed)) { \
	        __typeof__(_c1) _f1; \
	        __typeof__(_c2) _f2; \
	        __typeof__(_c3) _f3; \
	        __typeof__(_c4) _f4; \
	        __typeof__(_c5) _f5; \
	        __typeof__(_c6) _f6; \
	        __typeof__(_c7) _f7; \
	        uint8_t _s[7]; \
	        uint8_t _cnt; \
	} _buf = { \
	        ._f1 = _c1, ._s[0] = sizeof(_c1), \
	        ._f2 = _c2, ._s[1] = sizeof(_c2), \
	        ._f3 = _c3, ._s[2] = sizeof(_c3), \
	        ._f4 = _c4, ._s[3] = sizeof(_c4), \
	        ._f5 = _c5, ._s[4] = sizeof(_c5), \
	        ._f6 = _c6, ._s[5] = sizeof(_c6), \
	        ._f7 = _c7, ._s[6] = sizeof(_c7), \
	        ._cnt = 7, \
	}; \
	_os_trace_with_buffer(&__dso_handle, _m, _t, &_buf, sizeof(_buf), NULL); \
	__asm__("");                      \
	_Pragma("clang diagnostic pop") \
})

#define _os_trace_with_payload_1(_l, _m, _t, _payload) __extension__({ \
	_os_trace_verify_printf(_l); \
	_os_trace_with_buffer(&__dso_handle, _m, _t, NULL, 0, _payload); \
	__asm__("");                      \
})

#define _os_trace_with_payload_2(_l, _m, _t, _1, _payload) __extension__({ \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wpacked\"") \
	const __typeof__(_1) _c1 = _1; \
	_os_trace_verify_printf(_l, _c1); \
	const struct __attribute__((packed)) { \
	        __typeof__(_c1) _f1; \
	        uint8_t _s[1]; \
	        uint8_t _cnt; \
	} _buf = { \
	        ._f1 = _c1, ._s[0] = sizeof(_c1), \
	        ._cnt = 1, \
	}; \
	_os_trace_with_buffer(&__dso_handle, _m, _t, &_buf, sizeof(_buf), _payload); \
	__asm__("");                      \
	_Pragma("clang diagnostic pop") \
})

#define _os_trace_with_payload_3(_l, _m, _t, _1, _2, _payload) __extension__({ \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wpacked\"") \
	const __typeof__(_1) _c1 = _1; \
	const __typeof__(_2) _c2 = _2; \
	_os_trace_verify_printf(_l, _c1, _c2); \
	const struct __attribute__((packed)) { \
	        __typeof__(_c1) _f1; \
	        __typeof__(_c2) _f2; \
	        uint8_t _s[2]; \
	        uint8_t _cnt; \
	} _buf = { \
	        ._f1 = _c1, ._s[0] = sizeof(_c1), \
	        ._f2 = _c2, ._s[1] = sizeof(_c2), \
	        ._cnt = 2, \
	}; \
	_os_trace_with_buffer(&__dso_handle, _m, _t, &_buf, sizeof(_buf), _payload); \
	__asm__("");                      \
	_Pragma("clang diagnostic pop") \
})

#define _os_trace_with_payload_4(_l, _m, _t, _1, _2, _3, _payload) __extension__({ \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wpacked\"") \
	const __typeof__(_1) _c1 = _1; \
	const __typeof__(_2) _c2 = _2; \
	const __typeof__(_3) _c3 = _3; \
	_os_trace_verify_printf(_l, _c1, _c2, _c3); \
	const struct __attribute__((packed)) { \
	        __typeof__(_c1) _f1; \
	        __typeof__(_c2) _f2; \
	        __typeof__(_c3) _f3; \
	        uint8_t _s[3]; \
	        uint8_t _cnt; \
	} _buf = { \
	        ._f1 = _c1, ._s[0] = sizeof(_c1), \
	        ._f2 = _c2, ._s[1] = sizeof(_c2), \
	        ._f3 = _c3, ._s[2] = sizeof(_c3), \
	        ._cnt = 3, \
	}; \
	_os_trace_with_buffer(&__dso_handle, _m, _t, &_buf, sizeof(_buf), _payload); \
	__asm__("");                      \
	_Pragma("clang diagnostic pop") \
})

#define _os_trace_with_payload_5(_l, _m, _t, _1, _2, _3, _4, _payload) __extension__({ \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wpacked\"") \
	const __typeof__(_1) _c1 = _1; \
	const __typeof__(_2) _c2 = _2; \
	const __typeof__(_3) _c3 = _3; \
	const __typeof__(_4) _c4 = _4; \
	_os_trace_verify_printf(_l, _c1, _c2, _c3, _c4); \
	const struct __attribute__((packed)) { \
	        __typeof__(_c1) _f1; \
	        __typeof__(_c2) _f2; \
	        __typeof__(_c3) _f3; \
	        __typeof__(_c4) _f4; \
	        uint8_t _s[4]; \
	        uint8_t _cnt; \
	} _buf = { \
	        ._f1 = _c1, ._s[0] = sizeof(_c1), \
	        ._f2 = _c2, ._s[1] = sizeof(_c2), \
	        ._f3 = _c3, ._s[2] = sizeof(_c3), \
	        ._f4 = _c4, ._s[3] = sizeof(_c4), \
	        ._cnt = 4, \
	}; \
	_os_trace_with_buffer(&__dso_handle, _m, _t, &_buf, sizeof(_buf), _payload); \
	__asm__("");                      \
	_Pragma("clang diagnostic pop") \
})

#define _os_trace_with_payload_6(_l, _m, _t, _1, _2, _3, _4, _5, _payload) __extension__({ \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wpacked\"") \
	const __typeof__(_1) _c1 = _1; \
	const __typeof__(_2) _c2 = _2; \
	const __typeof__(_3) _c3 = _3; \
	const __typeof__(_4) _c4 = _4; \
	const __typeof__(_4) _c5 = _5; \
	_os_trace_verify_printf(_l, _c1, _c2, _c3, _c4, _c5); \
	const struct __attribute__((packed)) { \
	        __typeof__(_c1) _f1; \
	        __typeof__(_c2) _f2; \
	        __typeof__(_c3) _f3; \
	        __typeof__(_c4) _f4; \
	        __typeof__(_c5) _f5; \
	        uint8_t _s[5]; \
	        uint8_t _cnt; \
	} _buf = { \
	        ._f1 = _c1, ._s[0] = sizeof(_c1), \
	        ._f2 = _c2, ._s[1] = sizeof(_c2), \
	        ._f3 = _c3, ._s[2] = sizeof(_c3), \
	        ._f4 = _c4, ._s[3] = sizeof(_c4), \
	        ._f5 = _c5, ._s[4] = sizeof(_c5), \
	        ._cnt = 5, \
	}; \
	_os_trace_with_buffer(&__dso_handle, _m, _t, &_buf, sizeof(_buf), _payload); \
	__asm__("");                      \
	_Pragma("clang diagnostic pop") \
})

#define _os_trace_with_payload_7(_l, _m, _t, _1, _2, _3, _4, _5, _6, _payload) __extension__({ \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wpacked\"") \
	const __typeof__(_1) _c1 = _1; \
	const __typeof__(_2) _c2 = _2; \
	const __typeof__(_3) _c3 = _3; \
	const __typeof__(_4) _c4 = _4; \
	const __typeof__(_5) _c5 = _5; \
	const __typeof__(_6) _c6 = _6; \
	_os_trace_verify_printf(_l, _c1, _c2, _c3, _c4, _c5, _c6); \
	const struct __attribute__((packed)) { \
	        __typeof__(_c1) _f1; \
	        __typeof__(_c2) _f2; \
	        __typeof__(_c3) _f3; \
	        __typeof__(_c4) _f4; \
	        __typeof__(_c5) _f5; \
	        __typeof__(_c6) _f6; \
	        uint8_t _s[6]; \
	        uint8_t _cnt; \
	} _buf = { \
	        ._f1 = _c1, ._s[0] = sizeof(_c1), \
	        ._f2 = _c2, ._s[1] = sizeof(_c2), \
	        ._f3 = _c3, ._s[2] = sizeof(_c3), \
	        ._f4 = _c4, ._s[3] = sizeof(_c4), \
	        ._f5 = _c5, ._s[4] = sizeof(_c5), \
	        ._f6 = _c6, ._s[5] = sizeof(_c6), \
	        ._cnt = 6, \
	}; \
	_os_trace_with_buffer(&__dso_handle, _m, _t, &_buf, sizeof(_buf), _payload); \
	__asm__("");                      \
	_Pragma("clang diagnostic pop") \
})

#define _os_trace_with_payload_8(_l, _m, _t, _1, _2, _3, _4, _5, _6, _7, _payload) __extension__({ \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wpacked\"") \
	const __typeof__(_1) _c1 = _1; \
	const __typeof__(_2) _c2 = _2; \
	const __typeof__(_3) _c3 = _3; \
	const __typeof__(_4) _c4 = _4; \
	const __typeof__(_5) _c5 = _5; \
	const __typeof__(_6) _c6 = _6; \
	const __typeof__(_7) _c7 = _7; \
	_os_trace_verify_printf(_l, _c1, _c2, _c3, _c4, _c5, _c6, _c7); \
	const struct __attribute__((packed)) { \
	        __typeof__(_c1) _f1; \
	        __typeof__(_c2) _f2; \
	        __typeof__(_c3) _f3; \
	        __typeof__(_c4) _f4; \
	        __typeof__(_c5) _f5; \
	        __typeof__(_c6) _f6; \
	        __typeof__(_c7) _f7; \
	        uint8_t _s[7]; \
	        uint8_t _cnt; \
	} _buf = { \
	        ._f1 = _c1, ._s[0] = sizeof(_c1), \
	        ._f2 = _c2, ._s[1] = sizeof(_c2), \
	        ._f3 = _c3, ._s[2] = sizeof(_c3), \
	        ._f4 = _c4, ._s[3] = sizeof(_c4), \
	        ._f5 = _c5, ._s[4] = sizeof(_c5), \
	        ._f6 = _c6, ._s[5] = sizeof(_c6), \
	        ._f7 = _c7, ._s[6] = sizeof(_c7), \
	        ._cnt = 7, \
	}; \
	_os_trace_with_buffer(&__dso_handle, _m, _t, &_buf, sizeof(_buf), _payload); \
	__asm__("");                      \
	_Pragma("clang diagnostic pop") \
})

#define OS_TRACE_CALL(format, _m, _t, ...) __extension__({                              \
    _Pragma("clang diagnostic push")                                                    \
    _Pragma("clang diagnostic ignored \"-Wc++98-compat-pedantic\"")                     \
    OS_CONCAT(_os_trace, OS_COUNT_ARGS(__VA_ARGS__))(format, _m, _t, ##__VA_ARGS__);    \
    _Pragma("clang diagnostic pop")                                                     \
})

#else

                                                  
#define OS_TRACE_CALL(_l, _m, _t, ...) __extension__({                          \
    uint8_t buf[1024];                                                          \
    _os_trace_verify_printf(_l, ##__VA_ARGS__);                                 \
    size_t buf_size = _os_trace_encode(buf, sizeof(buf), _m, ##__VA_ARGS__);    \
    _os_trace_internal(&__dso_handle, _t, _m, buf, buf_size, NULL);             \
    __asm__("");                                                                \
})

#define _os_trace_with_payload_1(_l, _m, _t, _payload) __extension__({  \
    _os_trace_verify_printf(_l);                                        \
    _os_trace_internal(&__dso_handle, _t, _m, NULL, 0, _payload);       \
    __asm__("");                                                        \
})

#define _os_trace_with_payload_2(_l, _m, _t, _1, _payload) __extension__({  \
    _os_trace_verify_printf(_l, _1);                                        \
    uint8_t buf[1024];                                                      \
    size_t buf_size = _os_trace_encode(buf, sizeof(buf), _m, _1);           \
    _os_trace_internal(&__dso_handle, _t, _m, buf, buf_size, _payload);     \
    __asm__("");                                                            \
})

#define _os_trace_with_payload_3(_l, _m, _t, _1, _2, _payload) __extension__({  \
    _os_trace_verify_printf(_l, _1, _2);                                        \
    uint8_t buf[1024];                                                          \
    size_t buf_size = _os_trace_encode(buf, sizeof(buf), _m, _1, _2);           \
    _os_trace_internal(&__dso_handle, _t, _m, buf, buf_size, _payload);         \
    __asm__("");                                                                \
})

#define _os_trace_with_payload_4(_l, _m, _t, _1, _2, _3, _payload) __extension__({  \
    _os_trace_verify_printf(_l, _1, _2, _3);                                        \
    uint8_t buf[1024];                                                              \
    size_t buf_size = _os_trace_encode(buf, sizeof(buf), _m, _1, _2, _3);           \
    _os_trace_internal(&__dso_handle, _t, _m, buf, buf_size, _payload);             \
    __asm__("");                                                                    \
})

#define _os_trace_with_payload_5(_l, _m, _t, _1, _2, _3, _4, _payload) __extension__({  \
    _os_trace_verify_printf(_l, _1, _2, _3, _4);                                        \
    uint8_t buf[1024];                                                                  \
    size_t buf_size = _os_trace_encode(buf, sizeof(buf), _m, _1, _2, _3, _4);           \
    _os_trace_internal(&__dso_handle, _t, _m, buf, buf_size, _payload);                 \
    __asm__("");                                                                        \
})

#define _os_trace_with_payload_6(_l, _m, _t, _1, _2, _3, _4, _5, _payload) __extension__({  \
    _os_trace_verify_printf(_l, _1, _2, _3, _4, _5);                                        \
    uint8_t buf[1024];                                                                      \
    size_t buf_size = _os_trace_encode(buf, sizeof(buf), _m, _1, _2, _3, _4, _5);           \
    _os_trace_internal(&__dso_handle, _t, _m, buf, buf_size, _payload);                     \
    __asm__("");                                                                            \
})

#define _os_trace_with_payload_7(_l, _m, _t, _1, _2, _3, _4, _5, _6, _payload) __extension__({  \
    _os_trace_verify_printf(_l, _1, _2, _3, _4, _5, _6);                                        \
    uint8_t buf[1024];                                                                          \
    size_t buf_size = _os_trace_encode(buf, sizeof(buf), _m, _1, _2, _3, _4, _5, _6);           \
    _os_trace_internal(&__dso_handle, _t, _m, buf, buf_size, _payload);                         \
    __asm__("");                                                                                \
})

#define _os_trace_with_payload_8(_l, _m, _t, _1, _2, _3, _4, _5, _6, _7, _payload) __extension__({  \
    _os_trace_verify_printf(_l, _1, _2, _3, _4, _5, _6, _7);                                        \
    uint8_t buf[1024];                                                                              \
    size_t buf_size = _os_trace_encode(buf, sizeof(buf), _m, _1, _2, _3, _4, _5, _6, _7);           \
    _os_trace_internal(&__dso_handle, _t, _m, buf, buf_size, _payload);                             \
    __asm__("");                                                                                    \
})

#endif                                              

   
  
            
                             
  
              
                                                                                                 
                                                                                                    
                                                                                            
  
                                        
                                                              
                                                            
                                                                                                                       
                                                                                                                             
                                                                                                             
                                                                                  
                                                       
                                             
                                                                                                                             
                                                                                                                        
                                                        
                                                                  
                                                              
                                                               
                                                                                         
                                                                                         
                                                                
                                                   
  
   

#pragma mark - Other defines

   
                                
                                                                            
                                                                           
                                                        
   
#define OS_TRACE_TYPE_RELEASE (1u << 0)

   
                              
                                                                            
                              
   
#define OS_TRACE_TYPE_DEBUG (1u << 1)

   
                             
                                                                          
                                                                         
   
#define OS_TRACE_TYPE_INFO (1u << 2)

   
                              
                                                                           
            
   
#define OS_TRACE_TYPE_ERROR ((1u << 6) | (1u << 0))

   
                              
                                                                                
                   
   
#define OS_TRACE_TYPE_FAULT ((1u << 7) | (1u << 6) | (1u << 0))

   
                              
                                                                                  
                                                                             
                                                                               
                                                                               
                                                                               
                                       
   
typedef void (^os_trace_payload_t)(xpc_object_t xdict);

#pragma mark - function declarations

   
                     
  
            
                                                                        
  
              
                                                                              
                                                                       
                                                                        
  
                
                                                                             
                                                                        
                                                                          
                                        
  
                                                                                      
   
#define os_trace(format, ...) __extension__({                                           \
    _Static_assert(__builtin_constant_p(format), "format must be a constant string");   \
    __attribute__((section("__TEXT,__os_trace"))) static const char _m[] = format;      \
   OS_TRACE_CALL(format, _m, OS_TRACE_TYPE_RELEASE, ##__VA_ARGS__);                     \
})


#if (defined(__IPHONE_OS_VERSION_MIN_REQUIRED) && __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_10_0) \
        || (defined(__WATCH_OS_VERSION_MIN_REQUIRED) && __WATCH_OS_VERSION_MIN_REQUIRED >= __WATCHOS_3_0) \
        || (defined(__TV_OS_VERSION_MIN_REQUIRED) && __TV_OS_VERSION_MIN_REQUIRED >= __TVOS_10_0) \
        || (defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_12)

   
                          
  
            
                                                                              
                                  
  
              
                                                                             
                                                                      
                                                                               
                                
  
                
                                                                             
                                                                        
                                                                          
                                        
  
                                                          
   
#define os_trace_info(format, ...) __extension__({                                      \
    _Static_assert(__builtin_constant_p(format), "format must be a constant string");   \
    __attribute__((section("__TEXT,__os_trace"))) static const char _m[] = format;      \
   OS_TRACE_CALL(format, _m, OS_TRACE_TYPE_INFO, ##__VA_ARGS__);                        \
})

#endif

   
                           
  
            
                                                                    
  
              
                                                                                 
                                                                        
                                                             
  
                
                                                                             
                                                                        
                                                                          
                                        
  
                                                           
   
#define os_trace_debug(format, ...) __extension__({                                     \
    _Static_assert(__builtin_constant_p(format), "format must be a constant string");   \
    __attribute__((section("__TEXT,__os_trace"))) static const char _m[] = format;      \
   OS_TRACE_CALL(format, _m, OS_TRACE_TYPE_DEBUG, ##__VA_ARGS__);                       \
})

   
                                  
  
            
                                                                                 
              
  
              
                                                                                 
                                                                                       
                                                                              
                                            
  
                                 
                                             
                                                                       
                                          
    
  
          
                                               
   
__OSX_AVAILABLE_STARTING(__MAC_10_12, __IPHONE_10_0)
OS_EXPORT OS_NOTHROW OS_WARN_RESULT
bool
os_trace_info_enabled(void);

   
                                   
  
            
                                                                                  
  
              
                                                                                  
                                                                           
                                                                              
                                            
  
                                  
                                              
                                               
                                     
    
  
          
                                         
   
__OSX_AVAILABLE(10.10) __IOS_AVAILABLE(8.0) __WATCHOS_AVAILABLE(1.0) __TVOS_AVAILABLE(9.0)
OS_EXPORT OS_NOTHROW OS_WARN_RESULT
bool
os_trace_debug_enabled(void);

   
                           
  
            
                                                                                
                           
  
              
                                                                                
                           
  
                
                                                                             
                                                                        
                                                                          
                                        
  
                                                                 
   
#define os_trace_error(format, ...) __extension__({                                     \
    _Static_assert(__builtin_constant_p(format), "format must be a constant string");   \
    __attribute__((section("__TEXT,__os_trace"))) static const char _m[] = format;      \
   OS_TRACE_CALL(format, _m, OS_TRACE_TYPE_ERROR, ##__VA_ARGS__);                       \
})

   
                           
  
            
                                                                             
                                  
  
              
                                                                             
                                  
  
                
                                                                             
                                                                        
                                                                          
                                        
  
                                                              
   
#define os_trace_fault(format, ...) __extension__({                                     \
    _Static_assert(__builtin_constant_p(format), "format must be a constant string");   \
    __attribute__((section("__TEXT,__os_trace"))) static const char _m[] = format;      \
    OS_TRACE_CALL(format, _m, OS_TRACE_TYPE_FAULT, ##__VA_ARGS__);                      \
})

#if __has_include(<xpc/xpc.h>)
   
                                  
  
            
                                                                         
               
  
              
                                                                          
                                                                               
                                                                               
                                        
  
                   
                                                                             
                                                                        
                                                                          
                                        
  
                                                                  
  
                                                                              
  
                                                                         
                                                               
                                                                      
        
   
#define os_trace_with_payload(format, ...) __extension__({                                                              \
    _Static_assert(__builtin_constant_p(format), "format must be a constant string");                                   \
    __attribute__((section("__TEXT,__os_trace"))) static const char _m[] = format;                                      \
    OS_CONCAT(_os_trace_with_payload, OS_COUNT_ARGS(__VA_ARGS__))(format, _m, OS_TRACE_TYPE_RELEASE, ##__VA_ARGS__);    \
})

#define os_trace_info_with_payload(format, ...) __extension__({                                                     \
    _Static_assert(__builtin_constant_p(format), "format must be a constant string");                               \
    __attribute__((section("__TEXT,__os_trace"))) static const char _m[] = format;                                  \
    OS_CONCAT(_os_trace_with_payload, OS_COUNT_ARGS(__VA_ARGS__))(format, _m, OS_TRACE_TYPE_INFO, ##__VA_ARGS__);   \
})

#define os_trace_debug_with_payload(format, ...) __extension__({                                                    \
    _Static_assert(__builtin_constant_p(format), "format must be a constant string");                               \
    __attribute__((section("__TEXT,__os_trace"))) static const char _m[] = format;                                  \
    OS_CONCAT(_os_trace_with_payload, OS_COUNT_ARGS(__VA_ARGS__))(format, _m, OS_TRACE_TYPE_DEBUG, ##__VA_ARGS__);  \
})

#define os_trace_error_with_payload(format, ...) __extension__({                                                    \
    _Static_assert(__builtin_constant_p(format), "format must be a constant string");                               \
    __attribute__((section("__TEXT,__os_trace"))) static const char _m[] = format;                                  \
    OS_CONCAT(_os_trace_with_payload, OS_COUNT_ARGS(__VA_ARGS__))(format, _m, OS_TRACE_TYPE_ERROR, ##__VA_ARGS__);  \
})

#define os_trace_fault_with_payload(format, ...) __extension__({                                                    \
    _Static_assert(__builtin_constant_p(format), "format must be a constant string");                               \
    __attribute__((section("__TEXT,__os_trace"))) static const char _m[] = format;                                  \
    OS_CONCAT(_os_trace_with_payload, OS_COUNT_ARGS(__VA_ARGS__))(format, _m, OS_TRACE_TYPE_FAULT, ##__VA_ARGS__);  \
})

#endif                              

                                                  
__OSX_AVAILABLE_STARTING(__MAC_10_12, __IPHONE_10_0)
OS_EXPORT OS_NOTHROW
size_t
_os_trace_encode(uint8_t *buf, size_t buf_size, const char *format, ...);

__OSX_AVAILABLE_STARTING(__MAC_10_12, __IPHONE_10_0)
OS_EXPORT OS_NOTHROW
void
_os_trace_internal(void *dso, uint8_t type, const char *format, const uint8_t *buf, size_t buf_size, os_trace_payload_t payload);

   
                                  
  
            
                                                   
   
__OSX_AVAILABLE(10.10) __IOS_AVAILABLE(8.0) __WATCHOS_AVAILABLE(1.0) __TVOS_AVAILABLE(9.0)
OS_EXPORT OS_NOTHROW
void
_os_trace_with_buffer(void *dso, const char *message, uint8_t type, const void *buffer, size_t buffer_size, os_trace_payload_t payload);

__END_DECLS

#endif                  
