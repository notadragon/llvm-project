//===--- ContractProvePremises.cpp - D4405 callee premise copies ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// D4405R0 Section 4.5: the callee premise copies' proving strategy,
// -fcontracts-prove-callee-premises (implied by -fcontracts-p4405; notadragon_wg21
// DECISIONS.md E199 (b)): the code generation side.
//
// A proof shadow takes an observe, noexcept_observe or ignore evaluation as a
// premise (CodeGenFunction::emitContractShadowAssumption), and a proof that
// rests on one is conditional, so not diagnosed.  But it does so only for
// the evaluations it emits itself: the functions a shadow calls, and inlines
// in the proof module, are the real ones, whose code holds a check that
// continues past a violation (observe) or nothing (ignore), so the premises
// of a callee's postconditions and contract assertions are lost there.
//
// So each real function whose code holds such an evaluation (in a module that
// makes proof shadows; a function that can have one) gets a *premise copy*,
// "<name>.contract.premise": the function emitted once more, internal and
// never emitted, its code the real code but for each such evaluation, which
// is assumed as a shadow assumes it (an ignore one after the assertion's
// analysis evaluation too, which a shadow leaves to the obligation), and the
// obligations, of which it holds none (an analysis evaluation emits nothing
// there, as in the real code without -fcontracts-prove-assume).  Like the shadows, the copies are made
// only once code generation has emitted everything else, and only if some
// shadow is made.
//
// The back end (ContractShadowDecide.cpp) decides the shadows as before,
// with the real functions, so every result, every assumption and every
// inlining the real module sees is what it is without the copies.  Only
// then, if an obligation is left unproven (or always false) and its shadow
// uses a function that has a copy, directly or through what it uses, the
// shadows holding such obligations are decided once more in a second proof
// module, in which every use of such a function is a use of its copy; an
// obligation proven there is proven conditionally: never diagnosed, never an
// assumption.  Then the copies are deleted with the shadows.
//
//===----------------------------------------------------------------------===//

#include "CodeGenFunction.h"
#include "CodeGenModule.h"
#include "ContractProveShadow.h"
#include "clang/AST/StmtCXX.h"
#include "clang/Basic/Diagnostic.h"
#include "llvm/IR/Module.h"

using namespace clang;
using namespace CodeGen;
namespace cs = contract_shadow;

void CodeGenFunction::noteContractPremiseCopy(const ContractStmt &S,
                                              ContractEvaluationSemantic Sem,
                                              bool Dynamic) {
  using CES = ContractEvaluationSemantic;
  if (!getLangOpts().ContractsProveCalleePremises || Dynamic ||
      S.isInvalid() || ContractShadow || !CurFn)
    return;
  // What a shadow assumes (emitContractShadowAssumption) and the real code
  // does not guarantee; an evaluation that does not evaluate its predicate
  // is assumed unless the predicate has side effects of its own (a call is
  // not one: a proof resting on the copy is conditional in any case).
  switch (Sem) {
  case CES::Observe:
  case CES::NoexceptObserve:
    break;
  case CES::Ignore:
    if (S.getCond()->HasSideEffects(getContext(),
                                    /*IncludePossibleEffects=*/false))
      return;
    break;
  default:
    return;
  }
  if (!isContractShadowActive(CGM) || !isContractShadowable())
    return;
  ContractShadowModuleState &St = CGM.getContractShadowState();
  StringRef Name = CurFn->getName();
  if (St.PremiseCopyNames.insert(Name).second)
    St.PremiseCopies.emplace_back(CurGD, Name.str());
}

bool CodeGenFunction::isContractPremiseCopy() const {
  return ContractShadow && ContractShadow->PremiseCopy;
}

void CodeGenModule::emitContractPremiseCopy(GlobalDecl GD, StringRef Name) {
  llvm::Function *Real = getModule().getFunction(Name);
  if (!Real || Real->isDeclaration())
    return;
  const CGFunctionInfo &FI = getTypes().arrangeGlobalDeclaration(GD);
  llvm::Function *Copy = llvm::Function::Create(
      Real->getFunctionType(), llvm::GlobalValue::InternalLinkage,
      Real->getAddressSpace(), (Name + ".contract.premise").str(),
      &getModule());
  Copy->copyAttributesFrom(Real);
  Copy->setLinkage(llvm::GlobalValue::InternalLinkage);
  Copy->setVisibility(llvm::GlobalValue::DefaultVisibility);
  Copy->setDLLStorageClass(llvm::GlobalValue::DefaultStorageClass);
  Copy->setComdat(nullptr);

  ContractShadowFunction State;
  State.PremiseCopy = true;
  DiagnosticsEngine &Diags = getDiags();
  bool SavedSuppress = Diags.getSuppressAllDiagnostics();
  Diags.setSuppressAllDiagnostics(true);
  {
    CodeGenFunction CGF(*this);
    CGF.disableDebugInfo();
    CGF.ContractShadow = &State;
    CGF.GenerateCode(GD, Copy, FI);
  }
  Diags.setSuppressAllDiagnostics(SavedSuppress);
  Copy->addFnAttr(cs::PremiseCopyAttr);
}
