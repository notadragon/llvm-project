//===--- StmtCXX.cpp - Classes for representing C++ statements ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the subclesses of Stmt class declared in StmtCXX.h
//
//===----------------------------------------------------------------------===//

#include "clang/AST/StmtCXX.h"
#include "clang/AST/ExprCXX.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Lex/Preprocessor.h"
#include "llvm/ADT/StringExtras.h"

using namespace clang;

QualType CXXCatchStmt::getCaughtType() const {
  if (ExceptionDecl)
    return ExceptionDecl->getType();
  return QualType();
}

CXXTryStmt *CXXTryStmt::Create(const ASTContext &C, SourceLocation tryLoc,
                               CompoundStmt *tryBlock,
                               ArrayRef<Stmt *> handlers) {
  const size_t Size = totalSizeToAlloc<Stmt *>(handlers.size() + 1);
  void *Mem = C.Allocate(Size, alignof(CXXTryStmt));
  return new (Mem) CXXTryStmt(tryLoc, tryBlock, handlers);
}

CXXTryStmt *CXXTryStmt::Create(const ASTContext &C, EmptyShell Empty,
                               unsigned numHandlers) {
  const size_t Size = totalSizeToAlloc<Stmt *>(numHandlers + 1);
  void *Mem = C.Allocate(Size, alignof(CXXTryStmt));
  return new (Mem) CXXTryStmt(Empty, numHandlers);
}

CXXTryStmt::CXXTryStmt(SourceLocation tryLoc, CompoundStmt *tryBlock,
                       ArrayRef<Stmt *> handlers)
    : Stmt(CXXTryStmtClass), TryLoc(tryLoc), NumHandlers(handlers.size()) {
  Stmt **Stmts = getStmts();
  Stmts[0] = tryBlock;
  llvm::copy(handlers, Stmts + 1);
}

CXXForRangeStmt::CXXForRangeStmt(Stmt *Init, DeclStmt *Range,
                                 DeclStmt *BeginStmt, DeclStmt *EndStmt,
                                 Expr *Cond, Expr *Inc, DeclStmt *LoopVar,
                                 Stmt *Body, SourceLocation FL,
                                 SourceLocation CAL, SourceLocation CL,
                                 SourceLocation RPL)
    : Stmt(CXXForRangeStmtClass), ForLoc(FL), CoawaitLoc(CAL), ColonLoc(CL),
      RParenLoc(RPL) {
  SubExprs[INIT] = Init;
  SubExprs[RANGE] = Range;
  SubExprs[BEGINSTMT] = BeginStmt;
  SubExprs[ENDSTMT] = EndStmt;
  SubExprs[COND] = Cond;
  SubExprs[INC] = Inc;
  SubExprs[LOOPVAR] = LoopVar;
  SubExprs[BODY] = Body;
}

Expr *CXXForRangeStmt::getRangeInit() {
  DeclStmt *RangeStmt = getRangeStmt();
  VarDecl *RangeDecl = dyn_cast_or_null<VarDecl>(RangeStmt->getSingleDecl());
  assert(RangeDecl && "for-range should have a single var decl");
  return RangeDecl->getInit();
}

const Expr *CXXForRangeStmt::getRangeInit() const {
  return const_cast<CXXForRangeStmt *>(this)->getRangeInit();
}

VarDecl *CXXForRangeStmt::getLoopVariable() {
  Decl *LV = cast<DeclStmt>(getLoopVarStmt())->getSingleDecl();
  assert(LV && "No loop variable in CXXForRangeStmt");
  return cast<VarDecl>(LV);
}

const VarDecl *CXXForRangeStmt::getLoopVariable() const {
  return const_cast<CXXForRangeStmt *>(this)->getLoopVariable();
}

CoroutineBodyStmt *CoroutineBodyStmt::Create(
    const ASTContext &C, CoroutineBodyStmt::CtorArgs const &Args) {
  std::size_t Size = totalSizeToAlloc<Stmt *>(
      CoroutineBodyStmt::FirstParamMove + Args.ParamMoves.size());

  void *Mem = C.Allocate(Size, alignof(CoroutineBodyStmt));
  return new (Mem) CoroutineBodyStmt(Args);
}

CoroutineBodyStmt *CoroutineBodyStmt::Create(const ASTContext &C, EmptyShell,
                                             unsigned NumParams) {
  std::size_t Size = totalSizeToAlloc<Stmt *>(
      CoroutineBodyStmt::FirstParamMove + NumParams);

  void *Mem = C.Allocate(Size, alignof(CoroutineBodyStmt));
  auto *Result = new (Mem) CoroutineBodyStmt(CtorArgs());
  Result->NumParams = NumParams;
  auto *ParamBegin = Result->getStoredStmts() + SubStmt::FirstParamMove;
  std::uninitialized_fill(ParamBegin, ParamBegin + NumParams,
                          static_cast<Stmt *>(nullptr));
  return Result;
}

CoroutineBodyStmt::CoroutineBodyStmt(CoroutineBodyStmt::CtorArgs const &Args)
    : Stmt(CoroutineBodyStmtClass), NumParams(Args.ParamMoves.size()) {
  Stmt **SubStmts = getStoredStmts();
  SubStmts[CoroutineBodyStmt::Body] = Args.Body;
  SubStmts[CoroutineBodyStmt::Promise] = Args.Promise;
  SubStmts[CoroutineBodyStmt::InitSuspend] = Args.InitialSuspend;
  SubStmts[CoroutineBodyStmt::FinalSuspend] = Args.FinalSuspend;
  SubStmts[CoroutineBodyStmt::OnException] = Args.OnException;
  SubStmts[CoroutineBodyStmt::OnFallthrough] = Args.OnFallthrough;
  SubStmts[CoroutineBodyStmt::Allocate] = Args.Allocate;
  SubStmts[CoroutineBodyStmt::Deallocate] = Args.Deallocate;
  SubStmts[CoroutineBodyStmt::ResultDecl] = Args.ResultDecl;
  SubStmts[CoroutineBodyStmt::ReturnValue] = Args.ReturnValue;
  SubStmts[CoroutineBodyStmt::ReturnStmt] = Args.ReturnStmt;
  SubStmts[CoroutineBodyStmt::ReturnStmtOnAllocFailure] =
      Args.ReturnStmtOnAllocFailure;
  llvm::copy(Args.ParamMoves, const_cast<Stmt **>(getParamMoves().data()));
}

CXXExpansionStmtPattern::CXXExpansionStmtPattern(ExpansionStmtKind PatternKind,
                                                 EmptyShell Empty)
    : Stmt(CXXExpansionStmtPatternClass, Empty), PatternKind(PatternKind) {}

CXXExpansionStmtPattern::CXXExpansionStmtPattern(
    ExpansionStmtKind PatternKind, CXXExpansionStmtDecl *ESD, Stmt *Init,
    DeclStmt *ExpansionVar, SourceLocation LParenLoc, SourceLocation ColonLoc,
    SourceLocation RParenLoc)
    : Stmt(CXXExpansionStmtPatternClass), PatternKind(PatternKind),
      LParenLoc(LParenLoc), ColonLoc(ColonLoc), RParenLoc(RParenLoc),
      ParentDecl(ESD) {
  setInit(Init);
  setExpansionVarStmt(ExpansionVar);
  setBody(nullptr);
}

template <typename... Args>
CXXExpansionStmtPattern *CXXExpansionStmtPattern::AllocateAndConstruct(
    ASTContext &Context, ExpansionStmtKind Kind, Args &&...Arguments) {
  std::size_t Size = totalSizeToAlloc<Stmt *>(getNumSubStmts(Kind));
  void *Mem = Context.Allocate(Size, alignof(CXXExpansionStmtPattern));
  return new (Mem)
      CXXExpansionStmtPattern(Kind, std::forward<Args>(Arguments)...);
}

CXXExpansionStmtPattern *CXXExpansionStmtPattern::CreateDependent(
    ASTContext &Context, CXXExpansionStmtDecl *ESD, Stmt *Init,
    DeclStmt *ExpansionVar, Expr *ExpansionInitializer,
    SourceLocation LParenLoc, SourceLocation ColonLoc,
    SourceLocation RParenLoc) {
  CXXExpansionStmtPattern *Pattern =
      AllocateAndConstruct(Context, ExpansionStmtKind::Dependent, ESD, Init,
                           ExpansionVar, LParenLoc, ColonLoc, RParenLoc);
  Pattern->setExpansionInitializer(ExpansionInitializer);
  return Pattern;
}

CXXExpansionStmtPattern *CXXExpansionStmtPattern::CreateDestructuring(
    ASTContext &Context, CXXExpansionStmtDecl *ESD, Stmt *Init,
    DeclStmt *ExpansionVar, Stmt *DecompositionDeclStmt,
    SourceLocation LParenLoc, SourceLocation ColonLoc,
    SourceLocation RParenLoc) {
  CXXExpansionStmtPattern *Pattern =
      AllocateAndConstruct(Context, ExpansionStmtKind::Destructuring, ESD, Init,
                           ExpansionVar, LParenLoc, ColonLoc, RParenLoc);
  Pattern->setDecompositionDeclStmt(DecompositionDeclStmt);
  return Pattern;
}

CXXExpansionStmtPattern *
CXXExpansionStmtPattern::CreateEmpty(ASTContext &Context, EmptyShell Empty,
                                     ExpansionStmtKind Kind) {
  return AllocateAndConstruct(Context, Kind, Empty);
}

CXXExpansionStmtPattern *CXXExpansionStmtPattern::CreateEnumerating(
    ASTContext &Context, CXXExpansionStmtDecl *ESD, Stmt *Init,
    DeclStmt *ExpansionVar, SourceLocation LParenLoc, SourceLocation ColonLoc,
    SourceLocation RParenLoc) {
  return AllocateAndConstruct(Context, ExpansionStmtKind::Enumerating, ESD,
                              Init, ExpansionVar, LParenLoc, ColonLoc,
                              RParenLoc);
}

CXXExpansionStmtPattern *CXXExpansionStmtPattern::CreateIterating(
    ASTContext &Context, CXXExpansionStmtDecl *ESD, Stmt *Init,
    DeclStmt *ExpansionVar, DeclStmt *Range, DeclStmt *Begin, DeclStmt *Iter,
    SourceLocation LParenLoc, SourceLocation ColonLoc,
    SourceLocation RParenLoc) {
  CXXExpansionStmtPattern *Pattern =
      AllocateAndConstruct(Context, ExpansionStmtKind::Iterating, ESD, Init,
                           ExpansionVar, LParenLoc, ColonLoc, RParenLoc);
  Pattern->setRangeVarStmt(Range);
  Pattern->setBeginVarStmt(Begin);
  Pattern->setIterVarStmt(Iter);
  return Pattern;
}

SourceLocation CXXExpansionStmtPattern::getBeginLoc() const {
  return ParentDecl->getLocation();
}

DecompositionDecl *CXXExpansionStmtPattern::getDecompositionDecl() {
  assert(isDestructuring());
  return cast<DecompositionDecl>(
      cast<DeclStmt>(getDecompositionDeclStmt())->getSingleDecl());
}

VarDecl *CXXExpansionStmtPattern::getExpansionVariable() {
  Decl *LV = cast<DeclStmt>(getExpansionVarStmt())->getSingleDecl();
  assert(LV && "No expansion variable in CXXExpansionStmtPattern");
  return cast<VarDecl>(LV);
}

unsigned
CXXExpansionStmtPattern::getNumSubStmts(ExpansionStmtKind PatternKind) {
  switch (PatternKind) {
  case ExpansionStmtKind::Enumerating:
    return COUNT_Enumerating;
  case ExpansionStmtKind::Iterating:
    return COUNT_Iterating;
  case ExpansionStmtKind::Destructuring:
    return COUNT_Destructuring;
  case ExpansionStmtKind::Dependent:
    return COUNT_Dependent;
  }

  llvm_unreachable("invalid pattern kind");
}

CXXExpansionStmtInstantiation::CXXExpansionStmtInstantiation(
    EmptyShell Empty, unsigned NumInstantiations, unsigned NumPreambleStmts)
    : Stmt(CXXExpansionStmtInstantiationClass, Empty),
      NumInstantiations(NumInstantiations), NumPreambleStmts(NumPreambleStmts) {
  assert(NumPreambleStmts <= 4 && "might have to allocate more bits for this");
}

CXXExpansionStmtInstantiation::CXXExpansionStmtInstantiation(
    CXXExpansionStmtDecl *Parent, ArrayRef<Stmt *> Instantiations,
    ArrayRef<Stmt *> PreambleStmts, bool ShouldApplyLifetimeExtensionToPreamble)
    : Stmt(CXXExpansionStmtInstantiationClass), Parent(Parent),
      NumInstantiations(unsigned(Instantiations.size())),
      NumPreambleStmts(unsigned(PreambleStmts.size())),
      ShouldApplyLifetimeExtensionToPreamble(
          ShouldApplyLifetimeExtensionToPreamble) {
  assert(NumPreambleStmts <= 4 && "might have to allocate more bits for this");
  llvm::uninitialized_copy(Instantiations, getTrailingObjects());
  llvm::uninitialized_copy(PreambleStmts,
                           getTrailingObjects() + NumInstantiations);
}

CXXExpansionStmtInstantiation *CXXExpansionStmtInstantiation::Create(
    ASTContext &C, CXXExpansionStmtDecl *Parent,
    ArrayRef<Stmt *> Instantiations, ArrayRef<Stmt *> PreambleStmts,
    bool ShouldApplyLifetimeExtensionToPreamble) {
  void *Mem = C.Allocate(
      totalSizeToAlloc<Stmt *>(Instantiations.size() + PreambleStmts.size()),
      alignof(CXXExpansionStmtInstantiation));
  return new (Mem)
      CXXExpansionStmtInstantiation(Parent, Instantiations, PreambleStmts,
                                    ShouldApplyLifetimeExtensionToPreamble);
}

CXXExpansionStmtInstantiation *
CXXExpansionStmtInstantiation::CreateEmpty(ASTContext &C, EmptyShell Empty,
                                           unsigned NumInstantiations,
                                           unsigned NumPreambleStmts) {
  void *Mem =
      C.Allocate(totalSizeToAlloc<Stmt *>(NumInstantiations + NumPreambleStmts),
                 alignof(CXXExpansionStmtInstantiation));
  return new (Mem)
      CXXExpansionStmtInstantiation(Empty, NumInstantiations, NumPreambleStmts);
}

SourceLocation CXXExpansionStmtInstantiation::getBeginLoc() const {
  return Parent->getExpansionPattern()->getBeginLoc();
}

SourceLocation CXXExpansionStmtInstantiation::getEndLoc() const {
  return Parent->getExpansionPattern()->getEndLoc();
}

ContractStmt *ContractStmt::CreateEmpty(const ASTContext &C, ContractKind Kind,
                                        bool HasResultName, bool HasMessage,
                                        bool HasLabel, bool HasCaptures,
                                        bool HasRequiresClause,
                                        unsigned NumAttrs) {
  void *Mem = C.Allocate(
      totalSizeToAlloc<Stmt *, const Attr *>(
          1 + HasResultName + HasMessage + HasLabel + HasCaptures +
              HasRequiresClause,
          NumAttrs),
      alignof(ContractStmt));
  return new (Mem)
      ContractStmt(EmptyShell(), Kind, HasResultName, HasMessage, HasLabel,
                   HasCaptures, HasRequiresClause, NumAttrs);
}

ContractStmt *ContractStmt::Create(const ASTContext &C, ContractKind Kind,
                                   SourceLocation KeywordLoc, Expr *Condition,
                                   DeclStmt *ResultNameDecl, Expr *Message,
                                   Expr *Label, DeclStmt *Captures,
                                   ArrayRef<const Attr *> Attrs,
                                   Expr *RequiresClause) {
  assert((ResultNameDecl == nullptr || Kind == ContractKind::Post) &&
         "Only a postcondition can have a result name declaration");
  void *Mem = C.Allocate(
      totalSizeToAlloc<Stmt *, const Attr *>(
          1 + (ResultNameDecl != nullptr) + (Message != nullptr) +
              (Label != nullptr) + (Captures != nullptr) +
              (RequiresClause != nullptr),
          Attrs.size()),
      alignof(ContractStmt));
  auto *CS = new (Mem)
      ContractStmt(Kind, KeywordLoc, Condition, ResultNameDecl, Message, Label,
                   Captures, Attrs, RequiresClause != nullptr);
  if (RequiresClause)
    CS->setRequiresClause(RequiresClause);
  return CS;
}

ResultNameDecl *ContractStmt::getResultName() const {
  if (!hasResultName())
    return nullptr;
  DeclStmt *D = getResultNameDeclStmt();
  assert(D);
  return cast<ResultNameDecl>(D->getSingleDecl());
}

std::string ContractStmt::getComment(const ASTContext &Ctx) const {
  if (hasTransformedComment())
    return TransformedComment.str();
  return getSourceText(Ctx);
}

std::string ContractStmt::getUserMessage(const ASTContext &Ctx) const {
  if (hasTransformedMessage())
    return TransformedMessage.str();
  if (hasMessage()) {
    if (auto *SL = dyn_cast<StringLiteral>(getMessageExpr()))
      return SL->getString().str();
  }
  if (auto *A = getAttrAs<ContractMessageAttr>())
    return A->getMessage().str();
  return {};
}

std::string ContractStmt::getMessage(const clang::ASTContext &Ctx) const {
  std::string Msg = getUserMessage(Ctx);
  if (!Msg.empty())
    return Msg;
  return getSourceText(Ctx);
}

std::string ContractStmt::getSourceText(const ASTContext &Ctx) const {
  auto &SM = Ctx.getSourceManager();
  auto Begin = hasResultName() ? getResultName()->getBeginLoc()
                               : getCond()->getBeginLoc();
  auto End = getCond()->getEndLoc();
  CharSourceRange ExprRange = Lexer::getAsCharRange(
      SM.getExpansionRange(SourceRange(Begin, End)), SM, Ctx.getLangOpts());
  std::string AssertStr =
      Lexer::getSourceText(ExprRange, SM, Ctx.getLangOpts()).str();
  return AssertStr;
}

StringRef ContractStmt::ContractKindAsString(ContractKind K) {
  switch (K) {
  case ContractKind::Assert:
    return "contract_assert";
  case ContractKind::Pre:
    return "pre";
  case ContractKind::Post:
    return "post";
  case ContractKind::Implicit:
    return "implicit";
  }
  llvm_unreachable("Unknown contract kind");
}

StringRef ContractStmt::SemanticAsString(ContractEvaluationSemantic Sem) {
  switch (Sem) {
  case ContractEvaluationSemantic::Ignore:
    return "ignore";
  case ContractEvaluationSemantic::Observe:
    return "observe";
  case ContractEvaluationSemantic::Enforce:
    return "enforce";
  case ContractEvaluationSemantic::QuickEnforce:
    return "quick_enforce";
  case ContractEvaluationSemantic::Assume:
    return "assume";
  case ContractEvaluationSemantic::NoexceptObserve:
    return "noexcept_observe";
  case ContractEvaluationSemantic::NoexceptEnforce:
    return "noexcept_enforce";
  }
  llvm_unreachable("Unknown contract evaluation semantic");
}

// P3100: apply the -fcontracts-allow-assume gate to a contract's
// (flag-independent) allowed-semantics restriction by intersecting it with the
// gated base set.  When the flag is off, "assume" is not in the gated base, so
// it cannot be present in the result -- a label cannot re-add it.  This runs at
// query construction, so it applies uniformly to every contract, including
// template instantiations.
static unsigned applyAssumeGate(unsigned Mask, const ASTContext &Ctx) {
  return Mask &
         gatedContractSemanticsMask(Ctx.getLangOpts().ContractOpts.AllowAssume,
                                    Ctx.getLangOpts().ContractsP4298);
}

ContractEvaluationSemantic
ContractStmt::getSemantic(const ASTContext &Ctx) const {
  if (hasTransformedSemantic())
    return getTransformedSemantic();
  ContractQuery Q;
  Q.Kind = getContractKind();
  Q.CallerSide = false;
  Q.AllowedMask = applyAssumeGate(getAllowedMask(), Ctx);
  return Ctx.getLangOpts().ContractOpts.resolveContractSemantic(Q);
}

ContractEvaluationSemantic
ContractStmt::ensureRuntimeSemantic(const ASTContext &Ctx,
                                    const DeclContext *FnCtx) const {
  if (CachedRuntimeSemantic_ != 0)
    return static_cast<ContractEvaluationSemantic>(CachedRuntimeSemantic_);

  ContractQuery Q;
  Q.Kind = getContractKind();
  Q.CallerSide = false;
  Q.InConstantEvaluation = false;
  Q.AllowedMask = applyAssumeGate(getAllowedMask(), Ctx);
  llvm::SmallVector<std::string, 1> GroupsVec;
  if (auto *A = getAttrAs<ContractGroupAttr>())
    GroupsVec.push_back(A->getGroup().str());
  Q.Groups = GroupsVec;
  Q.FnContext = FnCtx;
  Q.Loc = getKeywordLoc();
  Q.SM = &Ctx.getSourceManager();

  auto Sem = Ctx.getLangOpts().ContractOpts.resolveContractSemantic(Q);
  CachedRuntimeSemantic_ = static_cast<uint8_t>(Sem);
  return Sem;
}

ContractEvaluationSemantic
ContractStmt::ensureCESemantic(const ASTContext &Ctx,
                               const DeclContext *FnCtx) const {
  if (CachedCESemantic_ != 0)
    return static_cast<ContractEvaluationSemantic>(CachedCESemantic_);

  ContractQuery Q;
  Q.Kind = getContractKind();
  Q.CallerSide = false;
  Q.InConstantEvaluation = true;
  Q.AllowedMask = applyAssumeGate(getAllowedMask(), Ctx);
  llvm::SmallVector<std::string, 1> GroupsVec;
  if (auto *A = getAttrAs<ContractGroupAttr>())
    GroupsVec.push_back(A->getGroup().str());
  Q.Groups = GroupsVec;
  Q.FnContext = FnCtx;
  Q.Loc = getKeywordLoc();
  Q.SM = &Ctx.getSourceManager();

  auto Sem = Ctx.getLangOpts().ContractOpts.resolveContractSemantic(Q);
  CachedCESemantic_ = static_cast<uint8_t>(Sem);
  return Sem;
}

ContractEvaluationSemantic
ContractStmt::ensureCallerSemantic(const ASTContext &Ctx,
                                   const DeclContext *FnCtx) const {
  if (CachedCallerSemantic_ != 0)
    return static_cast<ContractEvaluationSemantic>(CachedCallerSemantic_);

  ContractQuery Q;
  Q.Kind = getContractKind();
  Q.CallerSide = true;
  Q.InConstantEvaluation = false;
  Q.AllowedMask = applyAssumeGate(
      getAllowedMask() |
          (1u << static_cast<unsigned>(ContractEvaluationSemantic::Ignore)),
      Ctx);
  llvm::SmallVector<std::string, 1> GroupsVec;
  if (auto *A = getAttrAs<ContractGroupAttr>())
    GroupsVec.push_back(A->getGroup().str());
  Q.Groups = GroupsVec;
  Q.FnContext = FnCtx;
  Q.Loc = getKeywordLoc();
  Q.SM = &Ctx.getSourceManager();

  auto Sem = Ctx.getLangOpts().ContractOpts.resolveContractSemantic(Q);
  CachedCallerSemantic_ = static_cast<uint8_t>(Sem);
  return Sem;
}
