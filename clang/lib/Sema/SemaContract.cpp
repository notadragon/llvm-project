//===--- SemaContract.cpp - Semantic Analysis for Contracts ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements semantic analysis for contracts.
//
//===----------------------------------------------------------------------===//

#include "TreeTransform.h"
#include "TypeLocBuilder.h"
#include "clang/AST/ASTConcept.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTDiagnostic.h"
#include "clang/AST/ASTLambda.h"
#include "clang/AST/ASTStructuralEquivalence.h"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/CharUnits.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ExprConcepts.h"
#include "clang/AST/ExprObjC.h"
#include "clang/AST/IgnoreExpr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtCXX.h"
#include "clang/AST/StmtObjC.h"
#include "clang/AST/TypeLoc.h"
#include "clang/AST/TypeOrdering.h"
#include "clang/Basic/ContractOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/EnterExpressionEvaluationContext.h"
#include "clang/Sema/Initialization.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Ownership.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/ScopeInfo.h"
#include "clang/Sema/SemaCUDA.h"
#include "clang/Sema/SemaInternal.h"
#include "clang/Sema/SemaObjC.h"
#include "clang/Sema/SemaOpenMP.h"
#include "clang/Sema/Template.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/STLForwardCompat.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"

using namespace clang;
using namespace sema;
using llvm::DenseMap;
using llvm::DenseSet;

class clang::SemaContractHelper {
public:
  static SmallVector<const Attr *>
  buildAttributesWithDummyNode(Sema &S, ParsedAttributes &Attrs,
                               SourceLocation Loc) {
    // We shouldn't end up actually emitting any diagnostics pointing an the
    // dummy node, but we need to have a valid source location for any
    // diagnostics. (Pray they don't call getEndLoc()), which will attempt to
    // read past the end of the buffer.

    ContractStmt Dummy(Stmt::EmptyShell(), ContractKind::Pre, false, false,
                       false, 0);
    Dummy.KeywordLoc = Loc;
    SmallVector<const Attr *> Result;
    S.ProcessStmtAttributes(&Dummy, Attrs, Result);
    return Result;
  }
};

namespace {
template <class T> struct ValueRAII {
  ValueRAII(T &XDest, T const &Value, bool Enter = true) : Dest(&XDest) {
    if (Enter)
      push(Value);
  }

  ~ValueRAII() { pop(); }

  ValueRAII(ValueRAII const &) = delete;
  ValueRAII &operator=(ValueRAII const &) = delete;

  void push(T const &Value) {
    assert(!Entered);
    OldValue = *Dest;
    *Dest = Value;
    Entered = true;
  }

  void pop() {
    if (Entered)
      *Dest = OldValue;
    Entered = false;
  }

private:
  bool Entered = false;
  T *Dest;
  T OldValue;
};

template <class T> ValueRAII(T &, T const &) -> ValueRAII<T>;

} // namespace

namespace {

/// Substitute the 'auto' specifier or deduced template specialization type
/// specifier within a type for a given replacement type.
class RebuildAutoResultName : public TreeTransform<RebuildAutoResultName> {
  QualType Replacement;
  using inherited = TreeTransform<RebuildAutoResultName>;

public:
  RebuildAutoResultName(Sema &SemaRef, QualType Replacement)
      : TreeTransform<RebuildAutoResultName>(SemaRef),
        Replacement(Replacement) {}

  QualType TransformAutoType(TypeLocBuilder &TLB, AutoTypeLoc TL) {
    // If we're building the type pattern to deduce against, don't wrap the
    // substituted type in an AutoType. Certain template deduction rules
    // apply only when a template type parameter appears directly (and not if
    // the parameter is found through desugaring). For instance:
    //   auto &&lref = lvalue;
    // must transform into "rvalue reference to T" not "rvalue reference to
    // auto type deduced as T" in order for [temp.deduct.call]p3 to apply.
    //
    // FIXME: Is this still necessary?

    QualType Result = SemaRef.Context.getAutoType(
        Replacement.isNull() ? DeducedKind::DeducedAsDependent
                             : DeducedKind::Deduced,
        Replacement, TL.getTypePtr()->getKeyword(),
        TL.getTypePtr()->getTypeConstraintConcept(),
        TL.getTypePtr()->getTypeConstraintArguments());
    auto NewTL = TLB.push<AutoTypeLoc>(Result);
    NewTL.copy(TL);
    return Result;
  }
};

} // namespace

ExprResult Sema::ActOnContractAssertCondition(Expr *Cond) {
  assert(currentEvaluationContext().isContractAssertionContext() &&
         "Wrong context for statement");

  if (Cond->isTypeDependent())
    return Cond;

  ConditionResult Res =
      ActOnCondition(getCurScope(), Cond->getExprLoc(), Cond,
                     Sema::ConditionKind::Boolean, /*MissingOK=*/false);
  if (Res.isInvalid())
    return ExprError();
  Cond = Res.get().second;
  assert(Cond);
  return Cond;
}

/// Drop facet candidates this context cannot name, leaving R empty if none
/// remain.
///
/// A facet concept is a requires-expression, so it is simply false for an
/// inaccessible member: the facet is absent, not an error.  P3400 requires
/// exactly that, because a label may carry private helpers named after a facet
/// (tag dispatch) and those must not make the program ill-formed.
///
/// Filtering the candidates up front, rather than letting the probe fail on
/// access, is deliberate.  Contracts are parsed as part of a declarator, and
/// Clang *delays* access checking across a declaration so it can be redone
/// once the declaration's context is known.  A probe run inside an SFINAE trap
/// therefore observes no access failure at all -- the check is not skipped, it
/// is parked, and it fires later against the contract, long after the trap has
/// been destroyed.  That is how a private helper turned into a hard error.
/// Removing the candidate means no member reference is ever built for it, so
/// no access check is queued in the first place.
static void dropInaccessibleFacetCandidates(Sema &S, const CXXRecordDecl *RD,
                                            QualType LabelTy, LookupResult &R) {
  LookupResult::Filter F = R.makeFilter();
  while (F.hasNext()) {
    NamedDecl *D = F.next();
    if (!S.IsSimplyAccessible(D, const_cast<CXXRecordDecl *>(RD), LabelTy))
      F.erase();
  }
  F.done();
  R.suppressAccessDiagnostics();
}

/// The library's std::contracts::contract_violation, or a null QualType when
/// <contracts> is not in scope.
///
/// Facet detection needs the real type.  The concepts ask whether the member
/// is callable with a `const contract_violation&', and probing with anything
/// else -- the member's own parameter type, say -- would be circular and would
/// admit a same-named helper of a different shape as a facet.
static QualType getContractViolationType(Sema &S, SourceLocation Loc) {
  NamespaceDecl *Std = S.getStdNamespace();
  if (!Std)
    return QualType();

  LookupResult NR(S, &S.Context.Idents.get("contracts"), Loc,
                  Sema::LookupNamespaceName);
  if (!S.LookupQualifiedName(NR, Std)) {
    NR.suppressDiagnostics();
    return QualType();
  }
  auto *Contracts = NR.getAsSingle<NamespaceDecl>();
  if (!Contracts)
    return QualType();

  LookupResult CR(S, &S.Context.Idents.get("contract_violation"), Loc,
                  Sema::LookupTagName);
  if (!S.LookupQualifiedName(CR, Contracts)) {
    CR.suppressDiagnostics();
    return QualType();
  }
  auto *RD = CR.getAsSingle<CXXRecordDecl>();
  if (!RD)
    return QualType();
  return S.Context.getCanonicalTagType(RD);
}

/// Constant-evaluate a facet call, reporting failure rather than hiding it.
///
/// No concept can ask whether a member is constexpr, so per D3400R5 a type
/// with the right member *does* participate in the facet, and the error
/// arrives when the result is consumed during translation -- which is here.
/// This used to evaluate silently and return "absent" on failure, which is the
/// quietest way to get this wrong: the label looked applied and the comment or
/// semantic was simply unchanged, with no diagnostic at all.
///
/// The diagnostic is NoSFINAE so the surrounding probe trap, which is there to
/// keep a wrong-shaped member from being reported, does not swallow a genuine
/// error.
static bool evaluateFacetConstant(Sema &S, Expr *Call, Expr::EvalResult &Eval,
                                  StringRef FacetName, QualType LabelTy,
                                  SourceLocation Loc) {
  SmallVector<PartialDiagnosticAt, 8> Notes;
  Eval.Diag = &Notes;
  if (Call->EvaluateAsConstantExpr(Eval, S.Context))
    return true;

  // Unqualified: every assertion-control object is constexpr and therefore
  // const, so printing that on the type is noise.
  S.Diag(Loc, diag::err_contract_facet_not_constant)
      << FacetName << LabelTy.getUnqualifiedType();
  for (const PartialDiagnosticAt &Note : Notes)
    S.Diag(Note.first, Note.second);
  return false;
}

/// The argument list a facet named FacetName is probed with.
///
/// Expr is not movable, so the placeholders live in fixed in-place storage
/// rather than a growable vector; no facet takes more than two arguments.
struct FacetProbeArgs {
  std::optional<OpaqueValueExpr> Storage[2];
  SmallVector<Expr *, 2> Args;

  /// False when the facet takes a shape this does not know how to probe.
  bool build(Sema &S, StringRef FacetName, NamedDecl *Found,
             SourceLocation Loc) {
    unsigned N = 0;
    auto push = [&](QualType T, ExprValueKind VK) {
      Storage[N].emplace(Loc, T, VK);
      Args.push_back(&*Storage[N]);
      ++N;
    };

    if (FacetName == "handle_contract_violation") {
      QualType CVTy = getContractViolationType(S, Loc);
      if (CVTy.isNull())
        return false;
      push(CVTy.withConst(), VK_LValue);
    } else if (FacetName == "query") {
      push(S.Context.VoidPtrTy, VK_PRValue);
      push(S.Context.getSizeType(), VK_PRValue);
    } else if (FacetName == "compute_comment" ||
               FacetName == "compute_message") {
      push(S.Context.getPointerType(S.Context.CharTy.withConst()), VK_PRValue);
    } else if (FacetName == "compute_semantic") {
      // Recover the enumeration from the member rather than guessing at it.
      auto *FD =
          Found ? dyn_cast<FunctionDecl>(Found->getUnderlyingDecl()) : nullptr;
      if (!FD || FD->getNumParams() != 1)
        return false;
      push(FD->getParamDecl(0)->getType().getNonReferenceType(), VK_PRValue);
    } else {
      return false;
    }
    return true;
  }
};

/// True when calling Found on Obj with the facet's arguments is viable.
static bool facetCallViable(Sema &S, Expr *Obj, QualType ObjTy,
                            const CXXRecordDecl *RD, LookupResult &R,
                            StringRef FacetName, SourceLocation Loc) {
  Sema::SFINAETrap Trap(S, /*WithAccessChecking=*/true);
  CXXScopeSpec SS;
  ExprResult MR = S.BuildMemberReferenceExpr(
      Obj, ObjTy, Loc, /*IsArrow=*/false, SS,
      /*TemplateKWLoc=*/SourceLocation(),
      /*FirstQualifierInScope=*/nullptr, R, /*TemplateArgs=*/nullptr,
      /*S=*/nullptr);
  if (MR.isInvalid())
    return false;

  FacetProbeArgs Probe;
  if (!Probe.build(S, FacetName, R.getRepresentativeDecl(), Loc))
    return false;

  ExprResult Call = S.BuildCallExpr(/*Scope=*/nullptr, MR.get(), Loc,
                                    Probe.Args, Loc, /*ExecConfig=*/nullptr);
  return !Call.isInvalid();
}

/// Warn when RD has a member named FacetName that almost provides a facet.
///
/// Only two near misses are reported, and each is recognized by relaxing
/// exactly one thing and seeing whether that alone makes the call viable: an
/// inaccessible member, and one that is not const.  Anything else stays
/// silent.  That matters: D3400R5 points out that a label may carry private
/// helpers sharing a facet's name -- tag-dispatch overloads, say -- and
/// warning on a name match alone would fire on every one of them.  Relaxing a
/// single dimension will not make such a helper's signature fit, so it never
/// reaches a warning.
static void warnNearMissFacet(Sema &S, Expr *LabelExpr, QualType LabelTy,
                              const CXXRecordDecl *RD, StringRef FacetName,
                              SourceLocation Loc) {
  DeclarationName Name = &S.Context.Idents.get(FacetName);

  auto lookup = [&](LookupResult &R) {
    return S.LookupQualifiedName(R, const_cast<CXXRecordDecl *>(RD)) &&
           !R.isAmbiguous();
  };

  LookupResult Raw(S, Name, Loc, Sema::LookupMemberName);
  if (!lookup(Raw)) {
    Raw.suppressDiagnostics();
    return;
  }
  Raw.suppressAccessDiagnostics();
  NamedDecl *Rep = Raw.getRepresentativeDecl();

  // The question detection asked: accessible candidates only, on the const
  // label.  If that is viable the facet is present and there is nothing to say.
  LookupResult Accessible(S, Name, Loc, Sema::LookupMemberName);
  if (!lookup(Accessible)) {
    Accessible.suppressDiagnostics();
    return;
  }
  dropInaccessibleFacetCandidates(S, RD, LabelTy, Accessible);
  bool AnyAccessible = !Accessible.empty();
  if (AnyAccessible &&
      facetCallViable(S, LabelExpr, LabelTy, RD, Accessible, FacetName, Loc))
    return;

  enum { Inaccessible, NotConst };

  // Relax access only.
  if (!AnyAccessible &&
      facetCallViable(S, LabelExpr, LabelTy, RD, Raw, FacetName, Loc)) {
    S.Diag(Loc, diag::warn_contract_invalid_label_facet)
        << FacetName << LabelTy.getUnqualifiedType() << Inaccessible;
    if (Rep)
      S.Diag(Rep->getLocation(), diag::note_contract_invalid_label_facet);
    return;
  }

  // Relax const only.  A facet is always invoked on a constexpr, therefore
  // const, control object, so a non-const member can never be one.
  if (AnyAccessible) {
    QualType NonConstTy = LabelTy.getUnqualifiedType();
    OpaqueValueExpr NonConstObj(Loc, NonConstTy, VK_LValue);
    LookupResult Again(S, Name, Loc, Sema::LookupMemberName);
    if (!lookup(Again)) {
      Again.suppressDiagnostics();
      return;
    }
    dropInaccessibleFacetCandidates(S, RD, NonConstTy, Again);
    if (!Again.empty() && facetCallViable(S, &NonConstObj, NonConstTy, RD,
                                          Again, FacetName, Loc)) {
      S.Diag(Loc, diag::warn_contract_invalid_label_facet)
          << FacetName << LabelTy.getUnqualifiedType() << NotConst;
      if (Rep)
        S.Diag(Rep->getLocation(), diag::note_contract_invalid_label_facet);
    }
  }
}

// Try to call label.member_name(semantic_arg) and constant-evaluate.
// Returns the integer result, or -1 on failure.
static int64_t callLabelMethod(Sema &S, Expr *LabelExpr, QualType LabelTy,
                               const CXXRecordDecl *RD, StringRef MethodName,
                               unsigned SemVal, SourceLocation Loc) {
  // Probing must not diagnose: a member of the wrong shape simply means the
  // facet is absent.  The trap covers only the construction of the call --
  // constant evaluation below deliberately runs outside it, because a failure
  // there IS reportable.
  Sema::SFINAETrap Trap(S, /*WithAccessChecking=*/true);
  DeclarationName Name = &S.Context.Idents.get(MethodName);
  LookupResult R(S, Name, Loc, Sema::LookupMemberName);
  if (!S.LookupQualifiedName(R, const_cast<CXXRecordDecl *>(RD)))
    return -1;
  if (R.isAmbiguous()) {
    R.suppressDiagnostics();
    return -1;
  }
  dropInaccessibleFacetCandidates(S, RD, LabelTy, R);
  if (R.empty())
    return -1;

  CXXScopeSpec SS;

  ExprResult MemberRef =
      S.BuildMemberReferenceExpr(LabelExpr, LabelTy, Loc, /*IsArrow=*/false, SS,
                                 /*TemplateKWLoc=*/SourceLocation(),
                                 /*FirstQualifierInScope=*/nullptr, R,
                                 /*TemplateArgs=*/nullptr, /*S=*/nullptr);
  if (MemberRef.isInvalid())
    return -1;

  FunctionDecl *FD = nullptr;
  if (auto *ME = dyn_cast<MemberExpr>(MemberRef.get()))
    FD = dyn_cast<FunctionDecl>(ME->getMemberDecl());
  else if (auto *DRE = dyn_cast<DeclRefExpr>(MemberRef.get()))
    FD = dyn_cast<FunctionDecl>(DRE->getDecl());

  QualType ParamTy;
  if (FD && FD->getNumParams() > 0)
    ParamTy = FD->getParamDecl(0)->getType();
  else
    ParamTy = S.Context.UnsignedCharTy;

  Expr *Arg = IntegerLiteral::Create(S.Context, llvm::APInt(8, SemVal),
                                     S.Context.UnsignedCharTy, Loc);
  Arg = ImplicitCastExpr::Create(S.Context, ParamTy, CK_IntegralCast, Arg,
                                 nullptr, VK_PRValue, FPOptionsOverride());

  ExprResult Call = S.BuildCallExpr(/*Scope=*/nullptr, MemberRef.get(), Loc,
                                    {Arg}, Loc, /*ExecConfig=*/nullptr);
  if (Call.isInvalid())
    return -1;

  Expr::EvalResult Eval;
  if (!evaluateFacetConstant(S, Call.get(), Eval, MethodName, LabelTy, Loc) ||
      !Eval.Val.isInt())
    return -1;
  return Eval.Val.getInt().getExtValue();
}

// Call label.method_name(const char* arg) and constant-evaluate the result.
// Returns the resulting string, or empty StringRef on failure or null result.
static StringRef
callLabelStringMethod(Sema &S, Expr *LabelExpr, QualType LabelTy,
                      const CXXRecordDecl *RD, StringRef MethodName,
                      StringRef CurrentVal, bool IsNull, SourceLocation Loc) {
  // Probing must not diagnose: a member of the wrong shape simply means the
  // facet is absent.  The trap covers only the construction of the call --
  // constant evaluation below deliberately runs outside it, because a failure
  // there IS reportable.
  Sema::SFINAETrap Trap(S, /*WithAccessChecking=*/true);
  DeclarationName Name = &S.Context.Idents.get(MethodName);
  LookupResult R(S, Name, Loc, Sema::LookupMemberName);
  if (!S.LookupQualifiedName(R, const_cast<CXXRecordDecl *>(RD)))
    return {};
  if (R.isAmbiguous()) {
    R.suppressDiagnostics();
    return {};
  }
  dropInaccessibleFacetCandidates(S, RD, LabelTy, R);
  if (R.empty())
    return {};

  CXXScopeSpec SS;

  ExprResult MemberRef =
      S.BuildMemberReferenceExpr(LabelExpr, LabelTy, Loc, /*IsArrow=*/false, SS,
                                 /*TemplateKWLoc=*/SourceLocation(),
                                 /*FirstQualifierInScope=*/nullptr, R,
                                 /*TemplateArgs=*/nullptr, /*S=*/nullptr);
  if (MemberRef.isInvalid())
    return {};

  QualType ConstCharPtrTy =
      S.Context.getPointerType(S.Context.CharTy.withConst());

  Expr *Arg;
  if (IsNull) {
    Arg = ImplicitCastExpr::Create(S.Context, ConstCharPtrTy, CK_NullToPointer,
                                   IntegerLiteral::Create(S.Context,
                                                          llvm::APInt(32, 0),
                                                          S.Context.IntTy, Loc),
                                   nullptr, VK_PRValue, FPOptionsOverride());
  } else {
    QualType ArrTy = S.Context.getStringLiteralArrayType(S.Context.CharTy,
                                                         CurrentVal.size());
    StringLiteral *SL = StringLiteral::Create(S.Context, CurrentVal,
                                              StringLiteralKind::Ordinary,
                                              /*Pascal=*/false, ArrTy, {Loc});
    Arg = ImplicitCastExpr::Create(S.Context, ConstCharPtrTy,
                                   CK_ArrayToPointerDecay, SL, nullptr,
                                   VK_PRValue, FPOptionsOverride());
  }

  ExprResult Call = S.BuildCallExpr(/*Scope=*/nullptr, MemberRef.get(), Loc,
                                    {Arg}, Loc, /*ExecConfig=*/nullptr);
  if (Call.isInvalid())
    return {};

  Expr::EvalResult Eval;
  if (!evaluateFacetConstant(S, Call.get(), Eval, MethodName, LabelTy, Loc))
    return {};

  const APValue &Val = Eval.Val;
  if (!Val.isLValue())
    return {};

  if (Val.getLValueBase().isNull())
    return {};

  if (const auto *Base = Val.getLValueBase().dyn_cast<const Expr *>()) {
    if (const auto *SL = dyn_cast<StringLiteral>(Base))
      return SL->getString();
  }

  return {};
}

// Extract the label's allowed_semantics restriction (bit N = semantic value N
// allowed), probing every valid semantic including "assume" and the D4298
// noexcept_enforce/noexcept_observe variants.  This is the flag-independent
// restriction; the -fcontracts-allow-assume / -fcontracts-p4298 gates are
// applied later, at query construction.  Returns
// AllContractSemanticsMaskWithExtensions (no restriction) if no
// allowed_semantics member exists.
static unsigned extractAllowedMask(Sema &S, Expr *LabelExpr, QualType LabelTy,
                                   const CXXRecordDecl *RD,
                                   SourceLocation Loc) {
  DeclarationName ASName = &S.Context.Idents.get("allowed_semantics");
  LookupResult R(S, ASName, Loc, Sema::LookupMemberName);
  if (!S.LookupQualifiedName(R, const_cast<CXXRecordDecl *>(RD)))
    return AllContractSemanticsMaskWithExtensions;
  if (R.isAmbiguous()) {
    R.suppressDiagnostics();
    return AllContractSemanticsMaskWithExtensions;
  }
  dropInaccessibleFacetCandidates(S, RD, LabelTy, R);
  if (R.empty())
    return AllContractSemanticsMaskWithExtensions;

  // The concept requires `__is_const(decltype(_T::allowed_semantics))'.  A
  // `static constexpr' member is const-qualified already, and so is a plain
  // `const' one; a non-const member is not a facet.  Honouring it anyway
  // narrowed the semantic set for a label the library says has no
  // allowed_semantics facet at all, so a bare label and its combined form
  // disagreed.
  bool AnyConst = false;
  for (NamedDecl *D : R)
    if (auto *VD = dyn_cast<ValueDecl>(D->getUnderlyingDecl()))
      if (VD->getType().isConstQualified())
        AnyConst = true;
  if (!AnyConst)
    return AllContractSemanticsMaskWithExtensions;

  Sema::SFINAETrap Trap(S);
  CXXScopeSpec SS;

  ExprResult ASRef =
      S.BuildMemberReferenceExpr(LabelExpr, LabelTy, Loc, /*IsArrow=*/false, SS,
                                 /*TemplateKWLoc=*/SourceLocation(),
                                 /*FirstQualifierInScope=*/nullptr, R,
                                 /*TemplateArgs=*/nullptr, /*S=*/nullptr);
  if (ASRef.isInvalid())
    return AllContractSemanticsMaskWithExtensions;

  QualType ASTy = ASRef.get()->getType();
  const auto *ASRD = ASTy->getAsCXXRecordDecl();
  if (!ASRD)
    return AllContractSemanticsMaskWithExtensions;

  DeclarationName ContainsName = &S.Context.Idents.get("contains");
  LookupResult CR(S, ContainsName, Loc, Sema::LookupMemberName);
  if (!S.LookupQualifiedName(CR, const_cast<CXXRecordDecl *>(ASRD)))
    return AllContractSemanticsMaskWithExtensions;
  if (CR.isAmbiguous()) {
    CR.suppressDiagnostics();
    return AllContractSemanticsMaskWithExtensions;
  }

  unsigned Mask = 0;
  // Query contains() for each valid evaluation-semantic value (Ignore=1 ..
  // NoexceptObserve=7).  Bit N of the mask corresponds to semantic value N,
  // matching resolveContractSemantic().  Assume and the D4298 noexcept_*
  // variants are probed here too; the flag gates that may exclude them
  // (-fcontracts-allow-assume / -fcontracts-p4298) are applied later, at
  // query construction.
  for (unsigned Sem = 1; Sem <= 7; ++Sem) {
    LookupResult CR2(S, ContainsName, Loc, Sema::LookupMemberName);
    S.LookupQualifiedName(CR2, const_cast<CXXRecordDecl *>(ASRD));

    ExprResult ContainsRef = S.BuildMemberReferenceExpr(
        ASRef.get(), ASTy, Loc, /*IsArrow=*/false, SS,
        /*TemplateKWLoc=*/SourceLocation(),
        /*FirstQualifierInScope=*/nullptr, CR2,
        /*TemplateArgs=*/nullptr, /*S=*/nullptr);
    if (ContainsRef.isInvalid())
      return AllContractSemanticsMaskWithExtensions;

    FunctionDecl *FD = nullptr;
    if (auto *ME = dyn_cast<MemberExpr>(ContainsRef.get()))
      FD = dyn_cast<FunctionDecl>(ME->getMemberDecl());

    QualType ParamTy;
    if (FD && FD->getNumParams() > 0)
      ParamTy = FD->getParamDecl(0)->getType();
    else
      ParamTy = S.Context.UnsignedCharTy;

    Expr *Arg = IntegerLiteral::Create(S.Context, llvm::APInt(8, Sem),
                                       S.Context.UnsignedCharTy, Loc);
    Arg = ImplicitCastExpr::Create(S.Context, ParamTy, CK_IntegralCast, Arg,
                                   nullptr, VK_PRValue, FPOptionsOverride());

    ExprResult Call = S.BuildCallExpr(/*Scope=*/nullptr, ContainsRef.get(), Loc,
                                      {Arg}, Loc, /*ExecConfig=*/nullptr);
    if (Call.isInvalid())
      return AllContractSemanticsMaskWithExtensions;

    Expr::EvalResult Eval;
    if (!Call.get()->EvaluateAsConstantExpr(Eval, S.Context))
      return AllContractSemanticsMaskWithExtensions;

    if (Eval.Val.getInt().getBoolValue())
      Mask |= (1u << Sem);
  }

  return Mask;
}

// Walk an APValue representing a char array (char[N]) and extract its string.
static std::string extractStringFromAPValue(const APValue &Val) {
  std::string Str;
  if (!Val.isArray())
    return Str;
  unsigned InitElts = Val.getArrayInitializedElts();
  for (unsigned I = 0, N = Val.getArraySize(); I < N; ++I) {
    const APValue &Ch =
        (I < InitElts) ? Val.getArrayInitializedElt(I) : Val.getArrayFiller();
    if (Ch.isInt()) {
      char C = static_cast<char>(Ch.getInt().getExtValue());
      if (C == '\0')
        break;
      Str += C;
    }
  }
  return Str;
}

/// Turn a `group_names' array value into the list of names it holds.
static SmallVector<std::string> decodeGroupNameArray(const APValue &Val) {
  if (!Val.isArray())
    return {};

  SmallVector<std::string> Groups;
  unsigned InitElts = Val.getArrayInitializedElts();
  for (unsigned I = 0, N = Val.getArraySize(); I < N; ++I) {
    const APValue &Elt =
        (I < InitElts) ? Val.getArrayInitializedElt(I) : Val.getArrayFiller();
    std::string Str = extractStringFromAPValue(Elt);
    if (!Str.empty())
      Groups.push_back(std::move(Str));
  }
  return Groups;
}

// Extract group names from label's group_names member (identification_label).
// Returns a vector of group name strings; empty if no group_names member.
static SmallVector<std::string> extractGroupNames(Sema &S, Expr *LabelExpr,
                                                  QualType LabelTy,
                                                  const CXXRecordDecl *RD,
                                                  SourceLocation Loc) {
  DeclarationName GNName = &S.Context.Idents.get("group_names");
  LookupResult R(S, GNName, Loc, Sema::LookupMemberName);
  if (!S.LookupQualifiedName(R, const_cast<CXXRecordDecl *>(RD)))
    return {};
  if (R.isAmbiguous()) {
    R.suppressDiagnostics();
    return {};
  }
  // As for the function facets: a member this context cannot name is not a
  // facet, and must not be an error.
  dropInaccessibleFacetCandidates(S, RD, LabelTy, R);
  if (R.empty())
    return {};

  // P3400 requires is_const_v<decltype(t.group_names)>, so that nothing
  // implies a label's group membership could change at run time and have an
  // effect -- the names are read during translation and never again.  An
  // array of const elements is itself a const-qualified type, so both the
  // `static constexpr' and the const-non-static spellings qualify.
  bool AnyConst = false;
  for (NamedDecl *D : R)
    if (auto *VD = dyn_cast<ValueDecl>(D->getUnderlyingDecl()))
      if (VD->getType().isConstQualified())
        AnyConst = true;
  if (!AnyConst)
    return {};

  // group_names may be a *static* data member, which is a VarDecl and so is
  // not part of the label object's value at all.  P3400's own example spells
  // it `static constexpr', and the concept accepts either form, so both have
  // to work.  Reading only fields silently produced no groups, and the
  // label's group-based configuration then never applied: with
  // -fcontract-group-evaluation-semantic=safety:observe the labelled contract
  // stayed at the default semantic and simply never fired.
  for (auto *D : R)
    if (auto *VD = dyn_cast<VarDecl>(D->getUnderlyingDecl()))
      if (const APValue *Val = VD->evaluateValue())
        return decodeGroupNameArray(*Val);

  // Find the field declaration for group_names.
  FieldDecl *GNField = nullptr;
  for (auto *D : R) {
    if (auto *FD = dyn_cast<FieldDecl>(D)) {
      GNField = FD;
      break;
    }
  }
  if (!GNField)
    return {};

  // Constant-evaluate the label expression to get the full struct value,
  // then extract the group_names field.
  Expr::EvalResult Eval;
  if (!LabelExpr->EvaluateAsConstantExpr(Eval, S.Context)) {
    return {};
  }

  const APValue *LabelValPtr = &Eval.Val;

  // If the result is an lvalue (e.g., a DeclRefExpr to a constexpr variable),
  // look through to the stored value of the referenced declaration.
  if (LabelValPtr->isLValue()) {
    APValue::LValueBase Base = LabelValPtr->getLValueBase();
    if (const auto *VD = Base.dyn_cast<const ValueDecl *>()) {
      if (const auto *VarD = dyn_cast<VarDecl>(VD)) {
        const APValue *InitVal = VarD->getEvaluatedValue();
        if (InitVal)
          LabelValPtr = InitVal;
      }
    }
  }

  const APValue &LabelVal = *LabelValPtr;
  if (!LabelVal.isStruct())
    return {};

  // group_names may be a direct field of RD (a plain group label) or inherited
  // from a base subobject (the __combined_identification_label base of an
  // operator|-combined label, which flattens the group names of both operands).
  // Try the direct field first, then fall back to searching bases below.
  const APValue *FieldVal = nullptr;

  unsigned FieldIdx = 0;
  bool FoundDirect = false;
  for (const auto *F : RD->fields()) {
    if (F == GNField) {
      FoundDirect = true;
      break;
    }
    ++FieldIdx;
  }
  if (FoundDirect && FieldIdx < LabelVal.getStructNumFields()) {
    FieldVal = &LabelVal.getStructField(FieldIdx);
  }

  // If not a direct field, search the base subobject that actually owns
  // group_names.  The APValue's bases are ordered to match RD->bases(), so walk
  // them in parallel and only read from the base whose type is the field's
  // owning record (e.g. __combined_identification_label for a combined label).
  if (!FieldVal || FieldVal->isAbsent()) {
    const auto *OwnerRD = dyn_cast<CXXRecordDecl>(GNField->getParent());
    unsigned BaseFieldIdx = 0;
    if (OwnerRD) {
      bool BaseFound = false;
      for (const auto *F : OwnerRD->fields()) {
        if (F == GNField) {
          BaseFound = true;
          break;
        }
        ++BaseFieldIdx;
      }
      if (BaseFound) {
        unsigned BI = 0;
        for (const CXXBaseSpecifier &BaseSpec : RD->bases()) {
          if (BI >= LabelVal.getStructNumBases())
            break;
          const APValue &Base = LabelVal.getStructBase(BI);
          ++BI;
          if (!Base.isStruct())
            continue;
          if (BaseSpec.getType()->getAsCXXRecordDecl() != OwnerRD)
            continue;
          if (BaseFieldIdx < Base.getStructNumFields()) {
            FieldVal = &Base.getStructField(BaseFieldIdx);
            break;
          }
        }
      }
    }
  }

  if (!FieldVal)
    return {};
  return decodeGroupNameArray(*FieldVal);
}

// Resolve contract config for any contract (with or without label).
// Collects groups from label group_names and/or ContractGroupAttr,
// then resolves via the P3595 ordered config system.
static void resolveContractConfig(Sema &S, ContractStmt *CS,
                                  unsigned AllowedMask,
                                  SmallVector<std::string> &Groups) {
  // Collect groups from [[clang::contract_group]] attribute as fallback.
  if (Groups.empty()) {
    if (auto *A = CS->getAttrAs<ContractGroupAttr>())
      Groups.push_back(A->getGroup().str());
  }

  ContractQuery Q;
  Q.Kind = CS->getContractKind();
  Q.CallerSide = false;
  Q.AllowedMask = AllowedMask;
  Q.Groups = Groups;
  Q.FnContext = S.CurContext;
  Q.Loc = CS->getKeywordLoc();
  Q.SM = &S.getSourceManager();

  const auto &Opts = S.Context.getLangOpts().ContractOpts;
  ContractEvaluationSemantic EffectiveSem = Opts.resolveContractSemantic(Q);

  CS->setTransformedSemantic(EffectiveSem);

  // Resolve caller-side semantic.
  ContractQuery CQ;
  CQ.Kind = CS->getContractKind();
  CQ.CallerSide = true;
  CQ.AllowedMask =
      AllowedMask |
      (1u << static_cast<unsigned>(ContractEvaluationSemantic::Ignore));
  CQ.Groups = Groups;
  CQ.FnContext = S.CurContext;
  CQ.Loc = CS->getKeywordLoc();
  CQ.SM = &S.getSourceManager();

  ContractEvaluationSemantic CallerSem = Opts.resolveContractSemantic(CQ);
  CS->setCallerSemantic(CallerSem);
}

// P3595 dynamic selection: when a labeled contract's runtime resolution matches
// a config entry carrying an "output.dynamic" descriptor, precompute the per-
// return-value transform table T(R) = compute_semantic(clamp_to_allowed(R)) for
// R in Ignore..QuickEnforce (1..4) and cache it (plus the descriptor) on the
// stmt.  Codegen (T4) turns this into a dispatch switch; it does no Sema work
// of its own.
//
// This mirrors the eagerly-resolved scalar path (steps 3-5 below) but differs
// in ONE way per P3595 design section 4: a compute_semantic result that lands
// outside the allowed set records the sentinel 0 for that R (-> runtime
// enforced violation in codegen) instead of emitting the compile error.  The
// scalar compile-error path (applyLabelFacets step 5) is unchanged for the
// eagerly-resolved default.
//
// LabelExpr/LabelTy/RD may be null for a dynamic contract with no label facets
// (matched by group/namespace/location); then compute_semantic is skipped and
// T(R) is just the clamp.
static void precomputeDynamicTable(Sema &S, ContractStmt *CS,
                                   unsigned AllowedMask,
                                   ArrayRef<std::string> Groups,
                                   Expr *LabelExpr, QualType LabelTy,
                                   const CXXRecordDecl *RD,
                                   SourceLocation Loc) {
  // As in applyLabelFacets: the label methods are constant-evaluated, and a
  // prvalue label's temporaries must not escape into the enclosing function.
  EnterExpressionEvaluationContext ConstantEvaluated(
      S, Sema::ExpressionEvaluationContext::ConstantEvaluated);
  // Match resolveContractConfig's group resolution: if no groups were supplied
  // (labeled via group_names, or unlabeled), fall back to the
  // [[clang::contract_group]] attribute so the dynamic scan matches the same
  // entry the scalar scan did.
  SmallVector<std::string> GroupsStorage(Groups.begin(), Groups.end());
  if (GroupsStorage.empty()) {
    if (auto *A = CS->getAttrAs<ContractGroupAttr>())
      GroupsStorage.push_back(A->getGroup().str());
  }

  // Build the same runtime query resolveContractConfig used, then ask for the
  // matched entry's dynamic descriptor (never in a constant-evaluation context:
  // a dynamic selector is never called at compile time).
  ContractQuery Q;
  Q.Kind = CS->getContractKind();
  Q.CallerSide = false;
  Q.InConstantEvaluation = false;
  Q.AllowedMask = AllowedMask;
  Q.Groups = GroupsStorage;
  Q.FnContext = S.CurContext;
  Q.Loc = CS->getKeywordLoc();
  Q.SM = &S.getSourceManager();

  const auto &Opts = S.Context.getLangOpts().ContractOpts;
  ContractDynamicResult Dyn = Opts.resolveContractDynamic(Q);
  if (!Dyn.Found)
    return;

  // Does the label carry a compute_semantic facet?  (Only labeled contracts
  // can.)  This governs whether stage b below applies a transform.
  bool HasComputeSemantic = false;
  if (RD) {
    DeclarationName CSName = &S.Context.Idents.get("compute_semantic");
    LookupResult CR(S, CSName, Loc, Sema::LookupMemberName);
    if (S.LookupQualifiedName(CR, const_cast<CXXRecordDecl *>(RD)) &&
        !CR.isAmbiguous())
      HasComputeSemantic = true;
    else
      CR.suppressDiagnostics();
  }

  // Compute T(R) for R in Ignore(1)..QuickEnforce(4).
  uint8_t Table[4] = {0, 0, 0, 0};
  for (unsigned R = 1; R <= 4; ++R) {
    // Stage a: clamp R into the allowed set using the SAME helper
    // resolveContractSemantic uses (fallback order + P3100 assume rule).
    ContractEvaluationSemantic Clamped = ContractOptions::clampToAllowed(
        static_cast<ContractEvaluationSemantic>(R), AllowedMask);

    ContractEvaluationSemantic Effective = Clamped;

    // Stage b: apply compute_semantic (if the label has it) to the clamped
    // value.  A result outside 1..7 means "no transform" -> keep the clamp.
    // (1..7 spans ignore..noexcept_observe; the D4298 noexcept_* results are
    // gated by the AllowedMask check in stage c below.)
    if (HasComputeSemantic) {
      int64_t Computed =
          callLabelMethod(S, LabelExpr, LabelTy, RD, "compute_semantic",
                          static_cast<unsigned>(Clamped), Loc);
      if (Computed >= 1 && Computed <= 7)
        Effective = static_cast<ContractEvaluationSemantic>(Computed);
    }

    // Stage c: unlike the eagerly-resolved scalar path (step 5), a disallowed
    // effective semantic here is NOT a compile error -- it records the sentinel
    // 0, which codegen dispatches to a runtime enforced violation.
    if (AllowedMask & (1u << static_cast<unsigned>(Effective)))
      Table[R - 1] = static_cast<uint8_t>(Effective);
    else
      Table[R - 1] = 0; // sentinel: disallowed transform
  }

  StringRef StoredName = S.Context.backupStr(Dyn.Name);
  CS->setDynamicInfo(StoredName, Dyn.Linkage, Dyn.ProvideWeak, Table);
}

static void applyLabelFacets(Sema &S, ContractStmt *CS) {
  Expr *LabelExpr = CS->getLabelExpr();
  if (!LabelExpr)
    return;
  QualType LabelTy = LabelExpr->getType();
  if (LabelTy->isDependentType())
    return;

  // Every facet below is read by constant-evaluating a member call built on
  // the label expression.  Say so, so that the temporaries that come with a
  // prvalue label -- pre<L{}> rather than pre<named> -- are torn down with
  // this context instead of leaking into the enclosing function, where they
  // trip ActOnFinishFunctionBody's "Unaccounted cleanups in function".
  EnterExpressionEvaluationContext ConstantEvaluated(
      S, Sema::ExpressionEvaluationContext::ConstantEvaluated);

  SourceLocation Loc = CS->getKeywordLoc();

  // An assertion-control label (P3400) must be a value of a class type that has
  // a member type named 'assertion_control_object'.  Reject anything else --
  // a non-class type (e.g. a bool comparison result or an integer), or a class
  // that lacks the marker member type.
  const auto *RD = LabelTy->getAsCXXRecordDecl();
  bool IsValidLabel = false;
  if (RD) {
    DeclarationName ACOName = &S.Context.Idents.get("assertion_control_object");
    LookupResult ACO(S, ACOName, Loc, Sema::LookupOrdinaryName);
    if (S.LookupQualifiedName(ACO, const_cast<CXXRecordDecl *>(RD)) &&
        !ACO.isAmbiguous() && ACO.getAsSingle<TypeDecl>())
      IsValidLabel = true;
    ACO.suppressDiagnostics();
  }
  if (!IsValidLabel) {
    S.Diag(Loc, diag::err_contract_invalid_label)
        << LabelTy << LabelExpr->getSourceRange();
    return;
  }

  // Step 1: Extract the flag-independent label restriction from the label's
  // allowed_semantics facet.
  unsigned LabelMask = extractAllowedMask(S, LabelExpr, LabelTy, RD, Loc);

  // Step 2: Extract group names from identification_label facet.
  SmallVector<std::string> LabelGroups =
      extractGroupNames(S, LabelExpr, LabelTy, RD, Loc);

  // Step 3: Apply the -fcontracts-allow-assume gate to get the effective
  // allowed set (base gated by the flag, intersected with the label), then
  // eagerly resolve.  Labeled contracts must resolve eagerly because label
  // facets (groups, compute_semantic) require Sema for evaluation.  The
  // flag-independent restriction is stored so the query builders re-apply the
  // gate uniformly (e.g. for template instantiations).
  unsigned AllowedMask =
      LabelMask & gatedContractSemanticsMask(
                      S.Context.getLangOpts().ContractOpts.AllowAssume,
                      S.Context.getLangOpts().ContractsP4298);
  if (AllowedMask == 0) {
    S.Diag(Loc, diag::err_typecheck_bool_condition)
        << "assertion-control label allows no evaluation semantics";
    return;
  }
  CS->setAllowedMask(LabelMask);
  resolveContractConfig(S, CS, AllowedMask, LabelGroups);

  ContractEvaluationSemantic EffectiveSem = CS->getSemantic(S.Context);

  // Step 4: Apply compute_semantic transformation if present.  It may return
  // any valid semantic, including assume and the D4298 noexcept_* variants
  // (1..7); a result outside the allowed set is diagnosed in Step 5.
  int64_t ComputeResult =
      callLabelMethod(S, LabelExpr, LabelTy, RD, "compute_semantic",
                      static_cast<unsigned>(EffectiveSem), Loc);
  if (ComputeResult >= 1 && ComputeResult <= 7)
    EffectiveSem = static_cast<ContractEvaluationSemantic>(ComputeResult);

  // Step 5: A compute_semantic result outside the effective allowed set is an
  // error -- including assume when -fcontracts-allow-assume is not set, since
  // the gate keeps assume out of the set entirely.
  if (!(AllowedMask & (1u << static_cast<unsigned>(EffectiveSem)))) {
    S.Diag(Loc, diag::err_typecheck_bool_condition)
        << "compute_semantic result is not in the allowed evaluation "
           "semantics";
    return;
  }

  if (EffectiveSem != CS->getSemantic(S.Context))
    CS->setTransformedSemantic(EffectiveSem);

  // Step 5b: P3595 dynamic selection.  If this contract's runtime resolution
  // matches a config entry with an "output.dynamic" descriptor, precompute the
  // per-return-value transform table for codegen.  The eagerly-resolved scalar
  // above remains the compile-time default (weak-def value + constant
  // evaluation); this is a separate, per-return-value computation whose
  // disallowed results become the runtime sentinel rather than a compile error.
  precomputeDynamicTable(S, CS, AllowedMask, LabelGroups, LabelExpr, LabelTy,
                         RD, Loc);

  // Also eagerly resolve CE semantic for labeled contracts.
  {
    ContractQuery CEQ;
    CEQ.Kind = CS->getContractKind();
    CEQ.CallerSide = false;
    CEQ.InConstantEvaluation = true;
    CEQ.AllowedMask = AllowedMask;
    CEQ.Groups = LabelGroups;
    CEQ.FnContext = S.CurContext;
    CEQ.Loc = CS->getKeywordLoc();
    CEQ.SM = &S.getSourceManager();

    const auto &Opts = S.Context.getLangOpts().ContractOpts;
    ContractEvaluationSemantic CESem = Opts.resolveContractSemantic(CEQ);

    if (ComputeResult >= 0) {
      int64_t CECompute =
          callLabelMethod(S, LabelExpr, LabelTy, RD, "compute_semantic",
                          static_cast<unsigned>(CESem), Loc);
      if (CECompute >= 1 && CECompute <= 7 &&
          (AllowedMask & (1u << static_cast<unsigned>(CECompute))))
        CESem = static_cast<ContractEvaluationSemantic>(CECompute);
    }
    CS->setCESemantic(CESem);
  }

  // Report members that look like they were meant to be facets but are not.
  // Once per label type: the answer depends only on the type, and a label is
  // typically named by many contracts.
  if (!S.getDiagnostics().isIgnored(diag::warn_contract_invalid_label_facet,
                                    Loc) &&
      S.ContractNearMissCheckedLabels.insert(RD).second) {
    static constexpr StringRef Facets[] = {
        "compute_semantic", "compute_comment", "compute_message",
        "handle_contract_violation", "query"};
    for (StringRef Facet : Facets)
      warnNearMissFacet(S, LabelExpr, LabelTy, RD, Facet, Loc);
  }

  // Step 6: Detect local_violation_label facet.
  {
    // See callLabelMethod: an inaccessible member means the facet is absent.
    Sema::SFINAETrap Trap(S, /*WithAccessChecking=*/true);
    DeclarationName HCVName =
        &S.Context.Idents.get("handle_contract_violation");
    LookupResult HR(S, HCVName, Loc, Sema::LookupMemberName);
    if (S.LookupQualifiedName(HR, const_cast<CXXRecordDecl *>(RD)) &&
        !HR.isAmbiguous()) {
      dropInaccessibleFacetCandidates(S, RD, LabelTy, HR);
      CXXScopeSpec SS;
      ExprResult MR = HR.empty()
                          ? ExprError()
                          : S.BuildMemberReferenceExpr(
                                LabelExpr, LabelTy, Loc, /*IsArrow=*/false, SS,
                                /*TemplateKWLoc=*/SourceLocation(),
                                /*FirstQualifierInScope=*/nullptr, HR,
                                /*TemplateArgs=*/nullptr, /*S=*/nullptr);
      // Forming the member reference is not enough to decide the facet: it
      // succeeds for a member of any signature, and on a const label it
      // succeeds even for a non-const member, because the constness of the
      // implicit object argument is only checked when the call is built.  The
      // concept asks whether `__t.handle_contract_violation(__v)' is valid for
      // a `const _T __t', so build exactly that and see.  Use BuildCallExpr
      // rather than BuildCallToMemberFunction, which asserts on the member
      // reference produced for a static member.
      bool Viable = false;
      if (!MR.isInvalid()) {
        QualType CVTy = getContractViolationType(S, Loc);
        if (!CVTy.isNull()) {
          OpaqueValueExpr CVArg(Loc, CVTy.withConst(), VK_LValue);
          Expr *ArgExprs[] = {&CVArg};
          ExprResult Call =
              S.BuildCallExpr(/*Scope=*/nullptr, MR.get(), Loc, ArgExprs, Loc,
                              /*ExecConfig=*/nullptr);
          Viable = !Call.isInvalid();
        }
      }
      if (Viable) {
        CS->setHasLocalHandler(true);

        // CodeGen's rethrowing-local-handler bypass reads the handler's body to
        // decide whether the predicate needs an EH region at all, and CodeGen
        // has no Sema to instantiate one with.  For a template specialization
        // --
        // __combined_label's handler above all, which is the case the
        // optimization most wants to see -- MarkFunctionReferenced would only
        // queue the definition until end of TU, by which point an eagerly
        // emitted guarded function has already been code-generated without
        // it.  Instantiate it here instead.  This does not cause an
        // instantiation that would not happen anyway: the trampoline odr-uses
        // the handler, so its definition is required in this TU regardless.
        // It only moves that instantiation earlier.
        for (const auto *M : RD->methods()) {
          if (!M->getDeclName().isIdentifier() ||
              M->getName() != "handle_contract_violation")
            continue;
          auto *MD = const_cast<CXXMethodDecl *>(M);
          if (!MD->hasBody() && MD->isTemplateInstantiation())
            S.InstantiateFunctionDefinition(Loc, MD, /*Recursive=*/false,
                                            /*DefinitionRequired=*/false,
                                            /*AtEndOfTU=*/false);
          break;
        }
      }
    } else {
      HR.suppressDiagnostics();
    }
  }

  // Step 6b: Detect queryable_label facet.
  {
    // See callLabelMethod: an inaccessible member means the facet is absent.
    Sema::SFINAETrap Trap(S, /*WithAccessChecking=*/true);
    DeclarationName QName = &S.Context.Idents.get("query");
    LookupResult QR(S, QName, Loc, Sema::LookupMemberName);
    if (S.LookupQualifiedName(QR, const_cast<CXXRecordDecl *>(RD)) &&
        !QR.isAmbiguous()) {
      dropInaccessibleFacetCandidates(S, RD, LabelTy, QR);
      CXXScopeSpec SS;
      ExprResult MR = QR.empty()
                          ? ExprError()
                          : S.BuildMemberReferenceExpr(
                                LabelExpr, LabelTy, Loc, /*IsArrow=*/false, SS,
                                /*TemplateKWLoc=*/SourceLocation(),
                                /*FirstQualifierInScope=*/nullptr, QR,
                                /*TemplateArgs=*/nullptr, /*S=*/nullptr);
      if (!MR.isInvalid()) {
        Expr *QueryMember = MR.get();
        OpaqueValueExpr KeyArg(Loc, S.Context.VoidPtrTy, VK_PRValue);
        OpaqueValueExpr IdxArg(Loc, S.Context.getSizeType(), VK_PRValue);
        Expr *ArgExprs[] = {&KeyArg, &IdxArg};
        // BuildCallToMemberFunction asserts that its callee has bound-member
        // or overload type, which the member reference for a *static* query
        // does not -- it is an ordinary function lvalue -- so calling it here
        // crashed the compiler on a static member.  A static member satisfies
        // the concept (`__t.query(...)' is valid for one), so this has to
        // work.  BuildCallExpr dispatches on the callee's actual form and
        // handles both.
        ExprResult Call =
            S.BuildCallExpr(/*Scope=*/nullptr, QueryMember, Loc, ArgExprs, Loc,
                            /*ExecConfig=*/nullptr);
        if (!Call.isInvalid() && Call.get()->getType()->isVoidPointerType())
          CS->setHasQuery(true);
      }
    } else {
      QR.suppressDiagnostics();
    }
  }

  // Step 7: Apply compute_comment facet.
  {
    std::string Comment = CS->getSourceText(S.Context);
    StringRef Result = callLabelStringMethod(S, LabelExpr, LabelTy, RD,
                                             "compute_comment", Comment,
                                             /*IsNull=*/false, Loc);
    if (!Result.empty())
      CS->setTransformedComment(S.Context.backupStr(Result));
  }

  // Step 8: Apply compute_message facet.
  {
    std::string Msg = CS->getUserMessage(S.Context);
    bool IsNull = Msg.empty() && !CS->hasMessage() &&
                  !CS->getAttrAs<ContractMessageAttr>();
    StringRef Result = callLabelStringMethod(
        S, LabelExpr, LabelTy, RD, "compute_message", Msg, IsNull, Loc);
    if (!Result.empty())
      CS->setTransformedMessage(S.Context.backupStr(Result));
  }
}

Decl *Sema::ActOnPostconditionCapture(Scope *S, SourceLocation IdLoc,
                                      IdentifierInfo *Id, Expr *Init,
                                      bool IsPackExpansion) {
  DeclContext *DC = CurContext;
  // When parsing contracts inline in a class body, DC is the CXXRecordDecl.
  // Captures need an access specifier in that context, and SC_Auto storage
  // to ensure CodeGen treats them as locals (not globals).
  bool InClassContext = isa<CXXRecordDecl>(DC);
  StorageClass CaptureSC = InClassContext ? SC_Auto : SC_None;

  if (!Init) {
    // Plain capture [i] — look up in function parameter scope.
    LookupResult R(*this, Id, IdLoc, LookupOrdinaryName);
    LookupName(R, S);
    auto *PVD = R.getAsSingle<ParmVarDecl>();
    if (!PVD) {
      Diag(IdLoc, diag::err_postcondition_capture_not_parameter);
      return nullptr;
    }

    // A pack-expansion capture [i...] is only well-formed when the captured
    // parameter is itself a function parameter pack.
    if (IsPackExpansion && !PVD->isParameterPack()) {
      Diag(IdLoc, diag::err_pack_expansion_without_parameter_packs);
      return nullptr;
    }

    QualType T = PVD->getType().getNonReferenceType();
    TypeSourceInfo *TInfo = Context.getTrivialTypeSourceInfo(T, IdLoc);
    auto *Cap = PostconditionCaptureDecl::Create(Context, DC, IdLoc, IdLoc, Id,
                                                 T, TInfo, CaptureSC);
    Cap->setIsParameterCapture(true);
    Cap->setIsPackExpansion(IsPackExpansion);
    if (InClassContext)
      Cap->setAccess(AS_public);

    // Build copy-init from the parameter.
    ExprResult CopyInit = BuildDeclRefExpr(
        PVD, PVD->getType().getNonReferenceType(), VK_LValue, IdLoc);
    if (!CopyInit.isInvalid()) {
      if (T->isDependentType()) {
        Cap->setInit(CopyInit.get());
      } else {
        ExprResult InitExpr = PerformCopyInitialization(
            InitializedEntity::InitializeVariable(Cap), IdLoc, CopyInit.get());
        if (!InitExpr.isInvalid())
          Cap->setInit(InitExpr.get());
      }
    }

    return Cap;
  }

  // A pack-expansion init-capture [...old = expr] is only well-formed when the
  // initializer contains an unexpanded parameter pack.
  if (IsPackExpansion && !Init->containsUnexpandedParameterPack()) {
    Diag(IdLoc, diag::err_pack_expansion_without_parameter_packs);
    return nullptr;
  }

  // Init-capture [old = expr] — deduce type from initializer.
  QualType DeducedType = Init->getType();
  if (DeducedType.isNull() || DeducedType->isDependentType()) {
    // In dependent context, just use the expression type as-is.
    DeducedType = Init->getType();
  }

  TypeSourceInfo *TInfo = Context.getTrivialTypeSourceInfo(DeducedType, IdLoc);
  auto *Cap = PostconditionCaptureDecl::Create(Context, DC, IdLoc, IdLoc, Id,
                                               DeducedType, TInfo, CaptureSC);
  Cap->setIsParameterCapture(false);
  Cap->setIsPackExpansion(IsPackExpansion);
  if (InClassContext)
    Cap->setAccess(AS_public);

  // Build a proper copy-init to the deduced capture type, matching the
  // parameter-capture path above, so both capture forms produce a uniform,
  // prvalue-producing initializer for codegen and constant evaluation.
  if (DeducedType->isDependentType()) {
    Cap->setInit(Init);
  } else {
    ExprResult InitExpr = PerformCopyInitialization(
        InitializedEntity::InitializeVariable(Cap), IdLoc, Init);
    if (!InitExpr.isInvalid())
      Cap->setInit(InitExpr.get());
  }

  return Cap;
}

void Sema::ActOnFinishPostconditionCaptures(Scope *S,
                                            ArrayRef<Decl *> Captures) {
  for (Decl *D : Captures) {
    if (auto *Cap = dyn_cast<PostconditionCaptureDecl>(D))
      PushOnScopeChains(Cap, S, /*AddToContext=*/false);
  }
}

StmtResult Sema::BuildContractStmt(ContractKind CK, SourceLocation KeywordLoc,
                                   Expr *Cond, DeclStmt *RND, Expr *Message,
                                   Expr *Label, DeclStmt *Captures,
                                   ArrayRef<const Attr *> Attrs,
                                   Expr *RequiresClause) {
  StmtResult Res =
      ContractStmt::Create(Context, CK, KeywordLoc, Cond, RND, Message, Label,
                           Captures, Attrs, RequiresClause);

  // Populate the dynamic descriptor + label facets here, on the shared hook
  // that BOTH the primary parse (via ActOnContractAssert) and every template
  // instantiation (via TreeTransform::RebuildContractStmt) run through.  This
  // must NOT run while parsing the uninstantiated template body -- the label
  // type is dependent and no config resolution is meaningful yet -- so it is
  // guarded on a non-dependent context.  It then runs exactly once per
  // non-dependent ContractStmt: once on the primary parse of a non-template
  // contract, and once per instantiation of a templated one.  Previously this
  // lived only in ActOnContractAssert, so instantiated contracts silently
  // skipped it: setDynamicInfo was never called, isDynamic() stayed false, and
  // codegen fell back to the static/eager semantic (no dynamic dispatch).
  //
  // Skip population when the contract is going to be rejected as ill-formed.
  // P4283 rejects a requires-clause on a non-templated function: the caller
  // (ActOnContractAssert) diagnoses it and returns StmtError(), discarding this
  // statement.  Detect that exact condition here (it depends only on state
  // available at this point) and skip population, rather than populating state
  // that is then thrown away.
  //
  // CRUCIAL: P4283 is diagnosed ONLY on the primary-parse path
  // (ActOnContractAssert).  The template-instantiation path
  // (TreeTransform::RebuildContractStmt -> BuildContractStmt) never runs that
  // check, and a requires-clause is perfectly legal on a templated contract.
  // At instantiation the substituted clause is non-dependent (and satisfied, or
  // the contract would already have been discarded in
  // InstantiateContractSpecifier), so without the !inTemplateInstantiation()
  // guard below WillBeRejectedByP4283 would be spuriously true and we would
  // skip population for a VALID instantiated contract -- leaving isDynamic()
  // false and falling back to the static semantic (the T6 bug).  Restricting
  // the skip to the primary parse keeps instantiations always populating.  The
  // predicate otherwise mirrors the P4283 check in ActOnContractAssert.
  if (auto *CS = Res.getAs<ContractStmt>()) {
    // Normalize a non-literal (user-generated, P2741-style) diagnostic message
    // to its evaluated string -- getUserMessage() otherwise only understands a
    // StringLiteral message and would return empty for a custom message type
    // (e.g. one with .size()/.data()).  Mirrors static_assert; store the result
    // where getUserMessage() reads it first, before the compute_message facet
    // (in populateContractSemanticState) runs.  Only for a non-dependent
    // message: a dependent one is normalized when the template is instantiated.
    if (Expr *ME = CS->getMessageExpr())
      if (!isa<StringLiteral>(ME) && !ME->isTypeDependent() &&
          !ME->isValueDependent() && !CS->hasTransformedMessage()) {
        std::string Str;
        if (EvaluateAsString(ME, Str, Context,
                             StringEvaluationContext::StaticAssert,
                             /*ErrorOnInvalidMessage=*/true))
          CS->setTransformedMessage(Context.backupStr(Str));
      }

    bool WillBeRejectedByP4283 =
        CS->hasRequiresClause() && !CurContext->isDependentContext() &&
        !CS->getRequiresClause()->isInstantiationDependent() &&
        !inTemplateInstantiation();
    if (!CurContext->isDependentContext() && !WillBeRejectedByP4283)
      populateContractSemanticState(CS);
  }

  return Res;
}

/// Populate a ContractStmt's label-facet and P3595 dynamic-selection state.
///
/// Shared by the primary-parse path (Sema::ActOnContractAssert ->
/// Sema::BuildContractStmt) and the template-instantiation path
/// (TreeTransform::RebuildContractStmt -> Sema::BuildContractStmt).  Callers
/// must only invoke this in a non-dependent context (the label type and any
/// config resolution must be concrete).
void Sema::populateContractSemanticState(ContractStmt *CS) {
  if (CS->hasLabel()) {
    applyLabelFacets(*this, CS);
  } else {
    // P3595 dynamic selection for UNLABELED contracts.  An unlabeled contract
    // never goes through applyLabelFacets, but it can still match a config
    // entry carrying an "output.dynamic" descriptor (by group / namespace /
    // location).  Populate the dynamic descriptor + transform table here so
    // codegen sees it -- mirroring how a labeled contract's dynamic state is
    // populated in applyLabelFacets.
    //
    // An unlabeled contract has no allowed_semantics narrowing and no
    // compute_semantic facet, so the transform table is pure clamp (identity
    // over the full gated set).  precomputeDynamicTable is null-RD-safe and no-
    // ops when the contract does not resolve to a dynamic entry, so
    // non-dynamic contracts are left untouched (isDynamic() stays false).
    unsigned AllowedMask = CS->getAllowedMask() &
                           gatedContractSemanticsMask(
                               Context.getLangOpts().ContractOpts.AllowAssume,
                               Context.getLangOpts().ContractsP4298);
    // Empty Groups: precomputeDynamicTable falls back to the
    // [[clang::contract_group]] attribute, matching resolveContractConfig.
    precomputeDynamicTable(*this, CS, AllowedMask, /*Groups=*/{},
                           /*LabelExpr=*/nullptr, /*LabelTy=*/QualType(),
                           /*RD=*/nullptr, CS->getKeywordLoc());
  }
}

StmtResult Sema::ActOnContractAssert(ContractKind CK, SourceLocation KeywordLoc,
                                     Expr *Cond, ResultNameDecl *RND,
                                     ParsedAttributes &ContractAttrs,
                                     Expr *MessageExpr, Expr *LabelExpr,
                                     DeclStmt *Captures, Expr *RequiresClause) {

  // A contract condition is a full-expression and, like any other, must not
  // contain an unexpanded parameter pack.  Diagnose this here on the primary
  // parse; this path is not taken during template instantiation (which goes
  // through TreeTransform::RebuildContractStmt -> BuildContractStmt), where the
  // pack has already been expanded.  Without this an unexpanded pack such as
  // `pre(args[0])` slips through to instantiation and trips an assertion in
  // DiagnoseUnexpandedParameterPack.  Pack expansions and pack indexing leave
  // no unexpanded pack behind, so they are unaffected.
  if (Cond && DiagnoseUnexpandedParameterPack(Cond))
    return StmtError();

  DeclStmt *RNDStmt = nullptr;
  if (RND) {
    StmtResult NewDeclStmt = ActOnDeclStmt(
        ConvertDeclToDeclGroup(RND), RND->getLocation(), RND->getLocation());

    // FIXME: Can this happen?
    if (NewDeclStmt.isInvalid())
      return StmtError();
    RNDStmt = NewDeclStmt.getAs<DeclStmt>();
  }

  auto BuiltAttrs = SemaContractHelper::buildAttributesWithDummyNode(
      *this, ContractAttrs, KeywordLoc);

  if (MessageExpr) {
    for (auto *A : BuiltAttrs) {
      if (isa<ContractMessageAttr>(A)) {
        Diag(MessageExpr->getBeginLoc(),
             diag::err_contract_message_and_attribute);
        MessageExpr = nullptr;
        break;
      }
    }
  }

  StmtResult Res =
      BuildContractStmt(CK, KeywordLoc, Cond, RNDStmt, MessageExpr, LabelExpr,
                        Captures, BuiltAttrs, RequiresClause);

  if (Res.isInvalid())
    return StmtError();

  if (auto *CS = Res.getAs<ContractStmt>()) {
    // P4283: Diagnose requires clause on non-templated function.
    // NOTE: BuildContractStmt already skipped label-facet / dynamic-descriptor
    // population for exactly this condition (see WillBeRejectedByP4283 there),
    // so no populated state is discarded on this error path.  Keep the two
    // predicates in sync.
    if (CS->hasRequiresClause() && !CurContext->isDependentContext() &&
        !CS->getRequiresClause()->isInstantiationDependent()) {
      Diag(CS->getRequiresClause()->getBeginLoc(),
           diag::err_contract_requires_clause_non_template);
      return StmtError();
    }

    // NOTE: label-facet application and P3595 dynamic-descriptor population no
    // longer happen here.  They are performed in Sema::BuildContractStmt (the
    // shared hook run by both this primary-parse path and the template-
    // instantiation path in TreeTransform::RebuildContractStmt) so that
    // instantiated contracts get the same dynamic/facet state.
  }

  if (RND && RND->getType()->isUndeducedAutoType()) {
    return Res;
  }

  // Deliberately NOT ActOnFinishFullStmt: a contract assertion is not an
  // expression-statement, and the generic full-statement finalization would
  // rewrite it into something that is no longer a ContractStmt.  Whenever
  // Cleanup is still dirty, MaybeCreateStmtWithCleanups wraps its argument in
  // CompoundStmt -> StmtExpr -> ExprWithCleanups; every caller then files the
  // result away with ActionResult::getAs<ContractStmt>(), which is a
  // static_cast, so the wrapper lands in the ContractSpecifierDecl as a bogus
  // ContractStmt and the next walk of the contract list reads garbage.
  //
  // Cleanup is dirty here exactly when the predicate is type-dependent -- so
  // ActOnContractAssertCondition returned early, before the ActOnCondition ->
  // ActOnFinishFullExpr that would otherwise have reset it -- and the
  // predicate nonetheless holds a non-dependent temporary with a non-trivial
  // destructor.
  //
  // Those cleanups must still be discharged rather than merely skipped: the
  // contract's expression evaluation context is potentially-evaluated, so
  // PopExpressionEvaluationContext MERGES its cleanup state into the parent
  // instead of restoring it, and the flag would escape all the way out to
  // ActOnFinishFunctionBody's "Unaccounted cleanups in function" assertion.
  // Discarding them is right, and is the only thing ActOnFinishFullStmt was
  // usefully doing here: a dependent predicate's temporaries are created
  // afresh -- and their cleanups re-registered -- when TransformCondition
  // re-finalizes the predicate at instantiation.  A non-dependent predicate
  // has nothing to discard, because ActOnContractAssertCondition already
  // turned its cleanups into an ExprWithCleanups on the condition itself.
  CleanupVarDeclMarking();
  DiscardCleanupsInEvaluationContext();
  return Res;
}

/// ActOnResultNameDeclarator - Called from Parser::ParseFunctionDeclarator()
/// to introduce parameters into function prototype scope.
ResultNameDecl *Sema::ActOnResultNameDeclarator(ContractKind CK, Scope *S,
                                                QualType RetType,
                                                SourceLocation IDLoc,
                                                IdentifierInfo *II,
                                                unsigned FunctionScopeDepth) {
  // assert(S && S->isContractAssertScope() && "Invalid scope for result name");
  assert(II && "ResultName requires an identifier");

  bool IsInvalid = false;

  // A result name is only meaningful on a postcondition.  Check that FIRST:
  // it is the fundamental error, and `contract_assert` has no return type at
  // all (the parser supplies no resolver), so RetType is null there and none
  // of the return-type reasoning below could run.  Substitute a placeholder
  // type so we can still build a node and keep parsing the predicate.
  if (CK != ContractKind::Post) {
    Diag(IDLoc, diag::err_result_name_not_allowed) << II;
    IsInvalid = true;
    if (RetType.isNull())
      RetType = Context.IntTy;
  } else if (RetType->isVoidType()) {
    // Adjust the type of the result name to be int so we can actually produce a
    // node.
    RetType = Context.IntTy;
    Diag(IDLoc, diag::err_void_result_name) << II;
    IsInvalid = true;
  }

  // If needed, invent a fake placeholder type to represent the result name.
  // This is needed when we encounter a deduced return type on a non-template
  // function. We'll replace this once we've completed the function definition
  // (which must be attached to this declaration).
  bool HasInventedPlaceholderTypes =
      RetType->isUndeducedAutoType() && !RetType->isDependentType();
  if (HasInventedPlaceholderTypes)
    RetType = Context.getAutoType(DeducedKind::DeducedAsDependent, QualType(),
                                  AutoTypeKeyword::Auto);
  auto *New =
      ResultNameDecl::Create(Context, CurContext, IDLoc, II, RetType,
                             HasInventedPlaceholderTypes, FunctionScopeDepth);

  if (IsInvalid)
    New->setInvalidDecl();

  // Check for redeclaration of parameters, e.g. int foo(int x, int x);
  if (II) {
    LookupResult R(*this, II, IDLoc, LookupOrdinaryName,
                   RedeclarationKind::ForVisibleRedeclaration);
    LookupName(R, S);
    if (!R.empty()) {
      NamedDecl *PrevDecl = *R.begin();
      if (R.isSingleResult() && PrevDecl->isTemplateParameter()) {
        // Maybe we will complain about the shadowed template parameter.
        DiagnoseTemplateParameterShadow(IDLoc, PrevDecl);
        // Just pretend that we didn't see the previous declaration.
        PrevDecl = nullptr;
      }
      // FIXME: Diagnose lookup conflicts with lambda captures and
      // parameter declarations.
      // NOTE: PrevDecl is deliberately cleared above for a shadowed template
      // parameter, so this must tolerate null.
      if (auto *PVD = dyn_cast_if_present<ParmVarDecl>(PrevDecl)) {
        Diag(IDLoc, diag::err_result_name_shadows_param) << II;
        Diag(PVD->getLocation(), diag::note_previous_declaration);
        New->setInvalidDecl(true);
      }
    }
  }

  assert(!S || S->isContractAssertScope());

  // Add the parameter declaration into this scope.
  if (S)
    S->AddDecl(New);

  IdResolver.AddDecl(New);

  return New;
}

using ContractScopeRecord = Sema::ContractScopeRecord;

struct ScopeEntry {
  const DeclContext *Ctx = nullptr;

  unsigned FunctionScopeIndex = -1;
  const FunctionScopeInfo *FSI = nullptr;

  unsigned ContractScopeIndex = 0;
  const ContractScopeRecord *CSR = nullptr;

  ScopeEntry(const DeclContext *DC, unsigned FSII, const FunctionScopeInfo *FSI,
             unsigned CSII, const ContractScopeRecord *CSR)
      : Ctx(DC), FunctionScopeIndex(FSII), FSI(FSI), ContractScopeIndex(CSII),
        CSR(CSR) {}

  bool capturesVariable(const ValueDecl *VD) const {
    return getCaptureIfCaptured(VD).has_value();
  }

  std::optional<Capture> getCaptureIfCaptured(const ValueDecl *VD) const {
    assert(FSI);
    auto *CSI = dyn_cast<CapturingScopeInfo>(FSI);
    if (!CSI)
      return std::nullopt;

    if (CSI->isCaptured(const_cast<ValueDecl *>(VD)))
      return CSI->getCapture(const_cast<ValueDecl *>(VD));
    return std::nullopt;
  }
};

struct ScopeWalker {
  explicit ScopeWalker(const Sema &S)
      : S(S), CurCtx(S.CurContext), FunctionScopes(S.FunctionScopes),
        FunctionScopeIndex(FunctionScopes.size()),
        ContractScopeIndex(S.getContractScopes().size()) {
    for (auto &Item : S.getContractScopes())
      ContractScopes.push_back(&Item);
    assert(CurCtx);
  }

  ScopeWalker(ScopeWalker const &) = delete;
  ScopeWalker &operator=(ScopeWalker const &) = delete;

  FunctionScopeInfo *nextFuncScope() {
    --FunctionScopeIndex;
    assert(FunctionScopeIndex < FunctionScopes.size());
    return FunctionScopes[FunctionScopeIndex];
  }

  const ContractScopeRecord *nextContractScope() {
    --ContractScopeIndex;
    assert(ContractScopeIndex < ContractScopes.size());
    return ContractScopes[ContractScopeIndex];
  }

  SmallVector<ScopeEntry> doIt() {
    while (CurCtx) {
      auto *FSI = nextFuncScope();
      assert(FSI);
      const ContractScopeRecord *CSR = nullptr;
      if (FSI->ContractScopeIndex != unsigned(-1)) {
        CSR = &S.ContractScopeStack[FSI->ContractScopeIndex];
        assert(CSR && CSR->FunctionScopeAtPush == FSI);
        Scopes.emplace_back(CurCtx, FunctionScopeIndex, FSI, ContractScopeIndex,
                            CSR);
      } else {
        Scopes.emplace_back(CurCtx, FunctionScopeIndex, FSI, 0, nullptr);
      }

      // We must be in a function declaration
      if (!CurCtx->isFunctionOrMethod()) {
        break;
      }

      CurCtx =
          getLambdaAwareParentOfDeclContext(const_cast<DeclContext *>(CurCtx));
      if (!CurCtx || !CurCtx->isFunctionOrMethod())
        break;
      if (Scopes.size() == FunctionScopes.size())
        break;
    }

    assert((Scopes.size() <= FunctionScopes.size() &&
            Scopes.size() >= S.getFunctionScopes().size()) ||
           (Scopes.size() == FunctionScopes.size() - 1 && CurCtx &&
            CurCtx->isRecord()));
    assert(Scopes.size() >= ContractScopes.size());
    SmallVector<ScopeEntry> Result{Scopes.rbegin(), Scopes.rend()};
    Scopes = std::move(Result);
    return Result;
  }

  const Sema &S;
  const DeclContext *CurCtx;

  SmallVector<FunctionScopeInfo *, 4> FunctionScopes;
  unsigned FunctionScopeIndex;

  SmallVector<const ContractScopeRecord *> ContractScopes;
  unsigned ContractScopeIndex;

  SmallVector<ScopeEntry> Scopes;
};

SmallVector<ScopeEntry> getScopeEntries(const Sema &S) {
  ScopeWalker Walker(S);
  return Walker.doIt();
}

SmallVector<ScopeEntry> getInterveningScopeEntries(const Sema &S,
                                                   const ValueDecl *Var) {
  VarDecl *VD = const_cast<VarDecl *>(dyn_cast<VarDecl>(Var));
  if (!VD)
    return {};
  if (!VD->isLocalVarDeclOrParm())
    return {};

  auto *OldVD = VD;
  VD = VD->getCanonicalDecl();

  SmallVector<ScopeEntry> Result = getScopeEntries(S);
  if (Result.empty())
    return Result;

  const DeclContext *VarCtx = VD->getDeclContext();
  assert(VarCtx && VarCtx->isFunctionOrMethod());

  ArrayRef ScopeEntries = Result;
  assert(std::any_of(Result.begin(), Result.end(),
                     [&](ScopeEntry Ent) { return Ent.Ctx->Equals(VarCtx); }));

  while (!ScopeEntries.empty() && !VarCtx->Equals(ScopeEntries.front().Ctx)) {
    ScopeEntries = ScopeEntries.drop_front(1);
  }

  std::optional<unsigned> LastCopyCaptureIdx;
  unsigned Idx = 0;
  for (auto Pos = ScopeEntries.begin(); Pos != ScopeEntries.end();
       ++Pos, ++Idx) {

    assert(VarCtx->Encloses(Pos->Ctx));
    auto *CSI = dyn_cast<CapturingScopeInfo>(Pos->FSI);
    if (!CSI)
      continue;
    assert(!CSI->isCaptured(OldVD));
    if (CSI && CSI->isCaptured(VD)) {
      Capture C = CSI->getCapture(VD);
      if (C.isCopyCapture()) {
        LastCopyCaptureIdx = Idx;
      }
    }
  }
  if (LastCopyCaptureIdx) {
    ScopeEntries = ScopeEntries.drop_front(LastCopyCaptureIdx.value() + 1);
  }
  return SmallVector<ScopeEntry>(ScopeEntries.begin(), ScopeEntries.end());
}

const ContractScopeRecord *getInterveningContractEntry(const Sema &S,
                                                       const ValueDecl *VD) {
  auto Entries = getInterveningScopeEntries(S, VD);
  for (auto &Ent : Entries) {
    if (Ent.CSR)
      return Ent.CSR;
  }
  return nullptr;
}

bool Sema::CheckEquivalentContractSequence(FunctionDecl *OldDecl,
                                           FunctionDecl *NewDecl) {
  // If the new declaration doesn't contain any contracts, that's fine, they can
  // be omitted.
  if (!NewDecl->hasContracts())
    return false;

  // For explicit specializations, we need to find the first declaration of the
  // explicit specialization itself that has spelled contracts, not the implicit
  // instantiation that may be in the redeclaration chain. The implicit
  // instantiation won't have contracts per [temp.expl.spec]p12.
  FunctionDecl *OrigDecl = nullptr;
  if (NewDecl->getTemplateSpecializationKind() == TSK_ExplicitSpecialization) {
    // If OldDecl is not an explicit specialization, then NewDecl is the first
    // explicit specialization declaration - no previous contracts to compare.
    if (OldDecl->getTemplateSpecializationKind() != TSK_ExplicitSpecialization)
      return false;

    // Find the first explicit specialization declaration that has spelled
    // contracts (i.e., contracts whose DeclContext matches the declaration).
    // Skip the implicit instantiation which won't have contracts.
    for (auto *Redecl : OldDecl->redecls()) {
      if (Redecl->getTemplateSpecializationKind() ==
              TSK_ExplicitSpecialization &&
          Redecl->hasContracts() &&
          Redecl->getContracts()->getDeclContext() == Redecl) {
        OrigDecl = Redecl;
        break;
      }
    }
    // If we couldn't find an explicit specialization with spelled contracts,
    // NewDecl is the first one - nothing to compare.
    if (!OrigDecl)
      return false;
  } else {
    OrigDecl = OldDecl->getCanonicalDecl();
  }

  assert(!NewDecl->getContracts()->isInvalidDecl());
  if (OrigDecl->hasContracts() && OrigDecl->getContracts()->isInvalidDecl()) {
    NewDecl->getContracts()->setInvalidDecl(true);
    NewDecl->setInvalidDecl(true);
    return true;
  }
  ContractSpecifierDecl *OrigContractSpec = OrigDecl->getContracts();
  ContractSpecifierDecl *NewContractSpec = NewDecl->getContracts();
  ArrayRef<const ContractStmt *> OrigContracts, NewContracts;
  if (OrigContractSpec)
    OrigContracts = OrigContractSpec->contracts();

  NewContracts = NewContractSpec->contracts();

  if (OrigContractSpec && OrigContractSpec->isInvalidDecl()) {
    NewContractSpec->setInvalidDecl(true);
    NewDecl->setInvalidDecl(true);
    return true;
  }

  enum DifferenceKind {
    DK_None = -1,
    DK_OrigMissing,
    DK_NumContracts,
    DK_Kind,
    DK_ResultName,
    DK_Cond,
    DK_Captures,
    DK_RequiresClause,
    DK_Message,
    DK_Label
  };
  unsigned ContractIndex = 0;

  // p2900 [basic.contract.func]
  // A declaration E of a function f that is not a first declaration shall have
  // either no function contract-specifier-seq or the same
  // function-contract-specifier-seq as any first declaration D reachable from
  // E.
  const DifferenceKind DK = [&] {
    // Contracts may be omitted from following declarations.
    if (NewContracts.empty())
      return DK_None;

    // ... But if they exist, they must be present on the original declaration.
    if (OrigContracts.empty())
      return DK_OrigMissing;

    if (OrigContracts.size() != NewContracts.size())
      return DK_NumContracts;

    // ... And if they exist on the original declaration, they must be the same.
    for (; ContractIndex < OrigContracts.size(); ++ContractIndex) {
      auto *OC = OrigContracts[ContractIndex];
      auto *NC = NewContracts[ContractIndex];

      if (OC->getContractKind() != NC->getContractKind())
        return DK_Kind;
      if (OC->hasResultName() != NC->hasResultName())
        return DK_ResultName;
      if (!Context.hasSameExpr(OC->getCond(), NC->getCond()))
        return DK_Cond;
      if (OC->hasCaptures() != NC->hasCaptures())
        return DK_Captures;
      if (OC->hasCaptures()) {
        auto ODecls = OC->getCapturesDeclStmt()->decls();
        auto NDecls = NC->getCapturesDeclStmt()->decls();
        auto OIt = ODecls.begin();
        auto NIt = NDecls.begin();
        for (; OIt != ODecls.end() && NIt != NDecls.end(); ++OIt, ++NIt) {
          auto *OCap = cast<PostconditionCaptureDecl>(*OIt);
          auto *NCap = cast<PostconditionCaptureDecl>(*NIt);
          if (OCap->getName() != NCap->getName())
            return DK_Captures;
          if (OCap->hasInit() != NCap->hasInit())
            return DK_Captures;
          if (OCap->hasInit() &&
              !Context.hasSameExpr(OCap->getInit(), NCap->getInit()))
            return DK_Captures;
        }
        if (OIt != ODecls.end() || NIt != NDecls.end())
          return DK_Captures;
      }
      if (OC->hasRequiresClause() != NC->hasRequiresClause())
        return DK_RequiresClause;
      if (OC->hasRequiresClause() &&
          !Context.hasSameExpr(OC->getRequiresClause(),
                               NC->getRequiresClause()))
        return DK_RequiresClause;
      // Diagnostic-message sameness (P3099) is based on the extracted message
      // *text*, not the expression structure (a constexpr message producing
      // different text on different lines is a mismatch).
      if (OC->getUserMessage(Context) != NC->getUserMessage(Context))
        return DK_Message;
      // Assertion-control label sameness (P3400).
      if (OC->hasLabel() != NC->hasLabel())
        return DK_Label;
      if (OC->hasLabel() &&
          !Context.hasSameExpr(OC->getLabelExpr(), NC->getLabelExpr()))
        return DK_Label;
    }
    return DK_None;
  }();

  // Nothing to diagnose.
  if (DK == DK_None)
    return false;

  NewContractSpec->setInvalidDecl(true);

  assert(!NewContracts.empty() && "Cannot diagnose empty contract sequence");
  SourceRange NewContractRange = SourceRange(
      NewContracts.front()->getBeginLoc(), NewContracts.back()->getEndLoc());
  SourceRange OrigContractRange =
      OrigContracts.empty()
          ? SourceRange(OrigDecl->getEndLoc(), OrigDecl->getEndLoc())
          : SourceRange(OrigContracts.front()->getBeginLoc(),
                        OrigContracts.back()->getEndLoc());

  // Otherwise, we're producing a diagnostic.
  Diag(NewDecl->getLocation(), diag::err_function_different_contract_seq)
      << isa<CXXMethodDecl>(NewDecl) << NewContractRange;

  if (DK == DK_NumContracts || DK == DK_OrigMissing) {
    int PluralSelect =
        OrigContracts.empty() + (OrigContracts.size() < NewContracts.size());
    Diag(OrigDecl->getLocation(), diag::note_contract_spec_seq_arity_mismatch)
        << PluralSelect << OrigContracts.size() << NewContracts.size()
        << OrigContractRange;
    return true;
  }

  auto *OC = OrigContracts[ContractIndex];
  auto *NC = NewContracts[ContractIndex];

  auto GetRangeForNote = [&](const ContractStmt *CS) {
    switch (DK) {
    case DK_Kind:
      return SourceRange(CS->getBeginLoc(), CS->getBeginLoc());
    case DK_ResultName:
      return CS->hasResultName() ? CS->getResultName()->getSourceRange()
                                 : CS->getCond()->getSourceRange();
    case DK_Cond:
      return CS->getCond()->getSourceRange();
    case DK_Captures:
      return CS->hasCaptures() ? CS->getCapturesDeclStmt()->getSourceRange()
                               : CS->getCond()->getSourceRange();
    case DK_RequiresClause:
      return CS->hasRequiresClause() ? CS->getRequiresClause()->getSourceRange()
                                     : CS->getCond()->getSourceRange();
    case DK_Message:
      return CS->hasMessage() ? CS->getMessageExpr()->getSourceRange()
                              : CS->getCond()->getSourceRange();
    case DK_Label:
      return CS->hasLabel() ? CS->getLabelExpr()->getSourceRange()
                            : CS->getCond()->getSourceRange();
    case DK_OrigMissing:
    case DK_NumContracts:
    case DK_None:
      llvm_unreachable("unhandled enum value");
    }
    llvm_unreachable("unhandled enum value");
  };

  Diag(NC->getBeginLoc(), diag::note_mismatched_contract)
      << GetRangeForNote(NC);
  Diag(OC->getBeginLoc(), diag::note_previous_contracts)
      << DK << (int)OC->getContractKind() << OC->hasResultName()
      << GetRangeForNote(OC);

  return true;
}

namespace {
/// A checker which white-lists certain expressions whose conversion
/// to or from retainable type would otherwise be forbidden in ARC.
struct ParamReferenceChecker : RecursiveASTVisitor<ParamReferenceChecker> {
  typedef RecursiveASTVisitor<ParamReferenceChecker> super;

private:
  Sema &Actions;
  const FunctionDecl *FD;

public:
  ContractStmt *CurrentContract = nullptr;
  ContractSpecifierDecl *CSD = nullptr;

public:
  ParamReferenceChecker(Sema &S, const FunctionDecl *FD)
      : Actions(S), FD(FD), CSD(FD->getContracts()) {
    assert(CSD);
  }

  bool TraverseContractStmt(ContractStmt *CS) {
    ValueRAII<ContractStmt *> SetCurrent(CurrentContract, CS,
                                         CurrentContract == nullptr);
    if (CS->getCond())
      TraverseStmt(CS->getCond());
    return true;
  }

  // [expr.context]: the left operand of a comma is a discarded-value
  // expression, and the lvalue-to-rvalue conversion is applied to one only
  // when it is a volatile glvalue.  So by [basic.def.odr]/5 a parameter that
  // is among its POTENTIAL RESULTS is not odr-used, and
  // [dcl.contract.func]/7 does not reach it -- PR126897,
  // `void f (bool b) post ((b, true)) {}`.
  bool TraverseBinaryOperator(BinaryOperator *E) {
    if (E->getOpcode() != BO_Comma)
      return super::TraverseBinaryOperator(E);
    if (!TraverseDiscardedValue(E->getLHS()))
      return false;
    return TraverseStmt(E->getRHS());
  }

  // Traverse E as a discarded-value expression.  Its potential results
  // ([basic.def.odr]/2) are exempt; everything else in it is traversed
  // normally, because a subexpression that is not a potential result -- an
  // argument to a call, the condition of a ?: -- is evaluated and odr-uses
  // whatever it names.  So `(f (b), true)` still diagnoses `b`.
  //
  // The AST already draws this line where the wording does: in `(b, true)`
  // the operand is a bare lvalue DeclRefExpr, while in `(true, b)` the
  // LValueToRValue conversion wraps the whole comma -- so the potential
  // result there IS converted, and stays an odr-use.
  bool TraverseDiscardedValue(Expr *E) {
    if (!E)
      return true;

    Expr *S = E->IgnoreParens();

    // A cast to void makes its operand a discarded-value expression
    // ([expr.static.cast]).
    if (auto *CE = dyn_cast<CastExpr>(S))
      if (CE->getCastKind() == CK_ToVoid)
        return TraverseDiscardedValue(CE->getSubExpr());

    if (auto *BO = dyn_cast<BinaryOperator>(S))
      if (BO->getOpcode() == BO_Comma) {
        // `(a, b)` discarded: b supplies the potential results, and a is
        // itself a discarded-value expression.  `(x, y, true)` parses as
        // `((x, y), true)`, so this has to recurse.
        if (!TraverseDiscardedValue(BO->getLHS()))
          return false;
        return TraverseDiscardedValue(BO->getRHS());
      }

    if (auto *CO = dyn_cast<ConditionalOperator>(S)) {
      // The condition is evaluated for its value; the arms supply the
      // potential results.
      if (!TraverseStmt(CO->getCond()))
        return false;
      if (!TraverseDiscardedValue(CO->getTrueExpr()))
        return false;
      return TraverseDiscardedValue(CO->getFalseExpr());
    }

    // A potential result naming a parameter: exempt, and nothing below it
    // needs visiting.
    if (isa<DeclRefExpr>(S))
      return true;

    return TraverseStmt(E);
  }

  bool TraversePackIndexingExpr(PackIndexingExpr *E) {
    if (!super::TraversePackIndexingExpr(E))
      return false;
    // A pack-indexing expression (args...[i]) odr-uses only the *selected*
    // element.  That element lives among the trailing substituted expressions,
    // not among children() -- which the base visitor traverses and which hold
    // only the dependent pack pattern and the index.  Traverse the selected
    // element so its parameter reference reaches VisitDeclRefExpr and is
    // checked.  In an uninstantiated template the index is not yet known (there
    // is no selected element); the dependent parameter is diagnosed once the
    // pack-indexing expression is substituted at instantiation.
    if (E->isFullySubstituted())
      return TraverseStmt(E->getSelectedExpr());
    return true;
  }

  enum DiagSelector {
    DS_None = -1,
    DS_Array = 0,
    DS_Function = 1,
    DS_NotConst = 2
  };

  /// Is USAGE an odr-use of one of FD's own non-reference parameters?  That is
  /// what makes [dcl.contract.func]'s const requirement apply at all, and it
  /// is a separate question from whether any particular declaration's
  /// parameter satisfies it -- which is why diagnoseRedeclParamConst needs it
  /// too.
  bool isOwnValueParmOdrUse(const ParmVarDecl *PVD,
                            const DeclRefExpr *Usage) const {
    if (PVD->getFunctionScopeIndex() >= FD->getNumParams() ||
        FD->getParamDecl(PVD->getFunctionScopeIndex()) != PVD)
      return false;
    // [dcl.contract.func] p2900r8 --
    //   If a  postcondition  odr-uses ([basic.def.odr]) a non-reference
    //   parameter...
    return !PVD->getType()->isReferenceType() && !Usage->isNonOdrUse();
  }

  DiagSelector classifyDiagnosableParmVar(const ParmVarDecl *PVD,
                                          const DeclRefExpr *Usage) const {
    // We only care about parameter's for the function with the contracts we're
    // evaluating. Ensure this parameter belongs to that function.
    if (![&] {
          if (PVD->getFunctionScopeIndex() < FD->getNumParams())
            return FD->getParamDecl(PVD->getFunctionScopeIndex()) == PVD;
          return false;
        }())
      return DS_None;

    // We'll diagnose this when it's non-dependent
    if (PVD->getType()->isDependentType())
      return DS_None;

    // Skip diagnosing this parameter if we've already done it.
    if (DiagnosedDecls.count(PVD))
      return DS_None;

    // [dcl.contract.func] p2900r8 --
    //   If a  postcondition  odr-uses ([basic.def.odr]) a non-reference
    //   parameter...
    if (PVD->getType()->isReferenceType() || Usage->isNonOdrUse())
      return DS_None;

    QualType PVDType = PVD->getOriginalType();

    // that parameter shall not have an array type...
    if (PVDType->isArrayType() || PVDType->isArrayParameterType())
      return DS_Array;
    assert(!PVD->getOriginalType()->isArrayType());

    // or function type...
    if (PVDType->isFunctionPointerType())
      return DS_Function;

    // ...and that parameter shall be declared const
    if (!PVDType.isConstQualified())
      return DS_NotConst;

    return DS_None;
  }

  bool VisitDeclRefExpr(DeclRefExpr *E) {
    assert(CurrentContract && "No current contract to use in diagnostics?");

    // [dcl.contract.func] p2900r8 --
    //   If a  postcondition  odr-uses ([basic.def.odr]) a non-reference
    //   parameter, ... that parameter shall be declared const and shall not
    //   have array or function type. [ Note: This requirement applies even to
    //   declarations that do not specify the postcondition-specifier.]
    auto *PVD = dyn_cast_or_null<ParmVarDecl>(E->getDecl());
    if (!PVD || PVD->getType()->isDependentType() || DiagnosedDecls.count(PVD))
      return true;

    // Record it whether or not THIS declaration's parameter is const: the rule
    // reaches the corresponding parameter on every declaration, and the one
    // that violates it may have been merged away.
    if (isOwnValueParmOdrUse(PVD, E))
      OdrUsedParms.try_emplace(PVD->getFunctionScopeIndex(), E->getLocation());

    if (DiagSelector DiagSelect = classifyDiagnosableParmVar(PVD, E);
        DiagSelect != DS_None) {
      DiagnosedDecls.insert(PVD);
      CSD->setInvalidDecl(true);

      Actions.Diag(E->getLocation(),
                   diag::err_contract_postcondition_parameter_type_invalid)
          << PVD->getIdentifier() << DiagSelect
          << CurrentContract->getSourceRange();
      Actions.Diag(PVD->getTypeSpecStartLoc(), diag::note_parameter_type)
          << (DiagSelect == DS_NotConst ? PVD->getType()
                                        : PVD->getOriginalType())
          << PVD->getSourceRange();
    }

    return true;
  }

  /// Index -> location of the first odr-use, for each of FD's own
  /// non-reference parameters a postcondition odr-uses.
  llvm::MapVector<unsigned, SourceLocation> OdrUsedParms;

private:
  DenseSet<ParmVarDecl *> DiagnosedDecls;
};

} // namespace

/// [dcl.contract.func]: a parameter a postcondition odr-uses must have const
/// type on that declaration "and the corresponding parameter on all
/// declarations of f".  A declaration that violates it may no longer exist by
/// the time the question can be answered: redeclarations are merged, one set
/// of parameters survives, and with a DEPENDENT parameter type there is
/// nothing to judge until the template arguments are known.  So consult the
/// other declarations of the pattern here, substituting each one's written
/// parameter type with this specialization's arguments.
///
/// Substituting is what makes this correct rather than approximate.  The
/// declarations may disagree about writing `const` and still both be const
/// after substitution -- `f<const int>` for `T a` and `T const a` -- so
/// comparing the written qualifiers would reject a well-formed program.
static void
diagnoseRedeclParamConst(Sema &S, FunctionDecl *FD,
                         const llvm::MapVector<unsigned, SourceLocation> &Used) {
  if (Used.empty())
    return;

  // Only an instantiation can have lost a declaration this way.  Every
  // declaration of a non-template function is checked as written, when it is
  // written.
  FunctionTemplateDecl *FTD = FD->getPrimaryTemplate();
  if (!FTD || !FD->getTemplateSpecializationArgs())
    return;

  for (const auto &[Idx, UseLoc] : Used) {
    // Already ill-formed on its own account and diagnosed there; saying it
    // twice for one parameter helps nobody.  This exists for the case where
    // the surviving parameter looks fine and another declaration did not.
    if (!FD->getParamDecl(Idx)->getType().isConstQualified())
      continue;

    // redecls() on a RedeclarableTemplateDecl yields the base type.
    for (auto *RD : FTD->redecls()) {
      const FunctionDecl *Pattern =
          cast<FunctionTemplateDecl>(RD)->getTemplatedDecl();

      // A pack breaks the index correspondence this relies on: one written
      // parameter becomes N in the instantiation, so Idx does not name the
      // same parameter on both sides.  Packs are covered where they are
      // expanded, not here.
      if (Pattern->getNumParams() != FD->getNumParams())
        continue;
      if (Idx >= Pattern->getNumParams())
        continue;

      const ParmVarDecl *P = Pattern->getParamDecl(Idx);
      if (P->isParameterPack())
        continue;

      QualType T = P->getType();
      if (T->isReferenceType() || T->containsUnexpandedParameterPack())
        continue;

      if (T->isDependentType()) {
        Sema::InstantiatingTemplate Inst(S, UseLoc, FD);
        if (Inst.isInvalid())
          return;
        Sema::SFINAETrap Trap(S);
        // Exactly one level, holding this function template's own arguments.
        // The pattern belongs to an already-instantiated enclosing class if
        // there is one, so its template parameters are at depth 0 either way;
        // adding the class's arguments as an outer level made `U` in
        // Foo<int>::bar<const int> resolve to the class's `int` and rejected a
        // well-formed program.
        MultiLevelTemplateArgumentList TemplateArgs(
            FTD, FD->getTemplateSpecializationArgs()->asArray(),
            /*Final=*/true);
        QualType Subst =
            S.SubstType(T, TemplateArgs, P->getLocation(), P->getDeclName());
        if (Subst.isNull() || Trap.hasErrorOccurred())
          continue;
        // Still dependent means this declaration's parameter type mentions
        // something these arguments do not supply; there is nothing to judge.
        if (Subst->isDependentType())
          continue;
        T = Subst;
      }

      if (T.isConstQualified())
        continue;

      S.Diag(UseLoc, diag::err_contract_postcondition_parameter_type_invalid)
          << FD->getParamDecl(Idx)->getIdentifier() << /*must be const*/ 2;
      S.Diag(P->getTypeSpecStartLoc(), diag::note_parameter_type)
          << T << P->getSourceRange();
      FD->getContracts()->setInvalidDecl(true);
      FD->setInvalidDecl(true);
      break;
    }
  }
}

static void diagnoseParamTypes(Sema &S, FunctionDecl *FD,
                               ContractSpecifierDecl *CSD) {

  // Diagnose postconditions that odr-use a non-reference parameter of
  // non-const, array, or function type ([dcl.contract.func]).  The checker
  // emits the diagnostics as it traverses each postcondition predicate.
  ParamReferenceChecker Checker(S, FD);
  for (auto *CS : CSD->postconditions())
    Checker.TraverseContractStmt(CS);

  diagnoseRedeclParamConst(S, FD, Checker.OdrUsedParms);
}

namespace {
/// Finds an odr-use of a non-reference parameter of FD in a postcondition.
class CoroutineParamUseFinder
    : public RecursiveASTVisitor<CoroutineParamUseFinder> {
  const FunctionDecl *FD;

public:
  llvm::SmallVector<const ParmVarDecl *, 4> Found;

  explicit CoroutineParamUseFinder(const FunctionDecl *FD) : FD(FD) {}

  bool VisitDeclRefExpr(DeclRefExpr *E) {
    const auto *PVD = dyn_cast_or_null<ParmVarDecl>(E->getDecl());
    if (!PVD || E->isNonOdrUse())
      return true;
    // A reference parameter is fine: [dcl.fct.def.coroutine]/5 binds its copy
    // to the same object.
    if (PVD->getType()->isReferenceType())
      return true;
    // Only this function's own parameters.
    if (PVD->getFunctionScopeIndex() >= FD->getNumParams() ||
        FD->getParamDecl(PVD->getFunctionScopeIndex()) != PVD)
      return true;
    if (!llvm::is_contained(Found, PVD))
      Found.push_back(PVD);
    return true;
  }
};
} // namespace

void Sema::diagnoseCoroutinePostconditionParams(FunctionDecl *FD) {
  const ContractSpecifierDecl *CSD = FD->getContracts();
  if (!CSD)
    return;

  // [dcl.contract.func] requires a non-reference parameter odr-used in a
  // postcondition to be const, and [dcl.fct.def.coroutine]/5 makes the
  // coroutine's copy of such a parameter direct-initialized from an xvalue of
  // the UNQUALIFIED type -- which cannot be formed from a const parameter.
  // The two cannot both be satisfied, and [dcl.fct.def.coroutine] says so
  // directly: "An odr-use of a non-reference parameter in a postcondition
  // assertion of a coroutine is ill-formed."
  //
  // So the non-const spelling is rejected by the const rule and the const
  // spelling by this one; there is no third.  A precondition is unaffected,
  // being outside the const rule.
  CoroutineParamUseFinder Finder(FD);
  for (auto *CS : CSD->postconditions())
    Finder.TraverseStmt(CS);

  for (const ParmVarDecl *PVD : Finder.Found) {
    Diag(PVD->getLocation(), diag::err_contract_coroutine_postcondition_param)
        << PVD;
    Diag(PVD->getLocation(),
         diag::note_contract_coroutine_postcondition_param);
    FD->setInvalidDecl();
  }
}

void Sema::CheckFunctionContracts(FunctionDecl *FD, bool IsDefinition,
                                  bool IsInstantiation) {
  assert(FD && FD->hasContracts());

  if (auto *MD = dyn_cast<CXXMethodDecl>(FD)) {
    if (MD->isVirtual() && !getLangOpts().ContractsP3097) {
      Diag(FD->getLocation(), diag::err_contracts_on_virtual_require_flag);
      return;
    }
  }

  diagnoseParamTypes(*this, FD, FD->getContracts());
}

bool Sema::holdsPatternContractSpecifier(const FunctionDecl *Instantiation,
                                         const FunctionDecl *Pattern) {
  const ContractSpecifierDecl *CSD = Instantiation->getContracts();
  if (!CSD)
    return false;
  for (const FunctionDecl *RD : Pattern->redecls())
    if (RD->getContracts() == CSD)
      return true;
  return false;
}

void Sema::InstantiateContractSpecifier(
    SourceLocation PointOfInstantiation, FunctionDecl *Instantiation,
    const FunctionDecl *Pattern,
    const MultiLevelTemplateArgumentList &TemplateArgs) {

  ContractSpecifierDecl *PatternCSD = Pattern->getContracts();
  if (!PatternCSD)
    return;

  // Idempotent: at class-template instantiation the member is given a
  // pattern's own (dependent) contract specifier as a placeholder
  // (VisitCXXMethodDecl).  Once we have replaced it with a substituted
  // specifier, the instantiation carries a ContractSpecifierDecl that belongs
  // to no redeclaration chain, so there is nothing more to do.  This lets the
  // contracts be instantiated on-demand at the point of an odr-use (see
  // InstantiateVirtualFunctionContractsOnUse) without being re-substituted
  // when the function's definition is later instantiated.
  //
  // Ask the whole chain, not just PatternCSD: for a member declared in-class
  // and defined out-of-line the placeholder comes from the declaration while
  // PatternCSD is the definition's own re-pointed copy, so comparing against
  // PatternCSD alone reads the placeholder as already-substituted and leaves
  // the instantiation holding a predicate that references the *pattern's*
  // parameters -- which CodeGen then trips over.
  if (Instantiation->getContracts() &&
      !holdsPatternContractSpecifier(Instantiation, Pattern))
    return;

  bool IsInvalid = false;

  auto *Method = const_cast<CXXMethodDecl *>(
      dyn_cast_if_present<CXXMethodDecl>(Instantiation));

  Sema::ContextRAII SavedContext(*this, Instantiation);
  Sema::CXXThisScopeRAII ThisxScope(
      SemaRef, Method ? Method->getParent() : nullptr,
      Method ? Method->getMethodQualifiers().withConst() : Qualifiers{},
      Method != nullptr);

  LocalInstantiationScope Scope(*this, true);

  SmallVector<ContractStmt *> NewContracts;
  for (auto *CS : PatternCSD->contracts()) {
    // P4283: Check requires clause BEFORE substituting the contract.
    // If the constraint is not satisfied, discard the contract entirely.
    if (CS->hasRequiresClause()) {
      Expr *RC = CS->getRequiresClause();
      ExprResult SubstRC = SubstExpr(RC, TemplateArgs);
      if (!SubstRC.isInvalid() && SubstRC.get()) {
        Expr *SubRC = SubstRC.get();
        if (!SubRC->isValueDependent()) {
          bool Satisfied = true;
          if (auto *CSE = dyn_cast<ConceptSpecializationExpr>(SubRC))
            Satisfied = CSE->isSatisfied();
          else
            SubRC->EvaluateAsBooleanCondition(Satisfied, Context);
          if (!Satisfied)
            continue; // Discard this contract
        }
      }
    }

    StmtResult NewStmt = SubstStmt(CS, TemplateArgs);
    if (NewStmt.isInvalid()) {
      IsInvalid = true;
      continue;
    }
    // dyn_cast, not ActionResult::getAs: the latter is a static_cast, so this
    // test never actually filtered anything.  TransformContractStmt hands back
    // a NullStmt for a contract P4283 discarded, and that must be dropped
    // rather than reinterpreted as a ContractStmt.
    if (auto *NewCS = dyn_cast_if_present<ContractStmt>(NewStmt.get()))
      NewContracts.push_back(NewCS);
  }

  // P4283: If every contract was discarded by an unsatisfied requires-clause,
  // clear the instantiation's contracts.  Returning early here would leave the
  // instantiation pointing at the pattern's dependent contract specifier (the
  // placeholder installed at class-template instantiation), so downstream
  // consumers -- in particular the P3097 virtual interface wrapper -- would
  // emit the pattern's discarded contract and reference the pattern's
  // parameters (crashing in CodeGen).  A null specifier makes hasContracts()
  // false, so the wrapper is skipped and no check is emitted; the on-use
  // re-entry guard (InstantiateVirtualFunctionContractsOnUse) also treats a
  // null specifier as "already resolved".
  if (NewContracts.empty() && !IsInvalid) {
    Instantiation->setContracts(nullptr);
    return;
  }

  ContractSpecifierDecl *NewCSD = BuildContractSpecifierDecl(
      NewContracts, Instantiation, PatternCSD->getLocation(), IsInvalid);
  assert(NewCSD);

  Instantiation->setContracts(NewCSD);
  if (!Instantiation->isDependentContext())
    CheckFunctionContracts(Instantiation, /*IsDefinition=*/false,
                           /*IsInstantiation=*/true);
}

ContractSpecifierDecl *
Sema::BuildContractSpecifierDecl(ArrayRef<ContractStmt *> Contracts,
                                 DeclContext *DC, SourceLocation Loc,
                                 bool IsInvalid) {
  ContractSpecifierDecl *CSD =
      ContractSpecifierDecl::Create(Context, DC, Loc, Contracts, IsInvalid);
  return CSD;
}

/// ActOnFinishContractSpecifierSequence - This is called after a
/// contract-specifier-seq has been parsed.
///
/// It's primary job is to set the canonical result name decl for each result
/// name.
ContractSpecifierDecl *
Sema::ActOnFinishContractSpecifierSequence(ArrayRef<ContractStmt *> Contracts,
                                           SourceLocation Loc, bool IsInvalid) {
  assert((!Contracts.empty() || IsInvalid) && "Expected at least one contract");

  return BuildContractSpecifierDecl(Contracts, CurContext, Loc, IsInvalid);
}

void Sema::ActOnContractsOnFinishFunctionDecl(FunctionDecl *D,
                                              bool IsDefinition) {

  FunctionDecl *FD;
  if (FunctionTemplateDecl *FunTmpl = dyn_cast<FunctionTemplateDecl>(D))
    FD = FunTmpl->getTemplatedDecl();
  else
    FD = D;

  // [temp.expl.spec]p12
  // ... and function-contract-specifier appearing in the declaration of a
  // template have no effect on an explicit specialization of
  // that template.
  const bool IsTemplateSpecialization =
      FD->getTemplateSpecializationKind() == TSK_ExplicitSpecialization;

  auto *First = FD->getFirstDecl();

  // If the new declaration/definition doesn't have contracts, and it's previous
  // declarations didn't have contracts or it's a specialiazation which doesn't
  // inherit the contracts, then there's nothing to do.

  if (!FD->hasContracts() && !First->hasContracts()) {
    return;
  }

  // If the definition has omitted the contracts, but the first declaration has
  // them, we need to rebuild the contracts to refer to the parameters of the
  // definition.
  //
  // For function templates, we'll create a copy when we instantiate the
  // definition.
  if (First->hasContracts() && !FD->hasContracts() && IsDefinition &&
      !FD->isTemplateInstantiation() && !IsTemplateSpecialization) {
    // Note: This case is mutually exclusive with the NonDependentPlaceholders
    // case, since we can't have a placeholder return type on a declaration that
    // isn't a definition.

    // TODO: Revisit whether the contract specifier needs rebuilding in this
    // case rather than reusing the pattern's.
    assert(FD->getTemplatedKind() !=
           FunctionDecl::TK_FunctionTemplateSpecialization);
    assert(FD->getTemplatedKind() != FunctionDecl::TK_MemberSpecialization);

    // Null when every contract was discarded by an unsatisfied P4283
    // requires-clause; RebuildContractSpecifierForDecl has already cleared the
    // definition's specifier in that case.
    if (ContractSpecifierDecl *NewCSD =
            RebuildContractSpecifierForDecl(First, FD)) {
      NewCSD->setOwningFunction(FD);
      FD->setContracts(NewCSD);
    }
  }

  if (!FD->hasContracts())
    return;

  assert(FD->hasContracts());

  ContractSpecifierDecl *CSD = FD->getContracts();

  if (!D->isTemplateInstantiation()) {
    // Note: IsInstantiation here means whether we're calling during the
    // instantiation of the contract specifier, rather than whether the FD
    // declares a function instantiation.
    CheckFunctionContracts(FD, IsDefinition, /*IsInstantiation=*/false);
  }

  // When the declared return type of a non-templated function contains a
  // placeholder type, a postcondition-specifier with a result-name-introducer
  // shall be present only on a definition.
  if (CSD->hasInventedPlaceholdersTypes() && !FD->isTemplateInstantiation() &&
      !FD->isTemplated()) {
    if (!IsDefinition && !FD->isThisDeclarationADefinition()) {
      Diag(CSD->getCanonicalResultName()->getLocation(),
           diag::err_auto_result_name_on_non_def_decl)
          << CSD->getCanonicalResultName();
      Diag(FD->getReturnTypeSourceRange().getBegin(),
           diag::note_function_return_type)
          << FD->getReturnTypeSourceRange();
      for (auto *RND : CSD->result_names())
        RND->setInvalidDecl(true);
    }
  }
}

namespace {

// There's got to be a better way to do this.
struct RebuildFunctionContracts
    : public TreeTransform<RebuildFunctionContracts> {

  using Inherited = TreeTransform<RebuildFunctionContracts>;
  bool IsAlwaysRebuild = false;
  bool AlwaysRebuild() const { return IsAlwaysRebuild; }

  RebuildFunctionContracts(Sema &S, bool Rebuild = false)
      : TreeTransform<RebuildFunctionContracts>(S), IsAlwaysRebuild(Rebuild) {}
};

} // namespace

/// Rebuild the contract specifier written on one declaration in the context of
/// the definition. This rebinds parameters and result names as needed.
ContractSpecifierDecl *
Sema::RebuildContractSpecifierForDecl(FunctionDecl *First, FunctionDecl *Def) {

  assert(Def->getContracts() == First->getContracts() || !Def->hasContracts());
  Def->setContracts(nullptr);
  Sema::ContextRAII SavedContext(*this, Def);
  assert(FunctionScopes.empty());

  FunctionScopeRAII SavedFunctionContext(*this);
  PushFunctionScope();

  std::optional<Sema::CXXThisScopeRAII> ThisScope;
  if (auto *CXXMethod = dyn_cast<CXXMethodDecl>(Def)) {
    Qualifiers MethodQuals = CXXMethod->getMethodQualifiers();
    if (LangOpts.ContractConstification)
      MethodQuals.addConst();
    ThisScope.emplace(*this, CXXMethod->getParent(), MethodQuals,
                      /*IsLambda*/ false);
  }
  RebuildFunctionContracts Rebuilder(*this, true);
  for (unsigned I = 0; I < First->getNumParams(); ++I) {
    Rebuilder.transformedLocalDecl(First->getParamDecl(I),
                                   Def->getParamDecl(I));
  }
  for (auto *RND : First->getContracts()->result_names()) {
    QualType Replacement = Def->getReturnType();
    auto *NewRND = ActOnResultNameDeclarator(
        ContractKind::Post, nullptr, Replacement, RND->getLocation(),
        RND->getIdentifier(), RND->getFunctionScopeDepth());
    Rebuilder.transformedLocalDecl(RND, NewRND);
  }
  for (auto *CS : First->getContracts()->contracts()) {
    if (!CS->hasCaptures())
      continue;
    for (auto *D : CS->getCapturesDeclStmt()->decls()) {
      auto *OldCap = cast<PostconditionCaptureDecl>(D);
      auto *NewCap = PostconditionCaptureDecl::Create(
          Context, Def, OldCap->getBeginLoc(), OldCap->getLocation(),
          OldCap->getIdentifier(), OldCap->getType(),
          OldCap->getTypeSourceInfo(), OldCap->getStorageClass());
      NewCap->setIsParameterCapture(OldCap->isParameterCapture());
      NewCap->setIsPackExpansion(OldCap->isPackExpansion());
      // Carry over the capture's initializer, rebuilt against the definition's
      // parameters (a plain capture [p] copy-initializes from parameter p; an
      // init-capture [old = e] snapshots e).  Without this the definition's
      // capture is left uninitialized and reads garbage at run time.
      if (Expr *OldInit = OldCap->getInit()) {
        ExprResult NewInit = Rebuilder.TransformExpr(OldInit);
        if (!NewInit.isInvalid())
          NewCap->setInit(NewInit.get());
      }
      Rebuilder.transformedLocalDecl(OldCap, NewCap);
    }
  }
  SmallVector<ContractStmt *> NewContracts;
  bool IsInvalid = false;
  for (auto *CS : First->contracts()) {
    StmtResult NewStmt = Rebuilder.TransformContractStmt(CS);
    if (NewStmt.isInvalid())
      IsInvalid = true;
    // dyn_cast, not ActionResult::getAs, which is a static_cast: a contract
    // discarded by an unsatisfied P4283 requires-clause comes back as a
    // NullStmt and must be dropped, not stored as a bogus ContractStmt.
    else if (auto *NewCS = dyn_cast_if_present<ContractStmt>(NewStmt.get()))
      NewContracts.push_back(NewCS);
  }

  // Every contract discarded by an unsatisfied P4283 requires-clause and none
  // left: the definition simply has no contracts.  ContractSpecifierDecl::
  // Create requires at least one contract unless the specifier is invalid, and
  // this is not an error, so leave the definition without a specifier.
  if (NewContracts.empty() && !IsInvalid) {
    Def->setContracts(nullptr);
    return nullptr;
  }

  auto *CSD = BuildContractSpecifierDecl(
      NewContracts, Def, First->getContracts()->getLocation(), IsInvalid);
  Def->setContracts(CSD);

  if (CSD->isInvalidDecl())
    Def->setInvalidDecl();
  return CSD;
}

DeclResult Sema::RebuildContractsWithPlaceholderReturnType(FunctionDecl *FD) {

  ContractSpecifierDecl *CSD = FD->getContracts();
  assert(CSD && CSD->hasInventedPlaceholdersTypes() &&
         "Cannot rebuild contracts without placeholders");
  if (FD->isInvalidDecl()) {
    CSD->setInvalidDecl(true);
    return DeclResult(/*IsInvalid*/ true);
  }

  RebuildFunctionContracts Rebuilder(*this, true);

  QualType Replacement;
  auto CheckFunctionReturnType = [&](SourceLocation Loc) -> bool {
    if (!Replacement.isNull())
      return false;

    if (FD->getReturnType()->isUndeducedType()) {
      assert(!FD->isInvalidDecl());
      assert(!CSD->isInvalidDecl());
      // DeduceReturnType emits its own diagnostic on failure.
      if (DeduceReturnType(FD, Loc, true))
        return true;
    }
    assert(!FD->getReturnType()->isUndeducedType());
    Replacement = FD->getReturnType();
    return false;
  };

  SmallVector<std::pair<ResultNameDecl *, ResultNameDecl *>> Transformed;

  for (auto *RND : FD->getContracts()->result_names()) {
    if (Replacement.isNull()) {
      if (CheckFunctionReturnType(RND->getLocation())) {
        FD->setInvalidDecl(true);
        return DeclResult(/*IsInvalid*/ true);
      }
    }
    assert(!Replacement.isNull());
    auto *NewRND = ActOnResultNameDeclarator(
        ContractKind::Post, nullptr, Replacement, RND->getLocation(),
        RND->getIdentifier(), RND->getFunctionScopeDepth());
    Transformed.emplace_back(RND, NewRND);
  }

  for (auto [K, V] : Transformed)
    Rebuilder.transformedLocalDecl(K, {V});

  SmallVector<ContractStmt *> NewContracts;
  bool IsInvalid = false;
  for (auto *CS : FD->contracts()) {
    StmtResult NewStmt = Rebuilder.TransformContractStmt(CS);
    if (NewStmt.isInvalid())
      IsInvalid = true;
    // dyn_cast, not ActionResult::getAs -- see RebuildContractSpecifierForDecl.
    else if (auto *NewCS = dyn_cast_if_present<ContractStmt>(NewStmt.get()))
      NewContracts.push_back(NewCS);
  }

  // As in RebuildContractSpecifierForDecl: everything discarded by an
  // unsatisfied requires-clause is not an error, and an empty specifier cannot
  // be built, so the function is left with none.
  if (NewContracts.empty() && !IsInvalid) {
    FD->setContracts(nullptr);
    return DeclResult(/*IsInvalid*/ false);
  }

  auto *NewCSD = BuildContractSpecifierDecl(
      NewContracts, FD, FD->getContracts()->getLocation(), IsInvalid);

  FD->setContracts(NewCSD);
  if (NewCSD->isInvalidDecl())
    FD->setInvalidDecl();

  return NewCSD;
}

struct ContractCapturePair {
  SmallVector<const Capture *> InContract;
  SmallVector<const Capture *> OutOfContract;
};

// ActOnContractsOnFinishFunctionBody
//
// This function ensures there is a usable version of the
// function contracts attached to the definition declaration.
//
// This is already the case if the definition declaration was spelled with
// contracts.
//
// Otherwise, we might have contracts on the first declaration.
// If we do,
//
// (1) attach them to the defining declaration.
// (2) rebind any references to parameters contained within the contracts.
//
// This is important because the defining definitions parameters will
// be the ones evaluated by ExprConstant/CodeGen.
//
// This is probably BAD BAD NOT GOOD.
// But it works nicely.
void Sema::ActOnContractsOnFinishFunctionBody(FunctionDecl *Def) {
  assert(Def);
  // If we had a deduced return type on a non-template function, we can now
  // attempt to deduce the return type and rebuild the contracts with the
  // deduced return type.
  if (Def->hasContracts() &&
      Def->getContracts()->hasInventedPlaceholdersTypes()) {
    assert((Def->isThisDeclarationADefinition() ||
            Def->isTemplateInstantiation()) &&
           "Wrong declaration passed?");

    DeclResult Res = RebuildContractsWithPlaceholderReturnType(Def);
    if (Res.isInvalid()) {
      Def->setInvalidDecl(true);
      return;
    }
    // Null when every contract was discarded by an unsatisfied P4283
    // requires-clause; the rebuild has already cleared the specifier.
    if (auto *NewCSD = dyn_cast_if_present<ContractSpecifierDecl>(Res.get())) {
      NewCSD->setOwningFunction(Def);
      Def->setContracts(NewCSD);
      if (NewCSD->isInvalidDecl())
        Def->setInvalidDecl(true);
    }
  }

  if (const LambdaScopeInfo *LSI =
          dyn_cast<LambdaScopeInfo>(getCurFunction())) {
    llvm::DenseMap<const ValueDecl *, ContractCapturePair> CheckedCaptures;
    for (auto &KV : LSI->ContractCaptureMap) {
      assert(false);
      if (LSI->CaptureMap.contains(KV.first))
        continue;
      const Capture &C = LSI->ContractCaptures[KV.second - 1];
      assert(C.isVariableCapture());

      const ValueDecl *VD = C.getVariable();
      assert(VD && isa<NamedDecl>(VD));
      auto *ND = cast<NamedDecl>(KV.first);
      Diag(C.getLocation(), diag::err_lambda_implicit_capture_in_contracts_only)
          << ND;
      Diag(LSI->CaptureDefaultLoc,
           diag::note_lambda_implicit_capture_in_contracts_only)
          << ND;
      Diag(C.getContractLoc(), diag::note_contract_context);
      Def->setInvalidDecl();
    }
  }
}

bool Sema::isUsageAcrossContract(const ValueDecl *VD) {
  // There's no contract scope anywhere above us.
  if (!getCurrentContractEntry())
    return false;

  // Fast Path: We're in an immediate contract assertion expression evaluation
  // context.
  if (isContractAssertionContext())
    return true;

  // A variable that needs no capture to be named is visible everywhere, so
  // there is nothing to walk: if a contract scope is current at all, naming it
  // here is a use from inside a predicate.  That covers both a variable that
  // is not local to a function AND a local one with static or thread storage
  // duration.
  //
  // The intervening-scope walk below exists to decide the case those exclude:
  // a variable an intervening lambda may have COPY-CAPTURED, where which
  // object is named depends on the captures in between.  A local static is
  // never captured, so putting it through that walk asked a question with no
  // answer and got none -- which is why it was constified when named directly
  // in a predicate (the fast path above) and not when named from inside a
  // lambda there.
  //
  // The non-local half of this used to return false, which is the pre-R9 rule
  // ("only variables with automatic storage duration") in its second hiding
  // place: it left namespace-scope variables, thread_locals and static data
  // members unconstified no matter what getContractConstification decided.
  if (const auto *Var = dyn_cast<VarDecl>(VD))
    if (!Var->isLocalVarDeclOrParm() ||
        Var->getStorageDuration() == SD_Static ||
        Var->getStorageDuration() == SD_Thread)
      return true;

  // Nothing has pushed a function scope, so there is no scope stack to walk
  // and nothing can lie between this use and the contract.  A contract scope
  // is current -- checked at the top -- so the use is inside a predicate.
  //
  // This is the eager path: a free function's contracts are parsed in its
  // declarator, and unlike the late-parsed path taken by a member function's
  // (ParseContracts.cpp, CES_Function) nothing pushes a FunctionScopeInfo for
  // it.  RebuildContractSpecifierForDecl asserts the same emptiness and pushes
  // a scope of its own before it does any work.
  //
  // Reaching here at all requires an unevaluated operand in the predicate: the
  // isContractAssertionContext() fast path above answers every use that is
  // directly in the predicate.  A requires-expression is the case that gets
  // this far, and it used to walk off the bottom of the scope stack and fail
  // ScopeWalker::nextFuncScope's assertion.
  if (FunctionScopes.empty())
    return true;

  assert(VD);
  return getInterveningContractEntry(*this, VD) != nullptr;
}

/// [expr.prim.id.unqual]/3+d
/// Within the predicate of a contract assertion C, an id-expression naming a
/// variable declared outside of C of object type T -- or of type "reference to
/// T", or a structured binding whose variable is declared outside of C -- has
/// type const T.  There is no storage-duration restriction: P2900R9 removed
/// one ("Made implicit const in contract predicates apply to all variables,
/// rather than just those with automatic storage duration"), and the
/// paragraph's own example opens with a namespace-scope `int n` and
/// `pre(++n) // error: attempting to modify const lvalue`.
ContractConstification Sema::getContractConstification(const ValueDecl *VD) {
  // WalkUpContractScopesTest();
  auto &S = *this;
  if (!S.LangOpts.ContractConstification)
    return CC_None;

  assert(VD);
  const ContractScopeRecord *CSR = S.getCurrentContractEntry();

  // Only constify entities that are declared *outside* the contract predicate
  // and referenced across the contract boundary.  An entity declared *inside*
  // the predicate itself (e.g. a local of a lambda that appears in the
  // predicate) is not constified.
  //
  // ContextAtPush is the DeclContext active when the contract scope was
  // entered: the enclosing namespace/class for a function pre/post (parsed at
  // declarator stage), or the function itself for a body-level
  // contract_assert.  A predicate-local entity is one whose DeclContext is
  // *strictly* enclosed by ContextAtPush.  A parameter, result name, or local
  // of the contracted function has a DeclContext that is equal to (or
  // encloses) ContextAtPush, and must be constified -- so we must not treat the
  // equal case as "declared inside the predicate" (DeclContext::Encloses is
  // reflexive, hence the explicit !Equals).
  //
  // The enclosure test is only a proxy for "declared inside the predicate",
  // and it is the wrong proxy for a variable that is not local to a function:
  // a static data member's DeclContext is its CLASS, which the enclosing
  // namespace strictly encloses, so `void f() pre(++H::s)` read as though
  // `H::s` had been declared inside the predicate.  Nothing declared inside a
  // predicate is a non-local variable -- what can be declared there are the
  // parameters and locals of lambdas within it -- so exempt those from the
  // proxy entirely.  This also covers [expr.prim.id.qual]/5+a, which P2900
  // adds alongside the unqualified rule in the same words.
  if (!CSR)
    return CC_None;
  const auto *AsVar = dyn_cast<VarDecl>(VD);
  const bool NonLocalVar = AsVar && !AsVar->isLocalVarDeclOrParm();
  if (!NonLocalVar && CSR->ContextAtPush->Encloses(VD->getDeclContext()) &&
      !CSR->ContextAtPush->Equals(VD->getDeclContext()))
    return CC_None;

  // If there is no contract scope that encloses the current context, then we
  // don't need to constify the variable.
  if (getLastEnclosingContractScopeForContext(CurContext) == nullptr)
    return CC_None;

  // Make sure that there's a contract scope interviening between the current
  // context and the declaration of the variable. If there isn't, we don't need
  // to constify the variable.
  if (!isUsageAcrossContract(VD))
    return CC_None;

  // if the unqualified-id appears in the predicate of a contract assertion
  //  ([basic.contract]) and the entity is
  // ...

  // — the result object of (possibly deduced, see [dcl.spec.auto]) type T of a
  // function
  //  call and the unqualified-id is the result name ([dcl.contract.res]) in a
  //  postcondition assertion,
  if (isa<ResultNameDecl>(VD))
    return CC_ApplyConst;

  // — a structured binding of type T whose corresponding variable is declared
  //  outside of C, or
  //
  // "Declared outside of C" is what the checks above already established, so
  // there is nothing further to test here: a binding of any storage duration
  // qualifies.
  if (auto *Bound = dyn_cast<BindingDecl>(VD)) {
    if (!Bound->getHoldingVar())
      return CC_None;
    return CC_ApplyConst;
  }

  // Postcondition captures are not constified (P3098 Section 4.4.1)
  if (isa<PostconditionCaptureDecl>(VD))
    return CC_None;

  // — a variable declared outside of C ...
  //
  // Any storage duration: a namespace-scope variable, a function-local static,
  // a thread_local and a static data member are all "a variable declared
  // outside of C".  Restricting this to automatic storage was P2900's pre-R9
  // rule.
  if (auto Var = dyn_cast<VarDecl>(VD)) {
    // ... of object type T, or
    if (Var->getType()->isObjectType())
      return CC_ApplyConst;

    // of type 'reference to T'
    if (Var->getType()->isReferenceType() &&
        Var->getType().getNonReferenceType()->isObjectType())
      return CC_ApplyConst;
  }

  return CC_None;
}

static const DeclContext *
walkUpDeclContextToFunction(const DeclContext *DC, bool AllowLambda = false) {
  while (true) {
    assert(DC);
    if (isa<BlockDecl>(DC) || isa<EnumDecl>(DC) || isa<CapturedDecl>(DC) ||
        isa<RequiresExprBodyDecl>(DC) || isa<LinkageSpecDecl>(DC) ||
        (isa<CXXRecordDecl>(DC) && cast<CXXRecordDecl>(DC)->isLambda())) {
      DC = DC->getParent();
    } else if (!AllowLambda && isa<CXXMethodDecl>(DC) &&
               cast<CXXMethodDecl>(DC)->getOverloadedOperator() == OO_Call &&
               cast<CXXRecordDecl>(DC->getParent())->isLambda()) {
      DC = DC->getParent()->getParent();
    } else
      break;
  }
  if (DC) {
    if (!DC->isFunctionOrMethod()) {
      assert(!DC->getParent() || !DC->getParent()->isFunctionOrMethod());
      assert(!DC->getLexicalParent() ||
             !DC->getLexicalParent()->isFunctionOrMethod());
    }
  }
  if (DC && !DC->isFunctionOrMethod())
    return nullptr;
  return DC;
}

QualType Sema::adjustCXXThisTypeForContracts(QualType QT) {
  if (!getCurrentContractEntry() || !LangOpts.ContractConstification)
    return QT;

  // 'this' is constified any time the `this` object that is captured by a
  // lambda which exists fully within a contract.
  //
  // We need to ensure that we haven't entered a nested member function context,
  // because in that case we don't want to constify the `this` object. For
  // example:
  // ```
  // struct A {
  //   void f() {
  //     [&]() {
  //        contract_assert([&] {
  //           struct B { int x; void f() { ++x; } };
  //            return true;
  //          }());
  //      }();
  //   }};
  // ```
  const DeclContext *ContractContext =
      walkUpDeclContextToFunction(getCurrentContractEntry()->ContextAtPush);
  if (!ContractContext)
    return QT;
  const DeclContext *QTContext = walkUpDeclContextToFunction(CurContext);
  if (!QTContext)
    return QT;
  if (!ContractContext->Equals(QTContext))
    return QT;

  return Context.getPointerType(QT->getPointeeType().withConst());
}

namespace {

struct CaptureUsage {
  clang::LambdaCapture Capture;
  const ContractStmt *UsedInContract = nullptr;
  const Expr *UsageExpr = nullptr;
  SourceLocation UsageLoc = SourceLocation();

  CaptureUsage(clang::LambdaCapture Capture) : Capture(Capture) {}
};

/// TODO: Remove this and do it inline instead. We currently
/// do this as a RecursiveASTVisitor because the changes to do it during
/// the initial parsing pass are too invasive to do dur
struct LambdaCaptureChecker : RecursiveASTVisitor<LambdaCaptureChecker> {
  typedef RecursiveASTVisitor<LambdaCaptureChecker> super;

  Sema &Actions;
  const LambdaExpr *const CurLambda = nullptr;
  const ContractStmt *CurContract = nullptr;

  // Note: The value `nullptr` is used to denote a capture of CXXThis.
  DenseMap<const ValueDecl *, CaptureUsage> Captures;

  void observeUsage(const ValueDecl *VD, const Expr *E, SourceLocation Loc) {
    if (auto Pos = Captures.find(VD); Pos != Captures.end()) {
      if (!CurContract)
        Captures.erase(VD);
      else {
        auto &Usage = Pos->second;
        if (Usage.UsageExpr == nullptr ||
            (isa<LambdaExpr>(Usage.UsageExpr) && !isa<LambdaExpr>(E))) {
          Usage.UsedInContract = CurContract;
          Usage.UsageExpr = E;
          Usage.UsageLoc = Loc;
        }
      }
    }
  }

private:
  LambdaCaptureChecker(Sema &S, LambdaExpr *LE) : Actions(S), CurLambda(LE) {
    Init();
  }

  void Run() {
    // Traverse the lambdas function-level contracts and body to find the bad
    // captures.
    FunctionDecl *FD = CurLambda->getCallOperator();
    assert(FD->getBody());
    if (FD->hasContracts())
      TraverseDecl(FD->getContracts());
    TraverseStmt(CurLambda->getBody());

    // Finally, diagnose any captures that still remain, since they do not have
    // any non-contrac usages.
    for (auto &[Var, Bad] : Captures) {
      // We likely didn't see the usage because there was a intervening lambda
      // that captured by copy.
      if (Bad.UsedInContract == nullptr)
        continue;
      Actions.Diag(CurLambda->getCaptureDefaultLoc(),
                   diag::err_lambda_implicit_capture_in_contracts_only)
          << (int)Bad.Capture.capturesThis() << cast_or_null<NamedDecl>(Var);
      SourceLocation UsageLoc = Bad.UsageLoc;
      Actions.Diag(UsageLoc,
                   diag::note_lambda_implicit_capture_in_contract_usage)
          << (int)Bad.Capture.capturesThis() << cast_or_null<NamedDecl>(Var);
      if (Bad.UsedInContract)
        Actions.Diag(Bad.UsedInContract->getBeginLoc(),
                     diag::note_contract_context);
    }
  }

  void Init() {
    // Collect all of the implicit captures of the lambda.
    // If the lambda capture hasn't been removed after traversing the tree then
    // that lambda capture is bad, and must be diagnosed as only being used
    // inside of a contract.
    for (auto C : CurLambda->captures()) {
      if (C.capturesThis() && C.isImplicit())
        Captures.insert({nullptr, {C}});
      if (!C.capturesVariable())
        continue;
      if (C.isExplicit())
        continue;
      Captures.insert({C.getCapturedVar(), {C}});
    }
  }

public:
  bool shouldVisitLambdaBody() const { return false; }

  bool TraverseContractStmt(ContractStmt *CS) {
    assert(CS->getCond());
    const ContractStmt *Prev = CurContract;
    CurContract = CS;
    TraverseStmt(CS->getCond());
    CurContract = Prev;
    return true;
  }

  bool TraverseLambdaCapture(LambdaExpr *LE, const LambdaCapture *C,
                             Expr *Init) {
    return true;
  }

  bool TraverseLambdaExpr(LambdaExpr *LE) {
    assert(LE != CurLambda && "Revisiting the root lambda?");
    // Iterate over the captures of the nested lambda, and mark any of our
    // captures as having been seen outside of a contract. This assumes that the
    // inner lambda has a valid usage of the capture. If it doesn't, we'll
    // diagnose that separately.
    for (auto C : LE->captures()) {
      // FIXME: Figure out how to deal with VLA captures here
      if (C.capturesVLAType())
        continue;

      assert(C.capturesThis() || C.capturesVariable());
      observeUsage(C.capturesThis() ? nullptr : C.getCapturedVar(), LE,
                   C.getLocation());
    }
    return true;
  }

  bool VisitDeclRefExpr(DeclRefExpr *E) {
    if (auto *VD = dyn_cast<ValueDecl>(E->getDecl());
        VD && E->isNonOdrUse() != NOUR_Unevaluated)
      observeUsage(VD, E, E->getExprLoc());
    return true;
  }

  bool VisitMemberExpr(MemberExpr *E) {
    if (E->isNonOdrUse() != NOUR_Unevaluated)
      observeUsage(nullptr, E, E->getExprLoc());
    return true;
  }

  bool VisitCXXThisExpr(CXXThisExpr *E) {
    observeUsage(nullptr, E, E->getExprLoc());
    return true;
  }

public:
  static void Check(Sema &Actions, LambdaExpr *LE) {
    if (!Actions.LangOpts.ContractLambdaCaptureRestrictions)
      return;
    LambdaCaptureChecker Checker(Actions, LE);
    Checker.Run();
  }
};

} // namespace

void Sema::CheckLambdaCapturesForContracts(LambdaExpr *LE) {
  // Check the contracts on the function declaration.
  LambdaCaptureChecker::Check(*this, LE);
}

std::optional<unsigned>
Sema::getFunctionScopeIndexForDeclaration(const ValueDecl *VD) {
  const DeclContext *Ctx = CurContext;
  assert(CurContext->isFunctionOrMethod());
  if (FunctionScopes.size() == 0)
    return std::nullopt;
  if (auto *PVD = dyn_cast<ParmVarDecl>(VD)) {
    assert(PVD->getDeclContext()->isFunctionOrMethod());
  }

  const DeclContext *const VarCtx = VD->getDeclContext();
  if (!VarCtx->Encloses(Ctx)) {
    assert(!VarCtx->Equals(Ctx));
    return std::nullopt;
  }
  unsigned StartScope = FunctionScopes.size() - 1;
  while (Ctx && !Ctx->Equals(VarCtx)) {
    assert(StartScope > 0);
    Ctx = walkUpDeclContextToFunction(Ctx, /*AllowLambda=*/true);
    --StartScope;
  }
  if (!Ctx)
    return std::nullopt;

  auto TestDC = getDeclContextForFunctionScopeIndex(StartScope);
  assert(TestDC && TestDC->Equals(Ctx));
  return StartScope;
}

const DeclContext *
Sema::getDeclContextForFunctionScopeIndex(unsigned ScopeIndex) {
  const DeclContext *Ctx = CurContext;
  assert(ScopeIndex < FunctionScopes.size());
  unsigned ScopesToGo = FunctionScopes.size() - ScopeIndex - 1;
  assert(Ctx->isFunctionOrMethod());
  if (!Ctx->isFunctionOrMethod())
    Ctx = walkUpDeclContextToFunction(Ctx, /*AllowLambda=*/true);
  assert(Ctx);
  while (ScopesToGo > 0) {
    --ScopesToGo;
    Ctx = walkUpDeclContextToFunction(Ctx, /*AllowLambda=*/true);
    assert(Ctx && Ctx->isFunctionOrMethod());
  }

  return Ctx;
}

bool Sema::isContractAssertionContext() const {
  auto *CR = getCurrentContractEntry();
  if (!CR)
    return false;

  return CR->ContextAtPush->Equals(CurContext);
}

ArrayRef<Sema::ContractScopeRecord> Sema::getAllContractScopes() const {
  return llvm::ArrayRef(ContractScopeStack.begin(), ContractScopeStack.end());
}
ArrayRef<Sema::ContractScopeRecord> Sema::getContractScopes() const {
  return getAllContractScopes();
}

ArrayRef<Sema::ContractScopeRecord>
Sema::getInterveningContractScopes(const ValueDecl *ValueD) const {
  assert(ValueD);

  auto *VD = dyn_cast<VarDecl>(ValueD);
  if (!VD)
    return {};

  if (!VD->isLocalVarDeclOrParm())
    return {};

  auto CScopes = [](auto CL) -> SmallVector<const ContractScopeRecord *> {
    SmallVector<const ContractScopeRecord *> Out;
    for (auto &CS : CL) {
      Out.push_back(&CS);
    }
    return Out;
  }(getContractScopes());
  if (CScopes.empty())
    return {};

  VD = VD->getCanonicalDecl();
  assert(VD && VD->getDeclContext());
  const DeclContext *VarCtx = VD->getDeclContext();
  assert(VarCtx);
  auto Pos = CScopes.end();
  auto LastPos = CScopes.end();

  auto ReturnRef = [&](auto Start) -> ArrayRef<ContractScopeRecord> {
    if (Start == CScopes.end())
      return {};
    unsigned StartIdx = (*Start)->Index;
    return llvm::ArrayRef(ContractScopeStack.begin() + StartIdx,
                          ContractScopeStack.end());
  };

  while (Pos != CScopes.begin()) {
    --Pos;
    const ContractScopeRecord *CS = *Pos;
    assert(CS->ContextAtPush);
    if (!CS->ContextAtPush->isFunctionOrMethod()) {
      return ReturnRef(Pos);
    }

    auto CCtx = (*Pos)->ContextAtPush;
    if (VarCtx->Encloses(CCtx) || VarCtx->Equals(CCtx))
      LastPos = Pos;
    else
      break;
  }
  return ReturnRef(LastPos);
}

SmallVector<sema::FunctionScopeInfo *>
Sema::getInterveningFunctionScopesForContracts(const ValueDecl *ValueD) const {
  return {};
}

const DeclContext *
Sema::ContractScopeRecord::getFunctionContext(bool AllowLambda) const {
  if (!ContextAtPush->isFunctionOrMethod() ||
      (isLambdaCallOperator(ContextAtPush) && !AllowLambda))
    return walkUpDeclContextToFunction(ContextAtPush, AllowLambda);
  return ContextAtPush;
}

const Sema::ContractScopeRecord *Sema::getCurrentContractEntry() const {
  auto EntryList = getContractScopes();
  if (EntryList.empty())
    return nullptr;
  return &EntryList.back();
}

void Sema::WalkUpContractScopesTest() const {

  if (ContractScopeStack.empty())
    return;
  if (FunctionScopes.empty())
    return;
  ScopeWalker Walker(*this);
  auto Scopes = Walker.doIt();
  ((void)Scopes);
}

Sema::ContractScopeRAII::ContractScopeRAII(Sema &S, ContractKind CK,
                                           ContractScopeOffset ScopeOffset,
                                           SourceLocation Loc)
    : S(S) {
  S.PushContractScope(CK, ScopeOffset, Loc);
}

Sema::ContractScopeRAII::~ContractScopeRAII() { S.PopContractScope(); }

void Sema::PushContractScope(ContractKind Kind, ContractScopeOffset ScopeOffset,
                             SourceLocation Loc) {
  //  assert(!FunctionScopes.empty());

  ContractScopeRecord Record{};
  Record.Index = static_cast<unsigned>(ContractScopeStack.size());
  Record.Kind = Kind;
  Record.ScopeOffset = ScopeOffset;
  Record.KeywordLoc = Loc;
  Record.ContextAtPush = CurContext;
  Record.PreviousCXXThisType = CXXThisTypeOverride;
  Record.FunctionIndex = static_cast<unsigned>(
      FunctionScopes.empty() ? 0ul : FunctionScopes.size() - 1);
  Record.StartFunctionIndex = FunctionScopesStart;
  Record.FunctionScopeAtPush = getCurFunction();
  Record.AddedConstToCXXThis = false;
  Record.WasInContractContext = ExprEvalContexts.back().InContractAssertion;
  Record.HadNoFunctionScope = FunctionScopes.empty();
  Record.FunctionScopeStartAtPush = FunctionScopesStart;

  // Setup the constification context when building declref expressions.
  ExprEvalContexts.back().InContractAssertion = true;
  assert(CurContext);

  assert(ContractScopeIndexMap.find(CurContext) == ContractScopeIndexMap.end());

  ContractScopeIndexMap[CurContext] = Record.Index;
  // P2900R8 [expr.prim.this]p2
  //   If the expression 'this' appears ... in a contract assertion
  //     (including as the result of the implicit transformation in the body of
  //     a non-static member function and including in the bodies of nested
  //     lambda-expressions),
  // ...
  //  const is combined with the cv-qualifier-seq used to generate the resulting
  //  type (see below
  if (!CXXThisTypeOverride.isNull()) {
    assert(CXXThisTypeOverride->isPointerType());
    QualType ClassType = CXXThisTypeOverride->getPointeeType();
    if ((not ClassType.isConstQualified()) && LangOpts.ContractConstification) {
      // If the 'this' object is const-qualified, we need to remove the
      // const-qualification for the contract check.
      ClassType.addConst();
      Record.AddedConstToCXXThis = true;
      CXXThisTypeOverride = Context.getPointerType(ClassType);
    }
  }

  // assert(!S.FunctionScopes.empty());

  if (Record.FunctionScopeAtPush) {
    auto *LastScope = Record.FunctionScopeAtPush;
    assert(LastScope && !LastScope->isInContract());

    LastScope->ContractScopeIndex = Record.Index;
  }

  ContractScopeStack.push_back(Record);
}

ContractScopeRecord Sema::PopContractScope() {
  assert(!ContractScopeStack.empty());

  auto Record = ContractScopeStack.back();
  ContractScopeStack.pop_back();

  assert(ContractScopeIndexMap.contains(Record.ContextAtPush) &&
         ContractScopeIndexMap[Record.ContextAtPush] == Record.Index);
  ContractScopeIndexMap.erase(Record.ContextAtPush);

  assert(ExprEvalContexts.back().InContractAssertion == true);
  ExprEvalContexts.back().InContractAssertion = Record.WasInContractContext;
  CXXThisTypeOverride = Record.PreviousCXXThisType;

  if (Record.FunctionScopeAtPush) {

    if (FunctionScopes.back() == Record.FunctionScopeAtPush) {
      assert(FunctionScopes.back()->ContractScopeIndex != unsigned(-1));
      FunctionScopes.back()->ContractScopeIndex = -1;
    } else {
      assert(getFunctionScopes().empty() ||
             FunctionScopes.size() < Record.FunctionIndex);
    }
  }

  return Record;
}

const ContractScopeRecord *
Sema::getContractScopeForContext(const DeclContext *DC) const {
  auto Pos = ContractScopeIndexMap.find(DC);
  if (Pos == ContractScopeIndexMap.end())
    return nullptr;
  unsigned Idx = Pos->second;
  assert(Idx < ContractScopeStack.size());
  return &ContractScopeStack[Idx];
}

const ContractScopeRecord *
Sema::getFirstEnclosingContractScopeForContext(const DeclContext *DC) const {
  for (unsigned I = 0; I < ContractScopeStack.size(); ++I) {
    if (ContractScopeStack[I].ContextAtPush->Encloses(DC)) {
      return &ContractScopeStack[I];
    }
  }
  return nullptr;
}

const ContractScopeRecord *
Sema::getLastEnclosingContractScopeForContext(const DeclContext *DC) const {
  unsigned LastEnclosingIdx = unsigned(-1);
  for (unsigned I = 0; I < ContractScopeStack.size(); ++I) {
    if (ContractScopeStack[I].ContextAtPush->Encloses(DC)) {
      LastEnclosingIdx = I;
    }
  }
  if (LastEnclosingIdx == unsigned(-1))
    return nullptr;
  assert(ContractScopeStack.size() > LastEnclosingIdx);
  return &ContractScopeStack[LastEnclosingIdx];
}

const ContractScopeRecord *
Sema::getFirstEnclosedContractScopeForContext(const DeclContext *DC) const {
  for (unsigned I = 0; I < ContractScopeStack.size(); ++I) {
    if (DC->Encloses(ContractScopeStack[I].ContextAtPush) ||
        DC->Equals(ContractScopeStack[I].ContextAtPush))
      return &ContractScopeStack[I];
  }
  return nullptr;
}

const ContractScopeRecord *
Sema::getLastEnclosedContractScopeForContext(const DeclContext *DC) const {
  unsigned LastIdx = unsigned(-1);
  for (unsigned I = 0; I < ContractScopeStack.size(); ++I) {
    if (DC->Encloses(ContractScopeStack[I].ContextAtPush) ||
        DC->Equals(ContractScopeStack[I].ContextAtPush)) {
      if (I > LastIdx || LastIdx == unsigned(-1))
        LastIdx = I;
    }
  }
  if (LastIdx == unsigned(-1))
    return nullptr;
  assert(LastIdx < ContractScopeStack.size());
  return &ContractScopeStack[LastIdx];
}

SourceLocation
Sema::getContractLocForFunctionScope(const sema::FunctionScopeInfo *FSI) const {
  assert(FSI->isInContract());
  assert(FSI->ContractScopeIndex < ContractScopeStack.size());
  return ContractScopeStack[FSI->ContractScopeIndex].KeywordLoc;
}
