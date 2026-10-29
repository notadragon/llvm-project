//===--- ContractProve.cpp - P4405 proving strategies ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The registry of P4405 proving strategies and the dispatcher that asks them
// (see ContractProve.h).
//
//===----------------------------------------------------------------------===//

#include "ContractProve.h"
#include "clang/Basic/LangOptions.h"
#include <cassert>

using namespace clang;
using namespace CodeGen;

/// Every strategy, in the order they are asked; null-terminated.
static const ContractProofStrategy *const ContractProofStrategies[] = {
    nullptr,
};

ContractProofResult CodeGen::proveContract(const ContractProofQuery &Q,
                                           const LangOptions &LangOpts) {
  bool AnyConditional = false, AnyDeferred = false;
  for (const ContractProofStrategy *const *I = ContractProofStrategies; *I;
       ++I) {
    const ContractProofStrategy *S = *I;
    if (!S->IsEnabled(LangOpts))
      continue;
    ContractProofResult R = S->Prove(Q);
    assert((R != ContractProofResult::Conditional ||
            S->MayDependOnAssumptions) &&
           "a conditional proof from a strategy that relies on nothing");
    assert((R != ContractProofResult::Deferred || S->PlaceDeferred) &&
           "a deferral from a strategy that cannot place it");
    switch (R) {
    case ContractProofResult::Sound:
    case ContractProofResult::AlwaysFalse:
      return R;
    case ContractProofResult::Conditional:
      AnyConditional = true;
      break;
    case ContractProofResult::Deferred:
      AnyDeferred = true;
      break;
    case ContractProofResult::Unproven:
      break;
    }
  }
  if (AnyConditional)
    return ContractProofResult::Conditional;
  return AnyDeferred ? ContractProofResult::Deferred
                     : ContractProofResult::Unproven;
}

bool CodeGen::placeDeferredContractProof(const ContractProofQuery &Q,
                                         const LangOptions &LangOpts,
                                         bool Defer) {
  for (const ContractProofStrategy *const *I = ContractProofStrategies; *I;
       ++I) {
    const ContractProofStrategy *S = *I;
    if (S->PlaceDeferred && S->IsEnabled(LangOpts) &&
        S->PlaceDeferred(Q, Defer))
      return true;
  }
  return false;
}

bool CodeGen::takeContractDiagnostic(const ContractProofQuery &Q,
                                     ContractProofResult R,
                                     const LangOptions &LangOpts) {
  for (const ContractProofStrategy *const *I = ContractProofStrategies; *I;
       ++I) {
    const ContractProofStrategy *S = *I;
    if (S->TakeDiagnostic && S->IsEnabled(LangOpts) && S->TakeDiagnostic(Q, R))
      return true;
  }
  return false;
}
