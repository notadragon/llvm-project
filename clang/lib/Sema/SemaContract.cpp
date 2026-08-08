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
#include "clang/AST/ExprConcepts.h"
#include "clang/AST/ASTDiagnostic.h"
#include "clang/Basic/ContractOptions.h"
#include "clang/AST/ASTLambda.h"
#include "clang/AST/ASTStructuralEquivalence.h"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/CharUnits.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ExprObjC.h"
#include "clang/AST/IgnoreExpr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtCXX.h"
#include "clang/AST/StmtObjC.h"
#include "clang/AST/TypeLoc.h"
#include "clang/AST/TypeOrdering.h"
#include "clang/Basic/EricWFDebug.h"
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

ExprResult Sema::ActOnContractAssertCondition(Expr *Cond)  {
  assert(currentEvaluationContext().isContractAssertionContext() &&
         "Wrong context for statement");

  if (Cond->isTypeDependent())
    return Cond;

  ConditionResult Res = ActOnCondition(getCurScope(), Cond->getExprLoc(), Cond, Sema::ConditionKind::Boolean, /*MissingOK=*/false);
  if (Res.isInvalid())
    return ExprError();
  Cond = Res.get().second;
  assert(Cond);
  if (auto KnownValue = Res.getKnownValue(); KnownValue.has_value()) {
    Diag(Cond->getExprLoc(), diag::warn_ericwf_fixme) <<
      (std::string("Condition always evaluates to ") + (*KnownValue ? "true" : "false")) << Cond;
  }
  return Cond;
}

// Try to call label.member_name(semantic_arg) and constant-evaluate.
// Returns the integer result, or -1 on failure.
static int64_t callLabelMethod(Sema &S, Expr *LabelExpr, QualType LabelTy,
                               const CXXRecordDecl *RD,
                               StringRef MethodName,
                               unsigned SemVal, SourceLocation Loc) {
  DeclarationName Name = &S.Context.Idents.get(MethodName);
  LookupResult R(S, Name, Loc, Sema::LookupMemberName);
  if (!S.LookupQualifiedName(R, const_cast<CXXRecordDecl *>(RD)))
    return -1;
  if (R.isAmbiguous()) {
    R.suppressDiagnostics();
    return -1;
  }

  Sema::SFINAETrap Trap(S);
  CXXScopeSpec SS;

  ExprResult MemberRef = S.BuildMemberReferenceExpr(
      LabelExpr, LabelTy, Loc, /*IsArrow=*/false, SS,
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

  Expr *Arg = IntegerLiteral::Create(
      S.Context, llvm::APInt(8, SemVal), S.Context.UnsignedCharTy, Loc);
  Arg = ImplicitCastExpr::Create(S.Context, ParamTy, CK_IntegralCast, Arg,
                                 nullptr, VK_PRValue, FPOptionsOverride());

  ExprResult Call = S.BuildCallExpr(/*Scope=*/nullptr, MemberRef.get(), Loc,
                                    {Arg}, Loc, /*ExecConfig=*/nullptr);
  if (Call.isInvalid())
    return -1;

  Expr::EvalResult Eval;
  if (!Call.get()->EvaluateAsConstantExpr(Eval, S.Context))
    return -1;

  return Eval.Val.getInt().getExtValue();
}

// Call label.method_name(const char* arg) and constant-evaluate the result.
// Returns the resulting string, or empty StringRef on failure or null result.
static StringRef callLabelStringMethod(Sema &S, Expr *LabelExpr,
                                       QualType LabelTy,
                                       const CXXRecordDecl *RD,
                                       StringRef MethodName,
                                       StringRef CurrentVal,
                                       bool IsNull,
                                       SourceLocation Loc) {
  DeclarationName Name = &S.Context.Idents.get(MethodName);
  LookupResult R(S, Name, Loc, Sema::LookupMemberName);
  if (!S.LookupQualifiedName(R, const_cast<CXXRecordDecl *>(RD)))
    return {};
  if (R.isAmbiguous()) {
    R.suppressDiagnostics();
    return {};
  }

  Sema::SFINAETrap Trap(S);
  CXXScopeSpec SS;

  ExprResult MemberRef = S.BuildMemberReferenceExpr(
      LabelExpr, LabelTy, Loc, /*IsArrow=*/false, SS,
      /*TemplateKWLoc=*/SourceLocation(),
      /*FirstQualifierInScope=*/nullptr, R,
      /*TemplateArgs=*/nullptr, /*S=*/nullptr);
  if (MemberRef.isInvalid())
    return {};

  QualType ConstCharPtrTy =
      S.Context.getPointerType(S.Context.CharTy.withConst());

  Expr *Arg;
  if (IsNull) {
    Arg = ImplicitCastExpr::Create(
        S.Context, ConstCharPtrTy, CK_NullToPointer,
        IntegerLiteral::Create(S.Context, llvm::APInt(32, 0),
                               S.Context.IntTy, Loc),
        nullptr, VK_PRValue, FPOptionsOverride());
  } else {
    QualType ArrTy = S.Context.getStringLiteralArrayType(
        S.Context.CharTy, CurrentVal.size());
    StringLiteral *SL = StringLiteral::Create(
        S.Context, CurrentVal, StringLiteralKind::Ordinary,
        /*Pascal=*/false, ArrTy, {Loc});
    Arg = ImplicitCastExpr::Create(
        S.Context, ConstCharPtrTy, CK_ArrayToPointerDecay, SL,
        nullptr, VK_PRValue, FPOptionsOverride());
  }

  ExprResult Call = S.BuildCallExpr(/*Scope=*/nullptr, MemberRef.get(),
                                    Loc, {Arg}, Loc, /*ExecConfig=*/nullptr);
  if (Call.isInvalid())
    return {};

  Expr::EvalResult Eval;
  if (!Call.get()->EvaluateAsConstantExpr(Eval, S.Context))
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

  Sema::SFINAETrap Trap(S);
  CXXScopeSpec SS;

  ExprResult ASRef = S.BuildMemberReferenceExpr(
      LabelExpr, LabelTy, Loc, /*IsArrow=*/false, SS,
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

    Expr *Arg = IntegerLiteral::Create(
        S.Context, llvm::APInt(8, Sem), S.Context.UnsignedCharTy, Loc);
    Arg = ImplicitCastExpr::Create(S.Context, ParamTy, CK_IntegralCast, Arg,
                                   nullptr, VK_PRValue, FPOptionsOverride());

    ExprResult Call = S.BuildCallExpr(/*Scope=*/nullptr, ContainsRef.get(),
                                      Loc, {Arg}, Loc, /*ExecConfig=*/nullptr);
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
    const APValue &Ch = (I < InitElts)
        ? Val.getArrayInitializedElt(I)
        : Val.getArrayFiller();
    if (Ch.isInt()) {
      char C = static_cast<char>(Ch.getInt().getExtValue());
      if (C == '\0')
        break;
      Str += C;
    }
  }
  return Str;
}

// Extract group names from label's group_names member (identification_label).
// Returns a vector of group name strings; empty if no group_names member.
static SmallVector<std::string>
extractGroupNames(Sema &S, Expr *LabelExpr, QualType LabelTy,
                  const CXXRecordDecl *RD, SourceLocation Loc) {
  DeclarationName GNName = &S.Context.Idents.get("group_names");
  LookupResult R(S, GNName, Loc, Sema::LookupMemberName);
  if (!S.LookupQualifiedName(R, const_cast<CXXRecordDecl *>(RD)))
    return {};
  if (R.isAmbiguous()) {
    R.suppressDiagnostics();
    return {};
  }

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

  if (!FieldVal || !FieldVal->isArray())
    return {};

  SmallVector<std::string> Groups;
  unsigned InitElts = FieldVal->getArrayInitializedElts();
  for (unsigned I = 0, N = FieldVal->getArraySize(); I < N; ++I) {
    const APValue &Elt = (I < InitElts)
        ? FieldVal->getArrayInitializedElt(I)
        : FieldVal->getArrayFiller();
    std::string Str = extractStringFromAPValue(Elt);
    if (!Str.empty())
      Groups.push_back(std::move(Str));
  }
  return Groups;
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
  CQ.AllowedMask = AllowedMask | (1u << static_cast<unsigned>(
      ContractEvaluationSemantic::Ignore));
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
// stmt.  Codegen (T4) turns this into a dispatch switch; it does no Sema work of
// its own.
//
// This mirrors the eagerly-resolved scalar path (steps 3-5 below) but differs in
// ONE way per P3595 design section 4: a compute_semantic result that lands
// outside the allowed set records the sentinel 0 for that R (-> runtime enforced
// violation in codegen) instead of emitting the compile error.  The scalar
// compile-error path (applyLabelFacets step 5) is unchanged for the
// eagerly-resolved default.
//
// LabelExpr/LabelTy/RD may be null for a dynamic contract with no label facets
// (matched by group/namespace/location); then compute_semantic is skipped and
// T(R) is just the clamp.
static void precomputeDynamicTable(Sema &S, ContractStmt *CS,
                                   unsigned AllowedMask,
                                   ArrayRef<std::string> Groups,
                                   Expr *LabelExpr, QualType LabelTy,
                                   const CXXRecordDecl *RD, SourceLocation Loc) {
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
    if (S.LookupQualifiedName(ACO, const_cast<CXXRecordDecl *>(RD))
        && !ACO.isAmbiguous() && ACO.getAsSingle<TypeDecl>())
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
  int64_t ComputeResult = callLabelMethod(
      S, LabelExpr, LabelTy, RD, "compute_semantic",
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
  precomputeDynamicTable(S, CS, AllowedMask, LabelGroups, LabelExpr, LabelTy, RD,
                         Loc);

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
      int64_t CECompute = callLabelMethod(
          S, LabelExpr, LabelTy, RD, "compute_semantic",
          static_cast<unsigned>(CESem), Loc);
      if (CECompute >= 1 && CECompute <= 7 &&
          (AllowedMask & (1u << static_cast<unsigned>(CECompute))))
        CESem = static_cast<ContractEvaluationSemantic>(CECompute);
    }
    CS->setCESemantic(CESem);
  }

  // Step 6: Detect local_violation_label facet.
  {
    DeclarationName HCVName =
        &S.Context.Idents.get("handle_contract_violation");
    LookupResult HR(S, HCVName, Loc, Sema::LookupMemberName);
    if (S.LookupQualifiedName(HR, const_cast<CXXRecordDecl *>(RD)) &&
        !HR.isAmbiguous()) {
      Sema::SFINAETrap Trap(S);
      CXXScopeSpec SS;
      ExprResult MR = S.BuildMemberReferenceExpr(
          LabelExpr, LabelTy, Loc, /*IsArrow=*/false, SS,
          /*TemplateKWLoc=*/SourceLocation(),
          /*FirstQualifierInScope=*/nullptr, HR,
          /*TemplateArgs=*/nullptr, /*S=*/nullptr);
      if (!MR.isInvalid())
        CS->setHasLocalHandler(true);
    } else {
      HR.suppressDiagnostics();
    }
  }

  // Step 6b: Detect queryable_label facet.
  {
    DeclarationName QName = &S.Context.Idents.get("query");
    LookupResult QR(S, QName, Loc, Sema::LookupMemberName);
    if (S.LookupQualifiedName(QR, const_cast<CXXRecordDecl *>(RD)) &&
        !QR.isAmbiguous()) {
      Sema::SFINAETrap Trap(S);
      CXXScopeSpec SS;
      ExprResult MR = S.BuildMemberReferenceExpr(
          LabelExpr, LabelTy, Loc, /*IsArrow=*/false, SS,
          /*TemplateKWLoc=*/SourceLocation(),
          /*FirstQualifierInScope=*/nullptr, QR,
          /*TemplateArgs=*/nullptr, /*S=*/nullptr);
      if (!MR.isInvalid()) {
        Expr *QueryMember = MR.get();
        OpaqueValueExpr KeyArg(Loc, S.Context.VoidPtrTy, VK_PRValue);
        OpaqueValueExpr IdxArg(Loc, S.Context.getSizeType(), VK_PRValue);
        Expr *ArgExprs[] = {&KeyArg, &IdxArg};
        ExprResult Call = S.BuildCallToMemberFunction(
            nullptr, QueryMember, Loc, ArgExprs, Loc);
        if (!Call.isInvalid() &&
            Call.get()->getType()->isVoidPointerType())
          CS->setHasQuery(true);
      }
    } else {
      QR.suppressDiagnostics();
    }
  }

  // Step 7: Apply compute_comment facet.
  {
    std::string Comment = CS->getSourceText(S.Context);
    StringRef Result = callLabelStringMethod(
        S, LabelExpr, LabelTy, RD, "compute_comment", Comment,
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
    auto *Cap = PostconditionCaptureDecl::Create(
        Context, DC, IdLoc, IdLoc, Id, T, TInfo, CaptureSC);
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
            InitializedEntity::InitializeVariable(Cap),
            IdLoc, CopyInit.get());
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
  auto *Cap = PostconditionCaptureDecl::Create(
      Context, DC, IdLoc, IdLoc, Id, DeducedType, TInfo, CaptureSC);
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
                                   Expr *Cond, DeclStmt *RND,
                                   Expr *Message, Expr *Label,
                                   DeclStmt *Captures,
                                   ArrayRef<const Attr *> Attrs,
                                   Expr *RequiresClause) {
  StmtResult Res = ContractStmt::Create(Context, CK, KeywordLoc, Cond, RND,
                                        Message, Label, Captures, Attrs,
                                        RequiresClause);

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
  // the contract would already have been discarded in InstantiateContractSpecifier),
  // so without the !inTemplateInstantiation() guard below WillBeRejectedByP4283
  // would be spuriously true and we would skip population for a VALID
  // instantiated contract -- leaving isDynamic() false and falling back to the
  // static semantic (the T6 bug).  Restricting the skip to the primary parse
  // keeps instantiations always populating.  The predicate otherwise mirrors the
  // P4283 check in ActOnContractAssert.
  if (auto *CS = Res.getAs<ContractStmt>()) {
    // Normalize a non-literal (user-generated, P2741-style) diagnostic message
    // to its evaluated string -- getUserMessage() otherwise only understands a
    // StringLiteral message and would return empty for a custom message type
    // (e.g. one with .size()/.data()).  Mirrors static_assert; store the result
    // where getUserMessage() reads it first, before the compute_message facet
    // (in populateContractSemanticState) runs.  Only for a non-dependent
    // message: a dependent one is normalized when the template is instantiated.
    if (Expr *ME = CS->getMessageExpr())
      if (!isa<StringLiteral>(ME) && !ME->isTypeDependent()
          && !ME->isValueDependent() && !CS->hasTransformedMessage()) {
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
    unsigned AllowedMask =
        CS->getAllowedMask() &
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
                                     DeclStmt *Captures,
                                     Expr *RequiresClause) {

  DeclStmt *RNDStmt = nullptr;
  if (RND) {
    StmtResult NewDeclStmt = ActOnDeclStmt(
        ConvertDeclToDeclGroup(RND), RND->getLocation(), RND->getLocation());

    // FIXME(EricWF): Can this happen?
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
    if (CS->hasRequiresClause() &&
        !CurContext->isDependentContext() &&
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

  return ActOnFinishFullStmt(Res.get());
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

  if (RetType->isVoidType()) {
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
  auto *New = ResultNameDecl::Create(Context, CurContext, IDLoc, II, RetType,
                                     nullptr, HasInventedPlaceholderTypes, FunctionScopeDepth);

  if (IsInvalid)
    New->isInvalidDecl();

  // Check for redeclaration of parameters, e.g. int foo(int x, int x);
  if (II) {
    LookupResult R(*this, II, IDLoc, LookupOrdinaryName,
                   RedeclarationKind::ForVisibleRedeclaration); // FIXME(EricWF)
    LookupName(R, S);
    if (!R.empty()) {
      NamedDecl *PrevDecl = *R.begin();
      if (R.isSingleResult() && PrevDecl->isTemplateParameter()) {
        // Maybe we will complain about the shadowed template parameter.
        // DiagnoseTemplateParameterShadow(D.getIdentifierLoc(), PrevDecl);
        // Just pretend that we didn't see the previous declaration.
        PrevDecl = nullptr;
      }
      // FIXME(EricWF): Diagnose lookup conflicts with lambda captures and
      // parameter declarations.
      if (auto *PVD = dyn_cast<ParmVarDecl>(PrevDecl)) {
        Diag(IDLoc, diag::err_result_name_shadows_param)
            << II; // FIXME(EricWF): Change the diagnostic here.
        Diag(PVD->getLocation(), diag::note_previous_declaration);
        New->setInvalidDecl(true);
      }
    }
  }

  if (CK != ContractKind::Post) {
    assert(II && "ResultName requires an identifier");

    Diag(IDLoc, diag::err_result_name_not_allowed) << II;
    New->setInvalidDecl(true);
  }

  assert(!S || S->isContractAssertScope());

  // Add the parameter declaration into this scope.
  if (S)
    S->AddDecl(New);

  IdResolver.AddDecl(New);

  return New;
}

const char *ScopeKindToString(unsigned Val) {
  switch (Val) {
  case 0:
    return "Function";
  case 1:
    return "Block";
  case 2:
    return "Lambda";
  case 3:
    return "CapturingRegion";
  default:
    return "<INVALID>";
  }
}

void showContextChain(const DeclContext *DC, bool Lexical = false) {
  unsigned Depth = 0;
  while (DC != nullptr) {
    EricWFDump(std::string(Lexical ? "Lexical " : "") + "Ctx #" +
                   std::to_string(Depth++),
               DC);
    DC = Lexical ? DC->getLexicalParent() : DC->getParent();
  }
  llvm::errs() << "Found #" << Depth << " Contexts\n\n\n";
}

void debugIt(const Sema &S) {
  llvm::errs() << "\n\n";
  llvm::errs() << "Partial Scopes:\n";
  for (auto *FSI : S.getFunctionScopes()) {
    llvm::errs() << ScopeKindToString(FSI->Kind) << "\n";
  }
  llvm::errs() << "Full Scopes\n";
  for (auto *FSI : S.FunctionScopes) {
    llvm::errs() << ScopeKindToString(FSI->Kind) << "\n";
  }

  llvm::errs() << "\n\nDumping Context\n\n";

  showContextChain(S.CurContext);
  llvm::errs() << "\n\n\nDumping Lexical Context\n\n";

  showContextChain(S.CurContext, true);
  llvm::errs() << "\n\n\n";
}

using ContractScopeRecord = Sema::ContractScopeRecord;

struct ScopeEntry {
  const DeclContext *Ctx = nullptr;

  unsigned FunctionScopeIndex = -1;
  const FunctionScopeInfo *FSI = nullptr;

  unsigned ContractScopeIndex = 0;
  const ContractScopeRecord *CSR = nullptr;

  ScopeEntry(const DeclContext *DC, unsigned FSII, const FunctionScopeInfo *FSI,
             unsigned CSII, const ContractScopeRecord *CSR) :
        Ctx(DC), FunctionScopeIndex(FSII), FSI(FSI),
        ContractScopeIndex(CSII), CSR(CSR) {
  }

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

  void dump() const {
    assert(Ctx != nullptr);
    assert(FSI != nullptr);
    llvm::errs() << "ScopeEntry " << Ctx->getDeclKindName() << " ";
    EricWFDump("Context is: ", Ctx);
    llvm::errs() << "FunctionScopeIndex: " << FunctionScopeIndex << " ";
    llvm::errs() << "ContractScopeIndex: " << ContractScopeIndex << " ";
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
    ERICWF_FANCY_ASSERT(FunctionScopeIndex < FunctionScopes.size()) {
      DumpScopes();
    }
    return FunctionScopes[FunctionScopeIndex];
  }

  const ContractScopeRecord *nextContractScope() {
    --ContractScopeIndex;
    ERICWF_FANCY_ASSERT(ContractScopeIndex < ContractScopes.size()) {
      DumpScopes();
    }
    return ContractScopes[ContractScopeIndex];
  }

  void DumpScopes() const {

    llvm::errs() << "Have # of scopes: " << Scopes.size() << "\n";
    llvm::errs() << "FunctionScopes.size() " << FunctionScopes.size() << "\n";
    llvm::errs() << "ContractScopes.size() " << ContractScopes.size() << "\n";
    llvm::errs() << "Actual Number of FunctionScopes: "
                 << S.FunctionScopes.size() << "\n";
    llvm::errs() << "Start FunctionScopeIndex: " << S.FunctionScopesStart
                 << "\n";
    unsigned Idx = 0;

    for (auto SC : llvm::reverse(Scopes)) {
      llvm::errs() << "ScopeEntry " << Idx++ << " ";
      SC.dump();
    }

    debugIt(S);
  }

  SmallVector<ScopeEntry> doIt() {
    while (CurCtx) {
      auto *FSI = nextFuncScope();
      ERICWF_FANCY_ASSERT(FSI) { DumpScopes(); }
      const ContractScopeRecord *CSR = nullptr;
      if (FSI->ContractScopeIndex != unsigned(-1)) {
        CSR = &S.ContractScopeStack[FSI->ContractScopeIndex];
        ERICWF_FANCY_ASSERT(CSR && CSR->FunctionScopeAtPush == FSI) {
          DumpScopes();
        }
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

    ERICWF_FANCY_ASSERT((Scopes.size() <= FunctionScopes.size() && Scopes.size() >= S.getFunctionScopes().size()) ||
                        (Scopes.size() == FunctionScopes.size() - 1 && CurCtx &&
                         CurCtx->isRecord())) {
      DumpScopes();
    }
    ERICWF_FANCY_ASSERT(Scopes.size() >= ContractScopes.size()) {
      DumpScopes();
    }
    SmallVector<ScopeEntry> Result{Scopes.rbegin(), Scopes.rend()};
    Scopes = std::move(Result);
    return Result;
  }

  const Sema &S;
  const DeclContext *CurCtx;

  SmallVector<FunctionScopeInfo *, 4> FunctionScopes;
  unsigned FunctionScopeIndex;

  SmallVector<const ContractScopeRecord*> ContractScopes;
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
  ERICWF_FANCY_ASSERT(VarCtx && VarCtx->isFunctionOrMethod()) {
    EricWFDump(VarCtx);
  }

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
      if (Redecl->getTemplateSpecializationKind() == TSK_ExplicitSpecialization &&
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
      return CS->hasCaptures()
                 ? CS->getCapturesDeclStmt()->getSourceRange()
                 : CS->getCond()->getSourceRange();
    case DK_RequiresClause:
      return CS->hasRequiresClause()
                 ? CS->getRequiresClause()->getSourceRange()
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

  enum DiagSelector {
    DS_None = -1,
    DS_Array = 0,
    DS_Function = 1,
    DS_NotConst = 2
  };

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

private:
  DenseSet<ParmVarDecl *> DiagnosedDecls;
};

} // namespace

static void diagnoseParamTypes(Sema &S, FunctionDecl *FD,
                               ContractSpecifierDecl *CSD) {

  // Check for post-conditions that reference non-const parameters.
  ParamReferenceChecker Checker(S, FD);
  for (auto *CS : CSD->postconditions()) {
    Checker.TraverseContractStmt(CS);
    // FIXME(EricWF): DIagnose non-const function param types.
  }
}

void Sema::CheckFunctionContracts(FunctionDecl *FD, bool IsDefinition, bool IsInstantiation) {
  assert(FD && FD->hasContracts());

  if (auto *MD = dyn_cast<CXXMethodDecl>(FD)) {
    if (MD->isVirtual() && !getLangOpts().ContractsP3097) {
      Diag(FD->getLocation(), diag::err_contracts_on_virtual_require_flag);
      return;
    }
  }

  diagnoseParamTypes(*this, FD, FD->getContracts());
}

void Sema::InstantiateContractSpecifier(
    SourceLocation PointOfInstantiation, FunctionDecl *Instantiation,
    const FunctionDecl *Pattern,
    const MultiLevelTemplateArgumentList &TemplateArgs) {

  ContractSpecifierDecl *PatternCSD = Pattern->getContracts();
  if (!PatternCSD)
    return;

  // Idempotent: at class-template instantiation the member is given the
  // pattern's own (dependent) contract specifier as a placeholder
  // (VisitCXXMethodDecl).  Once we have replaced it with a substituted
  // specifier, the instantiation carries a distinct ContractSpecifierDecl, so
  // there is nothing more to do.  This lets the contracts be instantiated
  // on-demand at the point of an odr-use (see
  // InstantiateVirtualFunctionContractsOnUse) without being re-substituted when
  // the function's definition is later instantiated.
  if (Instantiation->getContracts() &&
      Instantiation->getContracts() != PatternCSD)
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
    if (auto *NewCS = NewStmt.getAs<ContractStmt>())
      NewContracts.push_back(NewCS);
  }

  // P4283: If all contracts were discarded by requires clauses, don't
  // create a ContractSpecifierDecl at all.
  if (NewContracts.empty() && !IsInvalid)
    return;

  ContractSpecifierDecl *NewCSD = BuildContractSpecifierDecl(
      NewContracts, Instantiation, PatternCSD->getLocation(), IsInvalid);
  assert(NewCSD);

  Instantiation->setContracts(NewCSD);
  if (!Instantiation->isDependentContext())
    CheckFunctionContracts(Instantiation, /*IsDefinition=*/false, /*IsInstantiation=*/true);
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
  const bool IsTemplateSpecialization = FD->getTemplateSpecializationKind() == TSK_ExplicitSpecialization;

  auto *First = FD->getFirstDecl();

  // If the new declaration/definition doesn't have contracts, and it's previous declarations didn't have
  // contracts or it's a specialiazation which doesn't inherit the contracts, then there's nothing to do.

  if (!FD->hasContracts() && !First->hasContracts()) {
    return;
  }



  // If the definition has omitted the contracts, but the first declaration has
  // them, we need to rebuild the contracts to refer to the parameters of the
  // definition.
  //
  // For function templates, we'll create a copy when we instantiate the definition.
  if (First->hasContracts() && !FD->hasContracts() && IsDefinition &&
      !FD->isTemplateInstantiation() && !IsTemplateSpecialization) {
    // Note: This case is mutually exclusive with the NonDependentPlaceholders
    // case, since we can't have a placeholder return type on a declaration that
    // isn't a definition.

    // TODO: I don't think we should be rebuilding for
    assert(FD->getTemplatedKind() != FunctionDecl::TK_FunctionTemplateSpecialization);
    assert(FD->getTemplatedKind() != FunctionDecl::TK_MemberSpecialization);

    ContractSpecifierDecl *NewCSD = RebuildContractSpecifierForDecl(First, FD);
    NewCSD->setOwningFunction(FD);
    FD->setContracts(NewCSD);
  }

  if (!FD->hasContracts())
    return;

  assert(FD->hasContracts());

  ContractSpecifierDecl *CSD = FD->getContracts();

  if (!D->isTemplateInstantiation()) {
    // Note: IsInstantiation here means whether we're calling during the instantiation of the contract specifier,
    // rather than whether the FD declares a function instantiation.
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
    ThisScope.emplace(*this, CXXMethod->getParent(),
                      MethodQuals,
                      /*IsLambda*/ false);
  }
  RebuildFunctionContracts Rebuilder(*this, true);
  for (unsigned I = 0; I < First->getNumParams(); ++I) {
    Rebuilder.transformedLocalDecl(First->getParamDecl(I),
                                   Def->getParamDecl(I));
  }
  for (auto *RND : First->getContracts()->result_names()) {
    QualType Replacement = Def->getReturnType();
    auto *NewRND =
        ActOnResultNameDeclarator(ContractKind::Post, nullptr, Replacement,
                                  RND->getLocation(), RND->getIdentifier(),
                                  RND->getFunctionScopeDepth());
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
    else
      NewContracts.push_back(NewStmt.getAs<ContractStmt>());
  }

  auto *CSD = BuildContractSpecifierDecl(
      NewContracts, Def, First->getContracts()->getLocation(), IsInvalid);
  Def->setContracts(CSD);

  if (CSD->isInvalidDecl())
    Def->isInvalidDecl();
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
      if (DeduceReturnType(FD, Loc, true)) {
        Diag(CSD->getLocation(), diag::err_ericwf_unimplemented)
            << "IDK what's wrong";
        return true;
      }
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
    auto *NewRND =
        ActOnResultNameDeclarator(ContractKind::Post, nullptr, Replacement,
                                  RND->getLocation(), RND->getIdentifier(), RND->getFunctionScopeDepth());
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
    else
      NewContracts.push_back(NewStmt.getAs<ContractStmt>());
  }

  auto *NewCSD = BuildContractSpecifierDecl(
      NewContracts, FD, FD->getContracts()->getLocation(), IsInvalid);

  FD->setContracts(NewCSD);
  if (NewCSD->isInvalidDecl())
    FD->isInvalidDecl();

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
    auto *NewCSD = Res.getAs<ContractSpecifierDecl>();
    assert(NewCSD);
    NewCSD->setOwningFunction(Def);
    Def->setContracts(NewCSD);
    if (NewCSD->isInvalidDecl())
      Def->setInvalidDecl(true);
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
      Def->isInvalidDecl();
    }
  }
}

bool Sema::isUsageAcrossContract(const ValueDecl *VD) {
  // There's no contract scope anywhere above us.
  if (!getCurrentContractEntry())
    return false;

  // Fast Path: We're in an immediate contract assertion expression evaluation context.
  if (isContractAssertionContext())
    return true;

  if (isa<VarDecl>(VD) && !cast<VarDecl>(VD)->isLocalVarDeclOrParm())
    return false;

  assert(VD);
  return getInterveningContractEntry(*this, VD) != nullptr;
  ;
#if 0
  if (!ContractScopes.empty())
    return true;

  // We're going to walk up from the DeclContext we captured when we entered the contract
  // scope to try and find the declaration context of the specified decl. If we do,
  // then the decl is used across a contract.

  // FIXME(EricWF): Why is this here?
  if (isa<VarDecl>(VD))
    VD = cast<VarDecl>(VD)->getCanonicalDecl();

  const DeclContext *DC = VD->getDeclContext();
  if (!DC->isFunctionOrMethod()) {
    return false;
  }



  assert(getCurrentContractEntry());


  // FIXME(EricWF): This seems expensive?
  // Make sure the ValueDecl has a DeclContext that is the same as or a parent
  // of the most recent contract entry
  auto *StartContext = getCurrentContractEntry()->ContextAtPush;

  while (StartContext) {
    if (StartContext->isFileContext())
      break;
    if (StartContext->Equals(VD->getDeclContext()))
      return true;
    StartContext = StartContext->getParent();
  }
  return false;
#endif
}

/// [basic.contract.general]
/// Within the predicate of a contract assertion, id-expressions referring to
/// variables with automatic storage duration are const ([expr.prim.id.unqual])
ContractConstification Sema::getContractConstification(const ValueDecl *VD) {
  //WalkUpContractScopesTest();
  auto &S = *this;
  if (!S.LangOpts.ContractConstification)
    return CC_None;

  assert(VD);
  const ContractScopeRecord *CSR = S.getCurrentContractEntry();

  if (!CSR || CSR->ContextAtPush->Encloses(VD->getDeclContext()))
    return CC_None;


  // If there is no contract scope that encloses the current context, then we don't need to constify the variable.
  if (getLastEnclosingContractScopeForContext(CurContext) == nullptr)
    return CC_None;

  CSR = getLastEnclosingContractScopeForContext(CurContext);
  if (CSR == nullptr)
    return CC_None;

  if (VD->getDeclContext()->Encloses(CSR->ContextAtPush)) {
    assert(!VD->getDeclContext()->Equals(CSR->ContextAtPush));
  }

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

  // — a structured binding of type T whose corresponding variable has automatic
  // storage
  //  duration, or
  if (auto *Bound = dyn_cast<BindingDecl>(VD)) {
    if (!Bound->getHoldingVar())
      return CC_None;
    auto Var = Bound->getHoldingVar();
    if (Var->isLocalVarDeclOrParm() &&
        (Var->getStorageDuration() == SD_Automatic ||
         Var->getKind() == Decl::ParmVar))
      return CC_ApplyConst;
    return CC_None;
  }

  // Postcondition captures are not constified (P3098 Section 4.4.1)
  if (isa<PostconditionCaptureDecl>(VD))
    return CC_None;

  // — a variable with automatic storage duration ...
  if (auto Var = dyn_cast<VarDecl>(VD);
      Var && Var->isLocalVarDeclOrParm() &&
      (Var->getStorageDuration() == SD_Automatic ||
       Var->getKind() == Decl::ParmVar)) {
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

static const DeclContext* walkUpDeclContextToFunction(const DeclContext *DC, bool AllowLambda = false) {
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
  } else {
    llvm::errs() << "Found Null DC";
  }
  if (DC && !DC->isFunctionOrMethod())
    return nullptr;
  return DC;
}

QualType Sema::adjustCXXThisTypeForContracts(QualType QT) {
  if (!getCurrentContractEntry() || !LangOpts.ContractConstification)
    return QT;

  // 'this' is constified any time the `this` object that is captured by a lambda which exists fully
  // within a contract.
  //
  // We need to ensure that we haven't entered a nested member function context, because in that case we
  // don't want to constify the `this` object.
  // For example:
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
  const DeclContext *QTContext = walkUpDeclContextToFunction(CurContext );
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

/// TODO(EricWF): Remove this and do it inline instead. We currently
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
        auto& Usage = Pos->second;
        if (Usage.UsageExpr == nullptr || (isa<LambdaExpr>(Usage.UsageExpr) && !isa<LambdaExpr>(E))) {
          Usage.UsedInContract = CurContract;
          Usage.UsageExpr = E;
          Usage.UsageLoc = Loc;
        }
      }
    }
  }

private:
  LambdaCaptureChecker(Sema &S, LambdaExpr *LE) : Actions(S), CurLambda(LE) { Init(); }


  void Run() {
    // Traverse the lambdas function-level contracts and body to find the bad captures.
    FunctionDecl *FD = CurLambda->getCallOperator();
    assert(FD->getBody());
    if (FD->hasContracts())
      TraverseDecl(FD->getContracts());
    TraverseStmt(CurLambda->getBody());

    // Finally, diagnose any captures that still remain, since they do not have any non-contrac
    // usages.
    for (auto& [Var, Bad] : Captures) {
      // We likely didn't see the usage because there was a intervening lambda that captured by copy.
      if (Bad.UsedInContract == nullptr)
        continue;
      Actions.Diag(CurLambda->getCaptureDefaultLoc(),
                   diag::err_lambda_implicit_capture_in_contracts_only)
          << (int)Bad.Capture.capturesThis() << cast_or_null<NamedDecl>(Var);
      SourceLocation UsageLoc = Bad.UsageLoc;
      Actions.Diag(UsageLoc, diag::note_lambda_implicit_capture_in_contract_usage)
            << (int)Bad.Capture.capturesThis() << cast_or_null<NamedDecl>(Var);
      if (Bad.UsedInContract)
        Actions.Diag(Bad.UsedInContract->getBeginLoc(),
                    diag::note_contract_context);

    }
  }

  void Init() {
    // Collect all of the implicit captures of the lambda.
    // If the lambda capture hasn't been removed after traversing the tree then
    // that lambda capture is bad, and must be diagnosed as only being used inside
    // of a contract.
    for (auto C : CurLambda->captures()) {
      if (C.capturesThis() && C.isImplicit())
        Captures.insert({nullptr, {C}});
      if (!C.capturesVariable())
        continue;
      if (C.isExplicit())
        continue;
      // EricWFDump("Inserting Capture ", C.getCapturedVar(), &Actions.Context);
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
    // Iterate over the captures of the nested lambda, and mark any of our captures as having been seen
    // outside of a contract. This assumes that the inner lambda has a valid usage of the capture.
    // If it doesn't, we'll diagnose that separately.
    for (auto C : LE->captures()) {
      // FIXME(EricWF): Figure out how to deal with VLA captures here
      if (C.capturesVLAType())
        continue;

      assert(C.capturesThis() || C.capturesVariable());
      observeUsage(C.capturesThis() ? nullptr : C.getCapturedVar(), LE, C.getLocation());
    }
    return true;
  }

  bool VisitDeclRefExpr(DeclRefExpr *E) {
    if (auto *VD = dyn_cast<ValueDecl>(E->getDecl()); VD && E->isNonOdrUse() != NOUR_Unevaluated)
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

#if 0
  return getCurrentContractEntry() && (getCurrentContractEntry()->FunctionScopeAtPush == getCurFunction() ||
      (getCurrentContractEntry()));
  if (FunctionScopes.empty()) {
    auto *CR = getCurrentContractEntry();
    return CR != nullptr;
    return ExprEvalContexts.back().isContractAssertionContext() ||
           (getCurrentContractEntry() &&
            getCurrentContractEntry()->HadNoFunctionScope);

  }
  return FunctionScopes.back()->InContract;
#endif
}

ArrayRef<Sema::ContractScopeRecord> Sema::getAllContractScopes() const {
  return llvm::ArrayRef(ContractScopeStack.begin(), ContractScopeStack.end());
}
ArrayRef<Sema::ContractScopeRecord> Sema::getContractScopes() const {
  return getAllContractScopes();
#if 0
  unsigned Offset = 0;
  for (auto Pos = ContractScopeStack.begin(); Pos != ContractScopeStack.end();
       ++Pos) {
    if (Pos->FunctionScopeStartAtPush == FunctionScopesStart)
      break;
    ++Offset;
  }
  return llvm::ArrayRef(ContractScopeStack.begin() + Offset,
                        ContractScopeStack.end());
#endif
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
    SmallVector<const ContractScopeRecord*> Out;
    for (auto & CS : CL) {
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
    return llvm::ArrayRef(ContractScopeStack.begin() + StartIdx, ContractScopeStack.end());

  };

  while (Pos != CScopes.begin()) {
    --Pos;
    const ContractScopeRecord *CS = *Pos;
    assert(CS->ContextAtPush);
    if (!CS->ContextAtPush->isFunctionOrMethod()) {
      llvm::errs() << "Had non-function context\n";
      EricWFDump(CS->ContextAtPush);
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


Sema::ContractScopeRAII::ContractScopeRAII(Sema &S, ContractKind CK, ContractScopeOffset ScopeOffset, SourceLocation Loc)
    : S(S) {
  S.PushContractScope(CK, ScopeOffset, Loc);
}

Sema::ContractScopeRAII::~ContractScopeRAII() {
  S.PopContractScope();
}


void Sema::PushContractScope(ContractKind Kind, ContractScopeOffset ScopeOffset, SourceLocation Loc) {
//  assert(!FunctionScopes.empty());

  ContractScopeRecord Record{
         .Index = static_cast<unsigned>(ContractScopeStack.size()),
         .Kind = Kind,
         .ScopeOffset = ScopeOffset,
         .KeywordLoc = Loc,
         .ContextAtPush = CurContext,
         .PreviousCXXThisType = CXXThisTypeOverride,
         .FunctionIndex = static_cast<unsigned>(
             FunctionScopes.empty() ? 0ul : FunctionScopes.size() - 1),
         .StartFunctionIndex = FunctionScopesStart,
         .FunctionScopeAtPush = getCurFunction(),
         .AddedConstToCXXThis = false,
         .WasInContractContext = ExprEvalContexts.back().InContractAssertion,
         .HadNoFunctionScope = FunctionScopes.empty(),
         .FunctionScopeStartAtPush = FunctionScopesStart};

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

    //assert(!S.FunctionScopes.empty());


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

  assert(ContractScopeIndexMap.contains(Record.ContextAtPush) && ContractScopeIndexMap[Record.ContextAtPush]  == Record.Index);
  ContractScopeIndexMap.erase(Record.ContextAtPush);


  assert(ExprEvalContexts.back().InContractAssertion == true);
  ExprEvalContexts.back().InContractAssertion = Record.WasInContractContext;
  CXXThisTypeOverride = Record.PreviousCXXThisType;


  if (Record.FunctionScopeAtPush) {

    if (FunctionScopes.back() == Record.FunctionScopeAtPush) {
      assert(FunctionScopes.back()->ContractScopeIndex != unsigned(-1));
      FunctionScopes.back()->ContractScopeIndex = -1;
    } else {
      assert(getFunctionScopes().empty() || FunctionScopes.size() < Record.FunctionIndex);
    }
  }

  return Record;
}

const ContractScopeRecord *Sema::getContractScopeForContext(const DeclContext *DC) const {
  auto Pos = ContractScopeIndexMap.find(DC);
  if (Pos == ContractScopeIndexMap.end())
    return nullptr;
  unsigned Idx = Pos->second;
  assert(Idx < ContractScopeStack.size());
  return &ContractScopeStack[Idx];
}

const ContractScopeRecord *Sema::getFirstEnclosingContractScopeForContext(const DeclContext *DC) const {
  for (unsigned I=0; I < ContractScopeStack.size(); ++I) {
    if (ContractScopeStack[I].ContextAtPush->Encloses(DC)) {
      return &ContractScopeStack[I];
    }
  }
  return nullptr;

}


const ContractScopeRecord *Sema::getLastEnclosingContractScopeForContext(const DeclContext *DC) const {
  unsigned LastEnclosingIdx = unsigned(-1);
  for (unsigned I=0; I < ContractScopeStack.size(); ++I) {
    if (ContractScopeStack[I].ContextAtPush->Encloses(DC)) {
      LastEnclosingIdx = I;
    }
  }
  if (LastEnclosingIdx == unsigned(-1))
    return nullptr;
  assert(ContractScopeStack.size() > LastEnclosingIdx);
  return &ContractScopeStack[LastEnclosingIdx];

}


const ContractScopeRecord *Sema::getFirstEnclosedContractScopeForContext(const DeclContext *DC) const {
  for (unsigned I=0; I < ContractScopeStack.size(); ++I) {
    if (DC->Encloses(ContractScopeStack[I].ContextAtPush) || DC->Equals(ContractScopeStack[I].ContextAtPush))
      return &ContractScopeStack[I];
  }
  return nullptr;
}


const ContractScopeRecord *Sema::getLastEnclosedContractScopeForContext(const DeclContext *DC) const {
  unsigned LastIdx = unsigned(-1);
  for (unsigned I=0; I < ContractScopeStack.size(); ++I) {
    if (DC->Encloses(ContractScopeStack[I].ContextAtPush) || DC->Equals(ContractScopeStack[I].ContextAtPush)) {
      if (I > LastIdx || LastIdx == unsigned(-1))
        LastIdx = I;
    }
  }
  if (LastIdx == unsigned(-1))
    return nullptr;
  assert(LastIdx < ContractScopeStack.size());
  return &ContractScopeStack[LastIdx];

}

SourceLocation Sema::getContractLocForFunctionScope(const sema::FunctionScopeInfo *FSI) const {
  assert(FSI->isInContract());
  assert(FSI->ContractScopeIndex < ContractScopeStack.size());
  return ContractScopeStack[FSI->ContractScopeIndex].KeywordLoc;
}
