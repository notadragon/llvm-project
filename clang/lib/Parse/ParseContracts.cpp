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
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/TimeProfiler.h"
#include <optional>

using namespace clang;

std::optional<ContractKind>
Parser::getContractKeyword(const Token &T) const {
  // C contracts (P4299): keyword tokens
  if (T.is(tok::kw__Pre))
    return ContractKind::Pre;
  if (T.is(tok::kw__Post))
    return ContractKind::Post;
  if (T.is(tok::kw__ContractAssert))
    return ContractKind::Assert;

  // We offer the reserved keywords as identifiers in C++11 mode.
  if (!getLangOpts().CPlusPlus11 ||
      (T.isNot(tok::identifier) && !T.is(tok::kw_contract_assert)))
    return std::nullopt;

  // contract_assert is a keyword only where it is enabled (as __contract_assert
  // it is one in every C++11 mode), so it needs no language check here.
  if (T.is(tok::kw_contract_assert))
    return ContractKind::Assert;

  const IdentifierInfo *II = T.getIdentifierInfo();
  assert(II && "Missing identifier info");

  if (!Ident_pre) {
    Ident_pre = &PP.getIdentifierTable().get("pre");
    Ident___pre = &PP.getIdentifierTable().get("__pre");

    Ident_post = &PP.getIdentifierTable().get("post");
    Ident___post = &PP.getIdentifierTable().get("__post");
  }

  // The extension spellings, like the standard ones, need contracts enabled.
  if (!getLangOpts().Contracts)
    return std::nullopt;

  if (II == Ident_pre || II == Ident___pre)
    return ContractKind::Pre;

  if (II == Ident_post || II == Ident___post)
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

/// Cache a '<'-delimited list -- a P3400 label, or a template argument list
/// in a P4283 requires-clause -- through its matching '>'.  A '>>' that closes
/// two levels is split into two '>' tokens, and a balanced group inside the
/// list is cached whole: its opening token is "special", so ConsumeToken
/// cannot consume it, and a '<' or '>' inside it is an operator rather than a
/// delimiter, so counting it would end the list early -- pre<L{}>,
/// pre<make_label()>, pre<(a > b ? l1 : l2)>.  Returns false after a
/// diagnostic.
bool Parser::LateParseAngleBracketedTokens(CachedTokens &Toks) {
  assert(Tok.is(tok::less) && "Not a '<'");
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
    } else if (Tok.isOneOf(tok::l_paren, tok::l_square, tok::l_brace)) {
      // StopAtSemi is off because the group is delimited by its own closing
      // token and a ';' can legitimately appear within one -- a label whose
      // expression is an immediately-invoked lambda, say.
      tok::TokenKind Close = Tok.is(tok::l_paren)    ? tok::r_paren
                             : Tok.is(tok::l_square) ? tok::r_square
                                                     : tok::r_brace;
      Toks.push_back(Tok);
      ConsumeAnyToken();
      if (!ConsumeAndStoreUntil(Close, Toks, /*StopAtSemi=*/false,
                                /*ConsumeFinalToken=*/true)) {
        Diag(Tok, diag::err_expected) << Close;
        return false;
      }
      continue;
    }
    Toks.push_back(Tok);
    // Not ConsumeToken: the list may contain string literals and annotation
    // tokens, which are "special" and must be consumed as such.
    ConsumeAnyToken();
  }
  return true;
}

bool Parser::LateParseFunctionContractSpecifier(CachedTokens &Toks) {
  assert(isFunctionContractKeyword(Tok) && "Not in a contract");
  ContractKind CK = getContractKeyword(Tok).value();
  const char *CKStr = getContractKeywordStr(CK);

  // A specifier that fails to cache before its predicate has been diagnosed
  // here; drop what was cached of it, or its replay would diagnose it a
  // second time (CLANG-638).
  const size_t Start = Toks.size();
  auto Fail = [&] {
    Toks.truncate(Start);
    return false;
  };

  Toks.push_back(Tok); // contract keyword
  ConsumeToken();

  // P3400: cache a label.  When P3400 is off, diagnose it here instead, before
  // anything wrong inside it (CLANG-648: a label with no '>' reported only
  // the missing '>'), and drop its tokens once consumed, so the re-parse
  // neither sees nor diagnoses it again.  The label must still be consumed:
  // leaving the cached token stream starting at '<' is what the re-parse
  // cannot make sense of -- a late-parsed (member function) contract then
  // produces the same cascade of unrelated errors this diagnostic exists to
  // replace ("expected '(' after 'pre'", "expected ';' at end of declaration
  // list").
  if (Tok.is(tok::less)) {
    const size_t LabelStart = Toks.size();
    if (!getLangOpts().ContractsP3400)
      Diag(Tok, diag::err_contract_label_require_flag);
    if (!LateParseAngleBracketedTokens(Toks))
      return Fail();
    if (!getLangOpts().ContractsP3400)
      Toks.truncate(LabelStart);
  }

  // P4283: cache an optional requires-clause before the
  // attributes/captures/predicate.
  if (Tok.is(tok::kw_requires) && !LateParseContractRequiresClause(Toks))
    return Fail();

  // Cache attribute tokens [[...]] and capture tokens [...].  An unterminated
  // group leaves Tok at the ';' or end of file that stopped it, which the
  // check for '(' below diagnoses.
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
  if (Tok.is(tok::l_square)) {
    Toks.push_back(Tok);
    ConsumeBracket();
    ConsumeAndStoreUntil(tok::r_square, Toks,
                         /*StopAtSemi=*/true,
                         /*ConsumeFinalToken=*/true);
  }

  if (!Tok.is(tok::l_paren)) {
    Diag(Tok, diag::err_expected_lparen_after) << CKStr;
    return Fail();
  }
  Toks.push_back(Tok); // '('
  ConsumeParen();
  // An unterminated predicate is not diagnosed here: its tokens stay cached
  // and the replay diagnoses it.
  return ConsumeAndStoreUntil(tok::r_paren, Toks,
                              /*StopAtSemi=*/true,
                              /*ConsumeFinalToken=*/true);
}

/// ParseContractAssertStatement
///
///  assertion-statement:
///     'contract_assert' attribute-specifier-seq[opt] '('
///     conditional-expression ')' ';'
///
/// Only the parts it shares with a function-contract-specifier -- the label,
/// the requires-clause, the predicate and the message -- go through the
/// helpers that ParseFunctionContractSpecifier uses too.
StmtResult Parser::ParseContractAssertStatement() {
  assert((Tok.is(tok::kw_contract_assert) || Tok.is(tok::kw__ContractAssert)) &&
         "Not a contract assert statement");
  // The extension spelling __contract_assert is a keyword whether or not
  // contracts are enabled; without them it cannot be used.
  if (Tok.is(tok::kw_contract_assert) && !getLangOpts().Contracts) {
    Diag(Tok, diag::err_contract_assert_requires_contracts)
        << Tok.getIdentifierInfo();
    SkipUntil(tok::semi);
    return StmtError();
  }
  const ContractKind CK = ContractKind::Assert;
  const char *CKStr = getContractKeywordStr(CK);
  SourceLocation KeywordLoc = ConsumeToken();

  ExprResult LabelExpr, RequiresClauseExpr;
  if (!ParseContractLabelAndRequiresClause(LabelExpr, RequiresClauseExpr))
    return StmtError();

  ParsedAttributes CXX11Attrs(AttrFactory);
  MaybeParseCXX11Attributes(CXX11Attrs);

  // Captures belong to a postcondition alone.
  if (Tok.is(tok::l_square)) {
    Diag(Tok.getLocation(), diag::err_postcondition_captures_on_non_post)
        << CKStr;
    SkipUntil(tok::l_paren, StopBeforeMatch);
  }

  BalancedDelimiterTracker T(*this, tok::l_paren);
  SourceLocation ExprLoc;
  if (!ParseContractOpenParen(CKStr, T, ExprLoc))
    return StmtError();

  ParseScope ContractScope(this, Scope::DeclScope | Scope::ContractAssertScope);
  EnterExpressionEvaluationContext EC(
      Actions, Sema::ExpressionEvaluationContext::PotentiallyEvaluated);

  // A result name is ill-formed here; it is parsed only so that Sema can say
  // so, and is not attached to the assertion.
  bool IsInvalid = false;
  ResultNameDecl *RND = nullptr;
  if (!ParseContractResultName(CK, /*ReturnTypeResolver=*/{}, ExprLoc, RND,
                               IsInvalid))
    return StmtError();

  return ParseContractPredicateAndFinish(
      CK, KeywordLoc, ContractScopeOffset::FunctionContext, T, ExprLoc,
      /*RND=*/nullptr, CXX11Attrs, LabelExpr, /*CapturesDeclStmt=*/nullptr,
      RequiresClauseExpr, IsInvalid);
}

/// ParseContractSpecifierSequence - Parse a series of pre/post contracts on
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
                                            bool EnterScope) {
  if (!isFunctionContractKeyword(Tok))
    return;

  // A contract specifier belongs to a function.  Written on anything else --
  // a data member, a variable, a bit-field, or a declarator the parser has
  // already given up on -- there is no function chunk to take the parameters
  // and the result type from, so say so and eat the specifier rather than
  // reading a chunk that is not there.  Caching the tokens is how the
  // specifier's true extent is known: it may carry a label, a requires-clause,
  // attributes and captures ahead of the predicate.
  //
  // isFunctionDeclarator() asks only about the declarator's CHUNKS, so it is
  // true for `typedef int F(int)` -- that declarator really does have a
  // function chunk.  What makes it not a function declaration is the storage
  // class, which lives on the leading DeclSpec and is not a chunk at all.
  // Without the second test the contract parses, is Sema-checked, is stored
  // on the declarator, and is then dropped on the floor when
  // ActOnTypedefDeclarator takes the declaration instead of
  // CreateNewFunctionDecl: no diagnostic, and no check at any call.
  //
  // Test SCS_typedef narrowly rather than reaching for
  // isFunctionDeclarationContext(), which folds in the same test but returns
  // false for DeclaratorContext::LambdaExpr -- and a lambda's contracts come
  // through this very function, so that would reject every
  // `[](int x) pre(x > 0) {}`.
  if (!DeclarationInfo.isFunctionDeclarator() ||
      DeclarationInfo.getDeclSpec().getStorageClassSpec() ==
          DeclSpec::SCS_typedef) {
    // Name the keyword as it was written -- `pre`, `__pre` or C's `_Pre`.
    Diag(Tok, diag::err_contract_on_non_function) << PP.getSpelling(Tok);
    CachedTokens Discarded;
    LateParseFunctionContractSpecifierSeq(Discarded);
    return;
  }

  std::optional<QualType> CachedType;
  auto ReturnTypeResolver = [&]() {
    if (!CachedType) {
      TypeSourceInfo *TInfo = Actions.GetTypeForDeclarator(DeclarationInfo);
      assert(TInfo && TInfo->getType()->isFunctionType() &&
             "a contract specifier's declarator has function type");
      CachedType = TInfo->getType()->getAs<FunctionType>()->getReturnType();
    }
    return CachedType.value();
  };
  std::optional<ParseScope> ParserScope;

  std::optional<Sema::CXXThisScopeRAII> ThisScope;

  if (EnterScope) {
    ParserScope.emplace(this, Scope::DeclScope | Scope::FunctionPrototypeScope |
                                  Scope::FunctionDeclarationScope);

    auto FTI = DeclarationInfo.getFunctionTypeInfo();

    for (unsigned i = 0; i != FTI.NumParams; ++i) {
      ParmVarDecl *Param = cast<ParmVarDecl>(FTI.Params[i].Param);
      Actions.ActOnReenterCXXMethodParameter(getCurScope(), Param);
    }
  }

  // [expr.prim.this]/1: `this` "shall not appear within the declaration of
  // either a static member function or an explicit object member function of
  // the current class".  A contract predicate is within that declaration, so
  // an explicit object member function must get no `this` scope here.
  //
  // Sema::CheckCXXThisType already diagnoses the explicit-object case
  // correctly, but only fires when the `this` type is null; pushing a scope
  // gives it a non-null one and makes that branch unreachable, so both `this`
  // and an unqualified member name (which means (*this).m) were accepted in a
  // predicate and then asserted in CodeGen on LoadCXXThis.  Leaving the scope
  // unpushed gives such a predicate the same diagnostics the function BODY
  // already gets.
  //
  // The condition is deliberately applied here rather than inside
  // InitCXXThisScopeForDeclaratorIfRelevant: that helper is shared with the
  // trailing-return-type path, whose handling of explicit object member
  // functions is a separate question from contracts.
  //
  // The DeclSpec handed over is the function chunk's MethodQualifiers, not
  // the declarator's leading one.  The cv-qualifiers that decide the type of
  // `this` are the TRAILING ones -- the `const` of `int S::f(int) const` --
  // and those live on the chunk; the leading DeclSpec holds `int`.  Passing
  // the leading one gives the predicate `S *` where the same function's
  // in-class declaration got `const S *`, which is a constness divergence
  // between two declarations of one function and would quietly defeat
  // P2900 const-ification.  The trailing-return-type caller in
  // ParseFunctionDeclarator passes the chunk's DeclSpec for this reason.
  if (!DeclarationInfo.isExplicitObjectMemberFunction()) {
    const DeclSpec *QualifierDS = &DeclarationInfo.getDeclSpec();
    if (DeclarationInfo.isFunctionDeclarator() &&
        DeclarationInfo.getFunctionTypeInfo().MethodQualifiers)
      QualifierDS = DeclarationInfo.getFunctionTypeInfo().MethodQualifiers;
    InitCXXThisScopeForDeclaratorIfRelevant(DeclarationInfo, *QualifierDS,
                                            ThisScope);
  }
  SourceLocation StartLoc = Tok.getLocation();

  SmallVector<ContractStmt *, 4> Contracts;
  while (isFunctionContractKeyword(Tok)) {
    ContractKind CK = *getContractKeyword(Tok);
    SourceLocation KeywordLoc = ConsumeToken();
    Contracts.push_back(ParseFunctionContractSpecifier(
        CK, KeywordLoc, ReturnTypeResolver,
        EnterScope ? ContractScopeOffset::ParentContext
                   : ContractScopeOffset::FunctionContext));
  }
  ContractSpecifierDecl *Seq =
      Actions.ActOnFinishContractSpecifierSequence(Contracts, StartLoc);

  assert(!DeclarationInfo.getContracts() && "Already have contracts?");

  DeclarationInfo.setContracts(Seq);
}

/// DiagnoseUnattachedLateParsedContracts - Report and discard contract tokens
/// that were cached for late parsing but never got a declaration to be
/// replayed against -- either because the declaration they were written on
/// was too malformed to produce one, or because what it did produce is not a
/// function.  \p DiagID picks which of those the caller is looking at.  Either
/// way this keeps the tokens away from Declarator::clear(), whose assertion
/// would otherwise turn a diagnosable input into a crash.
void Parser::DiagnoseUnattachedLateParsedContracts(Declarator &D,
                                                   unsigned DiagID) {
  if (D.getLateParsedContracts().empty())
    return;

  const Token &Start = D.getLateParsedContracts().front();
  Diag(Start.getLocation(), DiagID) << PP.getSpelling(Start);
  D.getLateParsedContracts().clear();
}

/// Parse a contract assertion's optional P3400 label and P4283
/// requires-clause.  Returns false after a diagnostic the caller cannot
/// recover from.
bool Parser::ParseContractLabelAndRequiresClause(
    ExprResult &LabelExpr, ExprResult &RequiresClauseExpr) {
  // Recognise the label syntax even when P3400 is off, so we can diagnose the
  // missing flag and still recover to the predicate.  Bailing out on the '<'
  // instead would derail the whole declaration and bury the real problem under
  // unrelated parse errors.  Mirrors the P4283 requires-clause handling below
  // and GCC's "assertion-control labels require %<-fcontracts-p3400%>".
  // The missing flag is diagnosed before anything wrong inside the label
  // (CLANG-648).
  if (Tok.is(tok::less)) {
    SourceLocation LabelLoc = ConsumeToken();
    if (!getLangOpts().ContractsP3400)
      Diag(LabelLoc, diag::err_contract_label_require_flag);
    llvm::SaveAndRestore OldGreater(GreaterThanIsOperator, false);
    llvm::SaveAndRestore SetFlag(Actions.InAssertionControlExpression, true);
    ExprResult Parsed = ParseConstantExpression();
    if (ExpectAndConsume(tok::greater))
      return false;
    if (getLangOpts().ContractsP3400)
      LabelExpr = Parsed;
  }

  // Parse optional requires clause (P4283): pre <label> requires C ...
  // P4283 uses the standard requires-clause grammar,
  //   requires constraint-logical-or-expression
  // with no mandatory parentheses: the constraint's atoms are primary-
  // expressions, so parsing stops before the contract predicate's '('.  This
  // matches the paper's `pre requires std::integral<T> (x > 0)` form and GCC.
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
        return false;
    }
  }
  return true;
}

/// Consume the '(' that opens a contract assertion's predicate, whose
/// location is returned in \p ExprLoc.  False after a diagnostic.
bool Parser::ParseContractOpenParen(const char *CKStr,
                                    BalancedDelimiterTracker &T,
                                    SourceLocation &ExprLoc) {
  if (Tok.isNot(tok::l_paren)) {
    Diag(Tok, diag::err_expected_lparen_after) << CKStr;
    SkipUntil({tok::equal, tok::l_brace, tok::arrow, tok::kw_try, tok::comma,
               tok::l_paren},
              StopAtSemi | StopBeforeMatch);
    return false;
  }
  ExprLoc = Tok.getLocation();
  return !T.expectAndConsume(diag::err_expected_lparen_after, CKStr,
                             tok::r_paren);
}

/// Parse a result-name-introducer, if one starts the predicate, into \p RND
/// (null unless \p CK is a postcondition), moving \p ExprLoc to its ':'.
/// False when no declaration could be made for it.
bool Parser::ParseContractResultName(
    ContractKind CK, llvm::function_ref<QualType()> ReturnTypeResolver,
    SourceLocation &ExprLoc, ResultNameDecl *&RND, bool &IsInvalid) {
  RND = nullptr;
  // Parse a result-name declarator for EVERY contract kind, not just `post`.
  // On `pre` and `contract_assert` it is ill-formed, but parsing it lets Sema
  // say so ("result name not allowed outside of post condition specifier");
  // leaving it to fall through to the predicate instead produces an unrelated
  // "use of undeclared identifier" for the result name and then a cascade.
  //
  // result-name-introducer: attributed-identifier `:', so the identifier may
  // be followed by an attribute-specifier-seq before the colon; look past it
  // to tell a result name from a predicate that starts with `x [['.
  bool IsResultName = Tok.is(tok::identifier) && NextToken().is(tok::colon);
  if (!IsResultName && Tok.is(tok::identifier) &&
      NextToken().is(tok::l_square) && GetLookAheadToken(2).is(tok::l_square)) {
    TentativeParsingAction TPA(*this);
    ConsumeToken();
    ParsedAttributes Probe(AttrFactory);
    MaybeParseCXX11Attributes(Probe);
    IsResultName = Tok.is(tok::colon);
    TPA.Revert();
  }
  if (!IsResultName)
    return true;

  IdentifierInfo *Id = Tok.getIdentifierInfo();
  SourceLocation IdLoc = ConsumeToken();
  ParsedAttributes ResultNameAttrs(AttrFactory);
  MaybeParseCXX11Attributes(ResultNameAttrs);

  ExprLoc = ConsumeToken();
  QualType ReturnType;
  if (ReturnTypeResolver)
    ReturnType = ReturnTypeResolver();

  ResultNameDecl *Decl = Actions.ActOnResultNameDeclarator(
      CK, getCurScope(), ReturnType, IdLoc, Id,
      getCurScope()->getFunctionPrototypeDepth());
  if (!Decl)
    return false;

  if (Decl->isInvalidDecl())
    IsInvalid = true;

  // [[maybe_unused]] is what a result name is given in practice, and a
  // result name is never diagnosed as unused, so it has nothing to do.
  // Anything else goes through the usual attribute processing, which
  // diagnoses what does not apply to it.
  SmallVector<ParsedAttr *, 2> MaybeUnused;
  for (ParsedAttr &A : ResultNameAttrs)
    if (A.getKind() == ParsedAttr::AT_Unused)
      MaybeUnused.push_back(&A);
  for (ParsedAttr *A : MaybeUnused)
    ResultNameAttrs.remove(A);
  if (!ResultNameAttrs.empty())
    Actions.ProcessDeclAttributeList(getCurScope(), Decl, ResultNameAttrs);

  // Only a postcondition can hold a result name.  Sema has already
  // diagnosed the misplacement above, and the declaration stays in scope so
  // the predicate still resolves the name instead of producing a spurious
  // "use of undeclared identifier"; just do not hand it to the statement.
  if (CK == ContractKind::Post)
    RND = Decl;
  return true;
}

/// Parse a contract assertion's predicate, its P3099 message and the closing
/// ')' that \p T tracks, and build the assertion from them and the parts
/// already parsed.  The assertion is invalid if \p IsInvalid (a part already
/// parsed was) or its predicate is.
StmtResult Parser::ParseContractPredicateAndFinish(
    ContractKind CK, SourceLocation KeywordLoc, ContractScopeOffset ScopeOffset,
    BalancedDelimiterTracker &T, SourceLocation ExprLoc, ResultNameDecl *RND,
    ParsedAttributes &CXX11Attrs, ExprResult LabelExpr,
    DeclStmt *CapturesDeclStmt, ExprResult RequiresClauseExpr, bool IsInvalid) {
  ExprResult Cond = [&]() {
    Sema::ContractScopeRAII SemaContractScope(Actions, CK, ScopeOffset,
                                              KeywordLoc);
    ExprResult CondResult = ParseConditionalExpression();
    if (CondResult.isInvalid())
      return CondResult;
    return Actions.ActOnContractAssertCondition(CondResult.get());
  }();

  ExprResult MessageExpr;
  if (getLangOpts().ContractsP3099 && Tok.is(tok::comma)) {
    ConsumeToken();

    if (isUserGeneratedMessage()) {
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
    IsInvalid = true;
    Cond =
        Actions.CreateRecoveryExpr(ExprLoc, EndLoc, {}, Actions.Context.BoolTy);
  }

  StmtResult Res = Actions.ActOnContractAssert(
      CK, KeywordLoc, Cond.get(), RND, CXX11Attrs, MessageExpr.get(),
      LabelExpr.get(), CapturesDeclStmt, RequiresClauseExpr.get());
  if (auto *CS = dyn_cast_if_present<ContractStmt>(Res.get())) {
    CS->setRParenLoc(T.getCloseLocation());
    if (IsInvalid)
      CS->setInvalid();
  }
  return Res;
}

/// ParseFunctionContractSpecifier - Parse one precondition-specifier or
/// postcondition-specifier whose keyword, at \p KeywordLoc, has been
/// consumed.  One that cannot be built at all becomes an invalid placeholder,
/// so the result is never null.
ContractStmt *Parser::ParseFunctionContractSpecifier(
    ContractKind CK, SourceLocation KeywordLoc,
    llvm::function_ref<QualType()> ReturnTypeResolver,
    ContractScopeOffset ScopeOffset) {
  assert(CK != ContractKind::Assert && ReturnTypeResolver &&
         "a function contract specifier is a pre or a post with a return "
         "type resolver");
  auto Invalid = [&] {
    return Actions.BuildInvalidContractStmt(CK, KeywordLoc);
  };

  const char *CKStr = getContractKeywordStr(CK);

  ExprResult LabelExpr, RequiresClauseExpr;
  if (!ParseContractLabelAndRequiresClause(LabelExpr, RequiresClauseExpr))
    return Invalid();

  ParsedAttributes CXX11Attrs(AttrFactory);
  MaybeParseCXX11Attributes(CXX11Attrs);

  SmallVector<Decl *, 4> CaptureDecls;
  if (Tok.is(tok::l_square)) {
    if (CK != ContractKind::Post) {
      Diag(Tok.getLocation(), diag::err_postcondition_captures_on_non_post)
          << CKStr;
      SkipUntil(tok::l_paren, StopBeforeMatch);
    } else if (!getLangOpts().ContractsP3098) {
      Diag(Tok.getLocation(), diag::err_postcondition_captures_require_flag);
      SkipUntil(tok::l_paren, StopBeforeMatch);
    } else {
      // A capture's initializer is part of the contract assertion: an entity
      // declared outside it, `this' included, is constified there as in the
      // predicate (P3098; CLANG-641).
      Sema::ContractScopeRAII SemaContractScope(Actions, CK, ScopeOffset,
                                                KeywordLoc);
      if (!ParsePostconditionCaptures(CaptureDecls))
        return Invalid();
    }
  }

  BalancedDelimiterTracker T(*this, tok::l_paren);
  SourceLocation ExprLoc;
  if (!ParseContractOpenParen(CKStr, T, ExprLoc))
    return Invalid();

  ParseScope ContractScope(this, Scope::DeclScope | Scope::ContractAssertScope);
  EnterExpressionEvaluationContext EC(
      Actions, Sema::ExpressionEvaluationContext::PotentiallyEvaluated);

  DeclStmt *CapturesDeclStmt = nullptr;
  if (!CaptureDecls.empty()) {
    Actions.ActOnFinishPostconditionCaptures(getCurScope(), CaptureDecls);
    SmallVector<Decl *, 4> CaptureVec(CaptureDecls);
    CapturesDeclStmt = new (Actions.Context)
        DeclStmt(DeclGroupRef::Create(Actions.Context, CaptureVec.data(),
                                      CaptureVec.size()),
                 CaptureDecls.front()->getLocation(),
                 CaptureDecls.back()->getLocation());
  }

  bool IsInvalid = false;
  ResultNameDecl *RND = nullptr;
  if (!ParseContractResultName(CK, ReturnTypeResolver, ExprLoc, RND, IsInvalid))
    return Invalid();

  StmtResult Res = ParseContractPredicateAndFinish(
      CK, KeywordLoc, ScopeOffset, T, ExprLoc, RND, CXX11Attrs, LabelExpr,
      CapturesDeclStmt, RequiresClauseExpr, IsInvalid);
  // dyn_cast, not ActionResult::getAs -- that is a static_cast, so anything
  // other than a ContractStmt would be stored as a bogus ContractStmt rather
  // than caught.  Nothing should produce one; treat it as invalid if it does
  // rather than filing garbage into the specifier.
  auto *CS = dyn_cast_if_present<ContractStmt>(Res.get());
  assert((CS || !Res.isUsable()) &&
         "contract specifier did not produce a ContractStmt");
  return CS ? CS : Invalid();
}

void Parser::ParseLexedFunctionContracts(
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

  // Parse the contract specifiers from their saved token stream.
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

  // An explicit object member function has no `this` ([expr.prim.this]/1), and
  // a contract predicate is within its declaration, so it gets no `this` scope
  // -- see the matching condition on the eager path above.  This is the
  // late-parsed path, which is the one a member function's contracts actually
  // take.
  std::optional<Sema::CXXThisScopeRAII> ThisScope;
  if ((ScopesToEnter & ContractEnterScopeKind::CES_CXXThis) &&
      !(Method && Method->isExplicitObjectMemberFunction()))
    ThisScope.emplace(Actions, Method ? Method->getParent() : nullptr,
                      Method ? Method->getMethodQualifiers() : Qualifiers{},
                      Method && getLangOpts().CPlusPlus11);

  // Parse the contract specifiers.
  SmallVector<ContractStmt *> Contracts;
  assert(isFunctionContractKeyword(Tok) && "cached tokens begin a specifier");

  SourceLocation StartLoc = Tok.getLocation();

  auto ReturnTypeResolver = [&]() { return FunctionToPush->getReturnType(); };
  while (isFunctionContractKeyword(Tok)) {
    assert(Actions.CurContext == FunctionToPush &&
           "contracts are parsed in the function's context");
    ContractKind CK = *getContractKeyword(Tok);
    SourceLocation KeywordLoc = ConsumeToken();
    Contracts.push_back(
        ParseFunctionContractSpecifier(CK, KeywordLoc, ReturnTypeResolver,
                                       ContractScopeOffset::FunctionContext));
  }
  ContractSpecifierDecl *Seq =
      Actions.ActOnFinishContractSpecifierSequence(Contracts, StartLoc);
  FunctionToPush->setContracts(Seq);

  // There could be leftover tokens (e.g. because of an error).
  // Skip through until we reach the original token position.
  while (Tok.isNot(tok::eof))
    ConsumeAnyToken();

  // Clean up the remaining EOF token.
  if (Tok.is(tok::eof) && Tok.getEofData() == FD)
    ConsumeAnyToken();

  ContractToks.clear();
}
