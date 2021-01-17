
/*
 * AVC helper functions for muxers
 * Copyright (c) 2006 Baptiste Coudurier <baptiste.coudurier@smartjog.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVC_H__
#define AVC_H__

 #include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct sbuf {
  uint8_t *sb_data;
  int      sb_ptr;
  int      sb_size;
  uint16_t sb_err;
  uint8_t  sb_bswap;
} sbuf_t;

class AvcParser {
 public:
	int avc_parse_nal_units(const uint8_t *buf_in, int size);

//const uint8_t * avc_find_startcode(const uint8_t *p, const uint8_t *end);

int avc_parse_nal_units(sbuf_t *sb, const uint8_t *buf_in, int size);

//int isom_write_avcc(sbuf_t *sb, const uint8_t *src, int len);

//th_pkt_t *avc_convert_pkt(th_pkt_t *src);
void sbuf_init(sbuf_t *sb);
int isom_write_avcc(sbuf_t *sb, const uint8_t *data, int len);
 private:
	static inline uint32_t RB32(const uint8_t *d);
	static inline uint32_t RB24(const uint8_t *d);
	void sbuf_free(sbuf_t *sb);
	void sbuf_put_be16(sbuf_t *sb, uint16_t u16);
	void sbuf_put_byte(sbuf_t *sb, uint8_t u8);
	void sbuf_put_be32(sbuf_t *sb, uint32_t u32);
	static void sbuf_alloc_fail(size_t len);
	static void sbuf_alloc_(sbuf_t *sb, int len);
	void sbuf_append(sbuf_t *sb, const void *data, int len);
	static const uint8_t * avc_find_startcode_internal(const uint8_t *p, const uint8_t *end);
	const uint8_t *	avc_find_startcode(const uint8_t *p, const uint8_t *end);
	void sbuf_append(uint8_t *sb, const void *data, int len);
};
#endif
