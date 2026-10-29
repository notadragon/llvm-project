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
#include "clang/AST/CharUnits.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ExprConcepts.h"
#include "clang/AST/IgnoreExpr.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtCXX.h"
#include "clang/AST/TypeLoc.h"
#include "clang/Basic/ContractOptions.h"
#include "clang/Basic/Module.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/EnterExpressionEvaluationContext.h"
#include "clang/Sema/Initialization.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Ownership.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/ScopeInfo.h"
#include "clang/Sema/SemaInternal.h"
#include "clang/Sema/Template.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallVector.h"

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
    assert(!Entered && "a ValueRAII sets its value once");
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

ExprResult Sema::ActOnContractAssertCondition(Expr *Cond) {
  assert(currentEvaluationContext().isContractAssertionContext() &&
         "Wrong context for statement");

  // A type-dependent predicate is finished as a full-expression all the same:
  // that is where a generic lambda commits the potential captures the
  // predicate names, and without it they wait for the next full-expression in
  // the body, if there is one, and are otherwise never made -- so an entity
  // the lambda's contract assertions alone name went uncaptured and was
  // reported at instantiation as a reference to a local of the enclosing
  // function, not as a capture for contract assertions only.
  if (Cond->isTypeDependent())
    return ActOnFinishFullExpr(Cond, Cond->getExprLoc(),
                               /*DiscardedValue=*/false);

  ConditionResult Res =
      ActOnCondition(getCurScope(), Cond->getExprLoc(), Cond,
                     Sema::ConditionKind::Boolean, /*MissingOK=*/false);
  if (Res.isInvalid())
    return ExprError();
  Cond = Res.get().second;
  assert(Cond && "a usable condition has an expression");
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
  // Access is judged as the concept judges it, from outside the label, not
  // from the contract: a contract in a member or friend of the label must not
  // see a private facet.
  Sema::ContextRAII OutsideLabel(S, S.Context.getTranslationUnitDecl());
  LookupResult::Filter F = R.makeFilter();
  while (F.hasNext()) {
    NamedDecl *D = F.next();
    if (!S.IsSimplyAccessible(D, const_cast<CXXRecordDecl *>(RD), LabelTy))
      F.erase();
  }
  F.done();
  R.suppressAccessDiagnostics();
}

/// Look up the member R names in label class RD as the facet concepts see
/// it: unambiguous, and with only the candidates accessible from outside the
/// label (dropInaccessibleFacetCandidates).  False, with R's diagnostics
/// suppressed, when no such member is left.
static bool lookupFacetMember(Sema &S, const CXXRecordDecl *RD,
                              QualType LabelTy, LookupResult &R) {
  if (!S.LookupQualifiedName(R, const_cast<CXXRecordDecl *>(RD)) ||
      R.isAmbiguous()) {
    R.suppressDiagnostics();
    return false;
  }
  dropInaccessibleFacetCandidates(S, RD, LabelTy, R);
  return !R.empty();
}

/// `Obj.member', the member R names, as a facet call's callee or a facet
/// variable's value.
static ExprResult buildFacetMemberRef(Sema &S, Expr *Obj, QualType ObjTy,
                                      LookupResult &R, SourceLocation Loc) {
  CXXScopeSpec SS;
  return S.BuildMemberReferenceExpr(Obj, ObjTy, Loc, /*IsArrow=*/false, SS,
                                    /*TemplateKWLoc=*/SourceLocation(),
                                    /*FirstQualifierInScope=*/nullptr, R,
                                    /*TemplateArgs=*/nullptr, /*S=*/nullptr);
}

/// The library's std::contracts::contract_violation, or a null QualType when
/// <contracts> is not in scope.
///
/// Facet detection needs the real type.  The concepts ask whether the member
/// is callable with a `const contract_violation&', and probing with anything
/// else -- the member's own parameter type, say -- would be circular and would
/// admit a same-named helper of a different shape as a facet.
static QualType getStdContractsTagType(Sema &S, StringRef Name,
                                       SourceLocation Loc) {
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

  LookupResult CR(S, &S.Context.Idents.get(Name), Loc, Sema::LookupTagName);
  if (!S.LookupQualifiedName(CR, Contracts)) {
    CR.suppressDiagnostics();
    return QualType();
  }
  auto *TD = CR.getAsSingle<TagDecl>();
  if (!TD)
    return QualType();
  return S.Context.getCanonicalTagType(TD);
}

static QualType getContractViolationType(Sema &S, SourceLocation Loc) {
  QualType T = getStdContractsTagType(S, "contract_violation", Loc);
  return !T.isNull() && T->getAsCXXRecordDecl() ? T : QualType();
}

std::optional<ExprResult> Sema::BuildImplicitMemberExprInContract(
    const CXXScopeSpec &SS, SourceLocation TemplateKWLoc, LookupResult &R,
    const TemplateArgumentListInfo *TemplateArgs) {
  // [over.call.func]/3.1 and [expr.prim.id.general]: an unqualified reference
  // to a non-static member -- which forms an implied 'this' access here -- is
  // ill-formed in a constructor precondition or a destructor postcondition,
  // because no object is within its lifetime at that point.  Only a
  // potentially evaluated reference is: the paragraph's own example has
  // `C() pre(sizeof(b) > 0);  // OK, b is not potentially evaluated`.
  //
  // P3098 adds the initializer of a postcondition capture of a
  // constructor, which runs at entry too.  The captures are parsed ahead of
  // the predicate's contract scope, so that case has its own flag.
  const ContractScopeRecord *CSR = getCurrentContractEntry();
  if ((CSR || InPostconditionCaptureInit) && !isUnevaluatedContext()) {
    const DeclContext *FnDC = getFunctionLevelDeclContext();
    bool CtorCapture =
        InPostconditionCaptureInit && isa_and_nonnull<CXXConstructorDecl>(FnDC);
    bool CtorPre = !InPostconditionCaptureInit && CSR &&
                   CSR->Kind == ContractKind::Pre &&
                   isa_and_nonnull<CXXConstructorDecl>(FnDC);
    bool DtorPost = !InPostconditionCaptureInit && CSR &&
                    CSR->Kind == ContractKind::Post &&
                    isa_and_nonnull<CXXDestructorDecl>(FnDC);
    if (CtorPre || DtorPost || CtorCapture) {
      // A call is ill-formed only if overload resolution selects a
      // non-static member function ([over.call.func]/3).  For an overload set
      // that also has a static member, build the reference as in a static
      // member function, where resolution diagnoses a non-static selection.
      bool HasStatic = llvm::any_of(R, [](const NamedDecl *D) {
        const auto *MD = dyn_cast<CXXMethodDecl>(D->getUnderlyingDecl());
        if (const auto *FTD =
                dyn_cast<FunctionTemplateDecl>(D->getUnderlyingDecl()))
          MD = dyn_cast<CXXMethodDecl>(FTD->getTemplatedDecl());
        return MD && MD->isStatic();
      });
      if (HasStatic) {
        if (TemplateArgs || TemplateKWLoc.isValid())
          return BuildTemplateIdExpr(SS, TemplateKWLoc, R,
                                     /*RequiresADL=*/false, TemplateArgs);
        return BuildDeclarationNameExpr(SS, R, /*NeedsADL=*/false,
                                        /*AcceptInvalidDecl=*/false);
      }
      Diag(R.getNameLoc(), diag::err_contract_member_access_without_object)
          << R.getLookupName()
          << (CtorCapture ? 2
              : DtorPost  ? 1
                          : 0);
      // Recover a data member as an erroneous expression of its type, so the
      // expression around it keeps its type: a lambda in the predicate that
      // returns it then deduces the right return type, instead of void and a
      // second error where the predicate is converted to bool.
      if (const auto *FD = R.getAsSingle<FieldDecl>())
        return CreateRecoveryExpr(R.getNameLoc(), R.getNameLoc(), {},
                                  FD->getType().getNonReferenceType());
      return ExprResult(ExprError());
    }
  }
  return std::nullopt;
}

bool Sema::isUsingDirectiveInEffect(const UsingDirectiveDecl *UD) {
  return isVisible(UD) &&
         (!UD->isContractControl() || InAssertionControlExpression);
}

void Sema::CheckContractViolationHandler(FunctionDecl *FD) {
  if (!getLangOpts().Contracts)
    return;
  if (!FD->getDeclName().isIdentifier() ||
      FD->getName() != "handle_contract_violation")
    return;
  if (!FD->getDeclContext()->getRedeclContext()->isTranslationUnit())
    return;

  SourceLocation Loc = FD->getLocation();

  if (FD->isInlineSpecified())
    Diag(Loc, diag::err_contract_violation_handler_invalid) << 0;

  if (FD->isExternC())
    Diag(Loc, diag::err_contract_violation_handler_invalid) << 1;

  if (FD->getOwningModule() && FD->getOwningModule()->isNamedModule())
    Diag(Loc, diag::err_contract_violation_handler_invalid) << 2;

  QualType RetTy = FD->getReturnType();
  if (!RetTy->isVoidType())
    Diag(Loc, diag::err_contract_violation_handler_invalid) << 3;

  if (FD->getNumParams() != 1) {
    Diag(Loc, diag::err_contract_violation_handler_invalid) << 4;
    return;
  }

  QualType ParmTy = FD->getParamDecl(0)->getType();
  if (!ParmTy->isLValueReferenceType()) {
    Diag(Loc, diag::err_contract_violation_handler_invalid) << 5;
    return;
  }

  // Exactly const: a const volatile reference is not the required type.
  QualType RefTy = ParmTy->getPointeeType();
  if (RefTy.getQualifiers() != Qualifiers::fromCVRMask(Qualifiers::Const)) {
    Diag(Loc, diag::err_contract_violation_handler_invalid) << 6;
    return;
  }

  QualType ViolationTy = getContractViolationType(*this, Loc);
  if (ViolationTy.isNull() ||
      !Context.hasSameUnqualifiedType(RefTy, ViolationTy))
    Diag(Loc, diag::err_contract_violation_handler_invalid) << 7;
}

namespace {
/// The members of an assertion-control object that P3400's label concepts
/// name.  The first five are member functions, called with arguments the
/// concept fixes; the next two are data members; the last, D4405's, is a
/// member function called with no argument.
enum class LabelFacet {
  ComputeSemantic,
  ComputeComment,
  ComputeMessage,
  HandleContractViolation,
  Query,
  AllowedSemantics,
  GroupNames,
  AnalysisSemantic,
};
} // namespace

static constexpr LabelFacet LabelFunctionFacets[] = {
    LabelFacet::ComputeSemantic, LabelFacet::ComputeComment,
    LabelFacet::ComputeMessage, LabelFacet::HandleContractViolation,
    LabelFacet::Query};

static StringRef getLabelFacetName(LabelFacet F) {
  switch (F) {
  case LabelFacet::ComputeSemantic:
    return "compute_semantic";
  case LabelFacet::ComputeComment:
    return "compute_comment";
  case LabelFacet::ComputeMessage:
    return "compute_message";
  case LabelFacet::HandleContractViolation:
    return "handle_contract_violation";
  case LabelFacet::Query:
    return "query";
  case LabelFacet::AllowedSemantics:
    return "allowed_semantics";
  case LabelFacet::GroupNames:
    return "group_names";
  case LabelFacet::AnalysisSemantic:
    return "analysis_semantic";
  }
  llvm_unreachable("unknown label facet");
}

/// A LookupResult for the label member that provides facet F.
static LookupResult makeFacetLookup(Sema &S, LabelFacet F, SourceLocation Loc) {
  return LookupResult(S, &S.Context.Idents.get(getLabelFacetName(F)), Loc,
                      Sema::LookupMemberName);
}

/// The function through which the runtime calls a label's local violation
/// handler (IsQuery false) or query facet (IsQuery true), one per label type:
///
///   static int H(const void *__label, const void *__violation) {
///     return int(violation_handled(
///         static_cast<const L *>(__label)->handle_contract_violation(
///             *static_cast<const contract_violation *>(__violation))));
///   }   // or, for a void handler, the call and then `return 0;'
///   static void *Q(const void *__label, const void *__key, size_t __index) {
///     return static_cast<const L *>(__label)->query(__key, __index);
///   }
///
/// These are the signatures the runtime calls.  The facet call is built with
/// ordinary lookup and overload resolution, so every shape the label concepts
/// admit -- a member template, a default argument, an explicit object
/// parameter, a result convertible to violation_handled, a static member, a
/// member of a base -- is called as the concept calls it.  Null if the call
/// cannot be built, which facet detection has already ruled out.
static FunctionDecl *getLabelFacetTrampoline(Sema &S, QualType LabelTy,
                                             const CXXRecordDecl *RD,
                                             bool IsQuery, SourceLocation Loc) {
  ASTContext &C = S.Context;
  QualType ObjTy = LabelTy.getUnqualifiedType().withConst();
  auto Key = std::make_pair(C.getCanonicalType(ObjTy).getTypePtr(),
                            static_cast<unsigned>(IsQuery));
  if (FunctionDecl *Cached = S.ContractLabelFacetTrampolines.lookup(Key))
    return Cached;

  QualType CVTy;
  QualType HandledTy;
  if (!IsQuery) {
    CVTy = getContractViolationType(S, Loc);
    if (CVTy.isNull())
      return nullptr;
  }

  // Name it after the label type: one per type, and the same in every TU.
  std::string Name =
      IsQuery ? "__clang_contract_query_" : "__clang_contract_local_handler_";
  {
    llvm::raw_string_ostream OS(Name);
    std::unique_ptr<MangleContext> MC(C.createMangleContext());
    MC->mangleCanonicalTypeName(LabelTy.getUnqualifiedType(), OS);
  }

  QualType ConstVoidPtr = C.getPointerType(C.VoidTy.withConst());
  SmallVector<QualType, 3> ParamTys = {ConstVoidPtr, ConstVoidPtr};
  if (IsQuery)
    ParamTys.push_back(C.getSizeType());
  QualType RetTy = IsQuery ? C.VoidPtrTy : C.IntTy;
  QualType FnTy =
      C.getFunctionType(RetTy, ParamTys, FunctionProtoType::ExtProtoInfo());
  FunctionDecl *FD = FunctionDecl::Create(
      C, C.getTranslationUnitDecl(), Loc, Loc, &C.Idents.get(Name), FnTy,
      C.getTrivialTypeSourceInfo(FnTy, Loc), SC_Static,
      S.getCurFPFeatures().isFPConstrained(), /*isInlineSpecified=*/false,
      /*hasWrittenPrototype=*/true);
  FD->setImplicit();

  static constexpr StringRef ParamNames[2][3] = {
      {"__label", "__violation", ""}, {"__label", "__key", "__index"}};
  SmallVector<ParmVarDecl *, 3> Params;
  for (unsigned I = 0; I != ParamTys.size(); ++I) {
    auto *P = ParmVarDecl::Create(
        C, FD, Loc, Loc, &C.Idents.get(ParamNames[IsQuery][I]), ParamTys[I],
        C.getTrivialTypeSourceInfo(ParamTys[I], Loc), SC_None, nullptr);
    P->setScopeInfo(0, I);
    Params.push_back(P);
  }
  FD->setParams(Params);

  // Build the body as the body of FD, in a scope of its own and potentially
  // evaluated (facet detection runs in a constant-evaluated context).
  Sema::ContextRAII SavedContext(S, FD);
  Sema::FunctionScopeRAII FunctionScope(S);
  S.PushFunctionScope();
  EnterExpressionEvaluationContext PotentiallyEvaluated(
      S, Sema::ExpressionEvaluationContext::PotentiallyEvaluated);

  auto ParamValue = [&](unsigned I) {
    return S.DefaultLvalueConversion(
        S.BuildDeclRefExpr(Params[I], Params[I]->getType(), VK_LValue, Loc));
  };
  // *static_cast<const T *>(param I)
  auto Deref = [&](unsigned I, QualType T) -> ExprResult {
    ExprResult V = ParamValue(I);
    if (V.isInvalid())
      return ExprError();
    QualType PtrTy = C.getPointerType(T);
    ExprResult Cast = S.BuildCXXNamedCast(
        Loc, tok::kw_static_cast, C.getTrivialTypeSourceInfo(PtrTy, Loc),
        V.get(), SourceRange(Loc, Loc), SourceRange(Loc, Loc));
    if (Cast.isInvalid())
      return ExprError();
    return S.CreateBuiltinUnaryOp(Loc, UO_Deref, Cast.get());
  };

  ExprResult Obj = Deref(0, ObjTy);
  if (Obj.isInvalid())
    return nullptr;
  LookupResult R = makeFacetLookup(
      S, IsQuery ? LabelFacet::Query : LabelFacet::HandleContractViolation,
      Loc);
  if (!lookupFacetMember(S, RD, LabelTy, R))
    return nullptr;
  ExprResult Member = buildFacetMemberRef(S, Obj.get(), ObjTy, R, Loc);
  if (Member.isInvalid())
    return nullptr;

  // The arguments are lvalues, as the concept's requirement parameters are.
  SmallVector<Expr *, 2> Args;
  if (IsQuery) {
    for (unsigned I : {1u, 2u})
      Args.push_back(
          S.BuildDeclRefExpr(Params[I], Params[I]->getType(), VK_LValue, Loc));
  } else {
    ExprResult A = Deref(1, CVTy.withConst());
    if (A.isInvalid())
      return nullptr;
    Args.push_back(A.get());
  }
  ExprResult Call = S.BuildCallExpr(/*Scope=*/nullptr, Member.get(), Loc, Args,
                                    Loc, /*ExecConfig=*/nullptr);
  if (Call.isInvalid())
    return nullptr;

  SmallVector<Stmt *, 2> Body;
  Expr *Result = Call.get();
  if (!IsQuery) {
    if (Result->getType()->isVoidType()) {
      ExprResult Full =
          S.ActOnFinishFullExpr(Result, Loc, /*DiscardedValue=*/true);
      if (Full.isInvalid())
        return nullptr;
      Body.push_back(Full.get());
      Result = IntegerLiteral::Create(C, llvm::APInt(C.getIntWidth(C.IntTy), 0),
                                      C.IntTy, Loc);
    } else {
      HandledTy = getStdContractsTagType(S, "violation_handled", Loc);
      if (HandledTy.isNull())
        return nullptr;
      ExprResult Handled = S.PerformImplicitConversion(
          Result, HandledTy, AssignmentAction::Converting);
      if (Handled.isInvalid())
        return nullptr;
      ExprResult AsInt = S.BuildCXXNamedCast(
          Loc, tok::kw_static_cast, C.getTrivialTypeSourceInfo(C.IntTy, Loc),
          Handled.get(), SourceRange(Loc, Loc), SourceRange(Loc, Loc));
      if (AsInt.isInvalid())
        return nullptr;
      Result = AsInt.get();
    }
  }
  StmtResult Ret = S.BuildReturnStmt(Loc, Result);
  if (Ret.isInvalid())
    return nullptr;
  Body.push_back(Ret.get());
  FD->setBody(CompoundStmt::Create(C, Body, FPOptionsOverride(), Loc, Loc));

  // CodeGen's rethrowing-local-handler bypass reads the handler's body to
  // decide whether the predicate needs an EH region at all, and CodeGen has no
  // Sema to instantiate one with.  For a template specialization --
  // __combined_label's handler above all, which is the case the optimization
  // most wants to see -- the reference above only queues the definition until
  // the end of the TU, by which point an eagerly emitted guarded function has
  // already been code-generated without it.  Instantiate it now: the
  // trampoline odr-uses it, so this only moves the instantiation earlier.
  if (!IsQuery)
    if (auto *CE = dyn_cast<CallExpr>(Call.get()->IgnoreImplicit()))
      if (FunctionDecl *Callee = CE->getDirectCallee())
        if (!Callee->hasBody() && Callee->isTemplateInstantiation())
          S.InstantiateFunctionDefinition(Loc, Callee, /*Recursive=*/false,
                                          /*DefinitionRequired=*/false,
                                          /*AtEndOfTU=*/false);

  S.ContractLabelFacetTrampolines[Key] = FD;
  return FD;
}

/// The argument a compute_semantic facet, or evaluation_semantic_set::contains,
/// is called with: a non-const lvalue of std::contracts::evaluation_semantic
/// holding \p SemVal, as in the library concept.  Passing that and letting
/// overload resolution choose is what makes a reference or deduced parameter,
/// an explicit object parameter and an overload set all work.  Null when
/// <contracts> is not in scope.
static Expr *buildSemanticFacetArg(Sema &S, unsigned SemVal,
                                   SourceLocation Loc) {
  QualType ES = getStdContractsTagType(S, "evaluation_semantic", Loc);
  if (ES.isNull() || !ES->isEnumeralType())
    return nullptr;
  Expr *Lit = IntegerLiteral::Create(
      S.Context, llvm::APInt(S.Context.getIntWidth(S.Context.IntTy), SemVal),
      S.Context.IntTy, Loc);
  Expr *Val =
      ImplicitCastExpr::Create(S.Context, ES, CK_IntegralCast, Lit, nullptr,
                               VK_PRValue, FPOptionsOverride());
  return S.CreateMaterializeTemporaryExpr(ES, Val,
                                          /*BoundToLvalueReference=*/true);
}

/// Constant-evaluate a facet call, reporting failure rather than hiding it.
///
/// No concept can ask whether a member is constexpr, so per P3400 a type
/// with the right member *does* participate in the facet, and the error
/// arrives when the result is consumed during translation -- which is here.
/// Failure must not be reported as "absent", which is the quietest way to get
/// this wrong: the label would look applied and the comment or semantic would
/// simply be unchanged, with no diagnostic at all.
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

/// The argument list function facet F is probed with.
///
/// Expr is not movable, so the placeholders live in fixed in-place storage
/// rather than a growable vector; no facet takes more than two arguments.
struct ContractFacetProbeArgs {
  std::optional<OpaqueValueExpr> Storage[2];
  SmallVector<Expr *, 2> Args;

  /// False when the facet takes a shape this does not know how to probe.
  bool build(Sema &S, LabelFacet F, NamedDecl *Found, SourceLocation Loc) {
    unsigned N = 0;
    auto push = [&](QualType T, ExprValueKind VK) {
      Storage[N].emplace(Loc, T, VK);
      Args.push_back(&*Storage[N]);
      ++N;
    };

    switch (F) {
    case LabelFacet::HandleContractViolation: {
      QualType CVTy = getContractViolationType(S, Loc);
      if (CVTy.isNull())
        return false;
      push(CVTy.withConst(), VK_LValue);
      return true;
    }
    case LabelFacet::Query:
      push(S.Context.VoidPtrTy, VK_PRValue);
      push(S.Context.getSizeType(), VK_PRValue);
      return true;
    case LabelFacet::ComputeComment:
    case LabelFacet::ComputeMessage:
      push(S.Context.getPointerType(S.Context.CharTy.withConst()), VK_PRValue);
      return true;
    case LabelFacet::ComputeSemantic: {
      // Recover the enumeration from the member rather than guessing at it.
      auto *FD =
          Found ? dyn_cast<FunctionDecl>(Found->getUnderlyingDecl()) : nullptr;
      if (!FD || FD->getNumParams() != 1)
        return false;
      push(FD->getParamDecl(0)->getType().getNonReferenceType(), VK_PRValue);
      return true;
    }
    case LabelFacet::AnalysisSemantic:
      return true;
    case LabelFacet::AllowedSemantics:
    case LabelFacet::GroupNames:
      return false;
    }
    llvm_unreachable("unknown label facet");
  }
};

/// True when calling Found on Obj with the facet's arguments is viable.
static bool facetCallViable(Sema &S, Expr *Obj, QualType ObjTy,
                            const CXXRecordDecl *RD, LookupResult &R,
                            LabelFacet F, SourceLocation Loc) {
  Sema::SFINAETrap Trap(S, /*WithAccessChecking=*/true);
  ExprResult MR = buildFacetMemberRef(S, Obj, ObjTy, R, Loc);
  if (MR.isInvalid())
    return false;

  ContractFacetProbeArgs Probe;
  if (!Probe.build(S, F, R.getRepresentativeDecl(), Loc))
    return false;

  ExprResult Call = S.BuildCallExpr(/*Scope=*/nullptr, MR.get(), Loc,
                                    Probe.Args, Loc, /*ExecConfig=*/nullptr);
  return !Call.isInvalid();
}

/// Warn when RD has a member named FacetName that almost provides a facet.
///
/// Only two near misses are reported, alone or together, and each is
/// recognized by relaxing it and seeing whether that makes the call viable: an
/// inaccessible member, one that is not const, and one that is both.
/// Anything else stays silent.  That matters: P3400 points out that a label
/// may carry private helpers sharing a facet's name -- tag-dispatch overloads,
/// say -- and warning on a name match alone would fire on every one of them.
/// Relaxing access and constness will not make such a helper's signature fit,
/// so it never reaches a warning.
static void warnNearMissFacet(Sema &S, Expr *LabelExpr, QualType LabelTy,
                              const CXXRecordDecl *RD, LabelFacet F,
                              SourceLocation Loc) {
  StringRef FacetName = getLabelFacetName(F);
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
  unsigned NumCandidates = 0;
  for (NamedDecl *D : Raw)
    (void)D, ++NumCandidates;

  // The question detection asked: accessible candidates only, on the const
  // label.  If that is viable the facet is present and there is nothing to say.
  LookupResult Accessible(S, Name, Loc, Sema::LookupMemberName);
  if (!lookup(Accessible)) {
    Accessible.suppressDiagnostics();
    return;
  }
  dropInaccessibleFacetCandidates(S, RD, LabelTy, Accessible);
  bool AnyAccessible = !Accessible.empty();
  unsigned NumAccessible = 0;
  for (NamedDecl *D : Accessible)
    (void)D, ++NumAccessible;
  bool AnyInaccessible = NumAccessible != NumCandidates;
  if (AnyAccessible &&
      facetCallViable(S, LabelExpr, LabelTy, RD, Accessible, F, Loc))
    return;

  enum { Inaccessible, NotConst, InaccessibleNotConst };
  auto Warn = [&](unsigned Why) {
    S.Diag(Loc, diag::warn_contract_invalid_label_facet)
        << FacetName << LabelTy.getUnqualifiedType() << Why;
    if (Rep)
      S.Diag(Rep->getLocation(), diag::note_contract_invalid_label_facet);
  };

  // Relax access only.
  if (!AnyAccessible &&
      facetCallViable(S, LabelExpr, LabelTy, RD, Raw, F, Loc)) {
    Warn(Inaccessible);
    return;
  }

  // Relax const only.  A facet is always invoked on a constexpr, therefore
  // const, control object, so a non-const member can never be one.
  QualType NonConstTy = LabelTy.getUnqualifiedType();
  OpaqueValueExpr NonConstObj(Loc, NonConstTy, VK_LValue);
  if (AnyAccessible) {
    LookupResult Again(S, Name, Loc, Sema::LookupMemberName);
    if (!lookup(Again)) {
      Again.suppressDiagnostics();
      return;
    }
    dropInaccessibleFacetCandidates(S, RD, NonConstTy, Again);
    if (!Again.empty() &&
        facetCallViable(S, &NonConstObj, NonConstTy, RD, Again, F, Loc)) {
      Warn(NotConst);
      return;
    }
  }

  // Relax both.
  if (AnyInaccessible) {
    LookupResult All(S, Name, Loc, Sema::LookupMemberName);
    if (!lookup(All)) {
      All.suppressDiagnostics();
      return;
    }
    All.suppressAccessDiagnostics();
    if (facetCallViable(S, &NonConstObj, NonConstTy, RD, All, F, Loc))
      Warn(InaccessibleNotConst);
  }
}

/// Whether an lvalue of type T has the shape of the data-member facet F, its
/// access and constness aside: for allowed_semantics,
/// `evaluation_semantic_set({m})' is valid; for group_names, `m[0]' is and
/// converts to `const char*' ([support.contract.control.allowed],
/// [support.contract.control.ident]).
static bool dataFacetShapeFits(Sema &S, QualType T, LabelFacet F,
                               SourceLocation Loc) {
  Sema::SFINAETrap Trap(S, /*WithAccessChecking=*/true);
  OpaqueValueExpr Obj(Loc, T.getUnqualifiedType(), VK_LValue);
  if (F == LabelFacet::AllowedSemantics) {
    QualType SetTy = getStdContractsTagType(S, "evaluation_semantic_set", Loc);
    if (SetTy.isNull())
      return false;
    Expr *Member = &Obj;
    ExprResult Init = S.ActOnInitList(Loc, Member, Loc);
    if (Init.isInvalid())
      return false;
    Expr *InitList = Init.get();
    ExprResult Set = S.BuildCXXTypeConstructExpr(
        S.Context.getTrivialTypeSourceInfo(SetTy, Loc), Loc, InitList, Loc,
        /*ListInitialization=*/false);
    // An error the trap swallowed -- narrowing into a constructor the set
    // does not offer publicly, say -- leaves a valid-looking expression.
    return !Set.isInvalid() && !Trap.hasErrorOccurred();
  }
  QualType SizeTy = S.Context.getSizeType();
  Expr *Zero = IntegerLiteral::Create(
      S.Context, llvm::APInt(S.Context.getTypeSize(SizeTy), 0), SizeTy, Loc);
  ExprResult Elt =
      S.ActOnArraySubscriptExpr(/*S=*/nullptr, &Obj, Loc, Zero, Loc);
  if (Elt.isInvalid())
    return false;
  ExprResult Conv = S.PerformImplicitConversion(
      Elt.get(), S.Context.getPointerType(S.Context.CharTy.withConst()),
      AssignmentAction::Converting);
  return !Conv.isInvalid() && !Trap.hasErrorOccurred();
}

/// Warn when RD has a data member named after the data-member facet F
/// (allowed_semantics or group_names) that has the facet's shape but is not
/// one because it is inaccessible, not const, or both.  A same-named member
/// of another shape or kind stays silent, as in warnNearMissFacet.
static void warnNearMissDataFacet(Sema &S, QualType LabelTy,
                                  const CXXRecordDecl *RD, LabelFacet F,
                                  SourceLocation Loc) {
  StringRef FacetName = getLabelFacetName(F);
  LookupResult R(S, &S.Context.Idents.get(FacetName), Loc,
                 Sema::LookupMemberName);
  if (!S.LookupQualifiedName(R, const_cast<CXXRecordDecl *>(RD)) ||
      R.isAmbiguous() || !R.isSingleResult()) {
    R.suppressDiagnostics();
    return;
  }
  R.suppressDiagnostics();
  NamedDecl *D = R.getFoundDecl();
  auto *Member = dyn_cast<ValueDecl>(D->getUnderlyingDecl());
  if (!Member || !(isa<VarDecl>(Member) || isa<FieldDecl>(Member)))
    return;

  bool Inaccessible;
  {
    // Judged from outside the label, as dropInaccessibleFacetCandidates does.
    Sema::ContextRAII OutsideLabel(S, S.Context.getTranslationUnitDecl());
    Inaccessible =
        !S.IsSimplyAccessible(D, const_cast<CXXRecordDecl *>(RD), LabelTy);
  }
  bool NonConst = !Member->getType().isConstQualified();
  if (!Inaccessible && !NonConst)
    return;
  if (Member->getType()->isDependentType() ||
      !dataFacetShapeFits(S, Member->getType(), F, Loc))
    return;

  enum { InaccessibleWhy, NotConstWhy, InaccessibleNotConstWhy };
  S.Diag(Loc, diag::warn_contract_invalid_label_facet)
      << FacetName << LabelTy.getUnqualifiedType()
      << (Inaccessible && NonConst ? InaccessibleNotConstWhy
          : Inaccessible           ? InaccessibleWhy
                                   : NotConstWhy);
  S.Diag(Member->getLocation(), diag::note_contract_invalid_label_facet);
}

/// Try to call label.compute_semantic(semantic_arg), convert the result
/// implicitly to std::contracts::evaluation_semantic, and constant-evaluate the
/// converted expression.  Returns the semantic's value, or std::nullopt when
/// the facet is absent or the call or its evaluation fails.  A label whose
/// result does not convert implicitly -- an int, another enumeration, or a
/// class with only an explicit conversion function -- is not a
/// computed-semantics control type ([basic.contract.control]) and so has no
/// facet; one whose result is a class with an implicit conversion function has
/// it, applied through that conversion.
static std::optional<int64_t> callLabelMethod(Sema &S, Expr *LabelExpr,
                                              QualType LabelTy,
                                              const CXXRecordDecl *RD,
                                              LabelFacet F, unsigned SemVal,
                                              SourceLocation Loc) {
  StringRef MethodName = getLabelFacetName(F);
  // Probing must not diagnose: a member of the wrong shape simply means the
  // facet is absent.  The trap covers only the construction of the call --
  // constant evaluation below deliberately runs outside it, because a failure
  // there IS reportable.
  Sema::SFINAETrap Trap(S, /*WithAccessChecking=*/true);
  LookupResult R = makeFacetLookup(S, F, Loc);
  if (!lookupFacetMember(S, RD, LabelTy, R))
    return std::nullopt;
  ExprResult MemberRef = buildFacetMemberRef(S, LabelExpr, LabelTy, R, Loc);
  if (MemberRef.isInvalid())
    return std::nullopt;

  Expr *Arg = buildSemanticFacetArg(S, SemVal, Loc);
  if (!Arg)
    return std::nullopt;

  ExprResult Call = S.BuildCallExpr(/*Scope=*/nullptr, MemberRef.get(), Loc,
                                    {Arg}, Loc, /*ExecConfig=*/nullptr);
  if (Call.isInvalid())
    return std::nullopt;
  // The concept's { t.compute_semantic(s) } -> convertible_to<...>.
  ExprResult Conv = S.PerformImplicitConversion(
      Call.get(), Arg->getType().getNonReferenceType().getUnqualifiedType(),
      AssignmentAction::Converting);
  if (Conv.isInvalid())
    return std::nullopt;

  Expr::EvalResult Eval;
  if (!evaluateFacetConstant(S, Conv.get(), Eval, MethodName, LabelTy, Loc) ||
      !Eval.Val.isInt())
    return std::nullopt;
  return Eval.Val.getInt().getExtValue();
}

/// Call label.method_name(const char* arg), convert the result to const char*
/// as the concept does, constant-evaluate it and read the string it points to
/// (from any base, offset included).  std::nullopt when the facet is absent or
/// yields no string; an empty string is a result like any other.
static std::optional<std::string>
callLabelStringMethod(Sema &S, Expr *LabelExpr, QualType LabelTy,
                      const CXXRecordDecl *RD, LabelFacet F,
                      StringRef CurrentVal, bool IsNull, SourceLocation Loc) {
  StringRef MethodName = getLabelFacetName(F);
  // Probing must not diagnose: a member of the wrong shape simply means the
  // facet is absent.  The trap covers only the construction of the call --
  // constant evaluation below deliberately runs outside it, because a failure
  // there IS reportable.
  Sema::SFINAETrap Trap(S, /*WithAccessChecking=*/true);
  LookupResult R = makeFacetLookup(S, F, Loc);
  if (!lookupFacetMember(S, RD, LabelTy, R))
    return std::nullopt;
  ExprResult MemberRef = buildFacetMemberRef(S, LabelExpr, LabelTy, R, Loc);
  if (MemberRef.isInvalid())
    return std::nullopt;

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
    return std::nullopt;
  // The result need only be convertible to const char*.
  ExprResult Conv = S.PerformImplicitConversion(Call.get(), ConstCharPtrTy,
                                                AssignmentAction::Converting);
  if (Conv.isInvalid())
    return std::nullopt;

  Expr::EvalResult Eval;
  if (!evaluateFacetConstant(S, Conv.get(), Eval, MethodName, LabelTy, Loc))
    return std::nullopt;
  if (!Eval.Val.isLValue() || Eval.Val.getLValueBase().isNull())
    return std::nullopt;
  return Conv.get()->tryEvaluateString(S.Context);
}

/// Extract the label's allowed_semantics restriction (bit N = semantic value N
/// allowed), probing every valid semantic including "assume" and the P4298
/// noexcept_enforce/noexcept_observe variants.  This is the flag-independent
/// restriction; the -fcontracts-allow-assume / -fcontracts-p4298 gates are
/// applied later, at query construction.  Returns
/// AllContractSemanticsMaskWithExtensions (no restriction) if no
/// allowed_semantics member exists.
static unsigned extractAllowedMask(Sema &S, Expr *LabelExpr, QualType LabelTy,
                                   const CXXRecordDecl *RD,
                                   SourceLocation Loc) {
  LookupResult R = makeFacetLookup(S, LabelFacet::AllowedSemantics, Loc);
  if (!lookupFacetMember(S, RD, LabelTy, R))
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
  ExprResult ASRef = buildFacetMemberRef(S, LabelExpr, LabelTy, R, Loc);
  if (ASRef.isInvalid())
    return AllContractSemanticsMaskWithExtensions;

  // The facet is `evaluation_semantic_set({label.allowed_semantics})'
  // ([support.contract.control.allowed]): build that set, as the library
  // concept does, and ask it -- a single semantic, or any type convertible to
  // the set, works that way, and only that way.  A type that does not convert
  // is not a facet.  A conversion or contains() that is not a constant
  // expression is diagnosed, once.
  QualType SetTy = getStdContractsTagType(S, "evaluation_semantic_set", Loc);
  const CXXRecordDecl *SetRD =
      SetTy.isNull() ? nullptr : SetTy->getAsCXXRecordDecl();
  if (!SetRD)
    return AllContractSemanticsMaskWithExtensions;
  DeclarationName ContainsName = &S.Context.Idents.get("contains");

  unsigned Mask = 0;
  // Query contains() for each evaluation-semantic value.  Bit N of the mask
  // corresponds to semantic value N, matching resolveContractSemantic().
  // Assume and the P4298 noexcept_* variants are probed here too; the flag
  // gates that may exclude them (-fcontracts-allow-assume / -fcontracts-p4298)
  // are applied later, at query construction.
  for (unsigned Sem = FirstContractEvaluationSemantic;
       Sem <= LastContractEvaluationSemantic; ++Sem) {
    Expr *Member = ASRef.get();
    ExprResult Init = S.ActOnInitList(Loc, Member, Loc);
    if (Init.isInvalid())
      return AllContractSemanticsMaskWithExtensions;
    Expr *InitList = Init.get();
    ExprResult Set = S.BuildCXXTypeConstructExpr(
        S.Context.getTrivialTypeSourceInfo(SetTy, Loc), Loc, InitList, Loc,
        /*ListInitialization=*/false);
    if (Set.isInvalid())
      return AllContractSemanticsMaskWithExtensions;
    LookupResult CR(S, ContainsName, Loc, Sema::LookupMemberName);
    if (!S.LookupQualifiedName(CR, const_cast<CXXRecordDecl *>(SetRD)))
      return AllContractSemanticsMaskWithExtensions;
    ExprResult ContainsRef = buildFacetMemberRef(S, Set.get(), SetTy, CR, Loc);
    Expr *Arg = buildSemanticFacetArg(S, Sem, Loc);
    if (ContainsRef.isInvalid() || !Arg)
      return AllContractSemanticsMaskWithExtensions;
    ExprResult Call = S.BuildCallExpr(/*Scope=*/nullptr, ContainsRef.get(), Loc,
                                      {Arg}, Loc, /*ExecConfig=*/nullptr);
    if (Call.isInvalid())
      return AllContractSemanticsMaskWithExtensions;

    Expr::EvalResult Eval;
    if (!evaluateFacetConstant(S, Call.get(), Eval,
                               getLabelFacetName(LabelFacet::AllowedSemantics),
                               LabelTy, Loc) ||
        !Eval.Val.isInt())
      return AllContractSemanticsMaskWithExtensions;
    if (Eval.Val.getInt().getBoolValue())
      Mask |= (1u << Sem);
  }

  return Mask;
}

/// Extract group names from label's group_names member (identification_label).
/// Returns a vector of group name strings; empty if no group_names member.
static SmallVector<std::string> extractGroupNames(Sema &S, Expr *LabelExpr,
                                                  QualType LabelTy,
                                                  const CXXRecordDecl *RD,
                                                  SourceLocation Loc) {
  // As for the function facets: a member this context cannot name is not a
  // facet, and must not be an error.
  LookupResult R = makeFacetLookup(S, LabelFacet::GroupNames, Loc);
  if (!lookupFacetMember(S, RD, LabelTy, R))
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

  // Read the names as the concept does ([support.contract.control.ident]):
  // group_names[i] converted to const char*, for each of the
  // sizeof(group_names) / sizeof(group_names[0]) elements -- an array of
  // strings, of pointers to strings, or a std::array of either; a static or a
  // non-static member, or one inherited from a combined label's base.
  Sema::SFINAETrap Trap(S);
  ExprResult GN = buildFacetMemberRef(S, LabelExpr, LabelTy, R, Loc);
  if (GN.isInvalid())
    return {};
  auto Index = [&](uint64_t I) -> Expr * {
    QualType SizeTy = S.Context.getSizeType();
    Expr *Idx = IntegerLiteral::Create(
        S.Context, llvm::APInt(S.Context.getTypeSize(SizeTy), I), SizeTy, Loc);
    ExprResult E =
        S.ActOnArraySubscriptExpr(/*S=*/nullptr, GN.get(), Loc, Idx, Loc);
    return E.isInvalid() ? nullptr : E.get();
  };
  Expr *Elt0 = Index(0);
  if (!Elt0 || GN.get()->getType()->isDependentType())
    return {};
  CharUnits Total = S.Context.getTypeSizeInChars(GN.get()->getType());
  CharUnits Each = S.Context.getTypeSizeInChars(Elt0->getType());
  if (Each.isZero())
    return {};
  QualType ConstCharPtrTy =
      S.Context.getPointerType(S.Context.CharTy.withConst());

  SmallVector<std::string> Groups;
  for (uint64_t I = 0, N = Total / Each; I != N; ++I) {
    Expr *Elt = Index(I);
    if (!Elt)
      continue;
    ExprResult Conv = S.PerformImplicitConversion(Elt, ConstCharPtrTy,
                                                  AssignmentAction::Converting);
    if (Conv.isInvalid())
      continue;
    std::optional<std::string> Str = Conv.get()->tryEvaluateString(S.Context);
    if (Str && !Str->empty())
      Groups.push_back(std::move(*Str));
  }
  return Groups;
}

/// Resolve the contract config of CS, in groups Groups, via the P3595 ordered
/// config system and D4405's steps 1 to 5 (ContractOptions::resolveContract),
/// recording the runtime evaluation's semantic and the analysis evaluation's,
/// callee-side and caller-side, and returning the callee-side runtime and analysis
/// semantics in \p Runtime and \p Analysis for step 6, and in \p Dynamic
/// whether the runtime evaluation is selected dynamically.  Returns false,
/// having diagnosed it, when the best fit finds no semantic in AllowedMask,
/// which only an empty mask gives; the contract is then recorded as ignore.
static bool resolveContractConfig(Sema &S, ContractStmt *CS,
                                  unsigned AllowedMask,
                                  ArrayRef<std::string> Groups,
                                  ContractEvaluationSemantic &Runtime,
                                  ContractEvaluationSemantic &Analysis,
                                  bool &Dynamic) {
  const auto &Opts = S.Context.getLangOpts().ContractOpts;
  ContractResolution Res = Opts.resolveContract(
      CS->buildConfigQuery(S.Context, S.CurContext, AllowedMask, Groups));
  ContractEvaluationSemantic Sem = Res.Semantic;
  Runtime = Res.Runtime;
  Analysis = Res.Analysis;
  Dynamic = Res.Dynamic;
  bool Valid = Sem != InvalidContractEvaluationSemantic;
  if (!Valid) {
    S.Diag(CS->getKeywordLoc(), diag::err_contract_no_valid_semantic);
    Sem = Runtime = Analysis = ContractEvaluationSemantic::Ignore;
  }
  CS->setTransformedSemantic(Sem);
  CS->setAnalysisSemantic(Analysis);

  // A caller-side runtime evaluation always allows ignore
  // (contractRuntimeAllowedMask), so it always resolves.
  ContractResolution Caller = Opts.resolveContract(CS->buildConfigQuery(
      S.Context, S.CurContext, AllowedMask, Groups, /*CallerSide=*/true));
  CS->setCallerSemantic(Caller.Runtime);
  CS->setCallerAnalysisSemantic(Caller.Analysis);
  return Valid;
}

/// P3595 dynamic selection: when a labeled contract's runtime resolution
/// matches a config entry carrying an "output.dynamic" descriptor, precompute
/// the per-return-value transform table
/// T(R) = compute_semantic(clamp_to_allowed(R)) for every selectable R
/// (1..ContractStmt::NumDynRawSemantics) and cache it (plus the descriptor) on
/// the stmt.  Codegen turns this into a dispatch switch; it does no Sema work
/// of its own.
///
/// This mirrors the eagerly-resolved scalar path (steps 3-5 below) but differs
/// in one way per P3595 design section 4: a compute_semantic result that lands
/// outside the allowed set records the sentinel 0 for that R (-> runtime
/// enforced violation in codegen) instead of emitting the compile error.  The
/// scalar compile-error path (applyLabelFacets step 5) is unchanged for the
/// eagerly-resolved default.
///
/// HasComputeSemantic says whether the label's compute_semantic facet exists
/// and evaluated when applied to the scalar semantic, as callLabelMethod
/// decides it: a label whose result does not convert implicitly to
/// evaluation_semantic has none.  Without it T(R) is just the clamp.  LabelExpr/LabelTy/RD may then be null, for a dynamic contract
/// with no label facets (matched by group/namespace/location).
static void
precomputeDynamicTable(Sema &S, ContractStmt *CS, unsigned AllowedMask,
                       ArrayRef<std::string> Groups, Expr *LabelExpr,
                       QualType LabelTy, const CXXRecordDecl *RD,
                       bool HasComputeSemantic, SourceLocation Loc) {
  // As in applyLabelFacets: the label methods are constant-evaluated, and a
  // prvalue label's temporaries must not escape into the enclosing function.
  EnterExpressionEvaluationContext ConstantEvaluated(
      S, Sema::ExpressionEvaluationContext::ConstantEvaluated);
  // Match resolveContractConfig's group resolution: if no groups were supplied
  // (labeled via group_names, or unlabeled), fall back to the
  // [[clang::contract_group]] attribute so the dynamic scan matches the same
  // entry the scalar scan did.  Build the same runtime query
  // resolveContractConfig used, then ask for the matched entry's dynamic
  // descriptor (never in a constant-evaluation context: a dynamic selector is
  // never called at compile time).
  SmallVector<std::string, 1> GroupsStorage;
  const auto &Opts = S.Context.getLangOpts().ContractOpts;
  ContractDynamicResult Dyn = Opts.resolveContractDynamic(
      CS->buildConfigQuery(S.Context, S.CurContext, AllowedMask,
                           CS->getConfigGroups(Groups, GroupsStorage)));
  if (!Dyn.Found)
    return;

  // Compute T(R) for each value R the selector may return: every semantic,
  // Ignore(1) to Diagnose(9).  One the contract does not allow -- assume
  // without -fcontracts-allow-assume, a noexcept one without
  // -fcontracts-p4298, prove or diagnose without -fcontracts-p4405, or one
  // the label excludes -- is adjusted to the allowed set (D3595R1, "Adjusting
  // a Semantic to the Allowed Set") by stage a.  Any other value keeps the
  // sentinel 0.
  //
  // When AllowedMask holds no throwing semantic (contractSemanticsNothrow),
  // stage a prefers noexcept_observe and noexcept_enforce to the throwing
  // semantics it cannot reach, but a compute_semantic result outside the
  // allowed set, an observe or enforce included, is an invalid selection
  // (DECISIONS.md E138), just as the static path rejects it, and codegen
  // reports it with no throwing semantic either (a noexcept_enforce
  // violation, or without -fcontracts-p4298 a quick_enforce trap).  The weak
  // default selector returns the configured semantic (Dyn.Configured), to
  // which neither the allowed set nor compute_semantic has been applied,
  // through the same table, so it is evaluated with the compile-time
  // semantic.
  //
  // D4405: the selected semantic is a runtime evaluation's, adjusted to the
  // runtime allowed set (contractRuntimeAllowedMask) as the compile-time
  // semantic is, so a selected prove or diagnose is taken as ignore, as the
  // configured one would be moved to the analysis evaluation, and the walk
  // never reaches either.  A compute_semantic prove or diagnose likewise
  // takes the evaluation to ignore, provided it is in the analysis allowed
  // set and ignore in the runtime one.  (gnu_gcc's transform_semantic.)
  unsigned RuntimeMask =
      contractRuntimeAllowedMask(AllowedMask, /*CallerSide=*/false);
  unsigned AnalysisMask = contractAnalysisAllowedMask(AllowedMask);
  uint8_t Table[ContractStmt::NumDynRawSemantics] = {};
  for (unsigned R = 1; R <= ContractStmt::NumDynRawSemantics; ++R) {
    // Stage a: clamp R into the runtime allowed set using the SAME helper
    // resolveContract uses (fallback order + P3100 assume rule).  An R the
    // best fit cannot adjust into the set is an invalid selection.
    auto Raw = static_cast<ContractEvaluationSemantic>(R);
    if (isProvingContractSemantic(Raw))
      Raw = ContractEvaluationSemantic::Ignore;
    ContractEvaluationSemantic Clamped =
        ContractOptions::clampToAllowed(Raw, RuntimeMask);
    if (Clamped == InvalidContractEvaluationSemantic) {
      Table[R - 1] = 0; // sentinel: no valid semantic
      continue;
    }

    ContractEvaluationSemantic Effective = Clamped;

    // Stage b: apply compute_semantic (if the label has it) to the clamped
    // value.  A result that is not a semantic value is never allowed, so it
    // records the sentinel; the others are gated by the AllowedMask check in
    // stage c below.
    if (HasComputeSemantic) {
      std::optional<int64_t> Computed = callLabelMethod(
          S, LabelExpr, LabelTy, RD, LabelFacet::ComputeSemantic,
          static_cast<unsigned>(Clamped), Loc);
      if (Computed && !isContractEvaluationSemanticValue(*Computed)) {
        Table[R - 1] = 0; // sentinel: not a semantic
        continue;
      }
      if (Computed)
        Effective = static_cast<ContractEvaluationSemantic>(*Computed);
    }

    // Stage c: unlike the eagerly-resolved scalar path (step 6), a disallowed
    // effective semantic here is NOT a compile error -- it records the sentinel
    // 0, which codegen dispatches to a runtime enforced violation.  A prove or
    // diagnose the analysis evaluation allows is recorded as ignore: the
    // selection happens at run time, where there is nothing to diagnose.
    if (isProvingContractSemantic(Effective))
      Effective = (AnalysisMask & (1u << static_cast<unsigned>(Effective)))
                      ? ContractEvaluationSemantic::Ignore
                      : InvalidContractEvaluationSemantic;
    if (Effective == InvalidContractEvaluationSemantic ||
        !(RuntimeMask & (1u << static_cast<unsigned>(Effective))))
      Table[R - 1] = 0; // sentinel: disallowed transform
    else
      Table[R - 1] = static_cast<uint8_t>(Effective);
  }

  StringRef StoredName = S.Context.backupStr(Dyn.Name);
  CS->setDynamicInfo(StoredName, Dyn.Linkage, Dyn.ProvideWeak, Dyn.Configured,
                     Table);
}

/// Record on CS what its label's facets determine: the allowed-semantic
/// restriction and group names, the semantic (compute_semantic, or a P3595
/// dynamic-selection table), the local_violation_label and queryable_label
/// facets, and the comment and message.  Does nothing for a missing or
/// dependent label.
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

  // The facets are probed and called on a const lvalue of the label type, as
  // the labels:: concepts ask (`const T&'): a non-const member is not a facet,
  // even for a prvalue label such as pre<L{}>.
  if (!LabelExpr->isLValue())
    LabelExpr = S.CreateMaterializeTemporaryExpr(
        LabelTy.withConst(), LabelExpr, /*BoundToLvalueReference=*/true);
  else if (!LabelTy.isConstQualified())
    LabelExpr = ImplicitCastExpr::Create(S.Context, LabelTy.withConst(),
                                         CK_NoOp, LabelExpr, nullptr, VK_LValue,
                                         FPOptionsOverride());
  LabelTy = LabelExpr->getType();

  // Step 1: Extract the flag-independent label restriction from the label's
  // allowed_semantics facet.
  unsigned LabelMask = extractAllowedMask(S, LabelExpr, LabelTy, RD, Loc);

  // Step 2: Extract group names from identification_label facet, falling back
  // to the [[clang::contract_group]] attribute.
  SmallVector<std::string> LabelNames =
      extractGroupNames(S, LabelExpr, LabelTy, RD, Loc);
  SmallVector<std::string, 1> LabelGroupsStorage;
  ArrayRef<std::string> LabelGroups =
      CS->getConfigGroups(LabelNames, LabelGroupsStorage);

  // Step 3: Apply the -fcontracts-allow-assume gate to get the effective
  // allowed set (base gated by the flag, intersected with the label), then
  // eagerly resolve.  Labeled contracts must resolve eagerly because label
  // facets (groups, compute_semantic) require Sema for evaluation.  The
  // flag-independent restriction is stored so the query builders re-apply the
  // gate uniformly (e.g. for template instantiations).
  unsigned AllowedMask =
      LabelMask & gatedContractSemanticsMask(
                      S.Context.getLangOpts().ContractOpts.AllowAssume,
                      S.Context.getLangOpts().ContractsP4298,
                      S.Context.getLangOpts().ContractsP4405);
  if (AllowedMask == 0) {
    S.Diag(Loc, diag::err_contract_label_no_semantics)
        << LabelTy.getUnqualifiedType();
    return;
  }
  CS->setAllowedMask(LabelMask);
  // D4405, step 3 of "Adjusting to the Allowed Semantics": the label's
  // analysis_semantic facet, which the resolution's query reads from CS.
  CS->setLabelAnalysisSemantic(
      labelAnalysisSemantic(S, LabelExpr, LabelTy, RD, Loc));
  ContractEvaluationSemantic Runtime, Analysis;
  bool Dynamic;
  if (!resolveContractConfig(S, CS, AllowedMask, LabelGroups, Runtime, Analysis,
                             Dynamic)) {
    CS->setCESemantic(ContractEvaluationSemantic::Ignore);
    return;
  }

  // Step 4: Apply compute_semantic transformation if present (D4405's step
  // 6) to the adjusted runtime semantic.  It may return any semantic,
  // including assume, the P4298 noexcept_* variants and the P4405 prove and
  // diagnose; whether the result is allowed is decided in Step 5.
  std::optional<int64_t> ComputeResult =
      callLabelMethod(S, LabelExpr, LabelTy, RD, LabelFacet::ComputeSemantic,
                      static_cast<unsigned>(Runtime), Loc);

  // Step 5: D4405R0, step 6 of "Adjusting to the Allowed Semantics".  A
  // prove or diagnose result moves to the analysis evaluation, which takes
  // the stronger and must allow it; the runtime evaluation then becomes
  // ignore, which, like any other result, must be in the runtime allowed set
  // -- R, or {ignore} only when R is empty, so a compute_semantic result never
  // lets a callee-side check be skipped that the label requires.  Anything
  // else -- assume when -fcontracts-allow-assume is not set, since the gate
  // keeps assume out of the set entirely, and a value that is not a
  // semantic, which no set contains -- is an error.  (gnu_gcc's
  // apply_compute_semantic_split.)
  if (ComputeResult) {
    unsigned RuntimeMask =
        contractRuntimeAllowedMask(AllowedMask, /*CallerSide=*/false);
    unsigned AnalysisMask = contractAnalysisAllowedMask(AllowedMask);
    auto IgnoreBit =
        1u << static_cast<unsigned>(ContractEvaluationSemantic::Ignore);
    bool OK;
    auto Computed = static_cast<ContractEvaluationSemantic>(*ComputeResult);
    if (isContractEvaluationSemanticValue(*ComputeResult) &&
        isProvingContractSemantic(Computed)) {
      ContractEvaluationSemantic A =
          contractAnalysisStronger(Analysis, Computed);
      OK = (AnalysisMask & (1u << static_cast<unsigned>(A))) &&
           (RuntimeMask & IgnoreBit);
      if (OK) {
        Analysis = A;
        Runtime = ContractEvaluationSemantic::Ignore;
      }
    } else {
      OK = isContractEvaluationSemanticValue(*ComputeResult) &&
           (RuntimeMask & (1u << static_cast<unsigned>(Computed)));
      if (OK)
        Runtime = Computed;
    }
    if (!OK) {
      S.Diag(Loc, diag::err_contract_compute_semantic_disallowed)
          << LabelTy.getUnqualifiedType();
      return;
    }
    CS->setTransformedSemantic(Runtime);
    CS->setAnalysisSemantic(Analysis);
  }

  // Step 5b: P3595 dynamic selection.  If this contract's runtime resolution
  // matches a config entry with an "output.dynamic" descriptor, precompute the
  // per-return-value transform table for codegen.  The eagerly-resolved scalar
  // above remains the compile-time semantic (constant evaluation; the weak
  // default returns the entry's configured semantic, which the table maps to
  // it); this is a separate, per-return-value computation whose disallowed
  // results become the runtime sentinel rather than a compile error.
  precomputeDynamicTable(S, CS, AllowedMask, LabelGroups, LabelExpr, LabelTy,
                         RD, ComputeResult.has_value(), Loc);

  // Also eagerly resolve CE semantic for labeled contracts.
  {
    const auto &Opts = S.Context.getLangOpts().ContractOpts;
    ContractEvaluationSemantic CESem =
        Opts.resolveContractSemantic(CS->buildConfigQuery(
            S.Context, S.CurContext, AllowedMask, LabelGroups,
            /*CallerSide=*/false, /*InConstantEvaluation=*/true));

    // The constant-evaluation query may be configured with another semantic,
    // but has the same allowed set, so only an empty set leaves it none.
    //
    // D4405 (notadragon_wg21 DECISIONS.md E142): constant evaluation has no
    // analysis evaluation.  Its compute_semantic result is accepted if it is
    // in the whole allowed set or in the callee-side runtime one, and a prove
    // or diagnose is then used as enforce or observe
    // (contractConstantEvaluationSemantic), as the resolution maps one it
    // reaches itself.  (gnu_gcc's apply_compute_semantic_split, in_ce.)
    if (CESem == InvalidContractEvaluationSemantic) {
      S.Diag(Loc, diag::err_contract_no_valid_semantic);
      CESem = ContractEvaluationSemantic::Ignore;
    } else if (ComputeResult) {
      std::optional<int64_t> CECompute = callLabelMethod(
          S, LabelExpr, LabelTy, RD, LabelFacet::ComputeSemantic,
          static_cast<unsigned>(CESem), Loc);
      unsigned CEMask = AllowedMask | contractRuntimeAllowedMask(
                                          AllowedMask, /*CallerSide=*/false);
      if (CECompute && (!isContractEvaluationSemanticValue(*CECompute) ||
                        !(CEMask & (1u << static_cast<unsigned>(*CECompute)))))
        S.Diag(Loc, diag::err_contract_compute_semantic_disallowed)
            << LabelTy.getUnqualifiedType();
      else if (CECompute)
        CESem = contractConstantEvaluationSemantic(
            static_cast<ContractEvaluationSemantic>(*CECompute));
    }
    CS->setCESemantic(CESem);
  }

  // Report members that look like they were meant to be facets but are not.
  // Once per label type: the answer depends only on the type, and a label is
  // typically named by many contracts.
  if (!S.getDiagnostics().isIgnored(diag::warn_contract_invalid_label_facet,
                                    Loc) &&
      S.ContractNearMissCheckedLabels.insert(RD).second) {
    for (LabelFacet F : LabelFunctionFacets)
      warnNearMissFacet(S, LabelExpr, LabelTy, RD, F, Loc);
    warnNearMissDataFacet(S, LabelTy, RD, LabelFacet::AllowedSemantics, Loc);
    warnNearMissDataFacet(S, LabelTy, RD, LabelFacet::GroupNames, Loc);
  }

  // Step 6: Detect local_violation_label facet.  Detection probes in a SFINAE
  // context; the trampoline that calls the facet is built outside it, below.
  bool HandlerViable = false;
  {
    // See callLabelMethod: an inaccessible member means the facet is absent.
    Sema::SFINAETrap Trap(S, /*WithAccessChecking=*/true);
    LookupResult HR =
        makeFacetLookup(S, LabelFacet::HandleContractViolation, Loc);
    if (lookupFacetMember(S, RD, LabelTy, HR)) {
      ExprResult MR = buildFacetMemberRef(S, LabelExpr, LabelTy, HR, Loc);
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
          // The concept also requires the result to be void or convertible to
          // violation_handled; a handler returning anything else is not the
          // facet.  The trampoline makes the same conversion.
          if (Viable && !Call.get()->getType()->isVoidType()) {
            QualType HandledTy =
                getStdContractsTagType(S, "violation_handled", Loc);
            Viable = !HandledTy.isNull() &&
                     !S.PerformImplicitConversion(Call.get(), HandledTy,
                                                  AssignmentAction::Converting)
                          .isInvalid();
          }
        }
      }
      HandlerViable = Viable;
    }
  }

  if (HandlerViable)
    if (FunctionDecl *FD = getLabelFacetTrampoline(S, LabelTy, RD,
                                                   /*IsQuery=*/false, Loc)) {
      CS->setHasLocalHandler(true);
      CS->setLocalHandlerFn(FD);
    }

  // Step 6b: Detect queryable_label facet.
  bool QueryViable = false;
  {
    // See callLabelMethod: an inaccessible member means the facet is absent.
    Sema::SFINAETrap Trap(S, /*WithAccessChecking=*/true);
    LookupResult QR = makeFacetLookup(S, LabelFacet::Query, Loc);
    if (lookupFacetMember(S, RD, LabelTy, QR)) {
      ExprResult MR = buildFacetMemberRef(S, LabelExpr, LabelTy, QR, Loc);
      if (!MR.isInvalid()) {
        Expr *QueryMember = MR.get();
        // As the concept: lvalues of `const void*' and `std::size_t'.
        OpaqueValueExpr KeyArg(
            Loc, S.Context.getPointerType(S.Context.VoidTy.withConst()),
            VK_LValue);
        OpaqueValueExpr IdxArg(Loc, S.Context.getSizeType(), VK_LValue);
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
        // labels::queryable_label requires exactly void*.
        QueryViable =
            !Call.isInvalid() &&
            S.Context.hasSameType(Call.get()->getType(), S.Context.VoidPtrTy);
      }
    }
  }

  if (QueryViable)
    if (FunctionDecl *FD = getLabelFacetTrampoline(S, LabelTy, RD,
                                                   /*IsQuery=*/true, Loc)) {
      CS->setHasQuery(true);
      CS->setQueryFn(FD);
    }

  // Step 7: Apply compute_comment facet.
  {
    std::string Comment = CS->getSourceText(S.Context);
    if (std::optional<std::string> Result = callLabelStringMethod(
            S, LabelExpr, LabelTy, RD, LabelFacet::ComputeComment, Comment,
            /*IsNull=*/false, Loc))
      CS->setTransformedComment(S.Context.backupStr(*Result));
  }

  // Step 8: Apply compute_message facet.
  {
    std::string Msg = CS->getUserMessage(S.Context);
    bool IsNull =
        Msg.empty() && !CS->hasMessage() && !CS->getAttr<ContractMessageAttr>();
    if (std::optional<std::string> Result =
            callLabelStringMethod(S, LabelExpr, LabelTy, RD,
                                  LabelFacet::ComputeMessage, Msg, IsNull, Loc))
      CS->setTransformedMessage(S.Context.backupStr(*Result));
  }
}

QualType Sema::deducePostconditionCaptureType(const Expr *Init) {
  // As `auto' deduces from it: an array or a function decays, and top-level
  // cv-qualifiers are dropped.  The initializer is in the contract assertion,
  // so an entity declared outside it is constified there; the capture is
  // declared inside it and is not (P3098; CLANG-641, CLANG-643).
  QualType T = Init->getType();
  if (T.isNull() || T->isDependentType())
    return T;
  if (T->isArrayType())
    T = Context.getArrayDecayedType(T);
  else if (T->isFunctionType())
    T = Context.getPointerType(T);
  return T.getUnqualifiedType();
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
    // Plain capture [i] -- look up in function parameter scope.
    LookupResult R(*this, Id, IdLoc, LookupOrdinaryName);
    LookupName(R, S);
    auto *PVD = R.getAsSingle<ParmVarDecl>();
    if (!PVD) {
      Diag(IdLoc, diag::err_contract_postcondition_capture_not_parameter);
      return nullptr;
    }

    // A pack-expansion capture [i...] is only well-formed when the captured
    // parameter is itself a function parameter pack.
    if (IsPackExpansion && !PVD->isParameterPack()) {
      Diag(IdLoc, diag::err_pack_expansion_without_parameter_packs);
      return nullptr;
    }

    // Conversely, a function parameter pack named without the `...' is an
    // unexpanded pack, ill-formed in the template definition as a lambda's
    // `[xs]' is; left alone, the capture's dependent pack type reaches the
    // instantiation unexpanded.
    if (!IsPackExpansion && PVD->isParameterPack()) {
      ExprResult Ref = BuildDeclRefExpr(
          PVD, PVD->getType().getNonReferenceType(), VK_LValue, IdLoc);
      if (Ref.isInvalid() ||
          DiagnoseUnexpandedParameterPack(Ref.get(), UPPC_Initializer))
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
        if (!InitExpr.isInvalid()) {
          Cap->setInit(InitExpr.get());
          // A capture is an object with automatic storage duration: CodeGen
          // destroys it when the call ends, and Runnable/p3098-dtor.cpp pins
          // the order.  PerformCopyInitialization above only establishes
          // that it can be constructed, so check the destructor too --
          // otherwise an inaccessible or deleted one is accepted here and
          // then called.
          if (auto *RD = T->getAsCXXRecordDecl())
            FinalizeVarWithDestructor(Cap, RD);
        }
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
  // And one that is not a pack expansion may not contain one ([y = xs]).
  if (!IsPackExpansion && DiagnoseUnexpandedParameterPack(Init, UPPC_Initializer))
    return nullptr;

  // Init-capture [old = expr] -- deduce type from initializer.
  QualType DeducedType = deducePostconditionCaptureType(Init);
  // A pack init-capture [...c = e] is itself a pack, as a lambda's is: its
  // type is a pack expansion, so a use of `c' contains an unexpanded pack and
  // can be expanded in the predicate.  Instantiation (TransformContractStmt)
  // expands it into one capture per element.
  if (IsPackExpansion)
    DeducedType = Context.getPackExpansionType(DeducedType, std::nullopt,
                                               /*ExpectPackInType=*/false);

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
    if (!InitExpr.isInvalid()) {
      Cap->setInit(InitExpr.get());
      // As in the parameter-capture path above: an init-capture is destroyed
      // when the call ends, so it has to be destructible here.
      if (auto *RD = DeducedType->getAsCXXRecordDecl())
        FinalizeVarWithDestructor(Cap, RD);
    }
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

/// Whether CS has a requires-clause on a contract of a non-templated function,
/// which P4283 makes ill-formed.  ActOnContractAssert diagnoses it (only the
/// parser reaches that, never an instantiation); BuildContractStmt skips the
/// label-facet and dynamic-descriptor work for it.
static bool hasNonTemplateRequiresClause(Sema &S, const ContractStmt *CS) {
  return CS->hasRequiresClause() && !S.CurContext->isDependentContext() &&
         !CS->getRequiresClause()->isInstantiationDependent() &&
         !S.inTemplateInstantiation();
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
  // contract, and once per instantiation of a templated one.  It must not be
  // narrowed to ActOnContractAssert alone: instantiated contracts would then
  // silently skip it -- setDynamicInfo never called, isDynamic() left false,
  // and codegen falling back to the static/eager semantic with no dynamic
  // dispatch.
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
  // false and falling back to the static semantic.  Restricting
  // the skip to the primary parse keeps instantiations always populating.  The
  // predicate otherwise mirrors the P4283 check in ActOnContractAssert.
  if (auto *CS = Res.getAs<ContractStmt>()) {
    // Whether the predicate can throw decides whether CodeGen guards it for an
    // evaluation_exception violation.  Only Sema can answer that completely
    // (callees' exception specifications, constructors, destructors of
    // temporaries and of deleted objects, default arguments and member
    // initializers, ...), so the answer is computed here and kept on the
    // contract.  A dependent predicate keeps the conservative default; its
    // instantiation comes back through here.
    if (getLangOpts().CXXExceptions && Cond &&
        !Cond->isInstantiationDependent())
      CS->setPredicateCanThrow(canThrow(Cond) != CT_Cannot);

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

    if (!CurContext->isDependentContext() &&
        !hasNonTemplateRequiresClause(*this, CS))
      populateContractSemanticState(CS);
  }

  return Res;
}

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
                               Context.getLangOpts().ContractsP4298,
                               Context.getLangOpts().ContractsP4405);
    // Empty Groups: precomputeDynamicTable falls back to the
    // [[clang::contract_group]] attribute, matching resolveContractConfig.
    precomputeDynamicTable(*this, CS, AllowedMask, /*Groups=*/{},
                           /*LabelExpr=*/nullptr, /*LabelTy=*/QualType(),
                           /*RD=*/nullptr, /*HasComputeSemantic=*/false,
                           CS->getKeywordLoc());
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
    // BuildContractStmt skipped label-facet / dynamic-descriptor population
    // for exactly this condition, so no populated state is discarded here.
    if (hasNonTemplateRequiresClause(*this, CS)) {
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
  // The predicate's own cleanups are already an ExprWithCleanups on the
  // condition: ActOnContractAssertCondition finishes it as a full-expression,
  // type-dependent or not.  Anything else left pending must still be
  // discharged rather than merely skipped: the contract's expression
  // evaluation context is potentially-evaluated, so
  // PopExpressionEvaluationContext MERGES its cleanup state into the parent
  // instead of restoring it, and the flag would escape all the way out to
  // ActOnFinishFunctionBody's "Unaccounted cleanups in function" assertion.
  // Discarding them is right, and is the only thing ActOnFinishFullStmt would
  // usefully do here.
  CleanupVarDeclMarking();
  DiscardCleanupsInEvaluationContext();
  return Res;
}

/// Whether \p PVD is a parameter of the function whose contract is being
/// parsed in the contract scope \p S: one declared between \p S and the
/// nearest function boundary.  That boundary is a function prototype scope
/// (a declarator's parameters, also when they are re-entered to late-parse
/// its contracts) or, for a lambda, its body scope, which holds the lambda's
/// parameters and init-captures.
static bool isOwnParameterScope(Scope *S, const ParmVarDecl *PVD) {
  for (Scope *Sc = S ? S->getParent() : nullptr; Sc; Sc = Sc->getParent()) {
    if (Sc->isDeclScope(PVD))
      return true;
    if (Sc->isFunctionPrototypeScope() || (Sc->getFlags() & Scope::BlockScope))
      return false;
  }
  return false;
}

/// ActOnResultNameDeclarator - Called from Parser::ParseFunctionDeclarator()
/// to introduce parameters into function prototype scope.
ResultNameDecl *Sema::ActOnResultNameDeclarator(ContractKind CK, Scope *S,
                                                QualType RetType,
                                                SourceLocation IDLoc,
                                                IdentifierInfo *II,
                                                unsigned FunctionScopeDepth) {
  assert(II && "ResultName requires an identifier");

  bool IsInvalid = false;

  // A result name is only meaningful on a postcondition.  Check that FIRST:
  // it is the fundamental error, and `contract_assert` has no return type at
  // all (the parser supplies no resolver), so RetType is null there and none
  // of the return-type reasoning below could run.  Substitute a placeholder
  // type so we can still build a node and keep parsing the predicate.
  if (CK != ContractKind::Post) {
    Diag(IDLoc, diag::err_contract_result_name_not_allowed) << II;
    IsInvalid = true;
    if (RetType.isNull())
      RetType = Context.IntTy;
  } else if (RetType->isVoidType()) {
    // Adjust the type of the result name to be int so we can actually produce a
    // node.
    RetType = Context.IntTy;
    Diag(IDLoc, diag::err_contract_result_name_void_return) << II;
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
  //
  // Only on the parser's path (S non-null).  The other callers -- template
  // instantiation and the rebuilds that replace a placeholder result type or
  // move a declaration's contracts onto its definition -- make a copy of a
  // result name that was checked when it was parsed, and have no scope to
  // look anything up in.
  //
  // A name-independent result name (`_`, [basic.scope.scope]) conflicts with
  // nothing: [basic.scope.contract] restricts only a result-name-introducer
  // that is not name-independent.
  if (II && S && !(getLangOpts().CPlusPlus && II->isPlaceholder())) {
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
      // NOTE: PrevDecl is deliberately cleared above for a shadowed template
      // parameter, so this must tolerate null.
      //
      // [basic.scope.contract]: the conflict is with a parameter of the
      // function this postcondition belongs to.  Lookup from a lambda's
      // contract scope carries on past the lambda into the enclosing
      // function, whose parameters are simply hidden by the result name.
      auto *PVD = dyn_cast_if_present<ParmVarDecl>(PrevDecl);
      if (PVD && !isOwnParameterScope(S, PVD))
        PVD = nullptr;
      if (PVD) {
        Diag(IDLoc, diag::err_contract_result_name_shadows_param) << II;
        Diag(PVD->getLocation(), diag::note_previous_declaration);
        New->setInvalidDecl(true);
      } else if (auto *VD = dyn_cast_if_present<VarDecl>(PrevDecl);
                 VD && VD->isInitCapture()) {
        // [basic.scope.contract]/2: for a lambda, a declaration whose target
        // scope is the nearest enclosing lambda scope -- an init-capture of
        // the lambda whose postcondition this is.  (A simple capture
        // declares nothing.)
        const LambdaScopeInfo *LSI = getCurLambda();
        if (LSI && LSI->CallOperator &&
            VD->getDeclContext() == LSI->CallOperator) {
          Diag(IDLoc, diag::err_contract_result_name_shadows_capture) << II;
          Diag(VD->getLocation(), diag::note_previous_declaration);
          New->setInvalidDecl(true);
        }
      }
    }
  }

  assert((!S || S->isContractAssertScope()) &&
         "a contract scope is a contract-assertion scope");

  // Add the parameter declaration into this scope.  Without a scope nothing
  // would ever remove it from the identifier resolver again, and the copy
  // would go on hiding whatever the name means in the enclosing scope for
  // the rest of the translation unit -- a lambda whose return type is
  // deduced has its result names rebuilt that way once the body is parsed.
  // The copies are found through the rebuild's own declaration map.
  if (S) {
    S->AddDecl(New);
    IdResolver.AddDecl(New);
  }

  return New;
}

using ContractScopeRecord = Sema::ContractScopeRecord;

namespace {
struct ScopeEntry {
  const DeclContext *Ctx = nullptr;
  const FunctionScopeInfo *FSI = nullptr;
  const ContractScopeRecord *CSR = nullptr;

  ScopeEntry(const DeclContext *DC, const FunctionScopeInfo *FSI,
             const ContractScopeRecord *CSR)
      : Ctx(DC), FSI(FSI), CSR(CSR) {}
};
} // namespace

/// The context a use in \p DC is judged from.  A requires-expression's body
/// is a DeclContext of its own but pushes no function scope, so a use inside
/// one (in a predicate, as `pre(!requires { ++x; })') is judged from the
/// enclosing context; judged from the body it matches no contract scope, and
/// the use is taken to be outside every contract.
static const DeclContext *walkStartContext(const DeclContext *DC) {
  while (DC && isa<RequiresExprBodyDecl>(DC))
    DC = DC->getParent();
  return DC;
}

namespace {
struct ScopeWalker {
  explicit ScopeWalker(const Sema &S)
      : S(S), CurCtx(walkStartContext(S.CurContext)),
        FunctionScopes(S.FunctionScopes),
        FunctionScopeIndex(FunctionScopes.size()) {
    assert(CurCtx && "the walk starts in a context");
  }

  ScopeWalker(ScopeWalker const &) = delete;
  ScopeWalker &operator=(ScopeWalker const &) = delete;

  FunctionScopeInfo *nextFuncScope() {
    --FunctionScopeIndex;
    assert(FunctionScopeIndex < FunctionScopes.size() &&
           "more contexts than function scopes");
    return FunctionScopes[FunctionScopeIndex];
  }

  SmallVector<ScopeEntry> doIt() {
    while (CurCtx) {
      auto *FSI = nextFuncScope();
      assert(FSI && "every function scope is live");
      const ContractScopeRecord *CSR = nullptr;
      if (FSI->ContractScopeIndex != unsigned(-1)) {
        CSR = &S.ContractScopeStack[FSI->ContractScopeIndex];
        assert(CSR->FunctionScopeAtPush == FSI &&
               "a contract scope belongs to the function scope that pushed it");
      }
      Scopes.emplace_back(CurCtx, FSI, CSR);

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

    // The walk stops early at a class: a use in a local class's member
    // function, at any depth of lambdas and contracts, sees nothing of the
    // enclosing functions' scopes.
    const bool StoppedAtClass = CurCtx && CurCtx->isRecord();
    assert((Scopes.size() <= FunctionScopes.size() &&
            Scopes.size() >= S.getFunctionScopes().size()) ||
           (Scopes.size() < FunctionScopes.size() && StoppedAtClass));
    assert(Scopes.size() >= S.getContractScopes().size() || StoppedAtClass);
    std::reverse(Scopes.begin(), Scopes.end());
    return Scopes;
  }

  const Sema &S;
  const DeclContext *CurCtx;

  SmallVector<FunctionScopeInfo *, 4> FunctionScopes;
  unsigned FunctionScopeIndex;

  SmallVector<ScopeEntry> Scopes;
};
} // namespace

static SmallVector<ScopeEntry> getScopeEntries(const Sema &S) {
  ScopeWalker Walker(S);
  return Walker.doIt();
}

static SmallVector<ScopeEntry>
getInterveningScopeEntries(const Sema &S, const ValueDecl *Var) {
  // A local variable or parameter, or a result name, which a lambda in a
  // postcondition captures as it would a parameter.
  ValueDecl *VD = const_cast<ValueDecl *>(Var);
  if (auto *V = dyn_cast<VarDecl>(VD)) {
    if (!V->isLocalVarDeclOrParm())
      return {};
    VD = V->getCanonicalDecl();
  } else if (!isa<ResultNameDecl>(VD)) {
    return {};
  }
  [[maybe_unused]] ValueDecl *OldVD = const_cast<ValueDecl *>(Var);

  SmallVector<ScopeEntry> Result = getScopeEntries(S);
  if (Result.empty())
    return Result;

  const DeclContext *VarCtx = VD->getDeclContext();
  assert(VarCtx && VarCtx->isFunctionOrMethod() &&
         "a local variable belongs to a function");

  // A use from a local class's member function does not reach the variable's
  // scope at all (the walk stops at the class).  Such a use is ill-formed,
  // and diagnosed when the capture is attempted; nothing intervenes.
  if (std::none_of(Result.begin(), Result.end(),
                   [&](ScopeEntry Ent) { return Ent.Ctx->Equals(VarCtx); }))
    return {};

  ArrayRef ScopeEntries = Result;

  while (!ScopeEntries.empty() && !VarCtx->Equals(ScopeEntries.front().Ctx)) {
    ScopeEntries = ScopeEntries.drop_front(1);
  }

  std::optional<unsigned> LastCopyCaptureIdx;
  unsigned Idx = 0;
  for (auto Pos = ScopeEntries.begin(); Pos != ScopeEntries.end();
       ++Pos, ++Idx) {

    assert(VarCtx->Encloses(Pos->Ctx) &&
           "the walk visits contexts inside the variable's");
    auto *CSI = dyn_cast<CapturingScopeInfo>(Pos->FSI);
    if (!CSI)
      continue;
    // A capture is recorded against the canonical declaration.
    assert((OldVD == VD || !CSI->isCaptured(OldVD)) &&
           "a capture is recorded against the canonical declaration");
    if (CSI->isCaptured(VD)) {
      Capture C = CSI->getCapture(VD);
      if (C.isCopyCapture()) {
        LastCopyCaptureIdx = Idx;
      }
    } else if (Idx != 0 &&
               CSI->ImpCaptureStyle == CapturingScopeInfo::ImpCap_LambdaByval) {
      // An implicit capture under a [=] default is a copy, although it is not
      // yet recorded when this is asked.  (Not in the variable's own scope,
      // where it is a local and nothing captures it.)
      LastCopyCaptureIdx = Idx;
    }
  }
  // The innermost copy names the lambda's own member, which is declared
  // outside every contract scope that lambda opened: keep its entry, whose
  // contract scope, if any, encloses this use (a contract assertion in the
  // body of a mutable `[=]` lambda whose predicate's lambda captures the copy
  // by reference).  Only the scopes outside the copy stop counting.
  if (LastCopyCaptureIdx) {
    ScopeEntries = ScopeEntries.drop_front(LastCopyCaptureIdx.value());
  }
  return SmallVector<ScopeEntry>(ScopeEntries.begin(), ScopeEntries.end());
}

static const ContractScopeRecord *
getInterveningContractEntry(const Sema &S, const ValueDecl *VD) {
  auto Entries = getInterveningScopeEntries(S, VD);
  for (auto &Ent : Entries) {
    if (Ent.CSR)
      return Ent.CSR;
  }
  return nullptr;
}

/// Whether \p FD is the pattern of a member template of a class template
/// specialization that has been explicitly specialized.  The member template
/// the class specialization instantiated is marked too, before the
/// specialization's own declaration is merged with it.
static bool isMemberTemplateSpecialization(const FunctionDecl *FD) {
  const FunctionTemplateDecl *FTD = FD->getDescribedFunctionTemplate();
  return FTD && FTD->isMemberSpecialization();
}

bool Sema::CheckEquivalentContractSequence(FunctionDecl *OldDecl,
                                           FunctionDecl *NewDecl) {
  // If the new declaration doesn't contain any contracts, that's fine, they can
  // be omitted.
  if (!NewDecl->hasContracts())
    return false;

  // For explicit specializations, we need to find the first declaration of the
  // explicit specialization itself, not the implicit instantiation that may be
  // in the redeclaration chain. The implicit instantiation won't have
  // contracts per [temp.expl.spec]p12.
  FunctionDecl *OrigDecl = nullptr;
  if (NewDecl->getTemplateSpecializationKind() == TSK_ExplicitSpecialization) {
    // If OldDecl is not an explicit specialization, then NewDecl is the first
    // explicit specialization declaration - no previous contracts to compare.
    if (OldDecl->getTemplateSpecializationKind() != TSK_ExplicitSpecialization)
      return false;

    // Compare against the first declaration of the explicit specialization
    // itself: the explicit specialization is a function of its own, and a
    // later declaration of it may not add contracts that its first
    // declaration lacks.  The canonical declaration is the specialization
    // that template argument deduction made, which
    // CheckFunctionTemplateSpecialization marks explicit and moves to the
    // first explicit declaration; that one follows it in the chain.
    FunctionDecl *Canonical = OldDecl->getCanonicalDecl();
    if (OldDecl == Canonical)
      return false;
    for (auto *Redecl : OldDecl->redecls()) {
      if (Redecl->getPreviousDecl() == Canonical) {
        OrigDecl = Redecl;
        break;
      }
    }
    if (!OrigDecl || OrigDecl == NewDecl)
      return false;
  } else if (NewDecl->getDescribedFunctionTemplate() &&
             isMemberTemplateSpecialization(OldDecl)) {
    // An explicit specialization of a member template of a class template
    // specialization -- `template<> template<class Q> void A<int>::f(Q)` --
    // is likewise a function template of its own, but it is not marked by
    // its specialization kind.  Its chain starts with the member template
    // the class specialization instantiated, whose contracts are the primary
    // template's: redeclaring that is the specialization's first
    // declaration, with nothing to compare against, and a later declaration
    // compares with the one that follows it.
    FunctionDecl *Canonical = OldDecl->getCanonicalDecl();
    if (OldDecl == Canonical)
      return false;
    for (auto *Redecl : OldDecl->redecls()) {
      if (Redecl->getPreviousDecl() == Canonical) {
        OrigDecl = Redecl;
        break;
      }
    }
    if (!OrigDecl || OrigDecl == NewDecl)
      return false;
  } else {
    OrigDecl = OldDecl->getCanonicalDecl();
  }

  // A predicate that failed to build leaves the whole sequence invalid, and
  // there is then nothing to compare: the contracts it should hold were never
  // formed.  The error has already been reported, so stop here.
  //
  // This must not be an assertion.  It is reachable with no `this`, no
  // member function and no contract on the other declaration at all --
  //
  //   void f();
  //   void f() pre(nonesuch) { }
  //
  // -- because ParseContractSpecifierSequence stores the sequence on the
  // declarator whether or not the predicate built, so an invalid one reaches
  // declaration merging like any other.
  if (NewDecl->getContracts()->isInvalidDecl()) {
    NewDecl->setInvalidDecl(true);
    return true;
  }
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

  // [dcl.contract.func] (P2900)
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
      // different text on different lines is a mismatch).  An empty message
      // is still a message: "" and no message are a mismatch.
      auto HasUserMessage = [](const ContractStmt *C) {
        if (C->getAttr<ContractMessageAttr>())
          return true;
        return C->hasMessage();
      };
      if (HasUserMessage(OC) != HasUserMessage(NC) ||
          OC->getUserMessage(Context) != NC->getUserMessage(Context))
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
  // A friend redeclaring a member of another class is not its out-of-line
  // definition.
  Diag(NewDecl->getLocation(), diag::err_function_different_contract_seq)
      << (isa<CXXMethodDecl>(NewDecl) && !NewDecl->getFriendObjectKind())
      << NewContractRange;

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
  // The note's %select starts at DK_Kind: the kinds before it are diagnosed
  // above without one.
  Diag(OC->getBeginLoc(), diag::note_previous_contracts)
      << (DK - DK_Kind) << (int)OC->getContractKind() << OC->hasResultName()
      << GetRangeForNote(OC);

  return true;
}

/// The name to give FD's parameter \p Idx in a diagnostic.  The declaration
/// whose parameters a check sees -- a definition, or an instantiation from
/// one -- may leave the parameter unnamed, where the declaration whose
/// postcondition names it does not: then take the name from another
/// declaration of FD, or of the template it was instantiated from
/// (CLANG-640: the diagnostic printed "(null)").
static const IdentifierInfo *getParamNameForDiag(const FunctionDecl *FD,
                                                 unsigned Idx) {
  auto NameIn = [&](const FunctionDecl *D) -> const IdentifierInfo * {
    for (const FunctionDecl *R : D->redecls())
      if (R->getNumParams() == FD->getNumParams())
        if (const IdentifierInfo *II = R->getParamDecl(Idx)->getIdentifier())
          return II;
    return nullptr;
  };
  if (const IdentifierInfo *II = FD->getParamDecl(Idx)->getIdentifier())
    return II;
  if (const IdentifierInfo *II = NameIn(FD))
    return II;
  if (const FunctionDecl *Pattern = FD->getTemplateInstantiationPattern())
    if (const IdentifierInfo *II = NameIn(Pattern))
      return II;
  if (const FunctionTemplateDecl *FTD = FD->getPrimaryTemplate())
    return NameIn(FTD->getTemplatedDecl());
  return nullptr;
}

/// Is USAGE an odr-use of one of FD's own non-reference parameters?  That is
/// what makes [dcl.contract.func]'s const requirement apply at all (and what
/// [dcl.fct.def.coroutine] forbids in a coroutine's postcondition), and it is
/// a separate question from whether any particular declaration's parameter
/// satisfies it.
static bool isOwnValueParmOdrUse(const FunctionDecl *FD, const ParmVarDecl *PVD,
                                 const DeclRefExpr *Usage) {
  if (PVD->getFunctionScopeIndex() >= FD->getNumParams() ||
      FD->getParamDecl(PVD->getFunctionScopeIndex()) != PVD)
    return false;
  // [dcl.contract.func] p2900r8 --
  //   If a  postcondition  odr-uses ([basic.def.odr]) a non-reference
  //   parameter...
  return !PVD->getType()->isReferenceType() && !Usage->isNonOdrUse();
}

namespace {
/// A visitor of a postcondition that does not visit the potential results of
/// a discarded-value expression in it, which are not odr-used: the visitors
/// that look for the odr-uses of a function's parameters there derive from
/// it.
template <typename Derived>
struct PostconditionOdrUseVisitor : RecursiveASTVisitor<Derived> {
  using super = RecursiveASTVisitor<Derived>;

  Derived &derived() { return *static_cast<Derived *>(this); }

  // [expr.context]: the left operand of a comma is a discarded-value
  // expression, and the lvalue-to-rvalue conversion is applied to one only
  // when it is a volatile glvalue.  So by [basic.def.odr]/5 a parameter that
  // is among its POTENTIAL RESULTS is not odr-used, and
  // [dcl.contract.func]/7 does not reach it:
  // `void f (bool b) post ((b, true)) {}`.
  bool TraverseBinaryOperator(BinaryOperator *E) {
    if (E->getOpcode() != BO_Comma)
      return super::TraverseBinaryOperator(E);
    if (!TraverseDiscardedValue(E->getLHS()))
      return false;
    return derived().TraverseStmt(E->getRHS());
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
      if (!derived().TraverseStmt(CO->getCond()))
        return false;
      if (!TraverseDiscardedValue(CO->getTrueExpr()))
        return false;
      return TraverseDiscardedValue(CO->getFalseExpr());
    }

    // A class member access naming a non-static data member has the
    // potential results of its object expression ([basic.def.odr]/2), so
    // `(s.m, true)` does not odr-use `s` either.  A `->` operand is a prvalue,
    // which is evaluated, and a volatile member is loaded: the lvalue-to-
    // rvalue conversion wraps it, as it wraps a volatile parameter, so neither
    // reaches here (CLANG-637).
    if (auto *ME = dyn_cast<MemberExpr>(S))
      if (!ME->isArrow() && isa<FieldDecl>(ME->getMemberDecl()))
        return TraverseDiscardedValue(ME->getBase());

    // A potential result naming a parameter: exempt, and nothing below it
    // needs visiting.
    if (isa<DeclRefExpr>(S))
      return true;

    return derived().TraverseStmt(E);
  }
};

/// Finds, in the postcondition predicates of a function, the odr-uses of its
/// non-reference parameters that [dcl.contract.func]/7 requires to be const.
struct ParamReferenceChecker
    : PostconditionOdrUseVisitor<ParamReferenceChecker> {
  using super = PostconditionOdrUseVisitor<ParamReferenceChecker>;

private:
  Sema &Actions;
  const FunctionDecl *FD;

public:
  ContractStmt *CurrentContract = nullptr;
  ContractSpecifierDecl *CSD = nullptr;

public:
  ParamReferenceChecker(Sema &S, const FunctionDecl *FD)
      : Actions(S), FD(FD), CSD(FD->getContracts()) {
    assert(CSD && "only functions with contracts are checked");
  }

  bool TraverseContractStmt(ContractStmt *CS) {
    ValueRAII<ContractStmt *> SetCurrent(CurrentContract, CS,
                                         CurrentContract == nullptr);
    if (CS->getCond())
      TraverseStmt(CS->getCond());
    return true;
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

  /// Which requirement, if any, the non-dependent PVD violates, given that a
  /// postcondition odr-uses it (isOwnValueParmOdrUse).
  DiagSelector classifyDiagnosableParmVar(const ParmVarDecl *PVD) const {
    QualType PVDType = PVD->getOriginalType();

    // that parameter shall not have an array type...
    if (PVDType->isArrayType() || PVDType->isArrayParameterType())
      return DS_Array;

    // or function type...
    if (PVDType->isFunctionType())
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
    auto *PVD = dyn_cast_if_present<ParmVarDecl>(E->getDecl());
    if (!PVD || PVD->getType()->isDependentType() || DiagnosedDecls.count(PVD))
      return true;

    // Record it whether or not THIS declaration's parameter is const: the rule
    // reaches the corresponding parameter on every declaration, and the one
    // that violates it may have been merged away.
    if (!isOwnValueParmOdrUse(FD, PVD, E))
      return true;
    OdrUsedParms.try_emplace(PVD->getFunctionScopeIndex(), E->getLocation());

    if (DiagSelector DiagSelect = classifyDiagnosableParmVar(PVD);
        DiagSelect != DS_None) {
      DiagnosedDecls.insert(PVD);
      CSD->setInvalidDecl(true);

      Actions.Diag(E->getLocation(),
                   diag::err_contract_postcondition_parameter_type_invalid)
          << getParamNameForDiag(FD, PVD->getFunctionScopeIndex())
          << DiagSelect
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
static void diagnoseRedeclParamConst(
    Sema &S, FunctionDecl *FD,
    const llvm::MapVector<unsigned, SourceLocation> &Used) {
  if (Used.empty())
    return;

  // Only an instantiation can have lost a declaration this way: every
  // declaration of a non-template function is checked when it is written
  // (Sema::CheckPostconditionParamsOnRedecl).  The declarations to consult are
  // those of the function template, or of the member function of a class
  // template that this specializes, with the matching template arguments.
  SmallVector<const FunctionDecl *, 4> Patterns;
  std::optional<MultiLevelTemplateArgumentList> TemplateArgs;
  FunctionTemplateDecl *FTD = FD->getPrimaryTemplate();
  if (FTD && FD->getTemplateSpecializationArgs()) {
    // redecls() on a RedeclarableTemplateDecl yields the base type.
    for (auto *RD : FTD->redecls())
      Patterns.push_back(cast<FunctionTemplateDecl>(RD)->getTemplatedDecl());
    // Exactly one level, holding this function template's own arguments.
    // The pattern belongs to an already-instantiated enclosing class if
    // there is one, so its template parameters are at depth 0 either way;
    // adding the class's arguments as an outer level made `U` in
    // Foo<int>::bar<const int> resolve to the class's `int` and rejected a
    // well-formed program.
    TemplateArgs.emplace(FTD, FD->getTemplateSpecializationArgs()->asArray(),
                         /*Final=*/true);
  } else if (FunctionDecl *Member = FD->getInstantiatedFromMemberFunction()) {
    for (const FunctionDecl *RD : Member->redecls())
      Patterns.push_back(RD);
    TemplateArgs = S.getTemplateInstantiationArgs(FD);
  } else {
    return;
  }

  for (const auto &[Idx, UseLoc] : Used) {
    // Already ill-formed on its own account and diagnosed there; saying it
    // twice for one parameter helps nobody.  This exists for the case where
    // the surviving parameter looks fine and another declaration did not.
    if (!FD->getParamDecl(Idx)->getType().isConstQualified())
      continue;

    for (const FunctionDecl *Pattern : Patterns) {

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
        QualType Subst =
            S.SubstType(T, *TemplateArgs, P->getLocation(), P->getDeclName());
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
          << getParamNameForDiag(FD, Idx) << ParamReferenceChecker::DS_NotConst;
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
/// Collects the parameters of a function -- by index, with the first odr-use
/// and the parameter it names -- that its postconditions odr-use:
/// non-reference parameters of the function itself, not of a lambda in a
/// predicate.
class PostconditionParamUseCollector
    : public PostconditionOdrUseVisitor<PostconditionParamUseCollector> {
public:
  struct Use {
    SourceLocation Loc;
    const ParmVarDecl *Parm;
  };
  llvm::MapVector<unsigned, Use> Used;

  bool VisitDeclRefExpr(DeclRefExpr *E) {
    const auto *PVD = dyn_cast_if_present<ParmVarDecl>(E->getDecl());
    if (!PVD || E->isNonOdrUse() || PVD->getFunctionScopeDepth() != 0 ||
        PVD->getType()->isReferenceType() ||
        !isa_and_present<FunctionDecl>(PVD->getDeclContext()))
      return true;
    Used.insert({PVD->getFunctionScopeIndex(), {E->getExprLoc(), PVD}});
    return true;
  }
};
} // namespace

void Sema::CheckPostconditionParamsOnRedecl(FunctionDecl *OldDecl,
                                            FunctionDecl *NewDecl) {
  ContractSpecifierDecl *CSD = OldDecl->getContracts();
  if (!CSD || CSD->isInvalidDecl())
    return;
  PostconditionParamUseCollector Collector;
  for (ContractStmt *CS : CSD->postconditions())
    Collector.TraverseStmt(CS);

  for (const auto &[Idx, Use] : Collector.Used) {
    if (Idx >= NewDecl->getNumParams())
      continue;
    const ParmVarDecl *P = NewDecl->getParamDecl(Idx);
    QualType T = P->getType();
    // A dependent type is judged on instantiation (diagnoseRedeclParamConst),
    // and a pack does not correspond by index.
    if (T->isDependentType() || T->isReferenceType() || P->isParameterPack() ||
        T.isConstQualified())
      continue;
    // The redeclaration may leave the parameter unnamed; then name it as the
    // postcondition does (CLANG-640).
    const IdentifierInfo *Name =
        P->getIdentifier() ? P->getIdentifier() : Use.Parm->getIdentifier();
    Diag(P->getTypeSpecStartLoc(),
         diag::err_contract_postcondition_parameter_type_invalid)
        << Name << ParamReferenceChecker::DS_NotConst;
    Diag(Use.Loc, diag::note_contract_postcondition_parameter_use);
  }
}

namespace {
/// Finds an odr-use of a non-reference parameter of FD in a postcondition.
class CoroutineParamUseFinder
    : public PostconditionOdrUseVisitor<CoroutineParamUseFinder> {
  const FunctionDecl *FD;

public:
  llvm::SmallVector<const ParmVarDecl *, 4> Found;

  explicit CoroutineParamUseFinder(const FunctionDecl *FD) : FD(FD) {}

  bool VisitDeclRefExpr(DeclRefExpr *E) {
    // A reference parameter is fine: [dcl.fct.def.coroutine]/5 binds its copy
    // to the same object.
    const auto *PVD = dyn_cast_if_present<ParmVarDecl>(E->getDecl());
    if (!PVD || !isOwnValueParmOdrUse(FD, PVD, E))
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
        << getParamNameForDiag(FD, PVD->getFunctionScopeIndex());
    Diag(PVD->getLocation(), diag::note_contract_coroutine_postcondition_param);
    FD->setInvalidDecl();
  }
}

void Sema::DiagnoseContractOnDeletedOrDefaulted(FunctionDecl *FD) {
  if (!FD->hasContracts())
    return;
  bool Deleted = FD->isDeletedAsWritten();
  if (!Deleted && !(FD->isExplicitlyDefaulted() && FD->isFirstDecl()))
    return;
  Diag(FD->getLocation(), diag::err_contract_on_deleted_or_defaulted)
      << !Deleted << FD;
}

void Sema::CheckFunctionContracts(FunctionDecl *FD, bool IsDefinition,
                                  bool IsInstantiation) {
  assert(FD && FD->hasContracts() &&
         "only functions with contracts are checked");

  DiagnoseContractOnDeletedOrDefaulted(FD);

  if (auto *MD = dyn_cast<CXXMethodDecl>(FD)) {
    if (MD->isVirtual() && !getLangOpts().ContractsP3097) {
      // A member already virtual in its pattern was diagnosed there.
      const auto *Pattern = dyn_cast_if_present<CXXMethodDecl>(
          IsInstantiation ? FD->getTemplateInstantiationPattern() : nullptr);
      if (!Pattern || !Pattern->isVirtual())
        Diag(FD->getLocation(), diag::err_contracts_on_virtual_require_flag);
      return;
    }
  }

  diagnoseParamTypes(*this, FD, FD->getContracts());
}

void Sema::InstantiateFunctionContractsOnUse(
    SourceLocation PointOfInstantiation, FunctionDecl *Function) {
  if (!getLangOpts().Contracts)
    return;

  // Why this exists is on its declaration.  Among what a never-substituted
  // predicate escapes is [dcl.contract.func]/7, the rule that a non-reference
  // parameter odr-used by a postcondition be const.

  // Only for template instantiations: the pattern must carry contracts, and
  // the instantiation must not already have a substituted specifier of its own
  // (e.g. because its definition was instantiated first), in which case there
  // is nothing to do.
  //
  // Two states mean "not yet substituted".  A member of a class template
  // instantiation is handed the pattern's own (dependent) specifier as a
  // placeholder by VisitCXXMethodDecl -- and it may have come from any
  // declaration on the pattern's redeclaration chain, not necessarily
  // PatternDecl, hence holdsPatternContractSpecifier.  A function template
  // specialization formed by deduction, on the other hand, simply has NO
  // contracts until its definition is instantiated, so the placeholder must
  // not be required here: requiring it makes this a no-op for a
  // declaration-only function template, which is the [dcl.contract.func]/9
  // case above.
  const FunctionDecl *PatternDecl = Function->getTemplateInstantiationPattern();
  if (!PatternDecl || !PatternDecl->hasContracts())
    return;
  if (Function->getContracts() &&
      !holdsPatternContractSpecifier(Function, PatternDecl))
    return;
  if (Function->isInvalidDecl() || Function->isDependentContext())
    return;

  // Guard against recursive / re-entrant instantiation.
  InstantiatingTemplate Inst(*this, PointOfInstantiation, Function);
  if (Inst.isInvalid())
    return;

  // Break cycles between contracts.  Substituting a predicate odr-uses what it
  // calls, so two functions whose predicates call each other ask for each
  // other's contracts without end.  Whichever request came first is still on
  // the stack and will set this function's contracts when it returns, so the
  // re-entrant one has nothing to add.
  if (!ContractInstantiationsInProgress.insert(Function).second)
    return;
  llvm::scope_exit LeaveInProgress(
      [&] { ContractInstantiationsInProgress.erase(Function); });

  Sema::ContextRAII SavedContext(*this, Function);

  // Substitute in a function scope and an expression evaluation context of our
  // own, rather than borrowing whatever the odr-use happened to occur in.
  //
  // This runs from MarkFunctionReferenced, so it is reachable while some
  // *other* function's contract predicate is still being transformed: that
  // predicate odr-uses a function which itself has contracts, and lands right
  // here.  PushContractScope records its state on getCurFunction() and asserts
  // that scope is not already inside a contract -- which the enclosing
  // function's scope is.  The two functions are unrelated, and neither should
  // see the other's scope, so give this substitution its own.
  //
  // The expression evaluation context matters for the same reason: entering
  // Function resets the immediate-function-context tracking, so a predicate
  // substituted from inside a consteval function's contract does not inherit
  // that function's immediate context.
  //
  // A lambda's call operator gets the lambda's own scope instead, rebuilt
  // from its closure type, as for its exception specification: a predicate
  // may name the lambda's captures, and capturing finds them only in a
  // lambda scope.  This path is reached for one before its body is
  // instantiated (which would otherwise provide that scope) when the
  // call operator's return type is not deduced, so that a call can be formed
  // without the definition.
  LocalInstantiationScope Scope(*this);

  MultiLevelTemplateArgumentList TemplateArgs = getTemplateInstantiationArgs(
      Function, Function->getLexicalDeclContext(), /*Final=*/false,
      /*Innermost=*/std::nullopt, /*RelativeToPrimary=*/false, PatternDecl);

  const bool IsLambda = isLambdaCallOperator(Function);
  std::optional<LambdaScopeForCallOperatorInstantiationRAII> LambdaScope;
  if (IsLambda)
    LambdaScope.emplace(*this, Function, TemplateArgs, Scope,
                        /*ShouldAddDeclsFromParentScope=*/false);
  else
    PushFunctionScope();
  PushExpressionEvaluationContextForFunction(
      ExpressionEvaluationContext::PotentiallyEvaluated, Function);
  llvm::scope_exit LeaveFunctionScope([&] {
    PopExpressionEvaluationContext();
    if (!IsLambda)
      PopFunctionScopeInfo();
  });

  // Make the instantiated parameters visible so the contract predicate's
  // references resolve to this function's own parameters.
  if (addInstantiatedParametersToScope(Function, PatternDecl, Scope,
                                       TemplateArgs))
    return;

  // And a lambda's init-captures, which the predicate names as the pattern's.
  // Their instantiations are the closure type's captures, which a closure
  // type still being instantiated does not have yet.
  if (IsLambda) {
    const CXXRecordDecl *Closure = cast<CXXMethodDecl>(Function)->getParent();
    const CXXRecordDecl *ClosurePattern =
        cast<CXXMethodDecl>(PatternDecl)->getParent();
    if (Closure->capture_size() || !ClosurePattern->capture_size())
      if (addInstantiatedCapturesToScope(Function, PatternDecl, Scope,
                                         TemplateArgs))
        return;
  }

  InstantiateContractSpecifier(PointOfInstantiation, Function, PatternDecl,
                               TemplateArgs);
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

  auto *Method = const_cast<CXXMethodDecl *>(
      dyn_cast_if_present<CXXMethodDecl>(Instantiation));

  Sema::ContextRAII SavedContext(*this, Instantiation);
  Sema::CXXThisScopeRAII ThisScope(
      SemaRef, Method ? Method->getParent() : nullptr,
      Method ? Method->getMethodQualifiers().withConst() : Qualifiers{},
      Method != nullptr);

  LocalInstantiationScope Scope(*this, true);

  // P4283: if every contract is discarded by an unsatisfied requires-clause,
  // the instantiation's contracts are cleared rather than left alone.  Leaving
  // them would leave the instantiation pointing at the pattern's dependent
  // contract specifier (the placeholder installed at class-template
  // instantiation), so downstream consumers -- in particular the P3097
  // virtual interface wrapper -- would emit the pattern's discarded contract
  // and reference the pattern's parameters (crashing in CodeGen).  A null
  // specifier makes hasContracts() false, so the wrapper is skipped and no
  // check is emitted; the on-use re-entry guard
  // (InstantiateVirtualFunctionContractsOnUse) also treats a null specifier as
  // "already resolved".
  if (!SetTransformedContracts(
          Instantiation, PatternCSD->contracts(), PatternCSD->getLocation(),
          [&](ContractStmt *CS) { return SubstStmt(CS, TemplateArgs); }))
    return;

  if (!Instantiation->isDependentContext())
    CheckFunctionContracts(Instantiation, /*IsDefinition=*/false,
                           /*IsInstantiation=*/true);
}

ContractSpecifierDecl *Sema::SetTransformedContracts(
    FunctionDecl *FD, ArrayRef<ContractStmt *> Contracts, SourceLocation Loc,
    llvm::function_ref<StmtResult(ContractStmt *)> Transform) {
  SmallVector<ContractStmt *> NewContracts;
  for (ContractStmt *CS : Contracts) {
    StmtResult NewStmt = Transform(CS);
    if (NewStmt.isInvalid())
      NewContracts.push_back(
          BuildInvalidContractStmt(CS->getContractKind(), CS->getKeywordLoc()));
    // dyn_cast, not ActionResult::getAs, which is a static_cast: a contract
    // discarded by an unsatisfied P4283 requires-clause comes back as a
    // NullStmt and must be dropped, not stored as a bogus ContractStmt.
    else if (auto *NewCS = dyn_cast_if_present<ContractStmt>(NewStmt.get()))
      NewContracts.push_back(NewCS);
  }

  // Every contract discarded is not an error: the function simply has no
  // contracts, and a specifier has at least one.
  if (NewContracts.empty()) {
    FD->setContracts(nullptr);
    return nullptr;
  }
  auto *CSD = ContractSpecifierDecl::Create(Context, FD, Loc, NewContracts);
  FD->setContracts(CSD);
  return CSD;
}

/// ActOnFinishContractSpecifierSequence - This is called after a
/// contract-specifier-seq has been parsed.
ContractSpecifierDecl *
Sema::ActOnFinishContractSpecifierSequence(ArrayRef<ContractStmt *> Contracts,
                                           SourceLocation Loc) {
  assert(!Contracts.empty() && "Expected at least one contract");

  return ContractSpecifierDecl::Create(Context, CurContext, Loc, Contracts);
}

PostconditionCaptureDecl *
Sema::BuildExpandedPostconditionCapture(PostconditionCaptureDecl *Pattern,
                                        QualType Type, ExprResult Init) {
  TypeSourceInfo *TInfo =
      Context.getTrivialTypeSourceInfo(Type, Pattern->getLocation());
  auto *Cap = PostconditionCaptureDecl::CreateClone(Context, CurContext,
                                                    Pattern, Type, TInfo);
  Cap->setIsPackExpansion(false);

  // Copy-initialize from the substituted initializer, as
  // TemplateDeclInstantiator::VisitPostconditionCaptureDecl does for a single
  // capture.
  if (Init.isUsable()) {
    if (!Type->isDependentType())
      Init =
          PerformCopyInitialization(InitializedEntity::InitializeVariable(Cap),
                                    Pattern->getLocation(), Init.get());
    if (Init.isInvalid())
      return nullptr;
    Cap->setInit(Init.get());
    if (auto *RD = Type->getAsCXXRecordDecl())
      FinalizeVarWithDestructor(Cap, RD);
  }
  return Cap;
}

ContractStmt *Sema::BuildInvalidContractStmt(ContractKind CK,
                                             SourceLocation KeywordLoc) {
  Expr *Cond =
      CreateRecoveryExpr(KeywordLoc, KeywordLoc, {}, Context.BoolTy).get();
  ContractStmt *CS = ContractStmt::Create(
      Context, CK, KeywordLoc, Cond, /*ResultName=*/nullptr,
      /*Message=*/nullptr, /*Label=*/nullptr, /*Captures=*/nullptr,
      /*Attrs=*/{}, /*RequiresClause=*/nullptr);
  CS->setInvalid();
  return CS;
}

void Sema::ActOnLateParsedFunctionContracts(FunctionDecl *FD,
                                            bool IsDefinition) {
  FunctionDecl *Prev = FD->getPreviousDecl();
  // As MergeFunctionDecl does for contracts parsed with the declarator, a
  // redeclaration whose contracts differ is invalid and goes no further.
  if (Prev && !FD->isInvalidDecl() &&
      CheckEquivalentContractSequence(Prev, FD)) {
    FD->setInvalidDecl();
    return;
  }
  ActOnContractsOnFinishFunctionDecl(FD, IsDefinition);
  if (!IsDefinition && Prev && !FD->isInvalidDecl())
    CheckPostconditionParamsOnRedecl(Prev, FD);
}

void Sema::ActOnContractsOnFinishFunctionDecl(FunctionDecl *D,
                                              bool IsDefinition) {
  // Checked with the contracts, once the parser attaches them.
  if (FunctionContractsPending)
    return;

  FunctionDecl *FD;
  if (FunctionTemplateDecl *FunTmpl = dyn_cast<FunctionTemplateDecl>(D))
    FD = FunTmpl->getTemplatedDecl();
  else
    FD = D;

  // [temp.expl.spec]p12
  // ... and function-contract-specifier appearing in the declaration of a
  // template have no effect on an explicit specialization of
  // that template.
  //
  // An explicit specialization inherits from an earlier declaration of that
  // same specialization that has contracts -- not from the declaration an
  // implicit instantiation of the enclosing class template made for the
  // member, which carries the template's.  (The specialization kind cannot
  // tell them apart: it is shared by the whole redeclaration chain.)
  FunctionDecl *First = FD->getFirstDecl();
  // An explicit specialization of a member template of a class template
  // specialization is not marked by its specialization kind, but its chain
  // starts the same way, with the member template the class specialization
  // instantiated (which is marked a member specialization); that
  // declaration's contracts are the primary template's, not its own.
  if (FD->getTemplateSpecializationKind() == TSK_ExplicitSpecialization ||
      (FD->getDescribedFunctionTemplate() && First != FD &&
       isMemberTemplateSpecialization(First))) {
    First = FD;
    for (FunctionDecl *P = FD->getPreviousDecl(); P; P = P->getPreviousDecl()) {
      auto *Cls =
          dyn_cast<ClassTemplateSpecializationDecl>(P->getLexicalDeclContext());
      if (Cls && Cls->getSpecializationKind() == TSK_ImplicitInstantiation)
        continue;
      if (P->hasContracts()) {
        First = P;
        break;
      }
    }
  }

  // If the new declaration/definition doesn't have contracts, and its previous
  // declarations didn't have contracts or it's a specialization which doesn't
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
      !FD->isTemplateInstantiation()) {
    // Note: This case is mutually exclusive with the NonDependentPlaceholders
    // case, since we can't have a placeholder return type on a declaration that
    // isn't a definition.

    // For an explicit specialization, First is its own earliest declaration
    // (see above).

    // Null when every contract was discarded by an unsatisfied P4283
    // requires-clause; RebuildContractSpecifierForDecl has already cleared the
    // definition's specifier in that case.
    if (ContractSpecifierDecl *NewCSD =
            RebuildContractSpecifierForDecl(First, FD))
      FD->setContracts(NewCSD);
  }

  if (!FD->hasContracts())
    return;

  assert(FD->hasContracts() && "only functions with contracts are checked");

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
  if (CSD->hasInventedPlaceholderTypes() && !FD->isTemplateInstantiation() &&
      !FD->isTemplated()) {
    if (!IsDefinition && !FD->isThisDeclarationADefinition()) {
      Diag(CSD->getCanonicalResultName()->getLocation(),
           diag::err_contract_result_name_deduced_non_definition)
          << CSD->getCanonicalResultName();
      Diag(FD->getReturnTypeSourceRange().getBegin(),
           diag::note_contract_function_return_type)
          << FD->getReturnTypeSourceRange();
      for (auto *RND : CSD->result_names())
        RND->setInvalidDecl(true);
    }
  }
}

namespace {

/// Rebuilds a contract specifier against another declaration of its function
/// (RebuildContractSpecifierForDecl); AlwaysRebuild forces a fresh copy of
/// every node rather than reusing unchanged ones.
struct RebuildFunctionContracts
    : public TreeTransform<RebuildFunctionContracts> {
  bool IsAlwaysRebuild = false;
  bool AlwaysRebuild() const { return IsAlwaysRebuild; }

  RebuildFunctionContracts(Sema &S, bool Rebuild = false)
      : TreeTransform<RebuildFunctionContracts>(S), IsAlwaysRebuild(Rebuild) {}

  /// The function whose own local variables the predicates may declare -- in
  /// a GNU statement expression, which only a lambda's contract can contain.
  /// Null when no such variable can occur.
  const DeclContext *LocalsOf = nullptr;

  /// A local variable declared in a predicate gets a copy whose type and
  /// initializer are rebuilt with the rest of it.  The default keeps the
  /// original declaration, whose initializer may name a result name of the
  /// placeholder type this rebuild exists to replace, and that type would
  /// then reach code generation undeduced.
  Decl *TransformDefinition(SourceLocation Loc, Decl *D) {
    auto *OldVar = dyn_cast<VarDecl>(D);
    if (!LocalsOf || !OldVar || !OldVar->hasLocalStorage() ||
        isa<ParmVarDecl, PostconditionCaptureDecl, DecompositionDecl>(
            OldVar) ||
        OldVar->getDeclContext() != LocalsOf)
      return TreeTransform::TransformDefinition(Loc, D);

    TypeSourceInfo *TSI =
        getDerived().TransformType(OldVar->getTypeSourceInfo());
    if (!TSI)
      return nullptr;
    auto *NewVar = VarDecl::Create(
        SemaRef.Context, OldVar->getDeclContext(), OldVar->getInnerLocStart(),
        OldVar->getLocation(), OldVar->getIdentifier(), TSI->getType(), TSI,
        OldVar->getStorageClass());
    NewVar->setConstexpr(OldVar->isConstexpr());
    OldVar->getDeclContext()->addHiddenDecl(NewVar);
    transformedLocalDecl(OldVar, {NewVar});

    if (Expr *OldInit = OldVar->getInit()) {
      ExprResult Init = getDerived().TransformInitializer(
          OldInit, OldVar->getInitStyle() == VarDecl::CallInit);
      if (Init.isInvalid())
        NewVar->setInvalidDecl();
      else
        SemaRef.AddInitializerToDecl(NewVar, Init.get(),
                                     OldVar->isDirectInit());
    } else {
      SemaRef.ActOnUninitializedDecl(NewVar);
    }
    return NewVar;
  }
};

} // namespace

/// Rebuild the contract specifier written on one declaration in the context of
/// the definition. This rebinds parameters and result names as needed.
ContractSpecifierDecl *
Sema::RebuildContractSpecifierForDecl(FunctionDecl *First, FunctionDecl *Def) {

  assert(
      (Def->getContracts() == First->getContracts() || !Def->hasContracts()) &&
      "the definition shares or lacks the first declaration's contracts");
  Def->setContracts(nullptr);
  Sema::ContextRAII SavedContext(*this, Def);
  assert(FunctionScopes.empty() && "no function is being parsed");

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
      auto *NewCap = PostconditionCaptureDecl::CreateClone(
          Context, Def, OldCap, OldCap->getType(), OldCap->getTypeSourceInfo());
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
  ContractSpecifierDecl *CSD = SetTransformedContracts(
      Def, First->contracts(), First->getContracts()->getLocation(),
      [&](ContractStmt *CS) { return Rebuilder.TransformContractStmt(CS); });
  if (CSD && CSD->isInvalidDecl())
    Def->setInvalidDecl();
  return CSD;
}

DeclResult Sema::RebuildContractsWithPlaceholderReturnType(FunctionDecl *FD) {

  ContractSpecifierDecl *CSD = FD->getContracts();
  assert(CSD && CSD->hasInventedPlaceholderTypes() &&
         "Cannot rebuild contracts without placeholders");
  if (FD->isInvalidDecl()) {
    CSD->setInvalidDecl(true);
    return DeclResult(/*IsInvalid*/ true);
  }

  RebuildFunctionContracts Rebuilder(*this, true);
  Rebuilder.LocalsOf = FD;

  QualType Replacement;
  auto CheckFunctionReturnType = [&](SourceLocation Loc) -> bool {
    if (!Replacement.isNull())
      return false;

    if (FD->getReturnType()->isUndeducedType()) {
      assert(!FD->isInvalidDecl() &&
             "an invalid function has no undeduced return type to wait for");
      assert(!CSD->isInvalidDecl() &&
             "an invalid specifier has no placeholder to rebuild");
      // DeduceReturnType emits its own diagnostic on failure.
      if (DeduceReturnType(FD, Loc, true))
        return true;
    }
    assert(!FD->getReturnType()->isUndeducedType() &&
           "the return type is deduced by now");
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
    assert(!Replacement.isNull() && "every placeholder has a replacement");
    auto *NewRND = ActOnResultNameDeclarator(
        ContractKind::Post, nullptr, Replacement, RND->getLocation(),
        RND->getIdentifier(), RND->getFunctionScopeDepth());
    Transformed.emplace_back(RND, NewRND);
  }

  for (auto [K, V] : Transformed)
    Rebuilder.transformedLocalDecl(K, {V});

  ContractSpecifierDecl *NewCSD = SetTransformedContracts(
      FD, FD->contracts(), FD->getContracts()->getLocation(),
      [&](ContractStmt *CS) { return Rebuilder.TransformContractStmt(CS); });
  if (!NewCSD)
    return DeclResult(/*IsInvalid*/ false);
  if (NewCSD->isInvalidDecl())
    FD->setInvalidDecl();
  return NewCSD;
}

/// ActOnContractsOnFinishFunctionBody
///
/// This function ensures there is a usable version of the
/// function contracts attached to the definition declaration.
///
/// This is already the case if the definition declaration was spelled with
/// contracts.
///
/// Otherwise, we might have contracts on the first declaration.
/// If we do,
///
/// (1) attach them to the defining declaration.
/// (2) rebind any references to parameters contained within the contracts.
///
/// This is important because the definition's parameters are the ones
/// evaluated by ExprConstant/CodeGen.
void Sema::DiagnoseMistypedDynamicContractSelectors() {
  for (StringRef Name :
       getLangOpts().ContractOpts.getCXXDynamicSelectorNames())
    ContractStmt::lookupDynamicSelector(
        Context, Name,
        [&](const NamedDecl *D, ContractStmt::DynamicSelectorMatch M) {
          if (M != ContractStmt::DynamicSelectorMatch::Mistyped)
            return;
          QualType T = cast<ValueDecl>(D)->getType();
          Diag(D->getLocation(), diag::warn_contract_dynamic_selector_wrong_type)
              << isa<VarDecl>(D) << D << Name << T;
        });
}

void Sema::ActOnContractsOnFinishFunctionBody(FunctionDecl *Def) {
  assert(Def && "the function has a definition");
  // If we had a deduced return type on a non-template function, we can now
  // attempt to deduce the return type and rebuild the contracts with the
  // deduced return type.
  if (Def->hasContracts() &&
      Def->getContracts()->hasInventedPlaceholderTypes()) {
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
      Def->setContracts(NewCSD);
      if (NewCSD->isInvalidDecl())
        Def->setInvalidDecl(true);
    }
  }
}

bool Sema::isUsageAcrossContract(const ValueDecl *VD) {
  // There's no contract scope anywhere above us.
  if (!getCurrentContractEntry())
    return false;

  // Fast Path: We're in an immediate contract assertion expression evaluation
  // context.
  if (isDirectlyInContractPredicate())
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
  // never captured, so putting it through that walk would ask a question with
  // no answer: it would be constified when named directly in a predicate (the
  // fast path above) and not when named from inside a lambda there.
  //
  // The non-local half must answer true as well: P2900R9 constifies every
  // variable, not only those with automatic storage duration, so
  // namespace-scope variables, thread_locals and static data members named in
  // a predicate are across the contract too.
  if (const auto *Var = dyn_cast<VarDecl>(VD))
    if (!Var->isLocalVarDeclOrParm() ||
        Var->getStorageDuration() == SD_Static ||
        Var->getStorageDuration() == SD_Thread)
      return true;
  // Nor is a template parameter captured.
  if (isa<NonTypeTemplateParmDecl>(VD))
    return true;

  // Nothing has pushed a function scope, so there is no scope stack to walk
  // and nothing can lie between this use and the contract.  A contract scope
  // is current -- checked at the top -- so the use is inside a predicate.
  //
  // A function's contracts are late-parsed (ParseContracts.cpp, CES_Function),
  // which pushes a FunctionScopeInfo, so this is only a contract parsed where
  // it appears on a declarator that declares no function, which is diagnosed
  // and discarded.  RebuildContractSpecifierForDecl asserts the same emptiness
  // and pushes a scope of its own before it does any work.
  //
  // Reaching here at all requires an unevaluated operand in the predicate: the
  // isDirectlyInContractPredicate() fast path above answers every use that is
  // directly in the predicate.  A requires-expression is the case that gets
  // this far, and without this guard it walks off the bottom of the scope
  // stack and fails ScopeWalker::nextFuncScope's assertion.
  if (FunctionScopes.empty())
    return true;

  assert(VD && "only a declaration is asked about");
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
  if (!LangOpts.ContractConstification)
    return CC_None;

  assert(VD && "only a declaration is asked about");
  const ContractScopeRecord *CSR = getCurrentContractEntry();

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

  // A local variable declared in a statement expression in the predicate is
  // declared inside C too, but a statement expression opens no DeclContext,
  // so the proxy above sees a local of the function.  While the contract is
  // being parsed or instantiated, a local variable declared after its keyword
  // can only be one of the predicate's own.  (The result name and a
  // postcondition capture follow the keyword too, and are not VarDecls of
  // this kind.)
  if (AsVar && AsVar->isLocalVarDecl() &&
      !isa<PostconditionCaptureDecl>(AsVar) && CSR->KeywordLoc.isValid() &&
      AsVar->getLocation().isValid() &&
      getSourceManager().isBeforeInTranslationUnit(CSR->KeywordLoc,
                                                   AsVar->getLocation()))
    return CC_None;

  // If there is no contract scope that encloses the current context, then we
  // don't need to constify the variable.
  if (getLastEnclosingContractScopeForContext(CurContext) == nullptr)
    return CC_None;

  // Make sure that there's a contract scope intervening between the current
  // context and the declaration of the variable. If there isn't, we don't need
  // to constify the variable.
  if (!isUsageAcrossContract(VD))
    return CC_None;

  // if the unqualified-id appears in the predicate of a contract assertion
  //  ([basic.contract]) and the entity is
  // ...

  // -- the result object of (possibly deduced, see [dcl.spec.auto]) type T of a
  // function
  //  call and the unqualified-id is the result name ([dcl.contract.res]) in a
  //  postcondition assertion,
  if (isa<ResultNameDecl>(VD))
    return CC_ApplyConst;

  // -- a structured binding of type T whose corresponding variable is declared
  //  outside of C, or
  //
  // "Declared outside of C" is what the checks above already established, so
  // there is nothing further to test here: a binding of any storage duration
  // qualifies.
  if (isa<BindingDecl>(VD))
    return CC_ApplyConst;

  // -- a template parameter of type "reference to T", which names an object
  //  declared outside of C as a reference variable does ([expr.prim.id.unqual]
  //  example, `R' in the pre of f).
  if (auto *NTTP = dyn_cast<NonTypeTemplateParmDecl>(VD))
    if (NTTP->getType()->isReferenceType() &&
        NTTP->getType().getNonReferenceType()->isObjectType())
      return CC_ApplyConst;

  // Postcondition captures are not constified (P3098 Section 4.4.1)
  if (isa<PostconditionCaptureDecl>(VD))
    return CC_None;

  // -- a variable declared outside of C ...
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
    assert(DC &&
           "the walk ends at a function before leaving the translation unit");
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
      assert((!DC->getParent() || !DC->getParent()->isFunctionOrMethod()) &&
             "a non-function context is outside every function");
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

/// Diagnoses a lambda's implicit captures that are odr-used only in contract
/// assertions (its own contracts or a contract_assert in its body), which
/// [expr.prim.lambda.capture] makes ill-formed.  Run once the lambda is
/// complete, since only then is it known whether a capture is used elsewhere.
struct LambdaCaptureChecker : RecursiveASTVisitor<LambdaCaptureChecker> {
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
    assert(FD->getBody() && "a lambda is checked once its body is parsed");
    if (FD->hasContracts())
      TraverseDecl(FD->getContracts());
    TraverseStmt(CurLambda->getBody());

    // Finally, diagnose any captures that still remain, since they do not have
    // any non-contract usages.
    for (auto &[Var, Bad] : Captures) {
      // We likely didn't see the usage because there was a intervening lambda
      // that captured by copy.
      if (Bad.UsedInContract == nullptr)
        continue;
      bool CapturesThis = Bad.Capture.capturesThis();
      auto *ND = cast_if_present<NamedDecl>(Var);
      Actions.Diag(CurLambda->getCaptureDefaultLoc(),
                   diag::err_contract_lambda_implicit_capture_only)
          << (int)CapturesThis << ND;
      SourceLocation UsageLoc = Bad.UsageLoc;
      Actions.Diag(UsageLoc, diag::note_contract_lambda_implicit_capture_usage)
          << (int)CapturesThis << ND;
      if (Bad.UsedInContract)
        Actions.Diag(Bad.UsedInContract->getBeginLoc(),
                     diag::note_contract_context);

      // Reaching here means the lambda has a capture-default (the capture is
      // implicit), so its introducer is never empty and the entity can always
      // be appended to the list as ", x" just inside the ']'.
      std::string Insertion =
          CapturesThis ? std::string(", this") : (", " + ND->getNameAsString());
      Actions.Diag(CurLambda->getIntroducerRange().getEnd(),
                   diag::note_contract_lambda_implicit_capture_explicit)
          << (int)CapturesThis << ND
          << FixItHint::CreateInsertion(
                 CurLambda->getIntroducerRange().getEnd(), Insertion);
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
    assert(CS->getCond() && "a contract has a predicate");
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
      // A capture of a VLA's type carries the array's bound, which names no
      // entity: the array itself, and any variable the bound uses, are
      // captured as variables wherever they are named.  So it takes no part
      // in the rule, here or among the captures Init collects.
      if (C.capturesVLAType())
        continue;

      assert((C.capturesThis() || C.capturesVariable()) &&
             "the remaining captures are of this or a variable");
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
  // Like the other contract hooks called from outside this file, decide here,
  // and cheaply, that there is nothing to do: without contracts there is no
  // contract assertion for a capture to be used in.
  if (!getLangOpts().Contracts)
    return;
  LambdaCaptureChecker::Check(*this, LE);
}

bool Sema::isDirectlyInContractPredicate() const {
  auto *CR = getCurrentContractEntry();
  if (!CR)
    return false;

  // A requires-expression in the predicate has a body DeclContext of its own;
  // a use inside it is still directly in the predicate.
  return CR->ContextAtPush->Equals(walkStartContext(CurContext));
}

ArrayRef<Sema::ContractScopeRecord> Sema::getContractScopes() const {
  return llvm::ArrayRef(ContractScopeStack.begin(), ContractScopeStack.end());
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

Sema::ContractScopeRAII::ContractScopeRAII(Sema &S, ContractKind CK,
                                           ContractScopeOffset ScopeOffset,
                                           SourceLocation Loc)
    : S(S) {
  S.PushContractScope(CK, ScopeOffset, Loc);
}

Sema::ContractScopeRAII::~ContractScopeRAII() { S.PopContractScope(); }

void Sema::PushContractScope(ContractKind Kind, ContractScopeOffset ScopeOffset,
                             SourceLocation Loc) {
  ContractScopeRecord Record{};
  Record.Index = static_cast<unsigned>(ContractScopeStack.size());
  Record.Kind = Kind;
  Record.ScopeOffset = ScopeOffset;
  Record.KeywordLoc = Loc;
  Record.ContextAtPush = CurContext;
  Record.PreviousCXXThisType = CXXThisTypeOverride;
  Record.FunctionIndex = static_cast<unsigned>(
      FunctionScopes.empty() ? 0ul : FunctionScopes.size() - 1);
  Record.FunctionScopeAtPush = getCurFunction();
  Record.AddedConstToCXXThis = false;
  Record.WasInContractContext = ExprEvalContexts.back().InContractAssertion;

  // Setup the constification context when building declref expressions.
  ExprEvalContexts.back().InContractAssertion = true;
  assert(CurContext && "a contract is parsed in a context");

  if (auto Pos = ContractScopeIndexMap.find(CurContext);
      Pos != ContractScopeIndexMap.end())
    Record.PrevIndexForContext = Pos->second;
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
    assert(CXXThisTypeOverride->isPointerType() && "'this' has pointer type");
    QualType ClassType = CXXThisTypeOverride->getPointeeType();
    if (!ClassType.isConstQualified() && LangOpts.ContractConstification) {
      // In the predicate `*this` is const: add the qualifier (popping the
      // scope restores PreviousCXXThisType), and record that the contract
      // added it, for the diagnostic when that const rejects a member call.
      ClassType.addConst();
      Record.AddedConstToCXXThis = true;
      CXXThisTypeOverride = Context.getPointerType(ClassType);
    }
  }

  if (Record.FunctionScopeAtPush) {
    auto *LastScope = Record.FunctionScopeAtPush;
    Record.PrevFunctionScopeContractIndex = LastScope->ContractScopeIndex;
    LastScope->ContractScopeIndex = Record.Index;
  }

  ContractScopeStack.push_back(Record);
}

ContractScopeRecord Sema::PopContractScope() {
  assert(!ContractScopeStack.empty() &&
         "a contract scope is popped only after it was pushed");

  auto Record = ContractScopeStack.back();
  ContractScopeStack.pop_back();

  assert(ContractScopeIndexMap.contains(Record.ContextAtPush) &&
         ContractScopeIndexMap[Record.ContextAtPush] == Record.Index);
  if (Record.PrevIndexForContext != unsigned(-1))
    ContractScopeIndexMap[Record.ContextAtPush] = Record.PrevIndexForContext;
  else
    ContractScopeIndexMap.erase(Record.ContextAtPush);

  assert(ExprEvalContexts.back().InContractAssertion == true &&
         "a contract scope is popped in its own evaluation context");
  ExprEvalContexts.back().InContractAssertion = Record.WasInContractContext;
  CXXThisTypeOverride = Record.PreviousCXXThisType;

  if (Record.FunctionScopeAtPush) {

    if (FunctionScopes.back() == Record.FunctionScopeAtPush) {
      assert(FunctionScopes.back()->ContractScopeIndex != unsigned(-1) &&
             "the function scope records the contract scope it pushed");
      FunctionScopes.back()->ContractScopeIndex =
          Record.PrevFunctionScopeContractIndex;
    } else {
      assert(getFunctionScopes().empty() ||
             FunctionScopes.size() < Record.FunctionIndex);
    }
  }

  return Record;
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
  assert(ContractScopeStack.size() > LastEnclosingIdx &&
         "the enclosing contract scope is on the stack");
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

ExprResult Sema::ActOnContractControlExpr(SourceLocation KwLoc, Expr *E,
                                          SourceLocation RParenLoc) {
  if (!E->isTypeDependent() && !E->isValueDependent())
    return BuildContractControlObject(KwLoc, E);

  // A dependent operand is kept as a call to __builtin_contract_control; the
  // call is rebuilt when the template is instantiated, and its builtin check
  // makes the object then.
  IdentifierInfo *II = &Context.Idents.get("__builtin_contract_control");
  LookupResult R(*this, II, KwLoc, LookupOrdinaryName);
  LookupName(R, TUScope, /*AllowBuiltinCreation=*/true);
  if (!R.isSingleResult())
    return ExprError();
  ExprResult Fn =
      BuildDeclarationNameExpr(CXXScopeSpec(), R, /*NeedsADL=*/false);
  if (Fn.isInvalid())
    return ExprError();
  // Built type-dependent, so that a merely value-dependent operand is not
  // checked (and made a prvalue) until instantiation either.
  return CallExpr::Create(Context, Fn.get(), {E}, Context.DependentTy,
                          VK_PRValue, RParenLoc, FPOptionsOverride());
}

ExprResult Sema::BuildContractControlObject(SourceLocation Loc, Expr *E) {
  // The type is deduced as for `constexpr auto v = E;'.
  ExprResult Conv = DefaultFunctionArrayLvalueConversion(E);
  if (Conv.isInvalid())
    return ExprError();
  QualType T = Conv.get()->getType().getNonReferenceType().getUnqualifiedType();
  if (!T->isRecordType() && !T->isScalarType()) {
    Diag(Loc, diag::err_contract_control_bad_type) << T << E->getSourceRange();
    return ExprError();
  }
  ExprResult Init = PerformCopyInitialization(
      InitializedEntity::InitializeTemporary(T), Loc, Conv.get());
  if (Init.isInvalid())
    return ExprError();

  // The object is made as a template parameter object is: one object for
  // each type and value.
  SmallVector<PartialDiagnosticAt, 8> Notes;
  Expr::EvalResult Eval;
  Eval.Diag = &Notes;
  if (!Init.get()->EvaluateAsConstantExpr(Eval, Context) ||
      Eval.HasSideEffects) {
    Diag(Loc, diag::err_contract_control_not_constant) << E->getSourceRange();
    for (const PartialDiagnosticAt &Note : Notes)
      Diag(Note.first, Note.second);
    return ExprError();
  }
  TemplateParamObjectDecl *Obj =
      Context.getTemplateParamObjectDecl(T, Eval.Val);
  return BuildDeclRefExpr(Obj, Obj->getType(), VK_LValue, Loc);
}

//===----------------------------------------------------------------------===//
// D4405: taking the address of a function whose preconditions must be proven
//===----------------------------------------------------------------------===//

