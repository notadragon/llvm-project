//===--- ContractProveOverride.cpp - D4405 override shadows ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// D4405R0, "Virtual Functions": the override shadows' proving strategy,
// -fcontracts-prove-override (implied by -fcontracts-p4405), and the override
// pairs it decides.
//
// For a defined virtual function D::f and each function B::f it overrides,
// directly or indirectly (an override pair), an override shadow of D::f takes
// B::f's preconditions as given and attempts to prove, in order, D::f's
// preconditions at entry, then at each return D::f's postconditions, each
// ahead of its own check, and B::f's postconditions, after all of D::f's
// checks (as the P3097 wrapper checks them after D::f returns) -- each an
// analysis evaluation of that assertion with its own analysis semantic,
// diagnosed at D::f's definition naming B::f (DECISIONS.md E155).  B::f's
// postconditions see D::f's result, converted to B::f's return type for a
// covariant one, `this' converted to the B subobject at entry, and D::f's
// parameters, a by-value one D::f receives as its own object read from a
// copy taken at entry (E153: the wrapper checks them with its own copy).
// A postcondition with captures is decided at once (folding only), as its
// own analysis evaluation is.
//
// Virtual destructors override too, implicitly defined ones included
// (DECISIONS.md E152).  Only the variant a delete through a base runs whole
// is analyzed: the base-object destructor of a class without virtual bases
// (the complete-object one calls it, or is an alias of it), and the
// complete-object destructor of a class with virtual bases, whose override
// shadow runs the whole destructor body instead of calling the base-object
// destructor (isContractOverrideWholeDestructor), so that B::~B's
// postconditions are proven after every base, virtual ones included, is
// destroyed and D::~D's own after its own checks' place.  The deleting
// destructor holds nothing.  A base class occurring as several subobjects
// gives one pair per subobject when B::f's contract assertions name `this',
// and one per overridden function otherwise (E154).  A B::f without contract assertions still gives a
// pair, with no premises, so D::f's preconditions must be proven tautological
// (E156).  B::f's preconditions are evaluated in the callers' translation
// units, so a proof relying on one is sound only when its label settles it
// (E151, isContractPremiseSettled); an ordinary obligation met in an override
// shadow is a premise, never decided there, and a proof relying on one is
// sound only if it is (ContractShadowDecide.cpp).  No override proof is an
// assumption in the real code: D::f serves direct calls too.
//
// An override shadow is one more proof shadow ("<name>.contract.override.<n>",
// emitted and decided as the proof shadows are, ContractProveShadow.cpp),
// made when the module is optimized, as for a proof shadow, and D::f can have
// one.  Without one -- the flag off (DECISIONS.md E157), or no shadow can be
// made -- the pair's obligations are D::f's own, with nothing given: D::f's
// preconditions are settled only by folding, D::f's postconditions need
// nothing beyond their own analysis evaluations, and B::f's postconditions
// are obligations at each return of D::f, proven in its proof shadow, read
// as in an override shadow.
//
// Clang generates only the functions a translation unit emits, so an inline
// overrider whose vtable is emitted elsewhere is never analyzed here
// (DECISIONS.md E150); nor is an available_externally copy of one.
//
//===----------------------------------------------------------------------===//

#include "CGCXXABI.h"
#include "CodeGenFunction.h"
#include "CodeGenModule.h"
#include "ContractProveShadow.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/StmtCXX.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace CodeGen;
namespace cs = contract_shadow;

//===----------------------------------------------------------------------===//
// Override pairs
//===----------------------------------------------------------------------===//

/// The declaration of \p FD that carries its contract assertions, or null.
static const FunctionDecl *getDeclWithContracts(const FunctionDecl *FD) {
  for (const FunctionDecl *R : FD->redecls())
    if (R->hasContracts())
      return R;
  return nullptr;
}

/// Add every function \p MD overrides, directly or indirectly, to \p Out,
/// once each.
static void collectOverridden(const CXXMethodDecl *MD,
                              SmallVectorImpl<const CXXMethodDecl *> &Out,
                              llvm::SmallPtrSetImpl<const Decl *> &Seen) {
  for (const CXXMethodDecl *O : MD->overridden_methods()) {
    if (!Seen.insert(O->getCanonicalDecl()).second)
      continue;
    Out.push_back(O);
    collectOverridden(O, Out, Seen);
  }
}

/// The analysis semantic of each assertion of \p FD's (its declaration with
/// them, \p WithContracts) of kind \p Kind that is analyzed with prove or
/// diagnose.
static void collectAnalyzed(
    ASTContext &Ctx, const FunctionDecl *WithContracts, ContractKind Kind,
    SmallVectorImpl<std::pair<const ContractStmt *, ContractEvaluationSemantic>>
        &Out) {
  if (!WithContracts)
    return;
  const DeclContext *DC = WithContracts->getDeclContext();
  for (const ContractStmt *CS : WithContracts->getContracts()->contracts()) {
    if (CS->getContractKind() != Kind || CS->isInvalid())
      continue;
    ContractEvaluationSemantic Sem = CS->ensureAnalysisSemantic(Ctx, DC);
    if (isProvingContractSemantic(Sem))
      Out.push_back({CS, Sem});
  }
}

using BasePath = SmallVector<const CXXBaseSpecifier *, 4>;

/// The derived-to-base paths from \p Derived to each \p Base subobject, as
/// Sema builds a cast's (BuildBasePathArray): from the path's last virtual
/// base on, which GetAddressOfBaseClass finds through \p Derived's vtable.
/// A virtual base reached by several paths is one subobject, its first path.
static void getBasePaths(const CXXRecordDecl *Derived,
                         const CXXRecordDecl *Base,
                         SmallVectorImpl<BasePath> &Out) {
  CXXBasePaths Paths(/*FindAmbiguities=*/true, /*RecordPaths=*/true,
                     /*DetectVirtual=*/false);
  if (!Derived->isDerivedFrom(Base, Paths))
    return;
  // A subobject is identified by its path from the last virtual base on,
  // the virtual base by its class (each class deriving from it virtually has
  // a base specifier of its own for it).
  SmallVector<SmallVector<const void *, 4>, 2> Keys;
  for (const CXXBasePath &Path : Paths) {
    unsigned Start = 0;
    for (unsigned I = Path.size(); I != 0; --I)
      if (Path[I - 1].Base->isVirtual()) {
        Start = I - 1;
        break;
      }
    SmallVector<const void *, 4> Key;
    BasePath Suffix;
    for (unsigned I = Start, E = Path.size(); I != E; ++I) {
      const CXXBaseSpecifier *Spec = Path[I].Base;
      Key.push_back(Spec->isVirtual() ? static_cast<const void *>(
                                            Spec->getType()
                                                ->getAsCXXRecordDecl()
                                                ->getCanonicalDecl())
                                      : Spec);
      Suffix.push_back(Spec);
    }
    if (llvm::is_contained(Keys, Key))
      continue;
    Keys.push_back(std::move(Key));
    Out.push_back(std::move(Suffix));
  }
}

/// The derived-to-base path from \p Derived to its first \p Base subobject.
static void getBasePath(const CXXRecordDecl *Derived, const CXXRecordDecl *Base,
                        BasePath &Path) {
  SmallVector<BasePath, 2> Paths;
  getBasePaths(Derived, Base, Paths);
  if (!Paths.empty())
    Path = Paths.front();
}

/// Whether a contract assertion of \p FD's (its declaration with them)
/// names `this', explicitly or by naming a member (DECISIONS.md E154).
static bool contractsNameThis(const FunctionDecl *WithContracts) {
  if (!WithContracts)
    return false;
  for (const ContractStmt *CS : WithContracts->getContracts()->contracts()) {
    SmallVector<const Stmt *, 16> Work{CS->getCond()};
    while (!Work.empty()) {
      const Stmt *S = Work.pop_back_val();
      if (!S)
        continue;
      if (isa<CXXThisExpr>(S))
        return true;
      for (const Stmt *Child : S->children())
        Work.push_back(Child);
    }
  }
  return false;
}

/// The override pairs of the function being emitted, if any were made.
static ArrayRef<unsigned> getOverridePairsOf(CodeGenModule &CGM,
                                             const CXXMethodDecl *MD) {
  ContractShadowModuleState &St = CGM.getContractShadowState();
  auto It = St.OverridePairsOf.find(MD->getCanonicalDecl());
  if (It == St.OverridePairsOf.end())
    return {};
  return It->second;
}

/// The override pair (\p MD, \p Base) for B's subobject number \p Subobject,
/// reached by \p Path, made if it is new.
static ContractOverridePair &
getOverridePair(CodeGenModule &CGM, const CXXMethodDecl *MD,
                const CXXMethodDecl *Base, unsigned Subobject,
                const BasePath &Path, bool Shadows) {
  ContractShadowModuleState &St = CGM.getContractShadowState();
  auto Key = std::make_tuple(static_cast<const Decl *>(MD->getCanonicalDecl()),
                             static_cast<const Decl *>(Base->getCanonicalDecl()),
                             Subobject);
  auto It = St.OverridePairIds.find(Key);
  if (It != St.OverridePairIds.end())
    return St.OverridePairs[It->second - 1];
  ContractOverridePair &P = St.OverridePairs.emplace_back();
  P.Index = St.OverridePairs.size();
  P.Derived = MD;
  P.Base = Base;
  P.BaseWithContracts = getDeclWithContracts(Base);
  P.Shadows = Shadows;
  P.Site = CGM.getContractImplicitCallSite(&P, MD->getLocation());
  llvm::raw_string_ostream OS(P.BaseName);
  Base->getNameForDiagnostic(OS, CGM.getContext().getPrintingPolicy(),
                             /*Qualified=*/true);
  P.Path.assign(Path.begin(), Path.end());
  St.OverridePairIds.insert({Key, P.Index});
  St.OverridePairsOf[MD->getCanonicalDecl()].push_back(P.Index);
  return P;
}

/// Whether a delete through a base class runs the destructor variant
/// \p Type of \p DD whole, so that its override pairs are analyzed there
/// (DECISIONS.md E152): the base-object destructor of a class without
/// virtual bases (the complete-object one calls it, or is an alias of it);
/// the complete-object destructor of one with virtual bases, which the
/// base-object destructor does not destroy, but not of an abstract class,
/// whose complete-object destructor is a trap (and never the final
/// overrider of a virtual destructor call anyway).  Never the deleting
/// destructor, which calls the complete-object one.
static bool isAnalyzedDestructorVariant(const CXXDestructorDecl *DD,
                                        CXXDtorType Type) {
  const CXXRecordDecl *RD = DD->getParent();
  if (RD->getNumVBases() == 0)
    return Type == Dtor_Base;
  return Type == Dtor_Complete && !RD->isAbstract();
}

/// The virtual function whose definition \p CGF emits, if its override pairs
/// are analyzed: not a pure virtual function (never the final overrider of a
/// call through a base), a P3097 wrapper, a destructor variant other than
/// isAnalyzedDestructorVariant's, or anything but the definition.
static const CXXMethodDecl *getOverrider(const CodeGenFunction &CGF) {
  const LangOptions &LO = CGF.getLangOpts();
  if (!LO.CPlusPlus || !LO.ContractsP4405 || !LO.ContractsP3097 ||
      CGF.InContractInterfaceEvaluation || CGF.CurFuncIsThunk)
    return nullptr;
  const auto *MD = dyn_cast_or_null<CXXMethodDecl>(CGF.CurGD.getDecl());
  if (!MD || !MD->isVirtual() || MD->isPureVirtual() || CGF.CurCodeDecl != MD)
    return nullptr;
  if (const auto *DD = dyn_cast<CXXDestructorDecl>(MD);
      DD && !isAnalyzedDestructorVariant(DD, CGF.CurGD.getDtorType()))
    return nullptr;
  return MD;
}

/// Whether \p CGF emits a shadow, or the real function whose override pairs
/// were made (E150: not an available_externally copy).
static bool isOverrideEmission(const CodeGenFunction &CGF) {
  if (CGF.ContractShadow)
    return CGF.ContractShadow->InShadow;
  return CGF.CurFn &&
         CGF.CurFn->getName() == CGF.CGM.getMangledName(CGF.CurGD) &&
         !CGF.CurFn->hasAvailableExternallyLinkage();
}

bool CodeGenFunction::isContractOverrideWholeDestructor() const {
  return ContractShadow && ContractShadow->InShadow &&
         ContractShadow->Override &&
         isa_and_nonnull<CXXDestructorDecl>(CurGD.getDecl()) &&
         CurGD.getDtorType() == Dtor_Complete && getOverrider(*this);
}

//===----------------------------------------------------------------------===//
// The strategy
//===----------------------------------------------------------------------===//

static bool isOverrideEnabled(const LangOptions &LangOpts) {
  return LangOpts.ContractsProveOverride;
}

/// Defer an override pair's evaluation made for its override shadow (the
/// pair was made with Shadows: optimized, and D::f can have one).
static ContractProofResult proveByOverride(const ContractProofQuery &Q) {
  if (!Q.Override || !Q.OverrideShadow || !Q.Override->Shadows || !Q.CGF ||
      Q.CGF->ContractShadow || Q.Predicate->isValueDependent() ||
      Q.Contract.hasCaptures())
    return ContractProofResult::Unproven;
  return ContractProofResult::Deferred;
}

/// In the real function, queue the pair's override shadow; in that override
/// shadow, emit the obligation (in any other shadow, nothing).
static bool placeOverrideObligation(const ContractProofQuery &Q, bool Defer) {
  CodeGenFunction *CGF = Q.CGF;
  if (!CGF || !Q.Override || !Q.OverrideShadow)
    return false;
  CodeGenModule &CGM = CGF->CGM;
  ContractShadowModuleState &St = CGM.getContractShadowState();
  unsigned Id;
  auto It = St.ObligationIds.find({&Q.Contract, Q.CallSite});
  if (It == St.ObligationIds.end()) {
    if (!Defer)
      return false;
    Id = addContractObligation(CGM, Q, cs::Deferred);
  } else {
    Id = It->second;
    if (St.Obligations[Id].Decision != cs::Deferred)
      return false;
  }
  ContractShadowFunction *State = CGF->ContractShadow;
  if (!State || !State->InShadow) {
    StringRef Name = CGF->CurFn->getName();
    if (St.PendingNames.insert((Name + "#" + Twine(Q.Override->Index)).str())
            .second)
      St.Pending.emplace_back(CGF->CurGD, Name.str(), Q.Override->Index);
    return true;
  }
  if (State->Override != Q.Override)
    return true;
  State->OverrideObligations.insert(&Q.Contract);
  CGF->emitContractShadowPredicate(Id, Q.Contract, Q.CallSite, Q.Callee,
                                   Q.Override);
  return true;
}

/// While override shadows can be made (C++ with P3097's virtual function
/// contracts, an optimized module), take every diagnostic decided at once,
/// as the proof shadows do (which take it first when they are made):
/// the back end issues it in order with theirs, and an error issued now
/// would keep the back end from deciding them.  Dropped in a shadow: the
/// real function has decided it.
static bool takeOverrideDiagnostic(const ContractProofQuery &Q,
                                   ContractProofResult R) {
  if (!Q.CGF || !Q.CGF->getLangOpts().CPlusPlus ||
      !Q.CGF->getLangOpts().ContractsP3097 ||
      !canMakeContractShadows(Q.CGF->CGM))
    return false;
  if (Q.CGF->ContractShadow)
    return true;
  addContractObligation(Q.CGF->CGM, Q,
                        R == ContractProofResult::AlwaysFalse
                            ? cs::TakenAlwaysFalse
                            : cs::TakenUnproven);
  return true;
}

const ContractProofStrategy CodeGen::ContractProveOverrideStrategy = {
    "override",
    /*MayDependOnAssumptions=*/true,
    isOverrideEnabled,
    proveByOverride,
    placeOverrideObligation,
    takeOverrideDiagnostic};

//===----------------------------------------------------------------------===//
// Emission
//===----------------------------------------------------------------------===//

/// The indexes of \p P's B::f parameters that its postconditions name.
static void collectNamedParams(const ContractOverridePair &P,
                               SmallVectorImpl<unsigned> &Out) {
  if (!P.BaseWithContracts)
    return;
  const Decl *Canon = P.Base->getCanonicalDecl();
  for (const ContractStmt *CS :
       P.BaseWithContracts->getContracts()->contracts()) {
    if (CS->getContractKind() != ContractKind::Post)
      continue;
    SmallVector<const Stmt *, 16> Work{CS->getCond()};
    while (!Work.empty()) {
      const Stmt *S = Work.pop_back_val();
      if (!S)
        continue;
      if (const auto *DRE = dyn_cast<DeclRefExpr>(S))
        if (const auto *PVD = dyn_cast<ParmVarDecl>(DRE->getDecl()))
          if (const auto *FD = dyn_cast<FunctionDecl>(PVD->getDeclContext()))
            if (FD->getCanonicalDecl() == Canon &&
                !llvm::is_contained(Out, PVD->getFunctionScopeIndex()))
              Out.push_back(PVD->getFunctionScopeIndex());
      for (const Stmt *Child : S->children())
        Work.push_back(Child);
    }
  }
}

/// Take \p P's snapshot at entry: `this' converted to the B subobject, and a
/// copy of each by-value parameter B::f's postconditions name that D::f
/// receives as its own object (DECISIONS.md E153: the P3097 wrapper checks
/// them with its own copy, which D::f cannot modify); a reference, or a
/// class passed by invisible reference, is the object the wrapper reads.
static void takeOverrideSnapshot(CodeGenFunction &CGF,
                                 const ContractOverridePair &P,
                                 ContractOverrideSnapshot &Snap) {
  Snap.This = getContractOverrideThis(CGF, P);
  SmallVector<unsigned, 4> Named;
  collectNamedParams(P, Named);
  Snap.Params.assign(P.Derived->getNumParams(), Address::invalid());
  for (unsigned I : Named) {
    if (I >= P.Derived->getNumParams())
      continue;
    const ParmVarDecl *PVD = P.Derived->getParamDecl(I);
    QualType T = PVD->getType();
    if (T->isReferenceType() || T.isVolatileQualified())
      continue;
    if (const auto *RD = T->getAsCXXRecordDecl();
        RD && CGF.CGM.getCXXABI().getRecordArgABI(RD) == CGCXXABI::RAA_Indirect)
      continue;
    LValue Src = CGF.MakeAddrLValue(CGF.GetAddrOfLocalVar(PVD), T);
    Address A = CGF.CreateMemTemp(T, "contract.override.parm");
    LValue Dst = CGF.MakeAddrLValue(A, T);
    switch (CodeGenFunction::getEvaluationKind(T)) {
    case TEK_Scalar:
      CGF.EmitStoreOfScalar(CGF.EmitLoadOfScalar(Src, PVD->getLocation()), Dst,
                            /*isInit=*/true);
      break;
    case TEK_Complex:
      CGF.EmitStoreOfComplex(CGF.EmitLoadOfComplex(Src, PVD->getLocation()),
                             Dst, /*isInit=*/true);
      break;
    case TEK_Aggregate:
      CGF.EmitAggregateCopy(Dst, Src, T, AggValueSlot::DoesNotOverlap);
      break;
    }
    Snap.Params[I] = A;
  }
}

void CodeGenFunction::emitContractOverrideEntry() {
  const CXXMethodDecl *MD = getOverrider(*this);
  if (!MD)
    return;
  ASTContext &Ctx = getContext();
  const FunctionDecl *WithContracts = getDeclWithContracts(MD);

  // An override shadow: its pair's premises, then its obligations.
  if (ContractShadow && ContractShadow->InShadow && ContractShadow->Override) {
    const ContractOverridePair &P = *ContractShadow->Override;
    ContractOverrideSnapshot &Snap = ContractShadow->Snapshots[&P];
    takeOverrideSnapshot(*this, P, Snap);
    if (P.BaseWithContracts)
      for (const ContractStmt *CS :
           P.BaseWithContracts->getContracts()->contracts()) {
        // A premise from a predicate with side effects would evaluate them
        // (DECISIONS.md E151 for when it is guaranteed).
        if (CS->getContractKind() != ContractKind::Pre || CS->isInvalid() ||
            CS->getCond()->HasSideEffects(Ctx))
          continue;
        EnsureInsertPoint();
        emitContractShadowUnreachableUnless(
            emitContractShadowCondition(*CS, nullptr, nullptr, &P, -1));
        if (!isContractPremiseSettled(*CS))
          ContractShadow->UnguaranteedAssumption = true;
      }
    SmallVector<std::pair<const ContractStmt *, ContractEvaluationSemantic>, 4>
        Pres;
    collectAnalyzed(Ctx, WithContracts, ContractKind::Pre, Pres);
    for (auto [CS, Sem] : Pres)
      CGM.diagnoseUnprovenContract(*CS, Sem, P.Site, /*Callee=*/nullptr, this,
                                   &P, /*OverrideShadow=*/true);
    return;
  }

  // A proof shadow: the snapshots of the pairs whose B::f postconditions are
  // its obligations.
  if (ContractShadow && ContractShadow->InShadow) {
    for (unsigned I : getOverridePairsOf(CGM, MD)) {
      const ContractOverridePair &P =
          CGM.getContractShadowState().OverridePairs[I - 1];
      if (!P.Shadows)
        takeOverrideSnapshot(*this, P, ContractShadow->Snapshots[&P]);
    }
    return;
  }

  // The real function: make its pairs, and offer each pair's obligations at
  // entry -- D::f's preconditions -- at once (those at each return are
  // offered where they are emitted).  E150: not an available_externally
  // copy, which the translation unit that emits D::f analyzes.
  if (ContractShadow || CurFn->getName() != CGM.getMangledName(CurGD) ||
      CurFn->hasAvailableExternallyLinkage())
    return;
  bool Shadows = getLangOpts().ContractsProveOverride &&
                 canMakeContractShadows(CGM) && isContractShadowable();
  SmallVector<std::pair<const ContractStmt *, ContractEvaluationSemantic>, 4>
      Pres, OwnPosts;
  collectAnalyzed(Ctx, WithContracts, ContractKind::Pre, Pres);
  // D::f's postconditions are obligations of each override shadow; without
  // override shadows their own analysis evaluations are all they need.
  if (Shadows)
    collectAnalyzed(Ctx, WithContracts, ContractKind::Post, OwnPosts);
  SmallVector<const CXXMethodDecl *, 4> Bases;
  llvm::SmallPtrSet<const Decl *, 4> Seen;
  collectOverridden(MD, Bases, Seen);
  for (const CXXMethodDecl *Base : Bases) {
    const FunctionDecl *BaseWithContracts = getDeclWithContracts(Base);
    SmallVector<std::pair<const ContractStmt *, ContractEvaluationSemantic>, 4>
        Posts;
    collectAnalyzed(Ctx, BaseWithContracts, ContractKind::Post, Posts);
    if (Pres.empty() && OwnPosts.empty() && Posts.empty())
      continue;
    // DECISIONS.md E154: one pair per B subobject if B::f's assertions name
    // `this', which reads a different object in each; otherwise one.
    SmallVector<BasePath, 2> Paths;
    getBasePaths(MD->getParent(), Base->getParent(), Paths);
    if (Paths.empty())
      Paths.emplace_back();
    if (!contractsNameThis(BaseWithContracts))
      Paths.resize(1);
    for (unsigned I = 0, E = Paths.size(); I != E; ++I) {
      ContractOverridePair &P =
          getOverridePair(CGM, MD, Base, I, Paths[I], Shadows);
      for (auto [CS, Sem] : Pres)
        CGM.diagnoseUnprovenContract(*CS, Sem, P.Site, /*Callee=*/nullptr,
                                     this, &P, P.Shadows);
    }
  }
}

void CodeGenFunction::emitContractOverrideOwnPost(const ContractStmt &S) {
  if (S.getContractKind() != ContractKind::Post || S.isInvalid())
    return;
  const CXXMethodDecl *MD = getOverrider(*this);
  if (!MD || !isOverrideEmission(*this))
    return;
  const FunctionDecl *WithContracts = getDeclWithContracts(MD);
  if (!WithContracts ||
      !llvm::is_contained(WithContracts->getContracts()->contracts(), &S))
    return;
  ContractEvaluationSemantic Sem =
      S.ensureAnalysisSemantic(getContext(), WithContracts->getDeclContext());
  if (!isProvingContractSemantic(Sem))
    return;
  for (unsigned I : getOverridePairsOf(CGM, MD)) {
    const ContractOverridePair &P =
        CGM.getContractShadowState().OverridePairs[I - 1];
    // In an override shadow, only its own pair's obligation is emitted.
    if (!P.Shadows || (ContractShadow && ContractShadow->InShadow &&
                       ContractShadow->Override != &P))
      continue;
    CGM.diagnoseUnprovenContract(S, Sem, P.Site, /*Callee=*/nullptr, this, &P,
                                 /*OverrideShadow=*/true);
  }
}

void CodeGenFunction::emitContractOverridePosts(llvm::Value *RV) {
  const CXXMethodDecl *MD = getOverrider(*this);
  if (!MD || !isOverrideEmission(*this))
    return;
  ArrayRef<unsigned> Pairs = getOverridePairsOf(CGM, MD);
  if (Pairs.empty())
    return;
  ASTContext &Ctx = getContext();
  bool InShadow = ContractShadow && ContractShadow->InShadow;
  // A complete-object destructor that calls the base-object one emits none
  // of D::~D's own postconditions: offer their obligations here, in the real
  // function (its override shadows run the whole destructor and emit them
  // ahead of their checks, isContractOverrideWholeDestructor).
  if (!InShadow && structorVariantDelegates())
    if (const FunctionDecl *WithContracts = getDeclWithContracts(MD))
      for (const ContractStmt *CS : WithContracts->getContracts()->contracts())
        emitContractOverrideOwnPost(*CS);
  for (unsigned I : Pairs) {
    const ContractOverridePair &P =
        CGM.getContractShadowState().OverridePairs[I - 1];
    // With override shadows, B::f's postconditions are emitted in the pair's
    // own; without them, in D::f's proof shadow.
    if (InShadow && (P.Shadows ? ContractShadow->Override != &P
                               : ContractShadow->Override != nullptr))
      continue;
    SmallVector<std::pair<const ContractStmt *, ContractEvaluationSemantic>, 4>
        Posts;
    collectAnalyzed(Ctx, P.BaseWithContracts, ContractKind::Post, Posts);
    if (Posts.empty())
      continue;
    // In a shadow, B::f's result is D::f's, converted to B::f's return type
    // for a covariant one; the real function emits nothing.
    Address SavedReturnValue = ReturnValue;
    if (InShadow && ReturnValue.isValid() &&
        !MD->getReturnType()->isVoidType()) {
      EnsureInsertPoint();
      QualType DRet = MD->getReturnType();
      Address Slot = ReturnValue;
      if (RV) {
        CharUnits Align = CharUnits::fromQuantity(
            CGM.getDataLayout().getABITypeAlign(RV->getType()));
        RawAddress Tmp =
            CreateTempAlloca(RV->getType(), Align, "contract.override.result");
        Builder.CreateStore(RV, Tmp);
        Slot = Address(Tmp.getPointer(), ConvertTypeForMem(DRet), Align);
      }
      QualType BRet = P.Base->getReturnType();
      const CXXRecordDecl *DClass = DRet->getPointeeCXXRecordDecl();
      const CXXRecordDecl *BClass = BRet->getPointeeCXXRecordDecl();
      if (DClass && BClass && !Ctx.hasSameUnqualifiedType(DRet, BRet) &&
          DClass != BClass) {
        SmallVector<const CXXBaseSpecifier *, 4> Path;
        getBasePath(DClass, BClass, Path);
        if (!Path.empty()) {
          llvm::Value *Ptr = Builder.CreateLoad(
              Slot.withElementType(Builder.getPtrTy()), "contract.override.r");
          Address Derived(Ptr,
                          ConvertTypeForMem(Ctx.getCanonicalTagType(DClass)),
                          CGM.getClassPointerAlignment(DClass));
          llvm::Value *Converted =
              GetAddressOfBaseClass(Derived, DClass, Path.begin(), Path.end(),
                                    /*NullCheckValue=*/DRet->isPointerType(),
                                    MD->getLocation())
                  .emitRawPointer(*this);
          RawAddress Conv =
              CreateTempAlloca(Converted->getType(), getPointerAlign(),
                               "contract.override.conv");
          Builder.CreateStore(Converted, Conv);
          Slot = Address(Conv.getPointer(), ConvertTypeForMem(BRet),
                         getPointerAlign());
        }
      }
      ReturnValue = Slot;
    }
    for (auto [CS, Sem] : Posts)
      CGM.diagnoseUnprovenContract(*CS, Sem, P.Site, /*Callee=*/nullptr, this,
                                   &P, P.Shadows);
    ReturnValue = SavedReturnValue;
  }
}
