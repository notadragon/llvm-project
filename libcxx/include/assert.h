// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

// Include the C library's <assert.h> first so its definition of `assert` is in
// place before the P3290 integration (below) redefines it.
#include_next <assert.h>

// P3290 assert integration and its feature-test macro, shared with <cassert>.
// This header has no include guard so `assert` is redefined per current NDEBUG.
#include <__cassert/assert_contract.h>
