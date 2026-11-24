/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef OSSD_REALTEK_WIFI_SHA256_H
#define OSSD_REALTEK_WIFI_SHA256_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Verify a byte buffer against a 64-character hexadecimal SHA-256 digest.
 * Returns 0 on a constant-time digest match, -1 for invalid input, and -2
 * when the digest does not match. No external crypto provider is required.
 */
int ossd_rtw_sha256_verify(void *context, const void *bytes, size_t length,
    const char *expected_sha256_hex);

#ifdef __cplusplus
}
#endif
#endif /* OSSD_REALTEK_WIFI_SHA256_H */
