//===--- ParseContracts.cpp - C++ Contracts Parsing -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements parsing for C++ contracts (pre, post, and
//  contract_assert), including P3400 labels, P3098 postcondition captures, and
//  P4283 requires-clauses.
//
//===----------------------------------------------------------------------===//

#include "clang/Parse/Parser.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/PrettyDeclStackTrace.h"
#include "clang/AST/StmtCXX.h"
#include "clang/Basic/CharInfo.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TokenKinds.h"
#include "clang/Lex/LiteralSupport.h"
#include "clang/Parse/RAIIObjectsForParser.h"
#include "clang/Sema/DeclSpec.h"
#include "clang/Sema/EnterExpressionEvaluationContext.h"
#include "clang/Sema/Scope.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/TimeProfiler.h"
#include <optional>

using namespace clang;

std::optional<ContractKind>
Parser::getContractKeyword(const Token &Token) const {
  // C contracts (D4299): keyword tokens
  if (Token.is(tok::kw__Pre))
    return ContractKind::Pre;
  if (Token.is(tok::kw__Post))
    return ContractKind::Post;
  if (Token.is(tok::kw__ContractAssert))
    return ContractKind::Assert;

  // We offer the reserved keywords as identifiers in C++11 mode.
  if (!getLangOpts().CPlusPlus11 ||
      (Token.isNot(tok::identifier) && !Token.is(tok::kw_contract_assert)))
    return std::nullopt;

  // If we have a contract_assert keyword, we've may be using contract as an
  // extension, so don't check the language options.
  if (Token.is(tok::kw_contract_assert))
    return ContractKind::Assert;

  const IdentifierInfo *II = Token.getIdentifierInfo();
  assert(II && "Missing identifier info");

  if (!Ident_pre) {
    Ident_pre = &PP.getIdentifierTable().get("pre");
    Ident___pre = &PP.getIdentifierTable().get("__pre");

    Ident_post = &PP.getIdentifierTable().get("post");
    Ident___post = &PP.getIdentifierTable().get("__post");
  }

  if ((II == Ident_pre && getLangOpts().Contracts) || II == Ident___pre)
    return ContractKind::Pre;

  if ((II == Ident_post && getLangOpts().Contracts) || II == Ident___post)
    return ContractKind::Post;

  return std::nullopt;
}

void Parser::LateParseFunctionContractSpecifierSeq(CachedTokens &Toks) {
  while (isFunctionContractKeyword(Tok)) {
    if (!LateParseFunctionContractSpecifier(Toks)) {
      return;
    }
  }
}

static const char *getContractKeywordStr(ContractKind CK) {
  switch (CK) {
  case ContractKind::Pre:
    return "pre";
  case ContractKind::Post:
    return "post";
  case ContractKind::Assert:
    return "contract_assert";
  case ContractKind::Implicit:
    return "implicit";
  }
  llvm_unreachable("unhandled case");
}

bool Parser::LateParseFunctionContractSpecifier(CachedTokens &Toks) {
  assert(isFunctionContractKeyword(Tok) && "Not in a contract");
  ContractKind CK = getContractKeyword(Tok).value();
  const char *CKStr = getContractKeywordStr(CK);

  // Consume and cache the starting token.
  Token StartTok = Tok;
  SourceRange ContractRange = SourceRange(ConsumeToken());

  // P3400: If there's a '<', cache the label expression tokens before '('.
  // Cache them even when P3400 is off, so the re-parse can diagnose the
  // missing flag.  Skipping the label here instead would drop it silently:
  // the cached stream would start at the predicate and a late-parsed (member
  // function) contract would compile as if no label had been written, quietly
  // using the default semantic rather than the one the label selects.
  if (Tok.is(tok::less)) {
    Toks.push_back(StartTok);
    Toks.push_back(Tok);
    ConsumeToken(); // '<'
    // Cache everything up to and including the matching '>'.
    // Use ConsumeAndStoreUntil with a depth counter for nested <>.
    unsigned Depth = 1;
    while (Depth > 0) {
      if (Tok.is(tok::less))
        ++Depth;
      else if (Tok.is(tok::greater))
        --Depth;
      else if (Tok.is(tok::greatergreater) && Depth >= 2) {
        Depth -= 2;
        // Split >> into > > for caching.
        Token GT;
        GT.startToken();
        GT.setKind(tok::greater);
        GT.setLocation(Tok.getLocation());
        Toks.push_back(GT);
        GT.setLocation(Tok.getLocation().getLocWithOffset(1));
        Toks.push_back(GT);
        ConsumeToken();
        continue;
      } else if (Tok.is(tok::eof) || Tok.is(tok::semi)) {
        Diag(Tok, diag::err_expected) << tok::greater;
        return false;
      }
      Toks.push_back(Tok);
      ConsumeToken();
    }
    // P4283: cache an optional requires-clause between the label and the
    // attributes/captures/predicate.
    if (Tok.is(tok::kw_requires) && !LateParseContractRequiresClause(Toks))
      return false;
    // Cache attribute tokens [[...]] after label.
    while (Tok.is(tok::l_square) && NextToken().is(tok::l_square)) {
      Toks.push_back(Tok);
      ConsumeBracket();
      ConsumeAndStoreUntil(tok::r_square, Toks,
                           /*StopAtSemi=*/true,
                           /*ConsumeFinalToken=*/true);
      if (Tok.is(tok::r_square)) {
        Toks.push_back(Tok);
        ConsumeBracket();
      }
    }

    // Cache capture tokens [...] if present.
    if (Tok.is(tok::l_square)) {
      Toks.push_back(Tok);
      ConsumeBracket();
      ConsumeAndStoreUntil(tok::r_square, Toks,
                           /*StopAtSemi=*/true,
                           /*ConsumeFinalToken=*/true);
    }

    // Now expect '('.
    if (!Tok.is(tok::l_paren)) {
      Diag(Tok, diag::err_expected_lparen_after) << CKStr;
      return false;
    }
    Toks.push_back(Tok);
    ContractRange.setEnd(ConsumeParen());
    ConsumeAndStoreUntil(tok::r_paren, Toks,
                         /*StopAtSemi=*/true,
                         /*ConsumeFinalToken=*/true);
    ContractRange.setEnd(Toks.back().getLocation());
    return true;
  }

  // Cache any [[attribute]] tokens before captures/paren.
  Toks.push_back(StartTok); // contract keyword

  // P4283: cache an optional requires-clause (no label) before the
  // attributes/captures/predicate.
  if (Tok.is(tok::kw_requires) && !LateParseContractRequiresClause(Toks))
    return false;

  // Cache attribute tokens [[...]].
  while (Tok.is(tok::l_square) && NextToken().is(tok::l_square)) {
    Toks.push_back(Tok);
    ConsumeBracket();
    ConsumeAndStoreUntil(tok::r_square, Toks,
                         /*StopAtSemi=*/true,
                         /*ConsumeFinalToken=*/true);
    if (Tok.is(tok::r_square)) {
      Toks.push_back(Tok);
      ConsumeBracket();
    }
  }

  // Cache capture tokens [...] if present.
  if (Tok.is(tok::l_square)) {
    Toks.push_back(Tok);
    ConsumeBracket();
    ConsumeAndStoreUntil(tok::r_square, Toks,
                         /*StopAtSemi=*/true,
                         /*ConsumeFinalToken=*/true);
  }

  // Check for a '('.
  if (!Tok.is(tok::l_paren)) {
    Diag(Tok, diag::err_expected_lparen_after) << CKStr;
    return false;
  }

  Toks.push_back(Tok);                  // '('
  ContractRange.setEnd(ConsumeParen()); // '('

  ConsumeAndStoreUntil(tok::r_paren, Toks,
                       /*StopAtSemi=*/true,
                       /*ConsumeFinalToken=*/true);
  ContractRange.setEnd(Toks.back().getLocation());
  return true;
}

/// LateParseContractRequiresClause - Cache the tokens of a P4283 requires-
/// clause on a member function's contract so they can be re-parsed later along
/// with the rest of the contract specifier.
///
/// The grammar is
///   requires constraint-logical-or-expression
/// i.e. a sequence of primary-expressions joined by top-level '&&' / '||' with
/// no mandatory parentheses.  We cache tokens while tracking bracket nesting;
/// the clause ends at the first depth-0 predicate '(' or capture/attribute '['
/// unless that token opens a parenthesized primary or a requires-expression's
/// parameter list.  Returns false on error; on success Tok is left at the
/// attribute/capture/predicate that follows the clause.
bool Parser::LateParseContractRequiresClause(CachedTokens &Toks) {
  assert(Tok.is(tok::kw_requires) && "Not a requires-clause");

  // Cache a depth-counted template argument list '<...>', modelled on the
  // P3400 label caching above (splitting a '>>' that closes two levels).
  // Assumes Tok is '<'.
  auto cacheAngles = [&]() -> bool {
    Toks.push_back(Tok);
    ConsumeToken(); // '<'
    unsigned Depth = 1;
    while (Depth > 0) {
      if (Tok.is(tok::less)) {
        ++Depth;
      } else if (Tok.is(tok::greater)) {
        --Depth;
      } else if (Tok.is(tok::greatergreater) && Depth >= 2) {
        Depth -= 2;
        Token GT;
        GT.startToken();
        GT.setKind(tok::greater);
        GT.setLocation(Tok.getLocation());
        Toks.push_back(GT);
        GT.setLocation(Tok.getLocation().getLocWithOffset(1));
        Toks.push_back(GT);
        ConsumeToken();
        continue;
      } else if (Tok.is(tok::eof) || Tok.is(tok::semi)) {
        Diag(Tok, diag::err_expected) << tok::greater;
        return false;
      }
      Toks.push_back(Tok);
      ConsumeToken();
    }
    return true;
  };

  // Cache a balanced '(...)' / '[...]' / '{...}' group.  Assumes Tok is the
  // opener; ConsumeAndStoreUntil handles inner nesting.
  auto cacheBalanced = [&](tok::TokenKind Close) {
    Toks.push_back(Tok);
    if (Close == tok::r_paren)
      ConsumeParen();
    else if (Close == tok::r_brace)
      ConsumeBrace();
    else
      ConsumeBracket();
    ConsumeAndStoreUntil(Close, Toks, /*StopAtSemi=*/false,
                         /*ConsumeFinalToken=*/true);
  };

  // Cache a single constraint primary-expression.
  auto cachePrimary = [&]() -> bool {
    // requires-expression: 'requires' requirement-parameter-list[opt]
    // requirement-body.
    if (Tok.is(tok::kw_requires)) {
      Toks.push_back(Tok);
      ConsumeToken(); // 'requires'
      if (Tok.is(tok::l_paren))
        cacheBalanced(tok::r_paren);
      if (Tok.is(tok::l_brace)) {
        cacheBalanced(tok::r_brace);
        return true;
      }
      Diag(Tok, diag::err_expected) << tok::l_brace;
      return false;
    }
    // Parenthesized primary: '(' expression ')'.
    if (Tok.is(tok::l_paren)) {
      cacheBalanced(tok::r_paren);
      return true;
    }
    // id-expression / literal, possibly with a nested-name-specifier and a
    // template argument list.  Cache tokens until a depth-0 terminator.
    bool CachedAny = false;
    while (true) {
      if (Tok.is(tok::less)) {
        if (!cacheAngles())
          return false;
        CachedAny = true;
        continue;
      }
      // A depth-0 connector, the predicate '(', the captures/attributes '[',
      // or an end marker terminates the primary.
      if (Tok.isOneOf(tok::ampamp, tok::pipepipe, tok::l_paren, tok::l_square,
                      tok::l_brace, tok::comma, tok::semi, tok::eof,
                      tok::r_paren, tok::r_square, tok::r_brace))
        break;
      Toks.push_back(Tok);
      ConsumeToken();
      CachedAny = true;
    }
    if (!CachedAny) {
      Diag(Tok, diag::err_expected_expression);
      return false;
    }
    return true;
  };

  // Cache the clause keyword and its constraint-logical-or-expression.
  Toks.push_back(Tok);
  ConsumeToken(); // 'requires'
  while (true) {
    if (!cachePrimary())
      return false;
    if (Tok.is(tok::ampamp) || Tok.is(tok::pipepipe)) {
      Toks.push_back(Tok);
      ConsumeToken();
      continue;
    }
    break;
  }
  return true;
}

/// ParseContractAssertStatement
///
///  assertion-statement:
///     'contract_assert' attribute-specifier-seq[opt] '('
///     conditional-expression ')' ';'
///
StmtResult Parser::ParseContractAssertStatement() {
  assert((Tok.is(tok::kw_contract_assert) || Tok.is(tok::kw__ContractAssert)) &&
         "Not a contract assert statement");
  bool IsInvalidTmp = false;
  return ParseFunctionContractSpecifierImpl(
      {}, ContractScopeOffset::FunctionContext, IsInvalidTmp);
}

/// ParseFunctionContractSpecifierSeq - Parse a series of pre/post contracts on
/// a function declaration.
///
///   function-contract-specifier-seq :
///       function-contract-specifier function-contract-specifier-seq
///
///   function-contract-specifier:
///       precondition-specifier
///       postcondition-specifier
///
///   precondition-specifier:
///       pre attribute-specifier-seq[opt] ( conditional-expression )
///
///   postcondition-specifier:
///       post attribute-specifier-seq[opt] ( result-name-introducer[opt]
///       conditional-expression )
///
///   result-name-introducer:
///       attributed-identifier :
void Parser::ParseContractSpecifierSequence(Declarator &DeclarationInfo,
                                            bool EnterScope,
                                            QualType TrailingReturnType) {
  if (!isFunctionContractKeyword(Tok))
    return;

  std::optional<QualType> CachedType;
  auto ReturnTypeResolver = [&]() {
    if (!CachedType) {
      QualType ReturnType = TrailingReturnType;
      if (ReturnType.isNull()) {
        TypeSourceInfo *TInfo = Actions.GetTypeForDeclarator(DeclarationInfo);
        assert(TInfo && TInfo->getType()->isFunctionType());
        ReturnType = TInfo->getType()->getAs<FunctionType>()->getReturnType();
      }
      CachedType = ReturnType;
    }
    return CachedType.value();
  };
  std::optional<ParseScope> ParserScope;

  std::optional<Sema::CXXThisScopeRAII> ThisScope;
  std::optional<Sema::FunctionScopeRAII> PopFnContext;

  if (EnterScope) {
    ParserScope.emplace(this, Scope::DeclScope | Scope::FunctionPrototypeScope |
                                  Scope::FunctionDeclarationScope);

    auto FTI = DeclarationInfo.getFunctionTypeInfo();

    for (unsigned i = 0; i != FTI.NumParams; ++i) {
      ParmVarDecl *Param = cast<ParmVarDecl>(FTI.Params[i].Param);
      Actions.ActOnReenterCXXMethodParameter(getCurScope(), Param);
    }
  }

  InitCXXThisScopeForDeclaratorIfRelevant(
      DeclarationInfo, DeclarationInfo.getDeclSpec(), ThisScope);
  bool IsInvalid = false;
  SourceLocation StartLoc = Tok.getLocation();

  SmallVector<ContractStmt *, 4> Contracts;
  while (isFunctionContractKeyword(Tok)) {
    bool IsInvalidTmp = false;
    StmtResult Contract = ParseFunctionContractSpecifierImpl(
        ReturnTypeResolver,
        EnterScope ? ContractScopeOffset::ParentContext
                   : ContractScopeOffset::FunctionContext,
        IsInvalidTmp);
    IsInvalid |= IsInvalidTmp;
    if (Contract.isUsable())
      Contracts.push_back(Contract.getAs<ContractStmt>());
  }
  ContractSpecifierDecl *Seq = Actions.ActOnFinishContractSpecifierSequence(
      Contracts, StartLoc, IsInvalid);

  assert(DeclarationInfo.Contracts == nullptr && "Already have contracts?");

  DeclarationInfo.Contracts = Seq;
}

StmtResult Parser::ParseFunctionContractSpecifierImpl(
    llvm::function_ref<QualType()> ReturnTypeResolver,
    ContractScopeOffset ScopeOffset, bool &IsInvalid) {
  assert(isAnyContractKeyword(Tok) && "Not a contract keyword?");
  ContractKind CK = getContractKeyword(Tok).value();
  assert((CK == ContractKind::Assert || ReturnTypeResolver) &&
         "Missing return type resolver for function contract sequence");
  assert((ScopeOffset == ContractScopeOffset::FunctionContext ||
          CK != ContractKind::Assert) &&
         "Incorrect scope offset for contract assert");
  auto SetInvalidOnExit = llvm::scope_exit([&]() { IsInvalid = true; });

  const char *CKStr = getContractKeywordStr(CK);

  SourceLocation KeywordLoc = Tok.getLocation();
  ConsumeToken();

  ExprResult LabelExpr;
  // Recognise the label syntax even when P3400 is off, so we can diagnose the
  // missing flag and still recover to the predicate.  Bailing out on the '<'
  // instead would derail the whole declaration and bury the real problem under
  // unrelated parse errors.  Mirrors the P4283 requires-clause handling below
  // and GCC's "assertion-control labels require %<-fcontracts-p3400%>".
  if (Tok.is(tok::less)) {
    SourceLocation LabelLoc = ConsumeToken();
    llvm::SaveAndRestore OldGreater(GreaterThanIsOperator, false);
    llvm::SaveAndRestore SetFlag(Actions.InAssertionControlExpression, true);
    ExprResult Parsed = ParseConstantExpression();
    if (ExpectAndConsume(tok::greater))
      return StmtError();
    if (getLangOpts().ContractsP3400)
      LabelExpr = Parsed;
    else
      Diag(LabelLoc, diag::err_contract_label_require_flag);
  }

  // Parse optional requires clause (P4283): pre <label> requires C ...
  // P4283 uses the standard requires-clause grammar,
  //   requires constraint-logical-or-expression
  // with no mandatory parentheses: the constraint's atoms are primary-
  // expressions, so parsing stops before the contract predicate's '('.  This
  // matches the paper's `pre requires std::integral<T> (x > 0)` form and GCC.
  ExprResult RequiresClauseExpr;
  if (Tok.is(tok::kw_requires)) {
    SourceLocation RequiresLoc = ConsumeToken(); // consume 'requires'
    if (!getLangOpts().ContractsP4283) {
      Diag(RequiresLoc, diag::err_contract_requires_clause_require_flag);
      // Parse and discard the constraint so we recover to the predicate.
      (void)ParseConstraintLogicalOrExpression(
          /*IsTrailingRequiresClause=*/false,
          /*IsContractRequiresClause=*/true);
    } else {
      RequiresClauseExpr = ParseConstraintLogicalOrExpression(
          /*IsTrailingRequiresClause=*/false,
          /*IsContractRequiresClause=*/true);
      if (RequiresClauseExpr.isInvalid())
        return StmtError();
    }
  }

  ParsedAttributes CXX11Attrs(AttrFactory);
  MaybeParseCXX11Attributes(CXX11Attrs);

  SmallVector<Decl *, 4> CaptureDecls;
  DeclStmt *CapturesDeclStmt = nullptr;
  if (Tok.is(tok::l_square)) {
    if (CK != ContractKind::Post) {
      Diag(Tok.getLocation(), diag::err_postcondition_captures_on_non_post)
          << CKStr;
      SkipUntil(tok::l_paren, StopBeforeMatch);
    } else if (!getLangOpts().ContractsP3098) {
      Diag(Tok.getLocation(), diag::err_postcondition_captures_require_flag);
      SkipUntil(tok::l_paren, StopBeforeMatch);
    } else {
      if (ParsePostconditionCaptures(CaptureDecls))
        return StmtError();
    }
  }

  if (Tok.isNot(tok::l_paren)) {
    Diag(Tok, diag::err_expected_lparen_after) << CKStr;
    SkipUntil({tok::equal, tok::l_brace, tok::arrow, tok::kw_try, tok::comma,
               tok::l_paren},
              StopAtSemi | StopBeforeMatch);

    return StmtError();
  }

  BalancedDelimiterTracker T(*this, tok::l_paren);
  SourceLocation ExprLoc = Tok.getLocation();

  if (T.expectAndConsume(diag::err_expected_lparen_after, CKStr,
                         tok::r_paren)) {
    return StmtError();
  }

  ParseScope ContractScope(this, Scope::DeclScope | Scope::ContractAssertScope);
  EnterExpressionEvaluationContext EC(
      Actions, Sema::ExpressionEvaluationContext::PotentiallyEvaluated);

  if (!CaptureDecls.empty()) {
    Actions.ActOnFinishPostconditionCaptures(getCurScope(), CaptureDecls);
    SmallVector<Decl *, 4> CaptureVec(CaptureDecls);
    CapturesDeclStmt = new (Actions.Context)
        DeclStmt(DeclGroupRef::Create(Actions.Context, CaptureVec.data(),
                                      CaptureVec.size()),
                 CaptureDecls.front()->getLocation(),
                 CaptureDecls.back()->getLocation());
  }

  ResultNameDecl *RND = nullptr;
  // Parse a result-name declarator for EVERY contract kind, not just `post`.
  // On `pre` and `contract_assert` it is ill-formed, but parsing it lets Sema
  // say so ("result name not allowed outside of post condition specifier");
  // leaving it to fall through to the predicate instead produces an unrelated
  // "use of undeclared identifier" for the result name and then a cascade.
  if (Tok.is(tok::identifier) && NextToken().is(tok::colon)) {
    IdentifierInfo *Id = Tok.getIdentifierInfo();
    SourceLocation IdLoc = ConsumeToken();

    ExprLoc = ConsumeToken();
    QualType ReturnType;
    if (ReturnTypeResolver)
      ReturnType = ReturnTypeResolver();

    RND = Actions.ActOnResultNameDeclarator(
        CK, getCurScope(), ReturnType, IdLoc, Id,
        getCurScope()->getFunctionPrototypeDepth());

    if (!RND)
      return StmtError();

    if (RND->isInvalidDecl())
      IsInvalid = true;

    // Only a postcondition can hold a result name.  Sema has already
    // diagnosed the misplacement above, and the declaration stays in scope so
    // the predicate still resolves the name instead of producing a spurious
    // "use of undeclared identifier"; just do not hand it to the statement.
    if (CK != ContractKind::Post)
      RND = nullptr;
  }

  ExprResult Cond = [&]() {
    Sema::ContractScopeRAII ContractScope(Actions, CK, ScopeOffset, KeywordLoc);
    ExprResult CondResult = ParseConditionalExpression();
    if (CondResult.isInvalid())
      return CondResult;
    return Actions.ActOnContractAssertCondition(CondResult.get());
  }();

  ExprResult MessageExpr;
  if (getLangOpts().ContractsP3099 && Tok.is(tok::comma)) {
    ConsumeToken();

    bool ParseAsExpression = false;
    if (getLangOpts().CPlusPlus11) {
      for (unsigned I = 0;; ++I) {
        const Token &T = GetLookAheadToken(I);
        if (T.is(tok::r_paren))
          break;
        if (!tokenIsLikeStringLiteral(T, getLangOpts()) || T.hasUDSuffix()) {
          ParseAsExpression = true;
          break;
        }
      }
    }

    if (ParseAsExpression) {
      // A non-literal (user-generated) diagnostic message is a constant
      // expression and must be parsed in a ConstantEvaluated context, just as
      // static_assert does (ParseStaticAssertDeclaration).  The enclosing
      // contract-specifier context is PotentiallyEvaluated, so enter the
      // constant-evaluated context here.
      EnterExpressionEvaluationContext ConstantEvaluated(
          Actions, Sema::ExpressionEvaluationContext::ConstantEvaluated);
      MessageExpr = ParseConstantExpressionInExprEvalContext();
    } else if (tokenIsLikeStringLiteral(Tok, getLangOpts()))
      MessageExpr = ParseUnevaluatedStringLiteralExpression();
    else {
      Diag(Tok, diag::err_expected_string_literal)
          << /*Source='static_assert'*/ 1;
    }
  }

  SourceLocation EndLoc = Tok.getLocation();

  T.consumeClose();

  if (Cond.isInvalid()) {
    Cond =
        Actions.CreateRecoveryExpr(ExprLoc, EndLoc, {}, Actions.Context.BoolTy);
  } else {
    SetInvalidOnExit.release();
  }

  StmtResult Res = Actions.ActOnContractAssert(
      CK, KeywordLoc, Cond.get(), RND, CXX11Attrs, MessageExpr.get(),
      LabelExpr.get(), CapturesDeclStmt, RequiresClauseExpr.get());
  if (Res.isInvalid())
    IsInvalid = true;
  return Res;
}

bool Parser::ParsePostconditionCaptures(SmallVectorImpl<Decl *> &Captures) {
  assert(Tok.is(tok::l_square) && "Expected '['");
  ConsumeBracket();

  if (Tok.is(tok::r_square)) {
    ConsumeBracket();
    return false;
  }

  while (true) {
    SourceLocation Loc = Tok.getLocation();
    bool IsPackExpansion = false;

    // Reject = (default by-copy)
    if (Tok.is(tok::equal)) {
      Diag(Loc, diag::err_postcondition_capture_default);
      SkipUntil(tok::r_square, StopBeforeMatch);
      ConsumeBracket();
      return false;
    }

    // Reject & as default or capture-by-reference
    if (Tok.is(tok::amp)) {
      if (NextToken().is(tok::r_square) || NextToken().is(tok::comma)) {
        Diag(Loc, diag::err_postcondition_capture_default);
        SkipUntil(tok::r_square, StopBeforeMatch);
        ConsumeBracket();
        return false;
      }
      Diag(Loc, diag::err_postcondition_capture_by_reference);
      ConsumeToken(); // skip & and try to recover
    }

    // Reject this / *this
    if (Tok.is(tok::kw_this)) {
      Diag(Loc, diag::err_postcondition_capture_this);
      ConsumeToken();
      if (Tok.is(tok::comma)) {
        ConsumeToken();
        continue;
      }
      break;
    }
    if (Tok.is(tok::star) && NextToken().is(tok::kw_this)) {
      Diag(Loc, diag::err_postcondition_capture_this);
      ConsumeToken(); // *
      ConsumeToken(); // this
      if (Tok.is(tok::comma)) {
        ConsumeToken();
        continue;
      }
      break;
    }

    // Check for ... prefix (pack init-capture: [...x = args])
    if (Tok.is(tok::ellipsis)) {
      IsPackExpansion = true;
      ConsumeToken();
    }

    if (Tok.isNot(tok::identifier)) {
      Diag(Tok, diag::err_expected) << tok::identifier;
      SkipUntil(tok::r_square, StopBeforeMatch);
      ConsumeBracket();
      return true;
    }

    IdentifierInfo *Id = Tok.getIdentifierInfo();
    SourceLocation IdLoc = ConsumeToken();

    // Check for pack expansion suffix: [args...]
    if (!IsPackExpansion && Tok.is(tok::ellipsis)) {
      IsPackExpansion = true;
      ConsumeToken();
    }

    ExprResult Init;
    if (Tok.is(tok::equal)) {
      ConsumeToken();
      Init = ParseAssignmentExpression();
      if (Init.isInvalid()) {
        SkipUntil(tok::r_square, StopBeforeMatch);
        ConsumeBracket();
        return true;
      }
    }

    Decl *Capture = Actions.ActOnPostconditionCapture(
        getCurScope(), IdLoc, Id, Init.get(), IsPackExpansion);
    if (Capture)
      Captures.push_back(Capture);

    if (Tok.is(tok::comma)) {
      ConsumeToken();
      continue;
    }
    break;
  }

  if (ExpectAndConsume(tok::r_square))
    return true;

  return false;
}

bool Parser::ParseLexedFunctionContracts(
    CachedTokens &ContractToks, Decl *FD,
    Parser::ContractEnterScopeKind ScopesToEnter) {

  // Add the 'stop' token.
  Token LastContractToken = ContractToks.back();
  Token ContractEnd;
  ContractEnd.startToken();
  ContractEnd.setKind(tok::eof);
  ContractEnd.setLocation(LastContractToken.getEndLoc());
  ContractEnd.setEofData(FD);
  ContractToks.push_back(ContractEnd);

  // Parse the default argument from its saved token stream.
  ContractToks.push_back(Tok); // So that the current token doesn't get lost
  PP.EnterTokenStream(ContractToks, true, /*IsReinject*/ true);

  // Consume the previously-pushed token.
  ConsumeAnyToken();

  // C++11 [expr.prim.general]p3:
  //   If a declaration declares a member function or member function
  //   template of a class X, the expression this is a prvalue of type
  //   "pointer to cv-qualifier-seq X" between the optional cv-qualifer-seq
  //   and the end of the function-definition, member-declarator, or
  //   declarator.
  CXXMethodDecl *Method;
  FunctionDecl *FunctionToPush;
  if (FunctionTemplateDecl *FunTmpl = dyn_cast<FunctionTemplateDecl>(FD))
    FunctionToPush = FunTmpl->getTemplatedDecl();
  else
    FunctionToPush = cast<FunctionDecl>(FD);

  std::optional<ParseScope> ParserScope;
  if (ScopesToEnter & ContractEnterScopeKind::CES_Prototype)
    ParserScope.emplace(this, Scope::DeclScope | Scope::FunctionPrototypeScope |
                                  Scope::FunctionDeclarationScope);

  if (ScopesToEnter & ContractEnterScopeKind::CES_Parameters) {
    for (auto *Param : FunctionToPush->parameters()) {
      Actions.ActOnReenterCXXMethodParameter(getCurScope(), Param);
    }
  }

  Method = dyn_cast<CXXMethodDecl>(FunctionToPush);

  std::optional<ParseScope> FnScope;
  std::optional<Sema::ContextRAII> FnContext;
  std::optional<Sema::FunctionScopeRAII> PopFnContext;
  if (ScopesToEnter & ContractEnterScopeKind::CES_Function) {
    FnScope.emplace(this, Scope::FnScope);
    FnContext.emplace(Actions, FunctionToPush, /*NewThisContext=*/true);
    PopFnContext.emplace(Actions);
    Actions.PushFunctionScope();
  }

  std::optional<Sema::CXXThisScopeRAII> ThisScope;
  if (ScopesToEnter & ContractEnterScopeKind::CES_CXXThis)
    ThisScope.emplace(Actions, Method ? Method->getParent() : nullptr,
                      Method ? Method->getMethodQualifiers() : Qualifiers{},
                      Method && getLangOpts().CPlusPlus11);

  // Parse the exception-specification.
  SmallVector<ContractStmt *> Contracts;
  assert(isFunctionContractKeyword(Tok));

  SourceLocation StartLoc = Tok.getLocation();

  auto ReturnTypeResolver = [&]() { return FunctionToPush->getReturnType(); };
  bool IsInvalid = false;
  while (isFunctionContractKeyword(Tok)) {
    assert(Actions.CurContext == FunctionToPush);
    bool IsInvalidTmp = false;
    StmtResult Contract = ParseFunctionContractSpecifierImpl(
        ReturnTypeResolver, ContractScopeOffset::FunctionContext, IsInvalidTmp);
    if (Contract.isUsable())
      Contracts.push_back(Contract.getAs<ContractStmt>());
    IsInvalid |= IsInvalidTmp;
  }
  ContractSpecifierDecl *Seq = Actions.ActOnFinishContractSpecifierSequence(
      Contracts, StartLoc, IsInvalid);
  FunctionToPush->setContracts(Seq);

  // There could be leftover tokens (e.g. because of an error).
  // Skip through until we reach the original token position.
  while (Tok.isNot(tok::eof))
    ConsumeAnyToken();

  // Clean up the remaining EOF token.
  if (Tok.is(tok::eof) && Tok.getEofData() == FD)
    ConsumeAnyToken();

  ContractToks.clear();
  return true;
}
