---
id: 1040-p2900-base-parse
subject: '[clang][contracts] parse contract specifiers and assertions'
depends: []
regenerates: []
fixes: []
coauthors: ['Eric Fiselier <eric@efcs.ca>']
---

## Rationale
The contract grammar: `ParseContracts.cpp`, the `Parser.h` hooks it needs,
and late parsing.

`pre` and `post` are **not keywords**.  They are contextual identifiers --
`Ident_pre`, `Ident_post`, and the `__pre` / `__post` alternative spellings
-- recognized by `getContractKeyword()` looking at the token in hand, so
that existing code naming a variable `pre` keeps working.
`alternative-keywords.cpp` is the test that pins this, and it is the first
one to read.

**Late parsing is the substance of this commit, not an implementation
detail.**  A contract on a member function declared inside a class body
cannot be parsed where it appears: its predicate may name members declared
later in the class, so it has to be cached as tokens and re-parsed once the
class is complete -- the same treatment default arguments and
noexcept-specifiers already get.  That is what `ContractTokens` in the
late-parsed method state, `LateParseFunctionContractSpecifier{,Seq}`,
`ParseLexedFunctionContracts` and `DiagnoseUnattachedLateParsedContracts`
are for, and `contract-complete-class-late-parse.cpp` is its 152-line
exercise.

Re-parsing later means **rebuilding the scopes the contract was written
in**, which is what `ContractEnterScopeKind` enumerates: the prototype
scope, the parameter scope, `this`, and the function scope, entered in
whatever combination the deferred context calls for.  A contract predicate
is const-qualified with respect to `this`, which is why
`ParseCXXInlineMethods.cpp`'s `CXXThisScopeRAII` helper grows an `AddConst`
parameter here rather than in Sema.

Two smaller things worth a reviewer's attention:

**The token cacher knows about `<...>` labels even though P3400 is nine
commits away.**  Caching has to reproduce the token stream exactly, and a
cache that started at `<` would give the re-parse something it cannot make
sense of -- producing a cascade of unrelated errors instead of the
"requires `-fcontracts-p3400`" diagnostic the feature gate exists to emit.
So the label tokens are cached whether or not the flag is on, including
the `>>` split and the balanced-group handling a label expression needs.
Only the *caching* is here; parsing a label is `4200-p3400-core`'s.

**`Scope::ScopeFlags` is carried through the Parser as `unsigned long`,
and it should be `uint64_t`.**  `1070-p2900-base-sema-predicate-scope`
gives `Scope::ScopeFlags` a `uint64_t` underlying type because
`ContractAssertScope` is bit 32.  The Parser-side carriers here are
`unsigned long`, which is 64 bits on LP64 and **32 on LLP64**, so on
Windows the flag is truncated on the way in and every
`isContractAssertScope()` below it answers false.  The lines are this
commit's, so this is where that is fixed.

## Compile gap
Needs `1020-p2900-base-ast` for `ContractStmt` and `ContractSpecifierDecl`,
and `1000-p2900-base-basic` for `ContractKind`; both precede it.

**`Scope::ContractAssertScope` does not exist yet.**
`ParseContractAssertStatement` opens a `ParseScope` with it, and both the
enumerator and the `uint64_t` widening of `ScopeFlags` that makes bit 32
representable are `1070-p2900-base-sema-predicate-scope`'s, two commits
later.  Standing alone this needs the enumerator and the widened underlying
type added to `Scope.h`; `1070-p2900-base-sema-predicate-scope` then
supplies them for real, with the `static_assert`s and the non-nesting
semantics the flag actually has.

Everything the parser calls into Sema -- `ActOnContractAssert`,
`ActOnResultNameDeclarator`, `ActOnContractsOnFinishFunctionDecl` -- is
`1060-p2900-base-sema-core`, which comes **after** this commit.  Standing
alone, that is what this needs stubbed: each as a declaration in `Sema.h`
plus a definition returning `ExprError()` / `StmtError()` / nothing, all of
which `1060-p2900-base-sema-core` then replaces with the real thing.

`ParseContracts.cpp` is claimed here except for two functions that sit in
the same file and belong to later papers:
`ParsePostconditionCaptureList` (`4000-p3098`) and
`LateParseContractRequiresClause` (`4600-p4283`).  Neither is called from
the base paths, so this commit needs no stub for them -- the file is simply
shorter here than its final text.

**Behaviour trap:** with Sema stubbed, contract syntax parses and is
silently discarded.  Nothing attaches to a declaration and nothing is
checked, so a test asserting that a violated precondition terminates would
pass a program that never evaluated it.  The tests in this commit are
parse-level (`-fsyntax-only`, `-verify`) for exactly that reason; the
end-to-end ones are `1180-p2900-base-tests`.

## Contents
- clang/include/clang/Basic/DiagnosticParseKinds.td : #0:3
- clang/include/clang/Parse/Parser.h : *
- clang/include/clang/Sema/DeclSpec.h : *
- clang/lib/Parse/CMakeLists.txt : *
- clang/lib/Parse/ParseCXXInlineMethods.cpp : *
- clang/lib/Parse/ParseContracts.cpp : /ParseContracts.cpp - C\+\+ Contracts Parsing/, @clang, @ContractKind, @LateParseFunctionContr*, @getContractKeywordStr, @ParseContractAssertSta*, @ParseContractSpecifier*, @DiagnoseUnattachedLate*, @ParseFunctionContractS*, @ParseLexedFunctionCont*
- clang/lib/Parse/ParseDecl.cpp : #0, #1, #2, #3, #4, #5, #6, #7, #8, #9, #10, #11, #12, #13, #14, #15, #16, #18
- clang/lib/Parse/ParseDeclCXX.cpp : #0, #3, #4, #5, #6, #7, #8, #9, #10, #11, #12, #13, #14, #15, #16, #17
- clang/lib/Parse/ParseExprCXX.cpp : *
- clang/lib/Parse/ParseStmt.cpp : *
- clang/lib/Parse/Parser.cpp : *
- clang/test/Contracts/alternative-keywords.cpp : *
- clang/test/Contracts/contract-complete-class-late-parse.cpp : *
- clang/test/Contracts/contract-declarator-positions.cpp : *
- clang/test/Contracts/contract-in-declarator-group.cpp : *
- clang/test/Contracts/contract-on-function-typedef.cpp : *
- clang/test/Contracts/contract-group-attr.cpp : *
- clang/test/Contracts/contract-on-non-function.cpp : *
- clang/test/Contracts/parse-contract-basics.cpp : *
- clang/test/Contracts/parse-contract-grammar.cpp : *
- clang/test/Contracts/result-name-misplaced.cpp : *
- clang/test/Parser/contract-inline-methods.cpp : *
- clang/test/Parser/cxx-contracts.cpp : *
