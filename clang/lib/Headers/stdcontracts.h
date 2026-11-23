//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Convenience macros for C contracts (P4299)

#ifndef __CLANG_STDCONTRACTS_H
#define __CLANG_STDCONTRACTS_H

#include <contracts.h>

#ifndef __cplusplus
#define pre _Pre
#define post _Post
#define contract_assert _ContractAssert
#endif

#endif // __CLANG_STDCONTRACTS_H
