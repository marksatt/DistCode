/* -*- c-file-style: "java"; indent-tabs-mode: nil; tab-width: 4; fill-column: 78 -*-
 * Copyright 2009 Google Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
*/

/* Author: Mark Mentovai */

/*
 * byte_swapping.h:
 * Performs byte swapping operations.
 */

#ifndef DISTCC_BYTE_SWAPPING_H_
#define DISTCC_BYTE_SWAPPING_H_

#include <stdint.h>

static inline uint16_t swap_16(uint16_t x);
static inline uint32_t swap_32(uint32_t x);
static inline uint64_t swap_64(uint64_t x);
static inline uint16_t swap_big_to_cpu_16(uint16_t x);
static inline uint32_t swap_big_to_cpu_32(uint32_t x);
static inline uint64_t swap_big_to_cpu_64(uint64_t x);
static inline uint16_t maybe_swap_16(int swap, uint16_t x);
static inline uint32_t maybe_swap_32(int swap, uint32_t x);
static inline uint64_t maybe_swap_64(int swap, uint64_t x);

static inline uint16_t maybe_swap_16(int swap, uint16_t x) {
  return swap ? swap_16(x) : x;
}

static inline uint32_t maybe_swap_32(int swap, uint32_t x) {
  return swap ? swap_32(x) : x;
}

static inline uint64_t maybe_swap_64(int swap, uint64_t x) {
  return swap ? swap_64(x) : x;
}

/* Use OS-provided swapping functions when they're known to be available. */

#if defined(__APPLE__)

#include <libkern/OSByteOrder.h>

static inline uint16_t swap_16(uint16_t x) {
  return OSSwapInt16(x);
}

static inline uint32_t swap_32(uint32_t x) {
  return OSSwapInt32(x);
}

static inline uint64_t swap_64(uint64_t x) {
  return OSSwapInt64(x);
}

static inline uint16_t swap_big_to_cpu_16(uint16_t x) {
  return OSSwapBigToHostInt16(x);
}

static inline uint32_t swap_big_to_cpu_32(uint32_t x) {
  return OSSwapBigToHostInt32(x);
}

static inline uint64_t swap_big_to_cpu_64(uint64_t x) {
  return OSSwapBigToHostInt64(x);
}

#elif defined(linux)

#include <asm/byteorder.h>

static inline uint16_t swap_16(uint16_t x) {
  return __swab16(x);
}

static inline uint32_t swap_32(uint32_t x) {
  return __swab32(x);
}

static inline uint64_t swap_64(uint64_t x) {
  return __swab64(x);
}

static inline uint16_t swap_big_to_cpu_16(uint16_t x) {
  return __be16_to_cpu(x);
}

static inline uint32_t swap_big_to_cpu_32(uint32_t x) {
  return __be32_to_cpu(x);
}

static inline uint64_t swap_big_to_cpu_64(uint64_t x) {
  return __be64_to_cpu(x);
}

#else  /* !apple & !linux */

/* If other systems provide swapping functions, they should be used in
 * preference to this fallback code. */

static inline uint16_t swap_16(uint16_t x) {
  return (x >> 8) | (x << 8);
}

static inline uint32_t swap_32(uint32_t x) {
  return  (x >> 24) |
         ((x >> 8) & 0x0000ff00) |
         ((x << 8) & 0x00ff0000) |
          (x << 24);
}

static inline uint64_t swap_64(uint64_t x) {
  uint32_t* x32 = (uint32_t*)&x;
  uint64_t y = swap_32(x32[0]);
  uint64_t z = swap_32(x32[1]);
  return (z << 32) | y;
}

static inline uint16_t swap_big_to_cpu_16(uint16_t x) {
#ifdef WORDS_BIGENDIAN
  return x;
#else
  return swap_16(x);
#endif
}

static inline uint32_t swap_big_to_cpu_32(uint32_t x) {
#ifdef WORDS_BIGENDIAN
  return x;
#else
  return swap_32(x);
#endif
}

static inline uint64_t swap_big_to_cpu_64(uint64_t x) {
#ifdef WORDS_BIGENDIAN
  return x;
#else
  return swap_64(x);
#endif
}

#endif /* !apple & !linux */

#endif  /* DISTCC_BYTE_SWAPPING_H_ */
