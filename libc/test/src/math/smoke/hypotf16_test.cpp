//===-- Unittests for hypotf16 --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "HypotTest.h"

#include "src/math/hypotf16.h"

using LlvmLibcHypotf16Test = HypotTestTemplate<float16>;

TEST_F(LlvmLibcHypotf16Test, SpecialNumbers) {
  test_special_numbers(&LIBC_NAMESPACE::hypotf16);
}

#ifdef LIBC_TEST_FTZ_DAZ

using namespace LIBC_NAMESPACE::testing;

TEST_F(LlvmLibcHypotf16Test, FTZMode) {
  ModifyMXCSR mxcsr(FTZ);

  EXPECT_FP_EQ(0.0f, LIBC_NAMESPACE::hypotf16(min_denormal, min_denormal));
  EXPECT_FP_EQ(0.0f, LIBC_NAMESPACE::hypotf16(min_denormal, max_denormal));
  EXPECT_FP_EQ(0.0f, LIBC_NAMESPACE::hypotf16(max_denormal, min_denormal));
  EXPECT_FP_EQ(0x1.0p-14f,
               LIBC_NAMESPACE::hypotf16(max_denormal, max_denormal));
}

TEST_F(LlvmLibcHypotf16Test, DAZMode) {
  ModifyMXCSR mxcsr(DAZ);

  EXPECT_FP_EQ(0.0f, LIBC_NAMESPACE::hypotf16(min_denormal, min_denormal));
  EXPECT_FP_EQ(0.0f, LIBC_NAMESPACE::hypotf16(min_denormal, max_denormal));
  EXPECT_FP_EQ(0.0f, LIBC_NAMESPACE::hypotf16(max_denormal, min_denormal));
  EXPECT_FP_EQ(0.0f, LIBC_NAMESPACE::hypotf16(max_denormal, max_denormal));
}

TEST_F(LlvmLibcHypotf16Test, FTZDAZMode) {
  ModifyMXCSR mxcsr(FTZ | DAZ);

  EXPECT_FP_EQ(0.0f, LIBC_NAMESPACE::hypotf16(min_denormal, min_denormal));
  EXPECT_FP_EQ(0.0f, LIBC_NAMESPACE::hypotf16(min_denormal, max_denormal));
  EXPECT_FP_EQ(0.0f, LIBC_NAMESPACE::hypotf16(max_denormal, min_denormal));
  EXPECT_FP_EQ(0.0f, LIBC_NAMESPACE::hypotf16(max_denormal, max_denormal));
}

#endif