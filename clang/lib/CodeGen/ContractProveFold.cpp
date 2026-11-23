//===--- ContractProveFold.cpp - P4405 proving by folding -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The folding strategy, -fcontracts-prove-fold: a predicate that folds to
// true is proven, whether or not it is a constant expression, and one that
// folds to false is always false.  Folding is the constant evaluator's
// folding mode, not manifestly constant-evaluated (so is_constant_evaluated()
// is false, as at run time).  It assumes nothing but the language's rules,
// so its proofs are always sound.
//
// A caller-side evaluation of a precondition is folded as if in a call to
// the callee, with each by-value parameter of scalar type whose argument
// folds to a constant without side effects (or to the address of a static
// object) holding that value, and `this` designating the object when that
// is a static one.  Every other parameter is an unknown value, which keeps
// the proof sound.  The arguments are folded, never evaluated: the call
// evaluates them, once.
//
//===----------------------------------------------------------------------===//

#include "ContractProve.h"
#include "clang/AST/ContractFold.h"

using namespace clang;
using namespace CodeGen;

static bool isFoldEnabled(const LangOptions &LangOpts) {
  return LangOpts.ContractsProveFold;
}

static ContractProofResult proveByFolding(const ContractProofQuery &Q) {
  // The frame and the folding itself are the AST's (ContractFold.h), which
  // Sema's check where a function's address is taken shares.
  ContractFoldFrame Frame(Q.Ctx, Q.Predicate, Q.CallSite, Q.Callee,
                          /*Outer=*/nullptr);
  switch (foldContractPredicate(Q.Ctx, Q.Predicate, Frame)) {
  case ContractFoldResult::True:
    return ContractProofResult::Sound;
  case ContractFoldResult::False:
    return ContractProofResult::AlwaysFalse;
  case ContractFoldResult::Unknown:
    break;
  }
  return ContractProofResult::Unproven;
}

const ContractProofStrategy CodeGen::ContractProveFoldStrategy = {
    "fold", /*MayDependOnAssumptions=*/false, isFoldEnabled, proveByFolding};
