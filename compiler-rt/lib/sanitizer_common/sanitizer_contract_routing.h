//===-- sanitizer_contract_routing.h ----------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The vocabulary shared by every sanitizer that can route its reports into a
// C++ contract-violation handler: the wire values the compiler writes, the
// populator the handler calls back through, and the handler entry point.
//
// This exists because each of asan, ubsan, tsan and msan previously spelled all
// three for itself.  The enum was written out four times, the populator struct
// four times under four names with identical layout, and -- worst -- the one
// `extern "C"` entry point was declared with four different prototypes, one per
// sanitizer's own populator type.  Being extern "C" the names do not mangle, so
// nothing diagnosed it, and libasan links libubsan, which put two of those
// declarations in a single shared object.
//
// Keeping it in one header also means an upstream compiler-rt sync sees this
// change in one file it does not itself touch, rather than in four it does.
//
//===----------------------------------------------------------------------===//

#ifndef SANITIZER_CONTRACT_ROUTING_H
#define SANITIZER_CONTRACT_ROUTING_H

#include "sanitizer_internal_defs.h"

namespace __sanitizer {

// The value of a routing wire byte, as written by the compiler.  A wire byte is
// weakly defined: an absent symbol reads as zero, which is kContractRouteStock,
// so a program built without contract routing behaves exactly as before.
enum ContractRoute : unsigned char {
  kContractRouteStock = 0,    // routing off: stock sanitizer behavior
  kContractRouteObserve = 1,  // noexcept_observe: call handler, then continue
  kContractRouteEnforce = 2,  // noexcept_enforce: call handler, then terminate
  kContractRouteQuick = 3,    // quick_enforce: terminate WITHOUT the handler
};

// How the handler retrieves the sanitizer's diagnostic text, if it asks for it
// at all.  Lazy by design: rendering a report can allocate and can be
// expensive, and the common case is a handler that never calls report().
struct ContractReportPopulator {
  const char *(*populate)(const void *ctx);
  const void *ctx;
};

}  // namespace __sanitizer

// The contract-violation handler leg, provided by the C++ runtime (libc++ or
// libstdc++).  Weak: when the C++ contracts runtime is not linked the address
// is null, and every caller must test it before routing.
extern "C" SANITIZER_WEAK_ATTRIBUTE void __cxa_contract_violation_sanitizer(
    const char *comment, const char *file, unsigned line,
    unsigned char semantic,
    const __sanitizer::ContractReportPopulator *report);

#endif  // SANITIZER_CONTRACT_ROUTING_H
