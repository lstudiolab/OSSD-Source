/*
 * Copyright 2026 OSSD contributors.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Small, standalone SHA-256 implementation for firmware integrity checks.
 * This implementation is maintained by OSSD and has no crypto-library
 * dependency. It is not a signature/authenticity mechanism; the manifest
 * provenance and digest must come from a trusted release process.
 */
#include <stdint.h>
#include <stddef.h>
#include <device/ossd_realtek_wifi_sha256.h>

#define SHA256_BLOCK_SIZE 64U
#define SHA256_DIGEST_SIZE 32U

struct sha256_context {
	uint32_t state[8];
	uint64_t bit_count;
	uint8_t block[SHA256_BLOCK_SIZE];
	size_t used;
};

static const uint32_t sha256_k[64] = {
	0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
	0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
	0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
	0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
	0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
	0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
	0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
	0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U
};

static uint32_t
ror32(uint32_t value, unsigned int shift)
{
	return (value >> shift) | (value << (32U - shift));
}

static uint32_t
load_be32(const uint8_t *p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
	    ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void
store_be32(uint8_t *p, uint32_t value)
{
	p[0] = (uint8_t)(value >> 24);
	p[1] = (uint8_t)(value >> 16);
	p[2] = (uint8_t)(value >> 8);
	p[3] = (uint8_t)value;
}

static void
sha256_transform(struct sha256_context *ctx, const uint8_t block[64])
{
	uint32_t w[64], a,b,c,d,e,f,g,h;
	unsigned int i;

	for (i = 0; i < 16; i++) w[i] = load_be32(block + i * 4U);
	for (; i < 64; i++) {
		uint32_t s0 = ror32(w[i-15],7) ^ ror32(w[i-15],18) ^ (w[i-15] >> 3);
		uint32_t s1 = ror32(w[i-2],17) ^ ror32(w[i-2],19) ^ (w[i-2] >> 10);
		w[i] = w[i-16] + s0 + w[i-7] + s1;
	}
	a=ctx->state[0]; b=ctx->state[1]; c=ctx->state[2]; d=ctx->state[3];
	e=ctx->state[4]; f=ctx->state[5]; g=ctx->state[6]; h=ctx->state[7];
	for (i = 0; i < 64; i++) {
		uint32_t s1 = ror32(e,6) ^ ror32(e,11) ^ ror32(e,25);
		uint32_t ch = (e & f) ^ (~e & g);
		uint32_t t1 = h + s1 + ch + sha256_k[i] + w[i];
		uint32_t s0 = ror32(a,2) ^ ror32(a,13) ^ ror32(a,22);
		uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
		uint32_t t2 = s0 + maj;
		h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
	}
	ctx->state[0]+=a; ctx->state[1]+=b; ctx->state[2]+=c; ctx->state[3]+=d;
	ctx->state[4]+=e; ctx->state[5]+=f; ctx->state[6]+=g; ctx->state[7]+=h;
}

static void
sha256_init(struct sha256_context *ctx)
{
	static const uint32_t initial[8] = {
		0x6a09e667U,0xbb67ae85U,0x3c6ef372U,0xa54ff53aU,
		0x510e527fU,0x9b05688cU,0x1f83d9abU,0x5be0cd19U
	};
	unsigned int i;
	for (i = 0; i < 8; i++) ctx->state[i] = initial[i];
	ctx->bit_count = 0;
	ctx->used = 0;
}

static void
sha256_update(struct sha256_context *ctx, const uint8_t *data, size_t length)
{
	while (length != 0) {
		size_t take = SHA256_BLOCK_SIZE - ctx->used;
		if (take > length) take = length;
		for (size_t i = 0; i < take; i++) ctx->block[ctx->used + i] = data[i];
		ctx->used += take;
		data += take;
		length -= take;
		ctx->bit_count += (uint64_t)take * 8U;
		if (ctx->used == SHA256_BLOCK_SIZE) {
			sha256_transform(ctx, ctx->block);
			ctx->used = 0;
		}
	}
}

static void
sha256_final(struct sha256_context *ctx, uint8_t digest[32])
{
	uint64_t bits = ctx->bit_count;
	unsigned int i;

	ctx->block[ctx->used++] = 0x80;
	if (ctx->used > 56) {
		while (ctx->used < 64) ctx->block[ctx->used++] = 0;
		sha256_transform(ctx, ctx->block);
		ctx->used = 0;
	}
	while (ctx->used < 56) ctx->block[ctx->used++] = 0;
	for (i = 0; i < 8; i++)
		ctx->block[56 + i] = (uint8_t)(bits >> (56U - i * 8U));
	sha256_transform(ctx, ctx->block);
	for (i = 0; i < 8; i++) store_be32(digest + i * 4U, ctx->state[i]);
}

static int
hex_nibble(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

int
ossd_rtw_sha256_verify(void *context, const void *bytes, size_t length,
    const char *expected_sha256_hex)
{
	struct sha256_context ctx;
	uint8_t digest[SHA256_DIGEST_SIZE];
	uint8_t difference = 0;
	unsigned int i;

	(void)context;
	if ((bytes == 0 && length != 0) || expected_sha256_hex == 0) return -1;
	for (i = 0; i < 64; i++) {
		if (expected_sha256_hex[i] == '\0' ||
		    hex_nibble(expected_sha256_hex[i]) < 0) return -1;
	}
	if (expected_sha256_hex[64] != '\0') return -1;

	sha256_init(&ctx);
	if (length != 0) sha256_update(&ctx, (const uint8_t *)bytes, length);
	sha256_final(&ctx, digest);
	for (i = 0; i < SHA256_DIGEST_SIZE; i++) {
		uint8_t expected = (uint8_t)((hex_nibble(expected_sha256_hex[i*2]) << 4) |
		    hex_nibble(expected_sha256_hex[i*2+1]));
		difference |= (uint8_t)(digest[i] ^ expected);
	}
	/* Clear intermediate state before returning. */
	{
		volatile uint8_t *p = (volatile uint8_t *)&ctx;
		for (i = 0; i < sizeof(ctx); i++) p[i] = 0;
		p = digest;
		for (i = 0; i < sizeof(digest); i++) p[i] = 0;
	}
	return difference == 0 ? 0 : -2;
}
