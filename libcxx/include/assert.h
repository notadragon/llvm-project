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
#if __has_include_next(<assert.h>)
#  include_next <assert.h>
#endif

// The P3290 assert integration.  This header has no include guard, so that,
// as in the C library's, `assert` is redefined for the current NDEBUG each
// time it is included.
#include <__cassert/assert_contract.h>
