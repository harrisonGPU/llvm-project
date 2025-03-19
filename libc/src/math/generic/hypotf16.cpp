//===-- Implementation of hypotf16 function -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/math/hypotf16.h"
#include "src/__support/FPUtil/FEnvImpl.h"
#include "src/__support/FPUtil/generic/sqrt.h"
#include "src/__support/common.h"
#include "src/__support/FPUtil/FPBits.h"
#include "src/__support/macros/config.h"
#include "src/__support/macros/properties/types.h"
#include "src/__support/FPUtil/sqrt.h"
#include "src/__support/FPUtil/multiply_add.h"

namespace LIBC_NAMESPACE_DECL {

LLVM_LIBC_FUNCTION(float16, hypotf16, (float16 x, float16 y)) {
  using FPBits = fputil::FPBits<float16>;
  using FloatBits = fputil::FPBits<float>;

  FPBits x_abs = FPBits(x).abs();
  FPBits y_abs = FPBits(y).abs();

  bool x_abs_larger = x_abs.uintval() >= y_abs.uintval();

  FPBits a_bits = x_abs_larger ? x_abs : y_abs;
  FPBits b_bits = x_abs_larger ? y_abs : x_abs;

  uint16_t a_u = a_bits.uintval();
  uint16_t b_u = b_bits.uintval();

  if (LIBC_UNLIKELY(a_u >= FPBits::EXP_MASK)) {
    // x or y is inf or nan
    if (a_bits.is_signaling_nan() || b_bits.is_signaling_nan()) {
      fputil::raise_except_if_required(FE_INVALID);
      return FPBits::quiet_nan().get_val();
    }
    if (a_bits.is_inf() || b_bits.is_inf())
      return FPBits::inf().get_val();
    return a_bits.get_val();
  }

  constexpr uint16_t FRACTION_LEN = FPBits::FRACTION_LEN;
  if (LIBC_UNLIKELY(a_u - b_u >= (FRACTION_LEN + 2) << FRACTION_LEN))
    return x_abs.get_val() + y_abs.get_val();
  
  float af = static_cast<float>(a_bits.get_val());
  float bf = static_cast<float>(b_bits.get_val());

  float a_sq = af * af;
#ifdef LIBC_TARGET_CPU_HAS_FMA_FLOAT
  float sum_sq = fputil::multiply_add(bf, bf, a_sq);
#else
  float b_sq = bf * bf;
  float sum_sq = a_sq + b_sq;
#endif

  FloatBits result(fputil::sqrt<float>(sum_sq));
  uint32_t r_u = result.uintval();

  if (LIBC_UNLIKELY(((r_u + 1) & 0x00000FFFE) == 0)) {
    float r_f = result.get_val();
  
#ifdef LIBC_TARGET_CPU_HAS_FMA_FLOAT
  float sum_sq_lo = fputil::multiply_add(bf, bf, a_sq - sum_sq);
  float err = sum_sq_lo - fputil::multiply_add(r_f, r_f, -sum_sq);
#else
  fputil::FloatFloat r_sq = fputil::exact_mult(r_f, r_f);
  float sum_sq_lo = b_sq - (sum_sq - a_sq);
  float err = (sum_sq - r_sq.hi) + (sum_sq_lo - r_sq.lo);
#endif

  if (err > 0) {
      r_u |= 1;
    } else if ((err < 0) && (r_u & 1) == 0) {
      r_u -= 1;
    } else if ((r_u & 0x00001FFFF) == 0) {
      // The rounded result is exact.
      fputil::clear_except_if_required(FE_INEXACT);
    }
    return fputil::cast<float16>(FloatBits(r_u).get_val());
  }

  return fputil::cast<float16>(result.get_val());
}

} // namespace LIBC_NAMESPACE_DECL
