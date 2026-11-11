//===-- sanitizer_termination.cpp -------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// This file contains the Sanitizer termination functions CheckFailed and Die,
/// and the callback functionalities associated with them.
///
//===----------------------------------------------------------------------===//

#include "sanitizer_common.h"
#include "sanitizer_contract_routing.h"
#include "sanitizer_libc.h"

namespace __sanitizer {

static const int kMaxNumOfInternalDieCallbacks = 5;
static DieCallbackType InternalDieCallbacks[kMaxNumOfInternalDieCallbacks];

bool AddDieCallback(DieCallbackType callback) {
  for (int i = 0; i < kMaxNumOfInternalDieCallbacks; i++) {
    if (InternalDieCallbacks[i] == nullptr) {
      InternalDieCallbacks[i] = callback;
      return true;
    }
  }
  return false;
}

bool RemoveDieCallback(DieCallbackType callback) {
  for (int i = 0; i < kMaxNumOfInternalDieCallbacks; i++) {
    if (InternalDieCallbacks[i] == callback) {
      internal_memmove(&InternalDieCallbacks[i], &InternalDieCallbacks[i + 1],
                       sizeof(InternalDieCallbacks[0]) *
                           (kMaxNumOfInternalDieCallbacks - i - 1));
      InternalDieCallbacks[kMaxNumOfInternalDieCallbacks - 1] = nullptr;
      return true;
    }
  }
  return false;
}

static DieCallbackType UserDieCallback;
void SetUserDieCallback(DieCallbackType callback) {
  UserDieCallback = callback;
}

void NORETURN Die() {
  if (UserDieCallback)
    UserDieCallback();
  for (int i = kMaxNumOfInternalDieCallbacks - 1; i >= 0; i--) {
    if (InternalDieCallbacks[i])
      InternalDieCallbacks[i]();
  }
  if (common_flags()->abort_on_error)
    Abort();
  internal__exit(common_flags()->exitcode);
}

static void (*CheckUnwindCallback)();
void SetCheckUnwindCallback(void (*callback)()) {
  CheckUnwindCallback = callback;
}

void NORETURN CheckFailed(const char *file, int line, const char *cond,
                          u64 v1, u64 v2) {
  u32 tid = GetTid();
  Printf("%s: CHECK failed: %s:%d \"%s\" (0x%zx, 0x%zx) (tid=%u)\n",
         SanitizerToolName, StripModuleName(file), line, cond, (uptr)v1,
         (uptr)v2, tid);
  static atomic_uint32_t first_tid;
  u32 cmp = 0;
  if (!atomic_compare_exchange_strong(&first_tid, &cmp, tid,
                                      memory_order_relaxed)) {
    if (cmp == tid) {
      // Recursing into CheckFailed.
    } else {
      // Another thread fails already, let it print the stack and terminate.
      SleepForSeconds(2);
    }
    Trap();
  }
  if (CheckUnwindCallback)
    CheckUnwindCallback();
  Die();
}

} // namespace __sanitizer

using namespace __sanitizer;

// P3100: runtime guardrail for the shared death-callback setter.
//
// __sanitizer_set_death_callback lives in sanitizer_common (shared by every
// sanitizer), and each routing runtime conveys its state through a descriptor
// of its own: ASan, TSan and MSan a scalar (__asan_contract_semantic,
// __tsan_contract_semantic, __msan_contract_semantic; wire 1 = observe,
// 2 = enforce; absent / 0 = stock), UBSan a per-check table only its runtime
// can size, read through __ubsan_contract_routing_active.  Every reference is
// weak and introduces no dependency on the runtime it names: when a runtime is
// not linked -- or routing is off, whether via the
// -fsanitize-noncontract-callbacks opt-out or a non-p3100 program -- its
// descriptor is absent (or 0) and this guard is a no-op, so stock behavior is
// byte-for-byte unchanged.  When routing IS active, installing a death callback
// would let the program interpose on the terminate path that contract
// enforcement drives, so we refuse with a fatal error naming the opt-out.
extern "C" SANITIZER_WEAK_ATTRIBUTE unsigned char __asan_contract_semantic;
extern "C" SANITIZER_WEAK_ATTRIBUTE unsigned char __tsan_contract_semantic;
extern "C" SANITIZER_WEAK_ATTRIBUTE unsigned char __msan_contract_semantic;
extern "C" SANITIZER_WEAK_ATTRIBUTE bool __ubsan_contract_routing_active();

static bool ContractRoutingActive() {
  auto Routed = [](unsigned char route) {
    return route == kContractRouteObserve || route == kContractRouteEnforce;
  };
  if (&__asan_contract_semantic != nullptr &&
      Routed(__asan_contract_semantic))
    return true;
  if (&__tsan_contract_semantic != nullptr &&
      Routed(__tsan_contract_semantic))
    return true;
  if (&__msan_contract_semantic != nullptr &&
      Routed(__msan_contract_semantic))
    return true;
  return &__ubsan_contract_routing_active != nullptr &&
         __ubsan_contract_routing_active();
}

extern "C" {
SANITIZER_INTERFACE_ATTRIBUTE
void __sanitizer_set_death_callback(void (*callback)(void)) {
  if (ContractRoutingActive()) {
    Report(
        "ERROR: stock death callbacks are disabled under contract routing "
        "(-fcontracts-p3100); rebuild with -fsanitize-noncontract-callbacks "
        "to use them\n");
    Die();
  }
  SetUserDieCallback(callback);
}
}  // extern "C"
