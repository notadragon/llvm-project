//===--- ContractShadowDecide.cpp - P4405 proof shadows, back end ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The proof shadow strategy, -fcontracts-prove-shadow: the back end side
// (see ContractProveShadow.h for the design; ContractProveShadow.cpp is the
// code generation side).  decideContractProofs runs from BackendUtil before
// the optimization pipeline.
//
// The proof module.  The shadows, and every function they use (directly or
// through the functions they use), are cloned into a module of their own;
// everything else there is a declaration, constant variables keep their
// initializers, other variables become external declarations (so that the
// optimizers assume nothing about who else writes them), and the
// static-initialization lists are dropped.  A real function there keeps no
// assumption (its __clang_contract_proven regions are deleted).  The shadows
// and the functions a predicate calls are kept alive (llvm.compiler.used),
// and the module is optimized with the default pipeline for the -O level,
// its diagnostics and remarks silenced.  The real module is untouched by
// this, so its optimization is the same as without shadows.
//
// The proof optimization level (D4405, -fcontracts-prove-optimize=,
// DECISIONS.md E162-E168).  The -O level BackendUtil passes is the proof
// level's, with tuning options for it, and what the code-generation -O level
// alone implies in the module is replaced in the proof module: the
// llvm.loop.unroll.disable of -O1 (ImpliedUnrollMD) and the optsize and
// minsize of -Os and -Oz (SizeMD), on every function there, the shadows and
// premise copies included (they copy their real functions' attributes), and
// on its calls.  What the user asked for -- -f[no-]unroll-loops, a loop
// pragma, a function's own cold or minsize -- is kept.
//
// An -O0 module proving at a proof level above 0 (O0MD): the proof module
// drops -O0's optnone and noinline but what the proof level gives (a
// function's own; -fno-inline...), adds inlinehint and optsize as the proof
// level would, and uses each available_externally function's body
// ("<name>.contract.external", ExternalAttr), as at -O1.  The real module
// loses those bodies with the shadows, its TBAA and !range/!noundef (made
// for the proofs only), and the allocas only a deleted assumption region
// used (RegionTempMD).  The proof module of a proof level of 0 is optimized
// as at -O0.
//
// The decision.  An obligation is unproven if a call to
// __clang_contract_unproven for it survives anywhere in the proof module
// (ids merged by the optimizers through a phi or select are followed), and
// "always false" if that call is in a shadow that is not conditional,
// reached from its entry through straight-line code, and the evaluation was
// not in a loop.  Otherwise it is proven: conditional if its shadow holds an
// assumption that is not guaranteed, holds any obligation unproven, or
// holds a predicate with a side effect of its own -- a store other than to a
// local, an indirect call, or a call of a function that, once optimized, is
// not known to only read memory, return and not throw (or, scanned, to do
// nothing else), or a call out of line of a replaceable function whose code
// holds an unsettled evaluation (another TU's copy may evaluate it with a
// violation handler's effects; DECISIONS.md E214) -- or assumes a predicate
// the real code does not evaluate (ignore, assume) that calls such a
// function or, even inlined, such a replaceable one (D4405, "Previous
// Assertions and Undefined Behavior"), and sound otherwise.  Copies of an obligation (in
// each variant of a constructor) are merged, the worst result winning.  An
// obligation no shadow holds is unproven.
//
// Premises decided by this TU (the spec's rule).  A terminating check whose
// allowed set admits a continuing semantic is a premise only where this
// TU's copy of its function runs: the functions holding one are listed by
// code generation (PremisesMD), and cloning keeps every linkage, so LLVM's
// refusal to reason from an inexact definition (linkonce_odr, weak_odr,
// available_externally, weak) holds in the proof module as in the real one.
// What the proof module's inliner inlined into what is recorded from its
// remarks.  A shadow is conditional if it, or a function with an exact
// definition it calls out of line (transitively), calls out of line a
// replaceable function whose code holds such a premise (or inlined one).  If
// its code relies on an inlined copy of a replaceable function whose code
// holds (or inlined) an unsettled evaluation (RefinableMD, a superset of the
// premises: another TU's copy may also differ in what it does, say an
// ignored assertion here whose handler stores elsewhere), a proof whose
// assumption the real code keeps stays sound by making each such call
// always_inline in the real module (checked to be possible, and at the end
// of the pipeline to have happened); otherwise it is conditional.
//
// Nested obligations (DECISIONS.md E169).  An obligation's nested ones are
// decided as any other, but a surviving failure of one is no premise for
// what follows (its region never rejoins the path), so it does not make the
// shadow's other proofs conditional.  Each obligation's result is then
// combined with those nested in it (combineNestedContractResults): that is
// the result diagnosed (with a note for each nested prove obligation not
// proven, which is not diagnosed on its own) and the one an assumption
// needs.  A nested diagnose obligation is diagnosed on its own, once even
// where the evaluation at the same call in the check of the same assertion
// in the same place gives the same diagnostic (E169 (9)), but once for each
// outer evaluation (E169 (11)): code generation records the place, the
// report-once group, with each obligation.
//
// The premise round (D4405, -fcontracts-prove-callee-premises;
// ContractProvePremises.cpp).  If code generation made premise copies, the
// shadows holding an obligation the first round left unproven or always
// false are decided once more, in a second proof module in which every use
// of a function that has a copy is a use of the copy (so an inlined callee's
// observe and ignore evaluations are premises too); one proven there is
// proven conditionally.  The first round is made as without copies, so the
// real module's assumptions and inlinings are the same with the option on or
// off.  Only the shadows that use a function with a copy, directly or
// through what they use, are decided again; with none, nothing is.
//
// The real module.  The shadows (and the premise copies) are deleted, each
// __clang_contract_proven region is kept (a sound proof: the predicate's
// calls are marked as only reading memory, so that nothing of it is left
// once the assumption is used) or deleted, and every global only they used
// is removed, so the output is what it would be without shadows.  Then every recorded
// evaluation not proven is diagnosed, in the order code generation met
// them, with a -Rcontract-proof remark for each the shadows decided.
//
//===----------------------------------------------------------------------===//

#include "ContractProveShadow.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Comdat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DiagnosticHandler.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/OptimizationLevel.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/VirtualFileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include <algorithm>
#include <list>
#include <set>
#include <string>
#include <tuple>

using namespace clang;
using namespace CodeGen;
using namespace llvm;
namespace cs = contract_shadow;

/// Testing aid: with it off, a sound proof that relies on the shadow's
/// inlining of a replaceable callee is conditional instead of making the
/// real function inline that callee too.
static cl::opt<bool> ContractProofForceInline(
    "contract-proof-force-inline", cl::init(true), cl::Hidden,
    cl::desc("P4405: make the real function inline a replaceable callee "
             "whose inlining into the proof shadow a sound proof relies on"));

namespace {
/// An obligation as code generation recorded it.
struct ObligationRecord {
  SourceLocation Loc;
  SourceRange Range, PredRange;
  unsigned Flags = 0;
  unsigned Decision = cs::Deferred;
  unsigned DiagContext = 0;
  /// D4405: for an override pair's obligation, B::f's name.
  std::string OverrideName;
  /// DECISIONS.md E169: the obligation it is nested in, or -1.
  int Parent = -1;
  /// DECISIONS.md E169 (9), (11): its report-once group (ContractObligation).
  unsigned Group = 0;
};

/// What the back end concluded, in increasing order of severity: copies of
/// an obligation merge to the most severe.
enum class Outcome : uint8_t {
  Pending,
  Sound,
  Conditional,
  Unproven,
  AlwaysFalse
};

/// What is known of a shadow before it is optimized.
struct ShadowFacts {
  Function *Fn = nullptr;
  /// The obligations it holds.
  SmallVector<unsigned, 4> Obligations;
  /// It holds an assumption that is not guaranteed, or a predicate with a
  /// side effect of its own.
  bool Conditional = false;
  /// The functions its predicates call, judged once it is optimized (one
  /// still called out of line that is replaceable and whose code holds an
  /// unsettled evaluation never passes).  Like the shadow, kept alive
  /// (llvm.compiler.used), so not deleted.
  SmallVector<Function *, 4> Callees;
  /// D4405 ("Previous Assertions and Undefined Behavior"): the functions
  /// called by the predicates it assumes that the real code does not evaluate
  /// (cs::PremiseTag), judged as Callees are -- and a replaceable one whose
  /// code holds an unsettled evaluation never passes, another TU's copy
  /// perhaps evaluating it with a violation handler's effects.  Kept alive
  /// too.
  SmallVector<Function *, 4> PremiseCallees;
  /// Once optimized: it relies on a premise this TU does not decide, through
  /// a call out of line (the replaceable-callee check).
  bool Undecided = false;
  /// Once optimized: the inlinings (caller, callee, by name in the proof
  /// module) of replaceable functions holding unsettled evaluations that its
  /// code relies on, which the real code must make too.
  SmallVector<std::pair<std::string, std::string>, 2> Inlinings;
  /// D4405: an override shadow, and the ordinary obligations it takes as
  /// premises (ReliedMD): its proofs are sound only if they all are.
  bool Override = false;
  SmallVector<unsigned, 4> Relied;
};

/// What the proof module knows of a function before it is optimized.
struct FunctionFacts {
  /// Another TU's definition may be the one the program uses: the
  /// definition is not exact (linkonce_odr, weak_odr, available_externally,
  /// weak, linkonce) or there is none here.
  bool Replaceable = true;
  /// Its code holds a terminating check whose premise only this TU's copy
  /// decides (the premise list).
  bool Premise = false;
  /// Its code holds an unsettled evaluation (the refinable list).
  bool Refinable = false;
};

/// Records which functions the proof module's inliner inlines into which
/// (its "inline" remarks), and swallows every other diagnostic and remark of
/// the proof module's optimization.  Only passed remarks of the inliner are
/// enabled, so the inliner computes nothing it would not compute otherwise.
struct InlineRecorder : DiagnosticHandler {
  /// The names of the functions by their remark spelling (no \1 prefix).
  const StringMap<std::string> &Names;
  /// Caller name -> callee names.
  StringMap<StringSet<>> &Inlined;
  InlineRecorder(const StringMap<std::string> &Names,
                 StringMap<StringSet<>> &Inlined)
      : Names(Names), Inlined(Inlined) {}
  bool isPassedOptRemarkEnabled(StringRef PassName) const override {
    return PassName == "inline";
  }
  bool isMissedOptRemarkEnabled(StringRef) const override { return false; }
  bool isAnalysisRemarkEnabled(StringRef) const override { return false; }
  bool isAnyRemarkEnabled() const override { return true; }
  bool handleDiagnostics(const DiagnosticInfo &DI) override {
    const auto *R = dyn_cast<OptimizationRemark>(&DI);
    if (!R || StringRef(R->getPassName()) != "inline")
      return true;
    for (const DiagnosticInfoOptimizationBase::Argument &A : R->getArgs())
      if (A.Key == "Callee") {
        auto It = Names.find(A.Val);
        Inlined[R->getFunction().getName()].insert(
            It != Names.end() ? StringRef(It->second) : StringRef(A.Val));
      }
    return true;
  }
};
} // namespace

bool CodeGen::moduleHasContractProofs(const llvm::Module &M) {
  return M.getNamedMetadata(cs::ObligationsMD) != nullptr ||
         M.getNamedMetadata(cs::O0MD) != nullptr ||
         M.getNamedMetadata(cs::PremisesMD) != nullptr ||
         M.getNamedMetadata(cs::RefinableMD) != nullptr ||
         M.getNamedMetadata(cs::ImpliedUnrollMD) != nullptr ||
         M.getNamedMetadata(cs::SizeMD) != nullptr;
}

//===----------------------------------------------------------------------===//
// Helpers
//===----------------------------------------------------------------------===//

static SourceLocation toLoc(const MDNode *N, unsigned I) {
  return SourceLocation::getFromRawEncoding(static_cast<SourceLocation::UIntTy>(
      mdconst::extract<ConstantInt>(N->getOperand(I))->getZExtValue()));
}

static unsigned toUnsigned(const MDNode *N, unsigned I) {
  return mdconst::extract<ConstantInt>(N->getOperand(I))->getZExtValue();
}

/// The obligation \p I's predicate tag on \p Inst names, or -1.
static int getPredicateTag(const Instruction &Inst) {
  const MDNode *N = Inst.getMetadata(cs::PredicateMD);
  if (!N)
    return -1;
  return toUnsigned(N, 0);
}

/// Add the globals \p V refers to, through constants, to \p Out.
static void collectGlobals(const Value *V, SmallPtrSetImpl<GlobalValue *> &Out,
                           SmallPtrSetImpl<const Constant *> &Seen) {
  if (const auto *GV = dyn_cast<GlobalValue>(V)) {
    Out.insert(const_cast<GlobalValue *>(GV));
    return;
  }
  const auto *C = dyn_cast<Constant>(V);
  if (!C || !Seen.insert(C).second)
    return;
  for (const Use &Op : C->operands())
    collectGlobals(Op.get(), Out, Seen);
}

static void collectReferences(const BasicBlock &BB,
                              SmallPtrSetImpl<GlobalValue *> &Out,
                              SmallPtrSetImpl<const Constant *> &Seen) {
  for (const Instruction &I : BB)
    for (const Use &Op : I.operands())
      collectGlobals(Op.get(), Out, Seen);
}

static void collectReferences(const GlobalValue &G,
                              SmallPtrSetImpl<GlobalValue *> &Out,
                              SmallPtrSetImpl<const Constant *> &Seen) {
  if (const auto *F = dyn_cast<Function>(&G)) {
    for (const BasicBlock &BB : *F)
      collectReferences(BB, Out, Seen);
    if (F->hasPersonalityFn())
      collectGlobals(F->getPersonalityFn(), Out, Seen);
  } else if (const auto *V = dyn_cast<GlobalVariable>(&G)) {
    if (V->hasInitializer())
      collectGlobals(V->getInitializer(), Out, Seen);
  } else if (const auto *A = dyn_cast<GlobalAlias>(&G)) {
    collectGlobals(A->getAliasee(), Out, Seen);
  }
}

/// Erase each of \p Candidates (and, in turn, what only it used) that is no
/// longer used and that the module need not keep: a declaration, or a
/// definition that may be discarded if unused (internal, private,
/// linkonce, available_externally) whose comdat holds nothing still used.
static void
eraseUnusedGlobals(llvm::Module &M,
                   const SmallPtrSetImpl<GlobalValue *> &Candidates) {
  DenseMap<const Comdat *, SmallVector<GlobalValue *, 2>> ComdatMembers;
  for (GlobalObject &GO : M.global_objects())
    if (const Comdat *C = GO.getComdat())
      ComdatMembers[C].push_back(&GO);

  SmallPtrSet<GlobalValue *, 32> Erased;
  auto IsDead = [&](GlobalValue *G) {
    if (Erased.count(G))
      return false;
    G->removeDeadConstantUsers();
    if (!G->use_empty())
      return false;
    return G->isDeclaration() || G->isDiscardableIfUnused();
  };

  SmallVector<GlobalValue *, 32> Work(Candidates.begin(), Candidates.end());
  bool Changed = true;
  while (Changed) {
    Changed = false;
    for (size_t I = 0; I != Work.size(); ++I) {
      GlobalValue *G = Work[I];
      if (!IsDead(G))
        continue;
      SmallVector<GlobalValue *, 2> Group{G};
      Comdat *C = nullptr;
      if (auto *GO = dyn_cast<GlobalObject>(G); GO && !G->isDeclaration())
        C = GO->getComdat();
      if (C) {
        Group = ComdatMembers[C];
        if (!llvm::all_of(Group,
                          [&](GlobalValue *X) { return X == G || IsDead(X); }))
          continue;
      }
      SmallPtrSet<GlobalValue *, 16> Refs;
      SmallPtrSet<const Constant *, 16> Seen;
      for (GlobalValue *X : Group)
        collectReferences(*X, Refs, Seen);
      for (GlobalValue *X : Group) {
        if (auto *F = dyn_cast<Function>(X))
          F->dropAllReferences();
        else if (auto *V = dyn_cast<GlobalVariable>(X))
          V->dropAllReferences();
        else
          cast<User>(X)->dropAllReferences();
      }
      for (GlobalValue *X : Group) {
        Erased.insert(X);
        X->eraseFromParent();
      }
      if (C && C->getUsers().empty())
        M.getComdatSymbolTable().erase(C->getName());
      for (GlobalValue *R : Refs)
        if (!Erased.count(R))
          Work.push_back(R);
      Changed = true;
    }
  }
}

static void reachableBlocks(Function &F, SmallPtrSetImpl<BasicBlock *> &Out) {
  SmallVector<BasicBlock *, 16> Work{&F.getEntryBlock()};
  while (!Work.empty()) {
    BasicBlock *BB = Work.pop_back_val();
    if (!Out.insert(BB).second)
      continue;
    for (BasicBlock *Succ : successors(BB))
      Work.push_back(Succ);
  }
}

/// Replace each call of __clang_contract_proven in \p M by whether \p Keep
/// keeps that obligation's assumption, and delete the regions that leaves
/// unreachable, adding what they referred to to \p Refs.  For a kept one,
/// call \p OnKept with the function and the obligation.
static void
foldPlaceholders(llvm::Module &M, function_ref<bool(unsigned)> Keep,
                 SmallPtrSetImpl<GlobalValue *> &Refs,
                 function_ref<void(Function &, unsigned)> OnKept = nullptr) {
  Function *Proven = M.getFunction(cs::ProvenFn);
  if (!Proven)
    return;
  MapVector<Function *, SmallVector<CallInst *, 4>> ByFunction;
  for (User *U : Proven->users())
    if (auto *CI = dyn_cast<CallInst>(U))
      ByFunction[CI->getFunction()].push_back(CI);
  SmallPtrSet<const Constant *, 16> Seen;
  for (auto &[F, Calls] : ByFunction) {
    SmallPtrSet<BasicBlock *, 32> Before, After;
    reachableBlocks(*F, Before);
    SmallVector<BasicBlock *, 4> Dropped;
    for (CallInst *CI : Calls) {
      unsigned Id = cast<ConstantInt>(CI->getArgOperand(0))->getZExtValue();
      bool Kept = Keep(Id);
      if (Kept && OnKept)
        OnKept(*F, Id);
      CI->replaceAllUsesWith(ConstantInt::getBool(M.getContext(), Kept));
      BasicBlock *BB = CI->getParent();
      CI->eraseFromParent();
      ConstantFoldTerminator(BB);
      if (!Kept)
        Dropped.push_back(BB);
    }
    reachableBlocks(*F, After);
    SmallVector<BasicBlock *, 8> Dead;
    SmallPtrSet<BasicBlock *, 8> DeadSet;
    for (BasicBlock &BB : *F)
      if (Before.count(&BB) && !After.count(&BB)) {
        Dead.push_back(&BB);
        DeadSet.insert(&BB);
        collectReferences(BB, Refs, Seen);
      }
    DeleteDeadBlocks(Dead);
    // Where a region was deleted, rejoin the code around it, as without it --
    // unless it was itself inside a region deleted (and so is gone).
    for (BasicBlock *BB : Dropped)
      if (!DeadSet.count(BB))
        if (BasicBlock *Succ = BB->getSingleSuccessor();
            Succ && Succ->getSinglePredecessor() == BB)
          MergeBlockIntoPredecessor(Succ);
  }
}

/// Whether \p CB has no effect but reading memory, returns, and does not
/// throw: by its attributes, or as an intrinsic that only touches a local.
static bool isHarmlessCall(const CallBase &CB) {
  if (CB.onlyReadsMemory() && CB.willReturn() && CB.doesNotThrow())
    return true;
  if (const auto *II = dyn_cast<IntrinsicInst>(&CB)) {
    if (II->isAssumeLikeIntrinsic())
      return true;
    if (const auto *MI = dyn_cast<MemIntrinsic>(II))
      return !MI->isVolatile() &&
             isa<AllocaInst>(getUnderlyingObject(MI->getDest()));
  }
  return false;
}

/// Whether \p I is a plain store to a local.
static bool isLocalStore(const Instruction &I) {
  const auto *SI = dyn_cast<StoreInst>(&I);
  return SI && SI->isSimple() &&
         isa<AllocaInst>(getUnderlyingObject(SI->getPointerOperand()));
}

/// Whether calling \p F, once optimized, can have no effect but reading
/// memory, returns and does not throw: by its attributes, or by scanning its
/// body (an inline function's attributes are not inferred, its definition
/// not being exact).  A function its body calls out of line for which
/// \p Unsettled holds (a replaceable one whose code holds an unsettled
/// evaluation, which another TU's copy may evaluate with a violation
/// handler's effects) never passes.  \p Memo also breaks recursion,
/// conservatively.
static bool isSideEffectFree(Function *F, DenseMap<Function *, bool> &Memo,
                             function_ref<bool(const Function *)> Unsettled,
                             unsigned Depth = 0) {
  if (F->onlyReadsMemory() && F->willReturn() && F->doesNotThrow())
    return true;
  if (F->isDeclaration() || F->isInterposable() || Depth > 8)
    return false;
  auto [It, Inserted] = Memo.try_emplace(F, false);
  if (!Inserted)
    return It->second;
  if (!F->mustProgress()) {
    SmallVector<std::pair<const BasicBlock *, const BasicBlock *>, 4> Backedges;
    FindFunctionBackedges(*F, Backedges);
    if (!Backedges.empty())
      return false;
  }
  for (Instruction &I : instructions(*F)) {
    if (auto *CB = dyn_cast<CallBase>(&I)) {
      if (isHarmlessCall(*CB))
        continue;
      Function *Callee = CB->getCalledFunction();
      if (!Callee || Callee->isIntrinsic() || Unsettled(Callee) ||
          !isSideEffectFree(Callee, Memo, Unsettled, Depth + 1))
        return false;
      continue;
    }
    if (I.mayThrow() || (I.mayWriteToMemory() && !isLocalStore(I)))
      return false;
  }
  Memo[F] = true;
  return true;
}

/// Add the obligation ids \p V may be to \p Ids; false if one is not a
/// constant.
static bool collectMarkerIds(const Value *V, SmallVectorImpl<unsigned> &Ids,
                             SmallPtrSetImpl<const Value *> &Seen) {
  if (!Seen.insert(V).second || isa<UndefValue>(V))
    return true;
  if (const auto *C = dyn_cast<ConstantInt>(V)) {
    Ids.push_back(C->getZExtValue());
    return true;
  }
  if (const auto *P = dyn_cast<PHINode>(V)) {
    for (const Value *In : P->incoming_values())
      if (!collectMarkerIds(In, Ids, Seen))
        return false;
    return true;
  }
  if (const auto *S = dyn_cast<SelectInst>(V))
    return collectMarkerIds(S->getTrueValue(), Ids, Seen) &&
           collectMarkerIds(S->getFalseValue(), Ids, Seen);
  return false;
}

/// Whether \p C is reached whenever its function is entered: through
/// straight-line code from the entry, every instruction before it bound to
/// pass execution on.
static bool isReachedUnconditionally(const CallBase &C) {
  const BasicBlock *BB = &C.getFunction()->getEntryBlock();
  SmallPtrSet<const BasicBlock *, 8> Seen;
  while (BB && Seen.insert(BB).second) {
    for (const Instruction &I : *BB) {
      if (&I == &C)
        return true;
      if (I.isTerminator())
        break;
      if (!isGuaranteedToTransferExecutionToSuccessor(&I))
        return false;
    }
    BB = BB->getSingleSuccessor();
  }
  return false;
}

/// D4405: what the proof module replaces of the -O level's settings
/// (decideContractProofs takes it from the module's lists).
struct ProofNormalization {
  /// The loop ids whose llvm.loop.unroll.disable the -O level alone implies.
  SmallPtrSet<const MDNode *, 16> ImpliedUnroll;
  /// The -O level implies optsize (and minsize at -Oz).
  bool Size = false;
  /// The functions whose own attribute asks for optsize (first) or minsize
  /// (second), by name.
  StringMap<std::pair<bool, bool>> UserSize;
  /// An -O0 module (O0MD): what the proof level does to each function's
  /// attributes that -O0 does not (O0OptNone...), by name.
  StringMap<unsigned> O0;
};

/// The name of the real function \p Name stands for in the proof module: a
/// shadow's "<name>.contract.proof" or "<name>.contract.override.<pair>", a
/// premise copy's "<name>.contract.premise", an -O0 module's
/// available_externally body "<name>.contract.external", or \p Name itself.
static StringRef getRealName(StringRef Name) {
  if (Name.consume_back(".contract.proof") ||
      Name.consume_back(".contract.premise") ||
      Name.consume_back(".contract.external"))
    return Name;
  size_t Pos = Name.rfind(".contract.override.");
  return Pos == StringRef::npos ? Name : Name.take_front(Pos);
}

/// The loop id \p Loop without llvm.loop.unroll.disable, in it and in the
/// follow-up loop ids it names (which repeat its properties).
static MDNode *dropUnrollDisable(MDNode *Loop) {
  LLVMContext &Ctx = Loop->getContext();
  SmallVector<Metadata *, 8> Ops{nullptr};
  bool Changed = false;
  for (unsigned I = 1, E = Loop->getNumOperands(); I != E; ++I) {
    Metadata *Op = Loop->getOperand(I);
    if (auto *N = dyn_cast_or_null<MDNode>(Op); N && N->getNumOperands())
      if (auto *S = dyn_cast<MDString>(N->getOperand(0))) {
        if (S->getString() == "llvm.loop.unroll.disable") {
          Changed = true;
          continue;
        }
        if (S->getString().contains(".followup") && N->getNumOperands() == 2)
          if (auto *F = dyn_cast_or_null<MDNode>(N->getOperand(1));
              F && F->isDistinct() && F->getNumOperands() &&
              F->getOperand(0) == F)
            if (MDNode *NF = dropUnrollDisable(F); NF != F) {
              Metadata *Followup[] = {S, NF};
              Op = MDNode::get(Ctx, Followup);
              Changed = true;
            }
      }
    Ops.push_back(Op);
  }
  if (!Changed)
    return Loop;
  MDNode *New = MDNode::getDistinct(Ctx, Ops);
  New->replaceOperandWith(0, New);
  return New;
}

/// Replace in the proof module \p P, cloned with \p VMap, what the -O level
/// alone implies (\p Norm) by what the proof level does: no
/// llvm.loop.unroll.disable the proof level would not add (it unrolls if the
/// list is made), and no optsize or minsize but a function's own; in an -O0
/// module, no optnone or noinline but what the proof level gives, and its
/// inlinehint and optsize.
static void normalizeProofModule(llvm::Module &P, ValueToValueMapTy &VMap,
                                 const ProofNormalization &Norm) {
  SmallPtrSet<const MDNode *, 16> Implied;
  for (const MDNode *L : Norm.ImpliedUnroll)
    if (std::optional<Metadata *> PL = VMap.getMappedMD(L))
      if (auto *N = dyn_cast_or_null<MDNode>(*PL))
        Implied.insert(N);
  DenseMap<MDNode *, MDNode *> Dropped;
  for (Function &F : P) {
    if (unsigned Bits = Norm.O0.lookup(getRealName(F.getName()))) {
      if (Bits & cs::O0OptNone)
        F.removeFnAttr(Attribute::OptimizeNone);
      if (Bits & cs::O0NoInline)
        F.removeFnAttr(Attribute::NoInline);
      if (Bits & cs::O0InlineHint)
        F.addFnAttr(Attribute::InlineHint);
      if (Bits & cs::O0OptSize)
        F.addFnAttr(Attribute::OptimizeForSize);
    }
    if (Norm.Size) {
      std::pair<bool, bool> Own = Norm.UserSize.lookup(getRealName(F.getName()));
      if (!Own.first)
        F.removeFnAttr(Attribute::OptimizeForSize);
      if (!Own.second)
        F.removeFnAttr(Attribute::MinSize);
    }
    for (Instruction &I : instructions(F)) {
      if (auto *CB = dyn_cast<CallBase>(&I); CB && Norm.Size) {
        CB->removeFnAttr(Attribute::OptimizeForSize);
        CB->removeFnAttr(Attribute::MinSize);
      }
      if (Implied.empty())
        continue;
      if (MDNode *L = I.getMetadata(LLVMContext::MD_loop);
          L && Implied.count(L)) {
        MDNode *&New = Dropped[L];
        if (!New)
          New = dropUnrollDisable(L);
        I.setMetadata(LLVMContext::MD_loop, New);
      }
    }
  }
}

/// Optimize the proof module \p P with the default pipeline for \p Level --
/// without identical-function merging (-fmerge-functions).  It runs after
/// every simplification, so it proves nothing, but it would replace a
/// function the decision holds (a predicate's callee, kept alive by
/// llvm.compiler.used, becomes a new thunk) and retarget a shadow's calls,
/// changing what the decision reads from what it is without the option.
static void optimizeProofModule(llvm::Module &P, TargetMachine *TM,
                                PipelineTuningOptions PTO,
                                const TargetLibraryInfoImpl &TLII,
                                OptimizationLevel Level,
                                IntrusiveRefCntPtr<vfs::FileSystem> FS) {
  PTO.MergeFunctions = false;
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  PassBuilder PB(P.getContext(), TM, PTO, /*PGOOpt=*/std::nullopt,
                 /*PIC=*/nullptr, std::move(FS));
  FAM.registerPass([&] { return TargetLibraryAnalysis(TLII); });
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
  ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(Level);
  MPM.run(P, MAM);
}

//===----------------------------------------------------------------------===//
// The decision
//===----------------------------------------------------------------------===//

/// The real function a proof-module function named \p Name stands for: the
/// shadow's real function, or the function of that name.
static Function *getRealFunction(llvm::Module &M, StringRef Name) {
  if (!Name.consume_back(".contract.proof")) {
    // D4405: "<name>.contract.override.<pair>".
    size_t Pos = Name.rfind(".contract.override.");
    if (Pos != StringRef::npos)
      Name = Name.take_front(Pos);
  }
  return M.getFunction(Name);
}

/// The calls in \p Caller of \p Callee that the real code makes (not those of
/// an assumption's predicate).
static SmallVector<CallBase *, 2> getRealCalls(Function &Caller,
                                               Function &Callee) {
  SmallVector<CallBase *, 2> Calls;
  for (Instruction &I : instructions(Caller))
    if (auto *CB = dyn_cast<CallBase>(&I);
        CB && CB->getCalledFunction() == &Callee &&
        getPredicateTag(*CB) < 0)
      Calls.push_back(CB);
  return Calls;
}

/// Whether every call the real code makes in \p Caller of \p Callee (at
/// least one) can be made always_inline and would then be inlined: what
/// LLVM's inliners require of an always_inline call.
static bool canForceInline(Function &Caller, Function &Callee,
                           TargetMachine *TM,
                           const TargetLibraryInfoImpl &TLII) {
  if (Callee.isDeclaration())
    return false;
  SmallVector<CallBase *, 2> Calls = getRealCalls(Caller, Callee);
  if (Calls.empty())
    return false;
  TargetTransformInfo TTI = TM ? TM->getTargetTransformInfo(Callee)
                               : TargetTransformInfo(Callee.getDataLayout());
  std::list<TargetLibraryInfo> TLIs;
  auto GetTLI = [&](Function &F) -> const TargetLibraryInfo & {
    return TLIs.emplace_back(TLII, &F);
  };
  for (CallBase *CB : Calls) {
    bool Had = CB->getAttributes().hasFnAttr(Attribute::AlwaysInline);
    if (!Had)
      CB->addFnAttr(Attribute::AlwaysInline);
    std::optional<InlineResult> R =
        getAttributeBasedInliningDecision(*CB, &Callee, TTI, GetTLI);
    if (!Had)
      CB->removeFnAttr(Attribute::AlwaysInline);
    if (!R || !R->isSuccess())
      return false;
  }
  return true;
}

/// Decide the obligations the shadows \p Shadows of \p M hold, into
/// \p Results.  \p Premises: the real functions whose code holds a premise
/// only this TU's copy decides; \p Refinable: those whose code holds an
/// unsettled evaluation.  \p Regions: the obligations whose
/// assumption the real code may keep (\p AllowAssumptions).  \p Forced
/// receives the calls (caller, callee) the real code must inline because a
/// sound proof relies on the shadow's inlining of them.  \p Copies (D4405,
/// -fcontracts-prove-callee-premises): if not null, the premise copies of the
/// real functions, which then stand for them everywhere in the proof module
/// (the second round, decidePremiseRound); nothing is decided if the proof
/// module would hold none of them.
static void decideShadows(llvm::Module &M, ArrayRef<Function *> Shadows,
                          ArrayRef<ObligationRecord> Records,
                          MutableArrayRef<Outcome> Results, TargetMachine *TM,
                          const PipelineTuningOptions &PTO,
                          const TargetLibraryInfoImpl &TLII,
                          OptimizationLevel Level, bool Verify,
                          IntrusiveRefCntPtr<vfs::FileSystem> FS,
                          const ProofNormalization &Norm,
                          const SmallPtrSetImpl<Function *> &Premises,
                          const SmallPtrSetImpl<Function *> &Refinable,
                          const DenseSet<unsigned> &Regions,
                          bool AllowAssumptions,
                          SmallVectorImpl<std::pair<Function *, Function *>>
                              &Forced,
                          const DenseMap<const Function *, Function *> &External,
                          const DenseMap<const Function *, Function *> *Copies =
                              nullptr) {
  // What the proof module holds: the shadows and, transitively, the
  // functions they use, and the constant variables any of them reads.  In
  // the premise round, a function's copy instead of the function.
  SmallPtrSet<const GlobalValue *, 32> Keep;
  SmallVector<const GlobalValue *, 64> Work;
  bool AnyCopy = false;
  auto Consider = [&](const GlobalValue *G, bool FromCode) {
    const GlobalObject *Base = nullptr;
    if (const auto *A = dyn_cast<GlobalAlias>(G)) {
      Base = A->getAliaseeObject();
      if (!Base)
        return;
    }
    const GlobalValue *Obj = Base ? Base : G;
    if (const auto *F = dyn_cast<Function>(Obj)) {
      // D4405: in an -O0 module, an available_externally function's body.
      if (auto It = External.find(F); It != External.end()) {
        F = It->second;
        Obj = F;
      }
      if (!FromCode || F->isDeclaration())
        return;
      if (Copies)
        if (auto It = Copies->find(F); It != Copies->end()) {
          Obj = It->second;
          AnyCopy = true;
        }
    } else if (const auto *V = dyn_cast<GlobalVariable>(Obj)) {
      if (!V->isConstant() || !V->hasInitializer())
        return;
    } else {
      return;
    }
    if (Base)
      Keep.insert(G);
    if (Keep.insert(Obj).second)
      Work.push_back(Obj);
  };
  for (Function *S : Shadows) {
    Keep.insert(S);
    Work.push_back(S);
  }
  while (!Work.empty()) {
    const GlobalValue *G = Work.pop_back_val();
    SmallPtrSet<GlobalValue *, 16> Refs;
    SmallPtrSet<const Constant *, 16> Seen;
    collectReferences(*G, Refs, Seen);
    for (GlobalValue *R : Refs)
      Consider(R, isa<Function>(G));
  }
  if (Copies && !AnyCopy)
    return;

  ValueToValueMapTy VMap;
  std::unique_ptr<llvm::Module> P = CloneModule(
      M, VMap, [&](const GlobalValue *G) { return Keep.count(G) != 0; });
  // D4405: in an -O0 module, each available_externally function used is its
  // body, as at -O1.
  for (auto &[F, Ext] : External) {
    auto *PF = dyn_cast_or_null<Function>(VMap.lookup(F));
    auto *PE = dyn_cast_or_null<Function>(VMap.lookup(Ext));
    if (!PF || !PE || !PF->isDeclaration())
      continue;
    PF->replaceAllUsesWith(PE);
    std::string Name = PF->getName().str();
    PF->eraseFromParent();
    if (!PE->isDeclaration()) {
      PE->setName(Name);
      PE->removeFnAttr(cs::ExternalAttr);
    }
  }
  // The premise round: every use of a function that has a copy -- in a
  // shadow, a copy, or any function either uses, and in the constants they
  // read -- is a use of its copy.
  if (Copies)
    for (auto &[F, C] : *Copies) {
      Value *MF = VMap.lookup(F), *MC = VMap.lookup(C);
      auto *PF = dyn_cast_or_null<Function>(MF);
      auto *PC = dyn_cast_or_null<Function>(MC);
      if (!PF || !PC || PC->isDeclaration() || !PF->isDeclaration())
        continue;
      PF->replaceAllUsesWith(PC);
      PF->eraseFromParent();
    }
  for (StringRef Name : {"llvm.used", "llvm.compiler.used", "llvm.global_ctors",
                         "llvm.global_dtors"})
    if (GlobalVariable *GV = P->getGlobalVariable(Name, /*AllowInternal=*/true))
      GV->eraseFromParent();
  normalizeProofModule(*P, VMap, Norm);
  // The real functions used keep no assumption.
  SmallPtrSet<GlobalValue *, 8> Unused;
  foldPlaceholders(*P, [](unsigned) { return false; }, Unused);

  // What the rule needs to know of each function: whether its definition is
  // exact (cloning keeps every linkage, so LLVM's own refusal to reason from
  // an inexact definition applies here as in the real module), whether its
  // code holds a premise only this TU's copy decides, and whether it holds an
  // unsettled evaluation.
  StringMap<FunctionFacts> FnFacts;
  StringMap<std::string> RemarkNames;
  for (Function &F : M) {
    auto It = VMap.find(&F);
    if (It == VMap.end() || F.hasFnAttribute(cs::ExternalAttr))
      continue;
    auto *PF = dyn_cast<Function>(It->second);
    if (!PF)
      continue;
    FunctionFacts &FF = FnFacts[PF->getName()];
    FF.Replaceable = !PF->hasExactDefinition();
    FF.Premise = Premises.count(&F) != 0;
    FF.Refinable = FF.Premise || Refinable.count(&F) != 0;
  }
  for (Function &PF : *P)
    RemarkNames[GlobalValue::dropLLVMManglingEscape(PF.getName())] =
        PF.getName().str();

  // What each shadow holds, before it is optimized.
  SmallVector<ShadowFacts, 8> Facts;
  SmallVector<GlobalValue *, 16> Used;
  for (Function *S : Shadows) {
    auto *PS = cast<Function>(VMap[S]);
    ShadowFacts SF;
    SF.Fn = PS;
    SF.Conditional =
        PS->getFnAttribute(cs::ShadowAttr).getValueAsString() == "conditional";
    SF.Override = PS->hasFnAttribute(cs::OverrideAttr);
    if (const MDNode *R = S->getMetadata(cs::ReliedMD))
      for (const MDOperand &Op : R->operands())
        SF.Relied.push_back(mdconst::extract<ConstantInt>(Op)->getZExtValue());
    SmallPtrSet<Function *, 4> Callees, PremiseCallees;
    for (Instruction &I : instructions(*PS)) {
      if (auto *CB = dyn_cast<CallBase>(&I))
        if (Function *Callee = CB->getCalledFunction();
            Callee && Callee->getName() == cs::UnprovenFn) {
          unsigned Id = cast<ConstantInt>(CB->getArgOperand(0))->getZExtValue();
          if (!llvm::is_contained(SF.Obligations, Id))
            SF.Obligations.push_back(Id);
          continue;
        }
      int Tag = getPredicateTag(I);
      if (Tag < 0)
        continue;
      if (auto *CB = dyn_cast<CallBase>(&I)) {
        Function *Callee = CB->getCalledFunction();
        if (!Callee)
          SF.Conditional = true;
        else if (Callee->isIntrinsic())
          SF.Conditional |= !isHarmlessCall(*CB);
        else if (Tag == cs::PremiseTag) {
          if (PremiseCallees.insert(Callee).second)
            SF.PremiseCallees.push_back(Callee);
        } else if (Callees.insert(Callee).second)
          SF.Callees.push_back(Callee);
      } else if (I.mayWriteToMemory() && !isLocalStore(I)) {
        SF.Conditional = true;
      }
    }
    Used.push_back(PS);
    llvm::append_range(Used, SF.Callees);
    llvm::append_range(Used, SF.PremiseCallees);
    Facts.push_back(std::move(SF));
  }
  appendToCompilerUsed(*P, Used);

  if (Verify && verifyModule(*P, &errs()))
    report_fatal_error("broken P4405 proof module");

  StringMap<StringSet<>> Inlined;
  {
    LLVMContext &Ctx = M.getContext();
    std::unique_ptr<DiagnosticHandler> Saved = Ctx.getDiagnosticHandler();
    Ctx.setDiagnosticHandler(
        std::make_unique<InlineRecorder>(RemarkNames, Inlined));
    optimizeProofModule(*P, TM, PTO, TLII, Level, std::move(FS));
    Ctx.setDiagnosticHandler(std::move(Saved));
  }

  // The premises decided by this TU.  A function made during optimization
  // is unknown: replaceable, holding a premise and an unsettled evaluation
  // (conservatively).
  auto IsReplaceable = [&](StringRef Name) {
    auto It = FnFacts.find(Name);
    return It == FnFacts.end() || It->second.Replaceable;
  };
  // The functions whose code, with what the proof module inlined into it
  // (transitively), holds what Has says of a function's own code.
  auto Closure = [&](bool FunctionFacts::*Has) {
    StringSet<> Holds;
    for (auto &Entry : FnFacts)
      if (Entry.second.*Has)
        Holds.insert(Entry.first());
    for (auto &Edges : Inlined)
      for (auto &Callee : Edges.second)
        if (!FnFacts.count(Callee.getKey()))
          Holds.insert(Callee.getKey());
    for (bool Changed = true; Changed;) {
      Changed = false;
      for (auto &Edges : Inlined)
        if (!Holds.count(Edges.first()))
          for (auto &Callee : Edges.second)
            if (Holds.count(Callee.getKey())) {
              Holds.insert(Edges.first());
              Changed = true;
              break;
            }
    }
    return Holds;
  };
  StringSet<> HoldsPremise = Closure(&FunctionFacts::Premise);
  StringSet<> HoldsRefinable = Closure(&FunctionFacts::Refinable);
  for (ShadowFacts &SF : Facts) {
    // The code whose facts reach the shadow: its own, with what it inlined,
    // and that of each function it calls out of line whose definition is
    // exact (LLVM derives nothing from an inexact one), transitively.
    SmallVector<Function *, 8> Reach{SF.Fn};
    SmallPtrSet<Function *, 8> InReach{SF.Fn};
    std::set<std::pair<std::string, std::string>> Needed;
    for (size_t I = 0; I != Reach.size(); ++I) {
      Function *X = Reach[I];
      // Rule 1: an inlined copy of a replaceable function holding (or
      // having inlined) an unsettled evaluation -- a premise, or what it does
      // or does not do -- is this TU's code only where the real code inlines
      // it too.
      SmallVector<StringRef, 8> Work{X->getName()};
      StringSet<> Seen;
      Seen.insert(X->getName());
      while (!Work.empty()) {
        StringRef A = Work.pop_back_val();
        auto It = Inlined.find(A);
        if (It == Inlined.end())
          continue;
        for (auto &Entry : It->second) {
          StringRef B = Entry.getKey();
          if (!HoldsRefinable.count(B))
            continue;
          if (IsReplaceable(B))
            Needed.insert({A.str(), B.str()});
          if (Seen.insert(B).second)
            Work.push_back(B);
        }
      }
      // The replaceable-callee check: a call out of line of a replaceable
      // function whose code holds a premise.
      for (Instruction &Inst : instructions(*X)) {
        auto *CB = dyn_cast<CallBase>(&Inst);
        Function *Z = CB ? CB->getCalledFunction() : nullptr;
        if (!Z || Z->isIntrinsic() || Z == SF.Fn ||
            Z->getName() == cs::UnprovenFn || Z->getName() == cs::ProvenFn ||
            Z->getName() == cs::NestedFn)
          continue;
        if (IsReplaceable(Z->getName())) {
          SF.Undecided |= HoldsPremise.count(Z->getName()) != 0;
        } else if (!Z->isDeclaration() && InReach.insert(Z).second) {
          Reach.push_back(Z);
        }
      }
    }
    SF.Inlinings.assign(Needed.begin(), Needed.end());
  }

  // Surviving failures.
  DenseMap<unsigned, Outcome> Surviving;
  if (Function *Unproven = P->getFunction(cs::UnprovenFn)) {
    SmallVector<unsigned, 4> All;
    for (const ShadowFacts &SF : Facts)
      llvm::append_range(All, SF.Obligations);
    for (User *U : Unproven->users()) {
      auto *CB = dyn_cast<CallBase>(U);
      if (!CB)
        continue;
      SmallVector<unsigned, 4> Ids;
      SmallPtrSet<const Value *, 8> Seen;
      if (!collectMarkerIds(CB->getArgOperand(0), Ids, Seen)) {
        // Cannot tell which: every obligation is unproven.
        for (unsigned Id : All)
          Surviving[Id] = Outcome::Unproven;
        continue;
      }
      // "Always false": a single obligation's failure, in a shadow without
      // any doubtful premise, reached whenever the shadow is, and not a
      // loop's (where the predicate may hold on other iterations).  (An
      // override shadow's premises from ordinary obligations count as
      // doubtful: they are decided only below.)
      bool AlwaysFalse = false;
      if (Ids.size() == 1 && Ids[0] < Records.size() &&
          !(Records[Ids[0]].Flags & cs::InLoop) &&
          isReachedUnconditionally(*CB)) {
        Function *F = CB->getFunction();
        for (const ShadowFacts &SF : Facts)
          if (SF.Fn == F && !SF.Conditional && SF.Callees.empty() &&
              SF.PremiseCallees.empty() && SF.Relied.empty())
            AlwaysFalse = true;
      }
      for (unsigned Id : Ids) {
        Outcome O = AlwaysFalse ? Outcome::AlwaysFalse : Outcome::Unproven;
        auto [It, Inserted] = Surviving.try_emplace(Id, O);
        if (!Inserted)
          It->second = std::max(It->second, O);
      }
    }
  }

  // D4405: the override shadows last, so that the ordinary obligations they
  // take as premises are decided when they are: one not proven soundly (or
  // not decided by any shadow) makes their proofs conditional.
  std::stable_partition(Facts.begin(), Facts.end(),
                        [](const ShadowFacts &SF) { return !SF.Override; });
  DenseMap<Function *, bool> Memo;
  // A replaceable function whose code holds an unsettled evaluation: called
  // out of line by a predicate, never side-effect free (DECISIONS.md E214;
  // inlined, Rule 1 below decides).
  auto Unsettled = [&](const Function *F) {
    auto It = FnFacts.find(F->getName());
    return It != FnFacts.end() && It->second.Replaceable &&
           It->second.Refinable;
  };
  auto CalledOutOfLine = [](const Function *S, const Function *F) {
    for (const User *U : F->users())
      if (auto *CB = dyn_cast<CallBase>(U))
        if (CB->getFunction() == S && CB->getCalledFunction() == F)
          return true;
    return false;
  };
  for (const ShadowFacts &SF : Facts) {
    bool Conditional = SF.Conditional || SF.Undecided;
    for (unsigned Id : SF.Relied)
      if (Id >= Results.size() || Results[Id] != Outcome::Sound ||
          (Records[Id].Flags & cs::Unshadowed) || Surviving.count(Id))
        Conditional = true;
    for (Function *F : SF.Callees)
      if ((Unsettled(F) && CalledOutOfLine(SF.Fn, F)) ||
          !isSideEffectFree(F, Memo, Unsettled))
        Conditional = true;
    for (Function *F : SF.PremiseCallees)
      if (Unsettled(F) || !isSideEffectFree(F, Memo, Unsettled))
        Conditional = true;
    // A surviving failure is a premise for what follows -- but for a nested
    // obligation's, on a side branch that never rejoins (DECISIONS.md E169).
    for (unsigned Id : SF.Obligations)
      if (Surviving.count(Id) &&
          (Id >= Records.size() || Records[Id].Parent < 0))
        Conditional = true;
    // Rule 1 for what the shadow inlined: the proof is sound only if the real
    // code inlines the same replaceable functions.  That is made so (the
    // calls become always_inline) only for a proof whose assumption the real
    // code keeps; otherwise it is conditional, and the real code unchanged.
    if (!Conditional && !SF.Inlinings.empty()) {
      bool Kept = AllowAssumptions && ContractProofForceInline &&
                  llvm::any_of(SF.Obligations, [&](unsigned Id) {
                    return Regions.contains(Id) && !Surviving.count(Id);
                  });
      SmallVector<std::pair<Function *, Function *>, 2> Calls;
      for (auto &[A, B] : SF.Inlinings) {
        Function *RA = getRealFunction(M, A);
        Function *RB = M.getFunction(B);
        if (!Kept || !RA || !RB || !canForceInline(*RA, *RB, TM, TLII)) {
          Kept = false;
          break;
        }
        Calls.push_back({RA, RB});
      }
      if (Kept)
        llvm::append_range(Forced, Calls);
      else
        Conditional = true;
    }
    for (unsigned Id : SF.Obligations) {
      if (Id >= Results.size())
        continue;
      auto It = Surviving.find(Id);
      Outcome O = It != Surviving.end()
                      ? It->second
                      : (Conditional ? Outcome::Conditional : Outcome::Sound);
      Results[Id] = std::max(Results[Id], O);
    }
  }
  // A failure moved out of the shadows (by outlining, say) still counts.
  for (auto &[Id, O] : Surviving)
    if (Id < Results.size())
      Results[Id] = std::max(Results[Id], O);
}

/// D4405 (-fcontracts-prove-callee-premises): the premise round.  The
/// obligations the shadows \p Shadows left unproven or always false, in
/// \p Results, are decided once more, by the shadows holding them, with
/// every function that has a premise copy (\p Copies) replaced by its copy;
/// one proven there is proven conditionally.  Only a shadow that uses such a
/// function, directly or through what it uses, is decided again.  Nothing
/// else changes: the first round's sound proofs, assumptions and inlinings
/// are its own.
static void decidePremiseRound(
    llvm::Module &M, ArrayRef<Function *> Shadows,
    ArrayRef<ObligationRecord> Records, MutableArrayRef<Outcome> Results,
    TargetMachine *TM, const PipelineTuningOptions &PTO,
    const TargetLibraryInfoImpl &TLII, OptimizationLevel Level, bool Verify,
    IntrusiveRefCntPtr<vfs::FileSystem> FS, const ProofNormalization &Norm,
    const SmallPtrSetImpl<Function *> &Premises,
    const SmallPtrSetImpl<Function *> &Refinable,
    const DenseSet<unsigned> &Regions,
    const DenseMap<const Function *, Function *> &External,
    const DenseMap<const Function *, Function *> &Copies) {
  auto IsOpen = [&](unsigned Id) {
    return Id < Results.size() && Id < Records.size() &&
           Records[Id].Decision == cs::Deferred &&
           !(Records[Id].Flags & cs::Unshadowed) &&
           (Results[Id] == Outcome::Unproven ||
            Results[Id] == Outcome::AlwaysFalse);
  };
  // The functions (and constants) that use a function with a copy, directly
  // or through what they use: only a shadow among them can gain anything.
  SmallPtrSet<const Value *, 32> Reaches;
  SmallVector<const Value *, 32> Work;
  for (auto &Entry : Copies)
    Work.push_back(Entry.first);
  // D4405: a copy of an -O0 module's available_externally body is used
  // through the declaration.
  for (auto &[F, Ext] : External)
    if (Copies.count(Ext))
      Work.push_back(F);
  while (!Work.empty()) {
    const Value *V = Work.pop_back_val();
    for (const User *U : V->users()) {
      const Value *By = U;
      if (const auto *I = dyn_cast<Instruction>(U))
        By = I->getFunction();
      else if (!isa<Constant>(U))
        continue;
      if (Reaches.insert(By).second)
        Work.push_back(By);
    }
  }
  SmallVector<Function *, 8> Again;
  for (Function *S : Shadows) {
    if (!Reaches.count(S))
      continue;
    bool Open = false;
    for (Instruction &I : instructions(*S))
      if (auto *CB = dyn_cast<CallBase>(&I))
        if (Function *Callee = CB->getCalledFunction();
            Callee && Callee->getName() == cs::UnprovenFn)
          if (auto *Id = dyn_cast<ConstantInt>(CB->getArgOperand(0));
              Id && IsOpen(Id->getZExtValue())) {
            Open = true;
            break;
          }
    if (Open)
      Again.push_back(S);
  }
  if (Again.empty())
    return;
  SmallVector<Outcome, 16> Second(Results.size(), Outcome::Pending);
  SmallVector<std::pair<Function *, Function *>, 2> Unused;
  decideShadows(M, Again, Records, Second, TM, PTO, TLII, Level, Verify,
                std::move(FS), Norm, Premises, Refinable, Regions,
                /*AllowAssumptions=*/false, Unused, External, &Copies);
  for (unsigned Id = 0; Id != Results.size(); ++Id)
    if (IsOpen(Id) &&
        (Second[Id] == Outcome::Sound || Second[Id] == Outcome::Conditional))
      Results[Id] = Outcome::Conditional;
}

/// D4405: in an -O0 module, strip from \p M's real code what code generation
/// made for the proofs only (\p Flags: O0StripTBAA, O0StripRange).
static void stripO0ProofMetadata(llvm::Module &M, unsigned Flags) {
  if (!Flags)
    return;
  if (Flags & cs::O0StripTBAA)
    if (NamedMDNode *Errno = M.getNamedMetadata("llvm.errno.tbaa"))
      M.eraseNamedMetadata(Errno);
  for (Function &F : M)
    for (Instruction &I : instructions(F)) {
      if (Flags & cs::O0StripTBAA) {
        I.setMetadata(LLVMContext::MD_tbaa, nullptr);
        I.setMetadata(LLVMContext::MD_tbaa_struct, nullptr);
      }
      if (Flags & cs::O0StripRange) {
        I.setMetadata(LLVMContext::MD_range, nullptr);
        I.setMetadata(LLVMContext::MD_noundef, nullptr);
      }
    }
}

/// D4405: delete each alloca made for an assumption region (RegionTempMD)
/// that nothing uses any more, and unmark the others -- but those a kept
/// region uses (\p KeepMarks), which ContractAssumeCleanupPass deletes with
/// the assumption.
static void eraseRegionTemporaries(llvm::Module &M, bool KeepMarks) {
  unsigned Kind = M.getContext().getMDKindID(cs::RegionTempMD);
  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    for (Instruction &I : llvm::make_early_inc_range(F.getEntryBlock())) {
      if (!isa<AllocaInst>(I) || !I.getMetadata(Kind))
        continue;
      if (I.use_empty() && !I.isUsedByMetadata())
        I.eraseFromParent();
      else if (!KeepMarks)
        I.setMetadata(Kind, nullptr);
    }
  }
}

bool CodeGen::decideContractProofs(
    llvm::Module &M, TargetMachine *TM, const PipelineTuningOptions &PTO,
    const TargetLibraryInfoImpl &TLII, OptimizationLevel Level, bool Verify,
    bool AllowAssumptions, IntrusiveRefCntPtr<vfs::FileSystem> FS,
    DiagnosticsEngine &Diags, const ASTContext *Ctx) {
  // The functions whose code holds a premise only this TU's copy decides,
  // and those whose code holds an unsettled evaluation.
  auto TakeList = [&M](StringRef Name, SmallPtrSetImpl<Function *> &Fns) {
    NamedMDNode *L = M.getNamedMetadata(Name);
    if (!L)
      return;
    for (const MDNode *N : L->operands())
      if (N->getNumOperands() == 1)
        if (auto *F = mdconst::dyn_extract_or_null<Function>(N->getOperand(0)))
          Fns.insert(F);
    M.eraseNamedMetadata(L);
  };
  SmallPtrSet<Function *, 16> Premises, Refinable;
  TakeList(cs::PremisesMD, Premises);
  TakeList(cs::RefinableMD, Refinable);
  // D4405: what the proof level replaces of the -O level's settings.
  ProofNormalization Norm;
  if (NamedMDNode *L = M.getNamedMetadata(cs::ImpliedUnrollMD)) {
    for (const MDNode *N : L->operands())
      Norm.ImpliedUnroll.insert(N);
    M.eraseNamedMetadata(L);
  }
  if (NamedMDNode *L = M.getNamedMetadata(cs::SizeMD)) {
    Norm.Size = true;
    for (const MDNode *N : L->operands())
      if (N->getNumOperands() == 3)
        if (auto *F = mdconst::dyn_extract_or_null<Function>(N->getOperand(0)))
          Norm.UserSize[F->getName()] = {
              mdconst::extract<ConstantInt>(N->getOperand(1))->isOne(),
              mdconst::extract<ConstantInt>(N->getOperand(2))->isOne()};
    M.eraseNamedMetadata(L);
  }
  // D4405: an -O0 module proving at a proof level above 0.
  unsigned O0Strip = 0;
  if (NamedMDNode *L = M.getNamedMetadata(cs::O0MD)) {
    for (const MDNode *N : L->operands()) {
      if (N->getNumOperands() == 1)
        O0Strip = mdconst::extract<ConstantInt>(N->getOperand(0))->getZExtValue();
      else if (N->getNumOperands() == 2)
        if (auto *F = mdconst::dyn_extract_or_null<Function>(N->getOperand(0)))
          Norm.O0[F->getName()] =
              mdconst::extract<ConstantInt>(N->getOperand(1))->getZExtValue();
    }
    M.eraseNamedMetadata(L);
  }
  NamedMDNode *List = M.getNamedMetadata(cs::ObligationsMD);
  if (!List) {
    stripO0ProofMetadata(M, O0Strip);
    return false;
  }
  SmallVector<ObligationRecord, 16> Records;
  for (const MDNode *N : List->operands()) {
    ObligationRecord R;
    R.Loc = toLoc(N, 0);
    R.Range = SourceRange(toLoc(N, 1), toLoc(N, 2));
    R.PredRange = SourceRange(toLoc(N, 3), toLoc(N, 4));
    R.Flags = toUnsigned(N, 5);
    R.Decision = toUnsigned(N, 6);
    R.DiagContext = toUnsigned(N, 7);
    if (N->getNumOperands() > 8)
      if (const auto *Name = dyn_cast<MDString>(N->getOperand(8)))
        R.OverrideName = Name->getString().str();
    if (N->getNumOperands() > 9)
      R.Parent = static_cast<int>(toUnsigned(N, 9)) - 1;
    if (N->getNumOperands() > 10)
      R.Group = toUnsigned(N, 10);
    Records.push_back(R);
  }

  SmallVector<Function *, 16> Shadows;
  // D4405: the premise copies, "<name>.contract.premise", by real function;
  // in an -O0 module, the available_externally bodies,
  // "<name>.contract.external", by real function (a declaration), which a
  // premise copy is of then.
  SmallVector<Function *, 16> CopyFns;
  DenseMap<const Function *, Function *> Copies, External;
  for (Function &F : M) {
    StringRef Name = F.getName();
    if (F.hasFnAttribute(cs::ExternalAttr)) {
      CopyFns.push_back(&F);
      if (Name.consume_back(".contract.external"))
        if (Function *Real = M.getFunction(Name); Real && Real->isDeclaration())
          External[Real] = &F;
    }
  }
  for (Function &F : M) {
    if (F.hasFnAttribute(cs::ShadowAttr))
      Shadows.push_back(&F);
    if (F.hasFnAttribute(cs::PremiseCopyAttr)) {
      CopyFns.push_back(&F);
      StringRef Name = F.getName();
      if (Name.consume_back(".contract.premise"))
        if (Function *Real = M.getFunction(Name)) {
          if (!Real->isDeclaration())
            Copies[Real] = &F;
          else if (Function *Ext = External.lookup(Real))
            Copies[Ext] = &F;
        }
    }
  }

  // The obligations whose assumption the real code may keep.
  DenseSet<unsigned> Regions;
  if (Function *Proven = M.getFunction(cs::ProvenFn))
    for (User *U : Proven->users())
      if (auto *CI = dyn_cast<CallInst>(U))
        Regions.insert(
            cast<ConstantInt>(CI->getArgOperand(0))->getZExtValue());

  SmallVector<Outcome, 16> Results(Records.size(), Outcome::Pending);
  SmallVector<std::pair<Function *, Function *>, 4> Forced;
  if (!Shadows.empty()) {
    decideShadows(M, Shadows, Records, Results, TM, PTO, TLII, Level, Verify,
                  FS, Norm, Premises, Refinable, Regions, AllowAssumptions,
                  Forced, External);
    if (!Copies.empty())
      decidePremiseRound(M, Shadows, Records, Results, TM, PTO, TLII, Level,
                         Verify, std::move(FS), Norm, Premises, Refinable,
                         Regions, External, Copies);
  }
  // An obligation no shadow decided, or one also emitted where no shadow
  // covers it, is unproven.
  for (unsigned Id = 0; Id != Records.size(); ++Id)
    if (Results[Id] == Outcome::Pending || (Records[Id].Flags & cs::Unshadowed))
      Results[Id] = std::max(Results[Id], Outcome::Unproven);

  // Each obligation's result with those nested in it (DECISIONS.md E169).
  SmallVector<int, 16> Parent;
  SmallVector<bool, 16> Prove;
  SmallVector<ContractProofResult, 16> Raw;
  SmallVector<bool, 16> HasNested(Records.size(), false);
  for (unsigned Id = 0; Id != Records.size(); ++Id) {
    const ObligationRecord &R = Records[Id];
    Parent.push_back(R.Parent);
    Prove.push_back(R.Flags & cs::Prove);
    if (R.Parent >= 0 && static_cast<unsigned>(R.Parent) < Records.size())
      HasNested[R.Parent] = true;
    Outcome O = Results[Id];
    Raw.push_back(
        R.Decision == cs::TakenAlwaysFalse ? ContractProofResult::AlwaysFalse
        : R.Decision == cs::TakenUnproven  ? ContractProofResult::Unproven
        : R.Decision == cs::TakenSound     ? ContractProofResult::Sound
        : O == Outcome::Sound              ? ContractProofResult::Sound
        : O == Outcome::Conditional        ? ContractProofResult::Conditional
        // DECISIONS.md E195: a predicate folding found false is always false
        // wherever its marker survives, and when nothing decided it.
        : O == Outcome::AlwaysFalse || (R.Flags & cs::FoldedFalse)
            ? ContractProofResult::AlwaysFalse
            : ContractProofResult::Unproven);
  }
  SmallVector<ContractProofResult, 16> Effective;
  SmallVector<SmallVector<unsigned, 2>, 16> Causes;
  combineNestedContractResults(Parent, Prove, Raw, Effective, Causes);

  // The real module: no shadow, no premise copy, and each assumption kept or
  // deleted.
  SmallPtrSet<GlobalValue *, 32> Refs;
  SmallPtrSet<const Constant *, 32> Seen;
  llvm::append_range(Shadows, CopyFns);
  for (Function *S : Shadows) {
    collectReferences(*S, Refs, Seen);
    S->dropAllReferences();
  }
  for (Function *S : Shadows) {
    Refs.erase(S);
    S->eraseFromParent();
  }
  SmallVector<bool, 16> Assumed(Records.size(), false);
  SmallPtrSet<Function *, 8> WithRegions;
  foldPlaceholders(
      M,
      [&](unsigned Id) {
        return AllowAssumptions && Id < Results.size() &&
               Results[Id] == Outcome::Sound &&
               Effective[Id] == ContractProofResult::Sound;
      },
      Refs,
      [&](Function &F, unsigned Id) {
        Assumed[Id] = true;
        WithRegions.insert(&F);
      });
  if (Function *Proven = M.getFunction(cs::ProvenFn))
    Refs.insert(Proven);
  if (Function *Unproven = M.getFunction(cs::UnprovenFn))
    Refs.insert(Unproven);
  if (Function *Nested = M.getFunction(cs::NestedFn))
    Refs.insert(Nested);
  // The calls a sound proof relies on the real code inlining (rule 1): made
  // always_inline, and tagged so that the end of the pipeline can check that
  // they were (ContractAssumeCleanupPass).
  for (auto &[Caller, Callee] : Forced)
    for (CallBase *CB : getRealCalls(*Caller, *Callee)) {
      CB->addFnAttr(Attribute::AlwaysInline);
      CB->setMetadata(cs::InlineMD, MDNode::get(M.getContext(), {}));
    }
  // What is left of a kept assumption is its predicate, whose calls the
  // shadow judged to have no effect: say so, so that the predicate is never
  // evaluated once the assumption has been used.
  unsigned PredKind = M.getContext().getMDKindID(cs::PredicateMD);
  for (Function *F : WithRegions)
    for (Instruction &I : instructions(*F)) {
      int Tag = getPredicateTag(I);
      if (Tag < 0)
        continue;
      if (auto *CB = dyn_cast<CallBase>(&I);
          CB && !isa<IntrinsicInst>(CB) &&
          static_cast<unsigned>(Tag) < Assumed.size() && Assumed[Tag]) {
        CB->setOnlyReadsMemory();
        CB->addFnAttr(Attribute::WillReturn);
        CB->setDoesNotThrow();
      }
      I.setMetadata(PredKind, nullptr);
    }
  eraseUnusedGlobals(M, Refs);
  M.eraseNamedMetadata(List);
  eraseRegionTemporaries(M, !WithRegions.empty());
  stripO0ProofMetadata(M, O0Strip);

  // The diagnostics, in the order code generation met the evaluations.  A
  // caller-side diagnostic is issued once, though a nested diagnose
  // obligation of an assertion's analysis evaluation and the one at the same
  // call in its check both make it (DECISIONS.md E169 (9)) -- once for each
  // place, the record's group (E169 (11)).
  using DiagKey = std::tuple<SourceLocation::UIntTy, SourceLocation::UIntTy,
                             bool, bool, unsigned, unsigned>;
  auto KeyOf = [&](unsigned Id) {
    const ObligationRecord &R = Records[Id];
    return DiagKey{R.Loc.getRawEncoding(),
                   R.PredRange.getBegin().getRawEncoding(),
                   bool(R.Flags & cs::Prove),
                   Effective[Id] == ContractProofResult::AlwaysFalse,
                   R.DiagContext,
                   R.Group};
  };
  std::set<DiagKey> Reported;
  for (unsigned Id = 0; Id != Records.size(); ++Id)
    if (Records[Id].Parent < 0 && (Records[Id].Flags & cs::CallerSide) &&
        !isContractProofProven(Effective[Id]))
      Reported.insert(KeyOf(Id));
  auto Index = [](ContractProofResult R) {
    return R == ContractProofResult::Sound         ? 0
           : R == ContractProofResult::Conditional ? 1
           : R == ContractProofResult::Unproven    ? 2
                                                   : 3;
  };
  for (unsigned Id = 0; Id != Records.size(); ++Id) {
    const ObligationRecord &R = Records[Id];
    bool Nested = R.Parent >= 0;
    if (R.Decision == cs::Deferred)
      Diags.Report(R.Loc, diag::remark_contract_proof)
          << Index(Raw[Id]) << Assumed[Id] << bool(R.Flags & cs::OverrideShadow)
          << Nested << bool(HasNested[Id]) << Index(Effective[Id]);
    // A nested prove obligation counts only for those it is nested in.
    if (isContractProofProven(Effective[Id]) ||
        (Nested && (R.Flags & cs::Prove)))
      continue;
    if (Nested && !Reported.insert(KeyOf(Id)).second)
      continue;
    SmallVector<std::pair<SourceRange, bool>, 2> Notes;
    for (unsigned C : Causes[Id]) {
      std::pair<SourceRange, bool> Note{
          Records[C].PredRange, Raw[C] == ContractProofResult::AlwaysFalse};
      if (!llvm::is_contained(Notes, Note))
        Notes.push_back(Note);
    }
    reportContractObligation(Diags, R.Flags, R.Loc, R.Range, R.PredRange,
                             Effective[Id], Ctx, R.DiagContext, R.OverrideName,
                             Notes);
  }
  return !WithRegions.empty() || !Forced.empty();
}

namespace {
/// Deletes the assumptions made of proven evaluations, at the end of the
/// optimization pipeline (registerContractAssumeCleanup).
struct ContractAssumeCleanupPass
    : RequiredPassInfoMixin<ContractAssumeCleanupPass> {
  PreservedAnalyses run(llvm::Module &M, ModuleAnalysisManager &) {
    unsigned Kind = M.getContext().getMDKindID(cs::AssumeMD);
    unsigned InlineKind = M.getContext().getMDKindID(cs::InlineMD);
    SmallVector<AssumeInst *, 8> Assumes;
    SmallVector<CallBase *, 2> NotInlined;
    for (Function &F : M)
      for (Instruction &I : instructions(F)) {
        if (auto *A = dyn_cast<AssumeInst>(&I); A && A->getMetadata(Kind))
          Assumes.push_back(A);
        if (auto *CB = dyn_cast<CallBase>(&I); CB && CB->getMetadata(InlineKind))
          NotInlined.push_back(CB);
      }
    // A call the real code was to inline for a sound proof never survives
    // (canForceInline checked what the inliners require); were one to, the
    // proof's assumption could rest on another TU's definition.
    for (CallBase *CB : NotInlined) {
      M.getContext().diagnose(DiagnosticInfoGeneric(
          Twine("P4405: a call of '") + CB->getCalledOperand()->getName() +
              "' in '" + CB->getFunction()->getName() +
              "' that a contract proof relies on inlining was not inlined",
          DS_Warning));
      CB->setMetadata(InlineKind, nullptr);
    }
    if (Assumes.empty())
      return NotInlined.empty() ? PreservedAnalyses::all()
                                : PreservedAnalyses::none();
    SmallPtrSet<GlobalValue *, 16> Refs;
    SmallPtrSet<const Constant *, 16> Seen;
    SmallVector<BasicBlock *, 8> Regions;
    for (AssumeInst *A : Assumes) {
      Value *Cond = A->getArgOperand(0);
      Regions.push_back(A->getParent());
      collectGlobals(A->getCalledOperand(), Refs, Seen);
      A->eraseFromParent();
      RecursivelyDeleteTriviallyDeadInstructions(
          Cond, /*TLI=*/nullptr, /*MSSAU=*/nullptr, [&](Value *V) {
            for (const Use &Op : cast<Instruction>(V)->operands())
              collectGlobals(Op.get(), Refs, Seen);
          });
    }
    // What is left of a region the pipeline kept as it was (-O0) rejoins
    // the code around it, as without it, and the allocas made for its
    // predicate go if nothing else uses them (RegionTempMD).
    SmallPtrSet<BasicBlock *, 8> Merged;
    for (BasicBlock *BB : Regions) {
      if (Merged.count(BB) || !isa<UncondBrInst>(BB->front()) ||
          !BB->getSingleSuccessor())
        continue;
      BasicBlock *Succ = BB->getSingleSuccessor();
      BasicBlock *Pred = BB->getSinglePredecessor();
      if (!Pred || Pred->getSingleSuccessor() != BB ||
          !MergeBlockIntoPredecessor(BB))
        continue;
      Merged.insert(BB);
      if (Succ->getSinglePredecessor() == Pred && MergeBlockIntoPredecessor(Succ))
        Merged.insert(Succ);
    }
    unsigned TempKind = M.getContext().getMDKindID(cs::RegionTempMD);
    for (Function &F : M)
      if (!F.isDeclaration())
        for (Instruction &I : make_early_inc_range(F.getEntryBlock()))
          if (isa<AllocaInst>(I) && I.getMetadata(TempKind)) {
            if (I.use_empty() && !I.isUsedByMetadata())
              I.eraseFromParent();
            else
              I.setMetadata(TempKind, nullptr);
          }
    eraseUnusedGlobals(M, Refs);
    return PreservedAnalyses::none();
  }
};
} // namespace

void CodeGen::registerContractAssumeCleanup(PassBuilder &PB) {
  PB.registerOptimizerLastEPCallback(
      [](ModulePassManager &MPM, OptimizationLevel, ThinOrFullLTOPhase) {
        MPM.addPass(ContractAssumeCleanupPass());
      });
}
