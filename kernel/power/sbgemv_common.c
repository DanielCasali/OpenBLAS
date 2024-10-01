/***************************************************************************
Copyright (c) 2024, The OpenBLAS Project
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.
3. Neither the name of the OpenBLAS project nor the names of
its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE OPENBLAS PROJECT OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#ifndef SBGEMV_COMMON_C
#define SBGEMV_COMMON_C
#include "gemm_common.c"

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BF16_HI(data, zero)     (vec_f32)vec_mergeh(data, zero)
#define BF16_LO(data, zero)     (vec_f32)vec_mergel(data, zero)
#else
#define BF16_HI(data, zero)     (vec_f32)vec_mergeh(zero, data)
#define BF16_LO(data, zero)     (vec_f32)vec_mergel(zero, data)
#endif

FORCEINLINE vec_f32 vec_loadNHi(void *src, BLASLONG n, vec_bf16 zero)
{
  vec_bf16 data = vec_loadN(src, n);
  return BF16_HI(data, zero);
}

FORCEINLINE vec_f32 vec_mult(vec_f32 *inp, vec_bf16 in0, vec_bf16 zero)
{
  vec_f32 v_in00 = BF16_HI(in0, zero);
  vec_f32 v_in01 = BF16_LO(in0, zero);

  return (inp[0] * v_in00) + (inp[1] * v_in01);
}

FORCEINLINE vec_f32 vec_load_mult(vec_bf16 *in, vec_f32 *inp, vec_bf16 zero)
{
  vec_bf16 in0 = (vec_bf16)vec_load_vec(in);

  return vec_mult(inp, in0, zero);
}

FORCEINLINE void vec_load_vec2(vec_bf16 *in, vec_f32 *v_x0, vec_bf16 zero)
{
  vec_bf16 inp = (vec_bf16)vec_load_vec(in);

  v_x0[0] = BF16_HI(inp, zero);
  v_x0[1] = BF16_LO(inp, zero);
}

FORCEINLINE void vec_mult2(vec_f32 v_x0, vec_bf16 in0, vec_bf16 zero, vec_f32 *vy0)
{
  vec_f32 v_in00 = BF16_HI(in0, zero);
  vec_f32 v_in01 = BF16_LO(in0, zero);

  vy0[0] += (v_x0 * v_in00);
  vy0[1] += (v_x0 * v_in01);
}

FORCEINLINE void vec_load_mult2(vec_f32 v_x0, vec_bf16 *in, vec_bf16 zero, vec_f32 *vy0)
{
  vec_bf16 in0 = (vec_bf16)vec_load_vec(in);

  vec_mult2(v_x0, in0, zero, vy0);
}

FORCEINLINE vec_f32 vec_loadN_mult(vec_bf16 *in, vec_f32 *inp, BLASLONG n, vec_bf16 zero)
{
  vec_bf16 in0 = vec_loadN(in, n);

  return vec_mult(inp, in0, zero);
}

FORCEINLINE void vec_loadN_vec2(vec_bf16 *in, vec_f32 *v_x0, BLASLONG n, vec_bf16 zero)
{
  vec_bf16 inp = vec_loadN(in, n);

  v_x0[0] = BF16_HI(inp, zero);
  v_x0[1] = BF16_LO(inp, zero);
}

FORCEINLINE void vec_loadN_mult2(vec_f32 v_x0, vec_bf16 *in, BLASLONG n, vec_bf16 zero, vec_f32 *vy0)
{
  vec_bf16 in0 = vec_loadN(in, n);

  vec_mult2(v_x0, in0, zero, vy0);
}

FORCEINLINE vec_f32 vec_loadNHi_mult(vec_bf16 *in, vec_f32 v_inp0, BLASLONG n, vec_bf16 zero)
{
  vec_f32 v_in00 = vec_loadNHi(in, n, zero);

  return (v_inp0 * v_in00);
}

FORCEINLINE void copy_x(BLASLONG n, IFLOAT *src, IFLOAT *dest, BLASLONG inc_src)
{
  for (BLASLONG i = 0; i < n; i++) {
    *dest++ = *src;
    src += inc_src;
  }
}

FORCEINLINE void copy_y_beta(BLASLONG n, FLOAT *src, FLOAT *dest, BLASLONG inc_src, FLOAT beta)
{
  if (beta == 0) {
    memset(dest, 0, sizeof(FLOAT) * n);
  } else if (beta == 1) {
    for (BLASLONG i = 0; i < n; i++) {
      *dest++ = *src;
      src += inc_src;
    }
  } else {
    for (BLASLONG i = 0; i < n; i++) {
      *dest++ = *src * beta;
      src += inc_src;
    }
  }
}

FORCEINLINE void copy_y(BLASLONG n, FLOAT *src, FLOAT *dest, BLASLONG inc_src, FLOAT beta)
{
  if (beta == 0) {
    for (BLASLONG i = 0; i < n; i++) {
      *dest = *src++;
      dest += inc_src;
    }
  } else if (beta == 1) {
    for (BLASLONG i = 0; i < n; i++) {
      *dest += *src++;
      dest += inc_src;
    }
  } else {
    for (BLASLONG i = 0; i < n; i++) {
      *dest = *src++ + (beta * *dest);
      dest += inc_src;
    }
  }
}

FORCEINLINE void move_y(BLASLONG n, FLOAT *src, FLOAT *dest, BLASLONG inc_dest)
{
  for (BLASLONG i = 0; i < n; i++) {
    *dest = *src++;
    dest += inc_dest;
  }
}
#endif
