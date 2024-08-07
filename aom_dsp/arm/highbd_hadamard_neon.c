/*
 * Copyright (c) 2023 The WebM project authors. All rights reserved.
 * Copyright (c) 2023, Alliance for Open Media. All rights reserved.
 *
 *  This source code is subject to the terms of the BSD 2 Clause License and
 *  the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 *  was not distributed with this source code in the LICENSE file, you can
 *  obtain it at www.aomedia.org/license/software. If the Alliance for Open
 *  Media Patent License 1.0 was not distributed with this source code in the
 *  PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <arm_neon.h>
#include "config/aom_dsp_rtcd.h"
#include "aom/aom_integer.h"
#include "aom_dsp/arm/mem_neon.h"
#include "aom_dsp/arm/transpose_neon.h"
#include "aom_dsp/arm/sum_neon.h"
#include "aom_ports/mem.h"

static inline void hadamard_highbd_col8_first_pass(int16x8_t *a0, int16x8_t *a1,
                                                   int16x8_t *a2, int16x8_t *a3,
                                                   int16x8_t *a4, int16x8_t *a5,
                                                   int16x8_t *a6,
                                                   int16x8_t *a7) {
  int16x8_t b0 = vaddq_s16(*a0, *a1);
  int16x8_t b1 = vsubq_s16(*a0, *a1);
  int16x8_t b2 = vaddq_s16(*a2, *a3);
  int16x8_t b3 = vsubq_s16(*a2, *a3);
  int16x8_t b4 = vaddq_s16(*a4, *a5);
  int16x8_t b5 = vsubq_s16(*a4, *a5);
  int16x8_t b6 = vaddq_s16(*a6, *a7);
  int16x8_t b7 = vsubq_s16(*a6, *a7);

  int16x8_t c0 = vaddq_s16(b0, b2);
  int16x8_t c2 = vsubq_s16(b0, b2);
  int16x8_t c1 = vaddq_s16(b1, b3);
  int16x8_t c3 = vsubq_s16(b1, b3);
  int16x8_t c4 = vaddq_s16(b4, b6);
  int16x8_t c6 = vsubq_s16(b4, b6);
  int16x8_t c5 = vaddq_s16(b5, b7);
  int16x8_t c7 = vsubq_s16(b5, b7);

  *a0 = vaddq_s16(c0, c4);
  *a2 = vsubq_s16(c0, c4);
  *a7 = vaddq_s16(c1, c5);
  *a6 = vsubq_s16(c1, c5);
  *a3 = vaddq_s16(c2, c6);
  *a1 = vsubq_s16(c2, c6);
  *a4 = vaddq_s16(c3, c7);
  *a5 = vsubq_s16(c3, c7);
}

static inline void hadamard_highbd_col4_second_pass(int16x4_t a0, int16x4_t a1,
                                                    int16x4_t a2, int16x4_t a3,
                                                    int16x4_t a4, int16x4_t a5,
                                                    int16x4_t a6, int16x4_t a7,
                                                    tran_low_t *coeff) {
  int32x4_t b0 = vaddl_s16(a0, a1);
  int32x4_t b1 = vsubl_s16(a0, a1);
  int32x4_t b2 = vaddl_s16(a2, a3);
  int32x4_t b3 = vsubl_s16(a2, a3);
  int32x4_t b4 = vaddl_s16(a4, a5);
  int32x4_t b5 = vsubl_s16(a4, a5);
  int32x4_t b6 = vaddl_s16(a6, a7);
  int32x4_t b7 = vsubl_s16(a6, a7);

  int32x4_t c0 = vaddq_s32(b0, b2);
  int32x4_t c2 = vsubq_s32(b0, b2);
  int32x4_t c1 = vaddq_s32(b1, b3);
  int32x4_t c3 = vsubq_s32(b1, b3);
  int32x4_t c4 = vaddq_s32(b4, b6);
  int32x4_t c6 = vsubq_s32(b4, b6);
  int32x4_t c5 = vaddq_s32(b5, b7);
  int32x4_t c7 = vsubq_s32(b5, b7);

  int32x4_t d0 = vaddq_s32(c0, c4);
  int32x4_t d2 = vsubq_s32(c0, c4);
  int32x4_t d7 = vaddq_s32(c1, c5);
  int32x4_t d6 = vsubq_s32(c1, c5);
  int32x4_t d3 = vaddq_s32(c2, c6);
  int32x4_t d1 = vsubq_s32(c2, c6);
  int32x4_t d4 = vaddq_s32(c3, c7);
  int32x4_t d5 = vsubq_s32(c3, c7);

  vst1q_s32(coeff + 0, d0);
  vst1q_s32(coeff + 4, d1);
  vst1q_s32(coeff + 8, d2);
  vst1q_s32(coeff + 12, d3);
  vst1q_s32(coeff + 16, d4);
  vst1q_s32(coeff + 20, d5);
  vst1q_s32(coeff + 24, d6);
  vst1q_s32(coeff + 28, d7);
}

void aom_highbd_hadamard_8x8_neon(const int16_t *src_diff, ptrdiff_t src_stride,
                                  tran_low_t *coeff) {
  int16x4_t b0, b1, b2, b3, b4, b5, b6, b7;

  int16x8_t s0 = vld1q_s16(src_diff + 0 * src_stride);
  int16x8_t s1 = vld1q_s16(src_diff + 1 * src_stride);
  int16x8_t s2 = vld1q_s16(src_diff + 2 * src_stride);
  int16x8_t s3 = vld1q_s16(src_diff + 3 * src_stride);
  int16x8_t s4 = vld1q_s16(src_diff + 4 * src_stride);
  int16x8_t s5 = vld1q_s16(src_diff + 5 * src_stride);
  int16x8_t s6 = vld1q_s16(src_diff + 6 * src_stride);
  int16x8_t s7 = vld1q_s16(src_diff + 7 * src_stride);

  // For the first pass we can stay in 16-bit elements (4095*8 = 32760).
  hadamard_highbd_col8_first_pass(&s0, &s1, &s2, &s3, &s4, &s5, &s6, &s7);

  transpose_elems_inplace_s16_8x8(&s0, &s1, &s2, &s3, &s4, &s5, &s6, &s7);

  // For the second pass we need to widen to 32-bit elements, so we're
  // processing 4 columns at a time.
  // Skip the second transpose because it is not required.

  b0 = vget_low_s16(s0);
  b1 = vget_low_s16(s1);
  b2 = vget_low_s16(s2);
  b3 = vget_low_s16(s3);
  b4 = vget_low_s16(s4);
  b5 = vget_low_s16(s5);
  b6 = vget_low_s16(s6);
  b7 = vget_low_s16(s7);

  hadamard_highbd_col4_second_pass(b0, b1, b2, b3, b4, b5, b6, b7, coeff);

  b0 = vget_high_s16(s0);
  b1 = vget_high_s16(s1);
  b2 = vget_high_s16(s2);
  b3 = vget_high_s16(s3);
  b4 = vget_high_s16(s4);
  b5 = vget_high_s16(s5);
  b6 = vget_high_s16(s6);
  b7 = vget_high_s16(s7);

  hadamard_highbd_col4_second_pass(b0, b1, b2, b3, b4, b5, b6, b7, coeff + 32);
}

void aom_highbd_hadamard_16x16_neon(const int16_t *src_diff,
                                    ptrdiff_t src_stride, tran_low_t *coeff) {
  // Rearrange 16x16 to 8x32 and remove stride.
  // Top left first.
  aom_highbd_hadamard_8x8_neon(src_diff, src_stride, coeff);
  // Top right.
  aom_highbd_hadamard_8x8_neon(src_diff + 8, src_stride, coeff + 64);
  // Bottom left.
  aom_highbd_hadamard_8x8_neon(src_diff + 8 * src_stride, src_stride,
                               coeff + 128);
  // Bottom right.
  aom_highbd_hadamard_8x8_neon(src_diff + 8 * src_stride + 8, src_stride,
                               coeff + 192);

  for (int i = 0; i < 16; i++) {
    int32x4_t a0 = vld1q_s32(coeff + 4 * i);
    int32x4_t a1 = vld1q_s32(coeff + 4 * i + 64);
    int32x4_t a2 = vld1q_s32(coeff + 4 * i + 128);
    int32x4_t a3 = vld1q_s32(coeff + 4 * i + 192);

    int32x4_t b0 = vhaddq_s32(a0, a1);
    int32x4_t b1 = vhsubq_s32(a0, a1);
    int32x4_t b2 = vhaddq_s32(a2, a3);
    int32x4_t b3 = vhsubq_s32(a2, a3);

    int32x4_t c0 = vaddq_s32(b0, b2);
    int32x4_t c1 = vaddq_s32(b1, b3);
    int32x4_t c2 = vsubq_s32(b0, b2);
    int32x4_t c3 = vsubq_s32(b1, b3);

    vst1q_s32(coeff + 4 * i, c0);
    vst1q_s32(coeff + 4 * i + 64, c1);
    vst1q_s32(coeff + 4 * i + 128, c2);
    vst1q_s32(coeff + 4 * i + 192, c3);
  }
}

void aom_highbd_hadamard_32x32_neon(const int16_t *src_diff,
                                    ptrdiff_t src_stride, tran_low_t *coeff) {
  // Rearrange 32x32 to 16x64 and remove stride.
  // Top left first.
  aom_highbd_hadamard_16x16_neon(src_diff, src_stride, coeff);
  // Top right.
  aom_highbd_hadamard_16x16_neon(src_diff + 16, src_stride, coeff + 256);
  // Bottom left.
  aom_highbd_hadamard_16x16_neon(src_diff + 16 * src_stride, src_stride,
                                 coeff + 512);
  // Bottom right.
  aom_highbd_hadamard_16x16_neon(src_diff + 16 * src_stride + 16, src_stride,
                                 coeff + 768);

  for (int i = 0; i < 64; i++) {
    int32x4_t a0 = vld1q_s32(coeff + 4 * i);
    int32x4_t a1 = vld1q_s32(coeff + 4 * i + 256);
    int32x4_t a2 = vld1q_s32(coeff + 4 * i + 512);
    int32x4_t a3 = vld1q_s32(coeff + 4 * i + 768);

    int32x4_t b0 = vshrq_n_s32(vaddq_s32(a0, a1), 2);
    int32x4_t b1 = vshrq_n_s32(vsubq_s32(a0, a1), 2);
    int32x4_t b2 = vshrq_n_s32(vaddq_s32(a2, a3), 2);
    int32x4_t b3 = vshrq_n_s32(vsubq_s32(a2, a3), 2);

    int32x4_t c0 = vaddq_s32(b0, b2);
    int32x4_t c1 = vaddq_s32(b1, b3);
    int32x4_t c2 = vsubq_s32(b0, b2);
    int32x4_t c3 = vsubq_s32(b1, b3);

    vst1q_s32(coeff + 4 * i, c0);
    vst1q_s32(coeff + 4 * i + 256, c1);
    vst1q_s32(coeff + 4 * i + 512, c2);
    vst1q_s32(coeff + 4 * i + 768, c3);
  }
}
