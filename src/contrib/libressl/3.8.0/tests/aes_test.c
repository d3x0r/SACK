/*	$OpenBSD: aes_test.c,v 1.2 2022/11/07 23:09:25 joshua Exp $ */
/*
 * Copyright (c) 2022 Joshua Sing <joshua@hypera.dev>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <openssl/evp.h>
#include <openssl/aes.h>

#include <stdint.h>
#include <string.h>

struct aes_test {
	const int mode;
	const uint8_t key[64];
	const uint8_t iv[64];
	const int iv_len;
	const uint8_t in[64];
	const int in_len;
	const uint8_t out[64];
	const int out_len;
	const int padding;
};

static const struct aes_test aes_tests[] = {
	/* ECB - Test vectors from FIPS-197, Appendix C. */
	{
		.mode = NID_aes_128_ecb,
		.key = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.in = {
			0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
			0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
		},
		.in_len = 16,
		.out = {
			0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
			0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a,
		},
		.out_len = 16,
	},
	{
		.mode = NID_aes_192_ecb,
		.key = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		},
		.in = {
			0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
			0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
		},
		.in_len = 16,
		.out = {
			0xdd, 0xa9, 0x7c, 0xa4, 0x86, 0x4c, 0xdf, 0xe0,
			0x6e, 0xaf, 0x70, 0xa0, 0xec, 0x0d, 0x71, 0x91,
		},
		.out_len = 16,
	},
	{
		.mode = NID_aes_256_ecb,
		.key = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
			0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		},
		.in = {
			0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
			0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
		},
		.in_len = 16,
		.out = {
			0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf,
			0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89,
		},
		.out_len = 16,
	},
	
	/* CBC - Test vectors from RFC 3602 */
	{
		.mode = NID_aes_128_cbc,
		.key = {
			0x06, 0xa9, 0x21, 0x40, 0x36, 0xb8, 0xa1, 0x5b,
			0x51, 0x2e, 0x03, 0xd5, 0x34, 0x12, 0x00, 0x06,
		},
		.iv = {
			0x3d, 0xaf, 0xba, 0x42, 0x9d, 0x9e, 0xb4, 0x30,
			0xb4, 0x22, 0xda, 0x80, 0x2c, 0x9f, 0xac, 0x41,
		},
		.iv_len = 16,
		.in = {
			0x53, 0x69, 0x6e, 0x67, 0x6c, 0x65, 0x20, 0x62,
			0x6c, 0x6f, 0x63, 0x6b, 0x20, 0x6d, 0x73, 0x67,
		},
		.in_len = 16,
		.out = {
			0xe3, 0x53, 0x77, 0x9c, 0x10, 0x79, 0xae, 0xb8,
			0x27, 0x08, 0x94, 0x2d, 0xbe, 0x77, 0x18, 0x1a,
		},
		.out_len = 16,
	},
	{
		.mode = NID_aes_128_cbc,
		.key = {
			0xc2, 0x86, 0x69, 0x6d, 0x88, 0x7c, 0x9a, 0xa0,
			0x61, 0x1b, 0xbb, 0x3e, 0x20, 0x25, 0xa4, 0x5a,
		},
		.iv = {
			0x56, 0x2e, 0x17, 0x99, 0x6d, 0x09, 0x3d, 0x28,
			0xdd, 0xb3, 0xba, 0x69, 0x5a, 0x2e, 0x6f, 0x58,
		},
		.iv_len = 16,
		.in = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
			0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		},
		.in_len = 32,
		.out = {
			0xd2, 0x96, 0xcd, 0x94, 0xc2, 0xcc, 0xcf, 0x8a,
			0x3a, 0x86, 0x30, 0x28, 0xb5, 0xe1, 0xdc, 0x0a,
			0x75, 0x86, 0x60, 0x2d, 0x25, 0x3c, 0xff, 0xf9,
			0x1b, 0x82, 0x66, 0xbe, 0xa6, 0xd6, 0x1a, 0xb1,
		},
		.out_len = 32,
	},
	{
		.mode = NID_aes_128_cbc,
		.key = {
			0x6c, 0x3e, 0xa0, 0x47, 0x76, 0x30, 0xce, 0x21,
			0xa2, 0xce, 0x33, 0x4a, 0xa7, 0x46, 0xc2, 0xcd,
		},
		.iv = {
			0xc7, 0x82, 0xdc, 0x4c, 0x09, 0x8c, 0x66, 0xcb,
			0xd9, 0xcd, 0x27, 0xd8, 0x25, 0x68, 0x2c, 0x81,
		},
		.iv_len = 16,
		.in = {
			0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20,
			0x61, 0x20, 0x34, 0x38, 0x2d, 0x62, 0x79, 0x74,
			0x65, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67,
			0x65, 0x20, 0x28, 0x65, 0x78, 0x61, 0x63, 0x74,
			0x6c, 0x79, 0x20, 0x33, 0x20, 0x41, 0x45, 0x53,
			0x20, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x73, 0x29,
		},
		.in_len = 48,
		.out = {
			0xd0, 0xa0, 0x2b, 0x38, 0x36, 0x45, 0x17, 0x53,
			0xd4, 0x93, 0x66, 0x5d, 0x33, 0xf0, 0xe8, 0x86,
			0x2d, 0xea, 0x54, 0xcd, 0xb2, 0x93, 0xab, 0xc7,
			0x50, 0x69, 0x39, 0x27, 0x67, 0x72, 0xf8, 0xd5,
			0x02, 0x1c, 0x19, 0x21, 0x6b, 0xad, 0x52, 0x5c,
			0x85, 0x79, 0x69, 0x5d, 0x83, 0xba, 0x26, 0x84,
		},
		.out_len = 48,
	},
	{
		.mode = NID_aes_128_cbc,
		.key = {
			0x56, 0xe4, 0x7a, 0x38, 0xc5, 0x59, 0x89, 0x74,
			0xbc, 0x46, 0x90, 0x3d, 0xba, 0x29, 0x03, 0x49,
		},
		.iv = {
			0x8c, 0xe8, 0x2e, 0xef, 0xbe, 0xa0, 0xda, 0x3c,
			0x44, 0x69, 0x9e, 0xd7, 0xdb, 0x51, 0xb7, 0xd9,
		},
		.iv_len = 16,
		.in = {
			0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
			0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
			0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
			0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
			0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
			0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
			0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
			0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
		},
		.in_len = 64,
		.out = {
			0xc3, 0x0e, 0x32, 0xff, 0xed, 0xc0, 0x77, 0x4e,
			0x6a, 0xff, 0x6a, 0xf0, 0x86, 0x9f, 0x71, 0xaa,
			0x0f, 0x3a, 0xf0, 0x7a, 0x9a, 0x31, 0xa9, 0xc6,
			0x84, 0xdb, 0x20, 0x7e, 0xb0, 0xef, 0x8e, 0x4e,
			0x35, 0x90, 0x7a, 0xa6, 0x32, 0xc3, 0xff, 0xdf,
			0x86, 0x8b, 0xb7, 0xb2, 0x9d, 0x3d, 0x46, 0xad,
			0x83, 0xce, 0x9f, 0x9a, 0x10, 0x2e, 0xe9, 0x9d,
			0x49, 0xa5, 0x3e, 0x87, 0xf4, 0xc3, 0xda, 0x55,
		},
		.out_len = 64,
	},

	/* CBC - Test vectors from NIST SP 800-38A */
	{
		.mode = NID_aes_128_cbc,
		.key = {
			0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
			0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
		},
		.iv = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.iv_len = 16,
		.in = {
			0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
			0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
			0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
			0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
			0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
			0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
			0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
			0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10,
		},
		.in_len = 64,
		.out = {
			0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46,
			0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d,
			0x50, 0x86, 0xcb, 0x9b, 0x50, 0x72, 0x19, 0xee,
			0x95, 0xdb, 0x11, 0x3a, 0x91, 0x76, 0x78, 0xb2,
			0x73, 0xbe, 0xd6, 0xb8, 0xe3, 0xc1, 0x74, 0x3b,
			0x71, 0x16, 0xe6, 0x9e, 0x22, 0x22, 0x95, 0x16,
			0x3f, 0xf1, 0xca, 0xa1, 0x68, 0x1f, 0xac, 0x09,
			0x12, 0x0e, 0xca, 0x30, 0x75, 0x86, 0xe1, 0xa7,
		},
		.out_len = 64,
	},
	{
		.mode = NID_aes_192_cbc,
		.key = {
			0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52,
			0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5,
			0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b,
		},
		.iv = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.iv_len = 16,
		.in = {
			0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
			0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
			0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
			0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
			0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
			0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
			0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
			0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10,
		},
		.in_len = 64,
		.out = {
			0x4f, 0x02, 0x1d, 0xb2, 0x43, 0xbc, 0x63, 0x3d,
			0x71, 0x78, 0x18, 0x3a, 0x9f, 0xa0, 0x71, 0xe8,
			0xb4, 0xd9, 0xad, 0xa9, 0xad, 0x7d, 0xed, 0xf4,
			0xe5, 0xe7, 0x38, 0x76, 0x3f, 0x69, 0x14, 0x5a,
			0x57, 0x1b, 0x24, 0x20, 0x12, 0xfb, 0x7a, 0xe0,
			0x7f, 0xa9, 0xba, 0xac, 0x3d, 0xf1, 0x02, 0xe0,
			0x08, 0xb0, 0xe2, 0x79, 0x88, 0x59, 0x88, 0x81,
			0xd9, 0x20, 0xa9, 0xe6, 0x4f, 0x56, 0x15, 0xcd,
		},
		.out_len = 64,
	},
	{
		.mode = NID_aes_256_cbc,
		.key = {
			0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
			0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
			0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
			0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4,
		},
		.iv = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.iv_len = 16,
		.in = {
			0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
			0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
			0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
			0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
			0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
			0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
			0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
			0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10,
		},
		.in_len = 64,
		.out = {
			0xf5, 0x8c, 0x4c, 0x04, 0xd6, 0xe5, 0xf1, 0xba,
			0x77, 0x9e, 0xab, 0xfb, 0x5f, 0x7b, 0xfb, 0xd6,
			0x9c, 0xfc, 0x4e, 0x96, 0x7e, 0xdb, 0x80, 0x8d,
			0x67, 0x9f, 0x77, 0x7b, 0xc6, 0x70, 0x2c, 0x7d,
			0x39, 0xf2, 0x33, 0x69, 0xa9, 0xd9, 0xba, 0xcf,
			0xa5, 0x30, 0xe2, 0x63, 0x04, 0x23, 0x14, 0x61,
			0xb2, 0xeb, 0x05, 0xe2, 0xc3, 0x9b, 0xe9, 0xfc,
			0xda, 0x6c, 0x19, 0x07, 0x8c, 0x6a, 0x9d, 0x1b,
		},
		.out_len = 64,
	},

	/* CFB128 - Test vectors from NIST SP 800-38A */
	{
		.mode = NID_aes_128_cfb128,
		.key = {
			0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
			0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
		},
		.iv = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.iv_len = 16,
		.in = {
			0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
			0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
			0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
			0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
			0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
			0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
			0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
			0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10,
		},
		.in_len = 64,
		.out = {
			0x3b, 0x3f, 0xd9, 0x2e, 0xb7, 0x2d, 0xad, 0x20,
			0x33, 0x34, 0x49, 0xf8, 0xe8, 0x3c, 0xfb, 0x4a,
			0xc8, 0xa6, 0x45, 0x37, 0xa0, 0xb3, 0xa9, 0x3f,
			0xcd, 0xe3, 0xcd, 0xad, 0x9f, 0x1c, 0xe5, 0x8b,
			0x26, 0x75, 0x1f, 0x67, 0xa3, 0xcb, 0xb1, 0x40,
			0xb1, 0x80, 0x8c, 0xf1, 0x87, 0xa4, 0xf4, 0xdf,
			0xc0, 0x4b, 0x05, 0x35, 0x7c, 0x5d, 0x1c, 0x0e,
			0xea, 0xc4, 0xc6, 0x6f, 0x9f, 0xf7, 0xf2, 0xe6,
		},
		.out_len = 64,
	},
	{
		.mode = NID_aes_192_cfb128,
		.key = {
			0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52,
			0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5,
			0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b,
		},
		.iv = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.iv_len = 16,
		.in = {
			0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
			0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
			0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
			0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
			0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
			0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
			0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
			0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10,
		},
		.in_len = 64,
		.out = {
			0xcd, 0xc8, 0x0d, 0x6f, 0xdd, 0xf1, 0x8c, 0xab,
			0x34, 0xc2, 0x59, 0x09, 0xc9, 0x9a, 0x41, 0x74,
			0x67, 0xce, 0x7f, 0x7f, 0x81, 0x17, 0x36, 0x21,
			0x96, 0x1a, 0x2b, 0x70, 0x17, 0x1d, 0x3d, 0x7a,
			0x2e, 0x1e, 0x8a, 0x1d, 0xd5, 0x9b, 0x88, 0xb1,
			0xc8, 0xe6, 0x0f, 0xed, 0x1e, 0xfa, 0xc4, 0xc9,
			0xc0, 0x5f, 0x9f, 0x9c, 0xa9, 0x83, 0x4f, 0xa0,
			0x42, 0xae, 0x8f, 0xba, 0x58, 0x4b, 0x09, 0xff,
		},
		.out_len = 64,
	},
	{
		.mode = NID_aes_256_cfb128,
		.key = {
			0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
			0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
			0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
			0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4,
		},
		.iv = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.iv_len = 16,
		.in = {
			0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
			0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
			0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
			0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
			0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
			0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
			0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
			0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10,
		},
		.in_len = 64,
		.out = {
			0xdc, 0x7e, 0x84, 0xbf, 0xda, 0x79, 0x16, 0x4b,
			0x7e, 0xcd, 0x84, 0x86, 0x98, 0x5d, 0x38, 0x60,
			0x39, 0xff, 0xed, 0x14, 0x3b, 0x28, 0xb1, 0xc8,
			0x32, 0x11, 0x3c, 0x63, 0x31, 0xe5, 0x40, 0x7b,
			0xdf, 0x10, 0x13, 0x24, 0x15, 0xe5, 0x4b, 0x92,
			0xa1, 0x3e, 0xd0, 0xa8, 0x26, 0x7a, 0xe2, 0xf9,
			0x75, 0xa3, 0x85, 0x74, 0x1a, 0xb9, 0xce, 0xf8,
			0x20, 0x31, 0x62, 0x3d, 0x55, 0xb1, 0xe4, 0x71,
		},
		.out_len = 64,
	},

	/* OFB128 - Test vectors from NIST SP 800-38A */
	{
		.mode = NID_aes_128_ofb128,
		.key = {
			0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
			0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
		},
		.iv = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.iv_len = 16,
		.in = {
			0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
			0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
			0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
			0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
			0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
			0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
			0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
			0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10,
		},
		.in_len = 64,
		.out = {
			0x3b, 0x3f, 0xd9, 0x2e, 0xb7, 0x2d, 0xad, 0x20,
			0x33, 0x34, 0x49, 0xf8, 0xe8, 0x3c, 0xfb, 0x4a,
			0x77, 0x89, 0x50, 0x8d, 0x16, 0x91, 0x8f, 0x03,
			0xf5, 0x3c, 0x52, 0xda, 0xc5, 0x4e, 0xd8, 0x25,
			0x97, 0x40, 0x05, 0x1e, 0x9c, 0x5f, 0xec, 0xf6,
			0x43, 0x44, 0xf7, 0xa8, 0x22, 0x60, 0xed, 0xcc,
			0x30, 0x4c, 0x65, 0x28, 0xf6, 0x59, 0xc7, 0x78,
			0x66, 0xa5, 0x10, 0xd9, 0xc1, 0xd6, 0xae, 0x5e,
		},
		.out_len = 64,
	},
	{
		.mode = NID_aes_192_ofb128,
		.key = {
			0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52,
			0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5,
			0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b,
		},
		.iv = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.iv_len = 16,
		.in = {
			0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
			0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
			0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
			0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
			0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
			0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
			0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
			0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10,
		},
		.in_len = 64,
		.out = {
			0xcd, 0xc8, 0x0d, 0x6f, 0xdd, 0xf1, 0x8c, 0xab,
			0x34, 0xc2, 0x59, 0x09, 0xc9, 0x9a, 0x41, 0x74,
			0xfc, 0xc2, 0x8b, 0x8d, 0x4c, 0x63, 0x83, 0x7c,
			0x09, 0xe8, 0x17, 0x00, 0xc1, 0x10, 0x04, 0x01,
			0x8d, 0x9a, 0x9a, 0xea, 0xc0, 0xf6, 0x59, 0x6f,
			0x55, 0x9c, 0x6d, 0x4d, 0xaf, 0x59, 0xa5, 0xf2,
			0x6d, 0x9f, 0x20, 0x08, 0x57, 0xca, 0x6c, 0x3e,
			0x9c, 0xac, 0x52, 0x4b, 0xd9, 0xac, 0xc9, 0x2a,
		},
		.out_len = 64,
	},
	{
		.mode = NID_aes_256_ofb128,
		.key = {
			0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
			0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
			0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
			0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4,
		},
		.iv = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.iv_len = 16,
		.in = {
			0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
			0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
			0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
			0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
			0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
			0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
			0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
			0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10,
		},
		.in_len = 64,
		.out = {
			0xdc, 0x7e, 0x84, 0xbf, 0xda, 0x79, 0x16, 0x4b,
			0x7e, 0xcd, 0x84, 0x86, 0x98, 0x5d, 0x38, 0x60,
			0x4f, 0xeb, 0xdc, 0x67, 0x40, 0xd2, 0x0b, 0x3a,
			0xc8, 0x8f, 0x6a, 0xd8, 0x2a, 0x4f, 0xb0, 0x8d,
			0x71, 0xab, 0x47, 0xa0, 0x86, 0xe8, 0x6e, 0xed,
			0xf3, 0x9d, 0x1c, 0x5b, 0xba, 0x97, 0xc4, 0x08,
			0x01, 0x26, 0x14, 0x1d, 0x67, 0xf3, 0x7b, 0xe8,
			0x53, 0x8f, 0x5a, 0x8b, 0xe7, 0x40, 0xe4, 0x84,
		},
		.out_len = 64,
	},
};

#define N_AES_TESTS (sizeof(aes_tests) / sizeof(aes_tests[0]))

static int
aes_ecb_test(size_t test_number, const char *label, int key_bits,
    const struct aes_test *at)
{
	AES_KEY key;
	uint8_t out[64];

	if (at->padding) {
		/* XXX - Handle padding */
		return 1;
	}

	/* Encryption */
	memset(out, 0, sizeof(out));
	AES_set_encrypt_key(at->key, key_bits, &key);
	AES_ecb_encrypt(at->in, out, &key, 1);

	if (memcmp(at->out, out, at->out_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): encryption mismatch\n",
		    label, test_number);
		return 0;
	}

	/* Decryption */
	memset(out, 0, sizeof(out));
	AES_set_decrypt_key(at->key, key_bits, &key);
	AES_ecb_encrypt(at->out, out, &key, 0);

	if (memcmp(at->in, out, at->in_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): decryption mismatch\n",
		    label, test_number);
		return 0;
	}

	return 1;
}


static int
aes_cbc_test(size_t test_number, const char *label, int key_bits,
    const struct aes_test *at)
{
	AES_KEY key;
	uint8_t out[64];
	uint8_t iv[16];

	if (at->padding) {
		/* XXX - Handle padding */
		return 1;
	}

	/* Encryption */
	memset(out, 0, sizeof(out));
	memcpy(iv, at->iv, at->iv_len);
	AES_set_encrypt_key(at->key, key_bits, &key);
	AES_cbc_encrypt(at->in, out, at->in_len, &key, iv, 1);

	if (memcmp(at->out, out, at->out_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): encryption mismatch\n",
		    label, test_number);
		return 0;
	}

	/* Decryption */
	memset(out, 0, sizeof(out));
	memcpy(iv, at->iv, at->iv_len);
	AES_set_decrypt_key(at->key, key_bits, &key);
	AES_cbc_encrypt(at->out, out, at->out_len, &key, iv, 0);

	if (memcmp(at->in, out, at->in_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): decryption mismatch\n",
		    label, test_number);
		return 0;
	}

	return 1;
}

static int
aes_evp_test(size_t test_number, const struct aes_test *at, const char *label,
    int key_bits, const EVP_CIPHER *cipher)
{
	EVP_CIPHER_CTX *ctx;
	uint8_t out[64];
	int in_len, out_len, total_len;
	int i;
	int success = 0;

	if ((ctx = EVP_CIPHER_CTX_new()) == NULL) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_CIPHER_CTX_new failed\n",
		    label, test_number);
		goto failed;
	}

	/* EVP encryption */
	total_len = 0;
	memset(out, 0, sizeof(out));
	if (!EVP_EncryptInit(ctx, cipher, NULL, NULL)) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_EncryptInit failed\n",
		    label, test_number);
		goto failed;
	}

	if (!EVP_CIPHER_CTX_set_padding(ctx, at->padding)) {
		fprintf(stderr,
		    "FAIL (%s:%zu): EVP_CIPHER_CTX_set_padding failed\n",
		    label, test_number);
		goto failed;
	}

	if (!EVP_EncryptInit(ctx, NULL, at->key, at->iv)) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_EncryptInit failed\n",
		    label, test_number);
		goto failed;
	}

	for (i = 0; i < at->in_len;) {
		in_len = arc4random_uniform(at->in_len / 2);
		if (in_len > at->in_len - i)
			in_len = at->in_len - i;

		if (!EVP_EncryptUpdate(ctx, out + total_len, &out_len,
		    at->in + i, in_len)) {
			fprintf(stderr,
			    "FAIL (%s:%zu): EVP_EncryptUpdate failed\n",
			    label, test_number);
			goto failed;
		}

		i += in_len;
		total_len += out_len;
	}

	if (!EVP_EncryptFinal_ex(ctx, out + total_len, &out_len)) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_EncryptFinal_ex failed\n",
		    label, test_number);
		goto failed;
	}
	total_len += out_len;

	if (!EVP_CIPHER_CTX_reset(ctx)) {
		fprintf(stderr,
		    "FAIL (%s:%zu): EVP_CIPHER_CTX_reset failed\n",
		    label, test_number);
		goto failed;
	}

	if (total_len != at->out_len) {
		fprintf(stderr,
		    "FAIL (%s:%zu): EVP encryption length mismatch "
		    "(%d != %d)\n", label, test_number, total_len, at->out_len);
		goto failed;
	}

	if (memcmp(at->out, out, at->out_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): EVP encryption mismatch\n",
		    label, test_number);
		goto failed;
	}

	/* EVP decryption */
	total_len = 0;
	memset(out, 0, sizeof(out));
	if (!EVP_DecryptInit(ctx, cipher, NULL, NULL)) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_DecryptInit failed\n",
		    label, test_number);
		goto failed;
	}

	if (!EVP_CIPHER_CTX_set_padding(ctx, at->padding)) {
		fprintf(stderr,
		    "FAIL (%s:%zu): EVP_CIPHER_CTX_set_padding failed\n",
		    label, test_number);
		goto failed;
	}

	if (!EVP_DecryptInit(ctx, NULL, at->key, at->iv)) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_DecryptInit failed\n",
		    label, test_number);
		goto failed;
	}

	for (i = 0; i < at->out_len;) {
		in_len = arc4random_uniform(at->out_len / 2);
		if (in_len > at->out_len - i)
			in_len = at->out_len - i;

		if (!EVP_DecryptUpdate(ctx, out + total_len, &out_len,
		    at->out + i, in_len)) {
			fprintf(stderr,
			    "FAIL (%s:%zu): EVP_DecryptUpdate failed\n",
			    label, test_number);
			goto failed;
		}

		i += in_len;
		total_len += out_len;
	}

	if (!EVP_DecryptFinal_ex(ctx, out + total_len, &out_len)) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_DecryptFinal_ex failed\n",
		    label, test_number);
		goto failed;
	}
	total_len += out_len;

	if (!EVP_CIPHER_CTX_reset(ctx)) {
		fprintf(stderr,
		    "FAIL (%s:%zu): EVP_CIPHER_CTX_reset failed\n",
		    label, test_number);
		goto failed;
	}

	if (total_len != at->in_len) {
		fprintf(stderr,
		    "FAIL (%s:%zu): EVP decryption length mismatch\n",
		    label, test_number);
		goto failed;
	}

	if (memcmp(at->in, out, at->in_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): EVP decryption mismatch\n",
		    label, test_number);
		goto failed;
	}

	success = 1;

 failed:
	EVP_CIPHER_CTX_free(ctx);
	return success;
}


static int
aes_key_bits_from_nid(int nid)
{
	switch (nid) {
	case NID_aes_128_ecb:
	case NID_aes_128_cbc:
	case NID_aes_128_cfb128:
	case NID_aes_128_ofb128:
	case NID_aes_128_gcm:
	case NID_aes_128_ccm:
		return 128;
	case NID_aes_192_ecb:
	case NID_aes_192_cbc:
	case NID_aes_192_cfb128:
	case NID_aes_192_ofb128:
	case NID_aes_192_gcm:
	case NID_aes_192_ccm:
		return 192;
	case NID_aes_256_ecb:
	case NID_aes_256_cbc:
	case NID_aes_256_cfb128:
	case NID_aes_256_ofb128:
	case NID_aes_256_gcm:
	case NID_aes_256_ccm:
		return 256;
	default:
		return -1;
	}
}

static int
aes_cipher_from_nid(int nid, const char **out_label,
    const EVP_CIPHER **out_cipher)
{
	switch (nid) {
	/* ECB */
	case NID_aes_128_ecb:
		*out_label = SN_aes_128_ecb;
		*out_cipher = EVP_aes_128_ecb();
		break;
	case NID_aes_192_ecb:
		*out_label = SN_aes_192_ecb;
		*out_cipher = EVP_aes_192_ecb();
		break;
	case NID_aes_256_ecb:
		*out_label = SN_aes_256_ecb;
		*out_cipher = EVP_aes_256_ecb();
		break;

	/* CBC */
	case NID_aes_128_cbc:
		*out_label = SN_aes_128_cbc;
		*out_cipher = EVP_aes_128_cbc();
		break;
	case NID_aes_192_cbc:
		*out_label = SN_aes_192_cbc;
		*out_cipher = EVP_aes_192_cbc();
		break;
	case NID_aes_256_cbc:
		*out_label = SN_aes_256_cbc;
		*out_cipher = EVP_aes_256_cbc();
		break;

	/* CFB128 */
	case NID_aes_128_cfb128:
		*out_label = SN_aes_128_cfb128;
		*out_cipher = EVP_aes_128_cfb128();
		break;
	case NID_aes_192_cfb128:
		*out_label = SN_aes_192_cfb128;
		*out_cipher = EVP_aes_192_cfb128();
		break;
	case NID_aes_256_cfb128:
		*out_label = SN_aes_256_cfb128;
		*out_cipher = EVP_aes_256_cfb128();
		break;

	/* OFB128 */
	case NID_aes_128_ofb128:
		*out_label = SN_aes_128_ofb128;
		*out_cipher = EVP_aes_128_ofb();
		break;
	case NID_aes_192_ofb128:
		*out_label = SN_aes_192_ofb128;
		*out_cipher = EVP_aes_192_ofb();
		break;
	case NID_aes_256_ofb128:
		*out_label = SN_aes_256_ofb128;
		*out_cipher = EVP_aes_256_ofb();
		break;

	/* GCM */
	case NID_aes_128_gcm:
		*out_label = SN_aes_128_gcm;
		*out_cipher = EVP_aes_128_gcm();
		break;
	case NID_aes_192_gcm:
		*out_label = SN_aes_192_gcm;
		*out_cipher = EVP_aes_192_gcm();
		break;
	case NID_aes_256_gcm:
		*out_label = SN_aes_256_gcm;
		*out_cipher = EVP_aes_256_gcm();
		break;

	/* CCM */
	case NID_aes_128_ccm:
		*out_label = SN_aes_128_ccm;
		*out_cipher = EVP_aes_128_ccm();
		break;
	case NID_aes_192_ccm:
		*out_label = SN_aes_192_ccm;
		*out_cipher = EVP_aes_192_ccm();
		break;
	case NID_aes_256_ccm:
		*out_label = SN_aes_256_ccm;
		*out_cipher = EVP_aes_256_ccm();
		break;

	/* Unknown */
	default:
		return 0;
	}

	return 1;
}

static int
aes_test(void)
{
	const struct aes_test *at;
	const char *label;
	const EVP_CIPHER *cipher;
	int key_bits;
	size_t i;
	int failed = 1;

	for (i = 0; i < N_AES_TESTS; i++) {
		at = &aes_tests[i];
		key_bits = aes_key_bits_from_nid(at->mode);
		if (!aes_cipher_from_nid(at->mode, &label, &cipher))
			goto failed;

		switch (at->mode) {
		/* ECB */
		case NID_aes_128_ecb:
		case NID_aes_192_ecb:
		case NID_aes_256_ecb:
			if (!aes_ecb_test(i, label, key_bits, at))
				goto failed;
			break;
		
		/* CBC */	
		case NID_aes_128_cbc:
		case NID_aes_192_cbc:
		case NID_aes_256_cbc:
			if (!aes_cbc_test(i, label, key_bits, at))
				goto failed;
			break;

		/* CFB128 */
		case NID_aes_128_cfb128:
		case NID_aes_192_cfb128:
		case NID_aes_256_cfb128:
			/* XXX - CFB128 non-EVP tests */
			break;

		/* OFB128 */
		case NID_aes_128_ofb128:
		case NID_aes_192_ofb128:
		case NID_aes_256_ofb128:
			/* XXX - OFB128 non-EVP tests */
			break;

		/* GCM */
		case NID_aes_128_gcm:
		case NID_aes_192_gcm:
		case NID_aes_256_gcm:
			/* GCM is EVP-only */
			break;

		/* CCM */
		case NID_aes_128_ccm:
		case NID_aes_192_ccm:
		case NID_aes_256_ccm:
			/* XXX - CCM non-EVP tests */
			break;

		/* Unknown */
		default:
			fprintf(stderr, "FAIL: unknown mode (%d)\n",
			    at->mode);
			goto failed;
		}

		if (!aes_evp_test(i, at, label, key_bits, cipher))
			goto failed;
	}

	failed = 0;

 failed:
	return failed;
}

int
main(int argc, char **argv)
{
	int failed = 0;

	failed |= aes_test();

	return failed;
}
