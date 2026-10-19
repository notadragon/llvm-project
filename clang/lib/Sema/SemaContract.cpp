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

