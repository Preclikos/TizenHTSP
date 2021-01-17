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

#include "parser_avc.h"
#include <cstdlib>
#include <netinet/in.h>

#define H264_NAL_SPS             7
#define H264_NAL_PPS             8
#define MIN(a,b) ((a) < (b) ? (a) : (b))

const uint8_t * AvcParser::avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
  const uint8_t *a = p + 4 - ((intptr_t)p & 3);

  for (end -= 3; p < a && p < end; p++) {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  for (end -= 3; p < end; p += 4) {
    uint32_t x = *(const uint32_t*)p;
//  if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
//  if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
    if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
      if (p[1] == 0) {
	if (p[0] == 0 && p[2] == 1)
	  return p;
	if (p[2] == 0 && p[3] == 1)
	  return p+1;
      }
      if (p[3] == 0) {
	if (p[2] == 0 && p[4] == 1)
	  return p+2;
	if (p[4] == 0 && p[5] == 1)
	  return p+3;
      }
    }
  }

  for (end += 3; p < end; p++) {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  return end + 3;
}



const uint8_t *
AvcParser::avc_find_startcode(const uint8_t *p, const uint8_t *end)
{
    const uint8_t *out= avc_find_startcode_internal(p, end);
    while(p<out && out<end && !out[-1]) out--;
    return out;
}


int AvcParser::avc_parse_nal_units(sbuf_t *sb, const uint8_t *buf_in, int size)
{
  const uint8_t *p = buf_in;
  const uint8_t *end = p + size;
  const uint8_t *nal_start, *nal_end;

  size = 0;
  nal_start = avc_find_startcode(p, end);
  for (;;) {
    while (nal_start < end && !*(nal_start++));
    if (nal_start == end)
      break;

    nal_end = avc_find_startcode(nal_start, end);

    int l = nal_end - nal_start;
    sbuf_put_be32(sb, l);
    sbuf_append(sb, nal_start, l);
    size += 4 + l;
    nal_start = nal_end;
  }
  return size;
}

void AvcParser::sbuf_append(sbuf_t *sb, const void *data, int len)
{
  sbuf_alloc_(sb, len);
  memcpy(sb->sb_data + sb->sb_ptr, data, len);
  sb->sb_ptr += len;
}

void AvcParser::sbuf_init(sbuf_t *sb)
{
  memset(sb, 0, sizeof(sbuf_t));
}

void AvcParser::sbuf_alloc_(sbuf_t *sb, int len)
{
  if(sb->sb_data == NULL) {
    sb->sb_size = len * 4 > 4000 ? len * 4 : 4000;
    sb->sb_data = (uint8_t*)malloc(sb->sb_size);
    return;
  } else {
    sb->sb_size += len * 4;
    sb->sb_data = (uint8_t*)realloc(sb->sb_data, sb->sb_size);
  }

  if(sb->sb_data == NULL)
    sbuf_alloc_fail(sb->sb_size);
}

void AvcParser::sbuf_alloc_fail(size_t len)
{
  fprintf(stderr, "Unable to allocate %zd bytes\n", len);
  abort();
}

void AvcParser::sbuf_put_be32(sbuf_t *sb, uint32_t u32)
{
  u32 = htonl(u32);
  sbuf_append(sb, &u32, 4);
}



int AvcParser::isom_write_avcc(sbuf_t *sb, const uint8_t *data, int len)
{
  sbuf_t b;
  uint8_t *buf, *end;
  uint32_t *sps_size_array, *pps_size_array;
  uint32_t pps_count, sps_count;
  uint8_t **sps_array, **pps_array;
  int i, ret;

  if (len <= 6)
    return 0;

  if (RB32(data) != 0x00000001 &&
      RB24(data) != 0x000001) {
    sbuf_append(sb, data, len);
    return 0;
  }

  sbuf_init(&b);

  len = avc_parse_nal_units(&b, data, len);

  buf = b.sb_data;
  end = buf + len;

  sps_size_array = pps_size_array = NULL;
  pps_count = sps_count = 0;
  sps_array = pps_array = NULL;
  ret = 0;

  // look for sps and pps
  while (end - buf > 4) {
    unsigned int size;
    uint8_t nal_type;
    size = MIN(RB32(buf), end - buf - 4);
    buf += 4;
    nal_type = buf[0] & 0x1f;

    if ((nal_type == H264_NAL_SPS) && (size >= 4) && (size <= UINT16_MAX)) {
      sps_array = (uint8_t**)realloc(sps_array,sizeof(uint8_t*)*(sps_count+1));
      sps_size_array = (uint32_t*)realloc(sps_size_array,sizeof(uint32_t)*(sps_count+1));
      sps_array[sps_count] = buf;
      sps_size_array[sps_count] = size;
      sps_count++;
    } else if ((nal_type == H264_NAL_PPS) && (size <= UINT16_MAX)) {
      pps_size_array = (uint32_t*)realloc(pps_size_array,sizeof(uint32_t)*(pps_count+1));
      pps_array = (uint8_t**)realloc(pps_array,sizeof (uint8_t*)*(pps_count+1));
      pps_array[pps_count] = buf;
      pps_size_array[pps_count] = size;
      pps_count++;
    }
    buf += size;
  }

  if(!sps_count || !pps_count) {
    ret = -1;
    goto end;
  }

  sbuf_put_byte(sb, 1); // version
  sbuf_put_byte(sb, sps_array[0][1]); // profile
  sbuf_put_byte(sb, sps_array[0][2]); // profile compat
  sbuf_put_byte(sb, sps_array[0][3]); // level
  sbuf_put_byte(sb, 0xff); // 6 bits reserved (111111) + 2 bits nal size length - 1 (11)
  sbuf_put_byte(sb, 0xe0+sps_count); // 3 bits reserved (111) + 5 bits number of sps (00001)
  for (i=0;i<sps_count;i++) {
    sbuf_put_be16(sb, sps_size_array[i]);
    sbuf_append(sb, sps_array[i], sps_size_array[i]);
  }

  sbuf_put_byte(sb, pps_count); // number of pps
  for (i=0;i<pps_count;i++) {
    sbuf_put_be16(sb, pps_size_array[i]);
    sbuf_append(sb, pps_array[i], pps_size_array[i]);
  }

end:
  if (sps_count) {
    free(sps_array);
    free(sps_size_array);
  }
  if (pps_count) {
    free(pps_array);
    free(pps_size_array);
  }

  sbuf_free(&b);
  return ret;
}

void AvcParser::sbuf_put_byte(sbuf_t *sb, uint8_t u8)
{
  sbuf_append(sb, &u8, 1);
}

void AvcParser::sbuf_put_be16(sbuf_t *sb, uint16_t u16)
{
  u16 = htons(u16);
  sbuf_append(sb, &u16, 2);
}

void AvcParser::sbuf_free(sbuf_t *sb)
{
  free(sb->sb_data);
  sb->sb_size = sb->sb_ptr = sb->sb_err = 0;
  sb->sb_data = NULL;
}

inline uint32_t AvcParser::RB32(const uint8_t *d)
{
  return (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | d[3];
}

inline uint32_t AvcParser::RB24(const uint8_t *d)
{
  return (d[0] << 16) | (d[1] << 8) | d[2];
}
/*
th_pkt_t *
avc_convert_pkt(th_pkt_t *src)
{
  sbuf_t payload;
  th_pkt_t *pkt = pkt_copy_nodata(src);

  sbuf_init(&payload);

  avc_parse_nal_units(&payload, pktbuf_ptr(src->pkt_payload),
		                pktbuf_len(src->pkt_payload));

  pkt->pkt_payload = pktbuf_make(payload.sb_data, payload.sb_ptr);
  return pkt;
}
*/
