# Segment cuts

Declared interior boundaries for segments that anchors.py cannot divide.

anchors.py cuts a run of added lines at column-0 declaration starts, so it
separates a run of several definitions but is blind INSIDE one definition's
body -- and blind inside a class body, where every member is indented and
nothing starts at column 0.  A segment whose interior belongs to more than
one planned commit is declared here, once, and every commit claiming a piece
of it sees the same division; two commits declaring conflicting cuts of one
segment would be unresolvable, so the declaration deliberately does not live
in the commit entries.

Format: a level-2 heading naming a segment key, then an ordered list of cut
specs.  `+/regex/` matches the first added line of the new piece and is
required; `-/regex/` matches the first removed line and is optional -- when
omitted, every removed line stays with piece 0.  N cut specs make N+1
pieces, numbered from 0, keyed `path#ordinal[.piece]:cut`.

The regexes anchor on content, never on line numbers, so a rebase does not
invalidate them.  A regex that stops matching, matches twice, or falls out
of order is reported by `branch_history.py check` under the `cuts`
invariant, and generation refuses.

**No prose line may begin with `-`.**  The parser treats every line whose
stripped form starts with a dash as a cut spec, so a paragraph that happens
to wrap onto a line beginning `-- like this` fails the file with a
confusing error about the prose.  Reflow instead.

**A cut piece is claimed by `#N:P`, never by a bare regex.**  Once a segment
is cut, a selector that does not name a piece claims nothing, so a forgotten
piece is a loud error rather than a silent omission.

**`:P` and `.P` are different things, and only `:P` is a selector.**  A
segment key may carry a `.P` suffix -- `path#1.3` -- when anchors.py split
one hunk at definition boundaries, but a selector matches on the segment's
ORDINAL, which every such piece shares.  So `#1.3` selects nothing and `#1`
selects all four pieces of hunk 1; to claim one anchor piece on its own, name
it by its `@anchor`.  `#N:P` addresses a piece of a cut DECLARED IN THIS
FILE, and nothing else.

**Anchor a `/regex/` selector with `^` when it names a function.**  A
selector matches anywhere in a segment, so a bare `/someFunction/` claims
every segment that CALLS it as well as the one that defines it -- which
shows up as a two-commit conflict, not as silence, but only if the caller
happens to be claimed too.  `emitContractDataBlock` and `CGContractData`
were each caught this way.  Prefer `/^someFunction\(/`.

**Two anchor pieces can share an anchor**, and then neither `@name` nor `#N`
can separate them -- `ASTContext.cpp#2.8` and `#2.9` are both
`@UnnamedGlobalConstantDecl`, being two overloads of one function.  Use a
`/regex/` on whatever actually differs (there, the parameter list) rather
than declaring a cut: the pieces are already separate segments, so a cut
would be the wrong tool.

## clang/include/clang/AST/ASTContext.h#1

One hunk declares the base facility's violation-object machinery and then,
with no blank line between them, P3100's overload of it.
`getBuiltinContractViolationRecordType`, `getBuiltinContractViolationRecordDecl`
and `BuildViolationObject(const ContractStmt *)` are the base facility: they
build the violation object for a contract assertion that a ContractStmt
exists for.  The second `BuildViolationObject` takes a raw location and
comment precisely because a compiler-synthesized implicit assertion has no
ContractStmt to take them from, which is P3100's whole premise.

Cut at the doc comment, so each overload travels with the feature that needs
it.

- +/^  \/\/\/ P3100: build a violation object/

## clang/include/clang/Basic/TokenKinds.def#0

The base facility's `CONTRACTS_KEYWORD` macro and P3400's
`CONTRACTS_P3400_KEYWORD` are declared back to back.  They key different
token flags -- `KEYCONTRACTS` against `KEYCONTRACTSP3400` -- so each belongs
with the feature whose keywords it spells.

- +/^#ifndef CONTRACTS_P3400_KEYWORD$/

## clang/include/clang/Basic/TokenKinds.def#2

One hunk declares three features' keywords in a row: the base facility's
`contract_assert` and its `__contract_assert` alias, P3400's
`contract_control`, and P4299's C spellings `_Pre` / `_Post` /
`_ContractAssert`.  Only adjacency joins them.

- +/^CONTRACTS_P3400_KEYWORD\(contract_control\)$/
- +/^\/\/ C contracts keywords \(D4299\)$/

## clang/include/clang/Basic/TokenKinds.def#4

The matching `#undef`s, in the reverse order of the definitions: P3400's
first, then the base facility's.

- +/^#undef CONTRACTS_KEYWORD$/

## clang/lib/Basic/IdentifierTable.cpp#0

Two `case` arms of one switch.  `KEYCONTRACTS` is enabled by the base
facility -- or by P4299, since the C keywords carry the same flag --
whereas `KEYCONTRACTSP3400` is gated on P3400 alone.

- +/^  case KEYCONTRACTSP3400:$/

## clang/include/clang/Basic/DiagnosticGroups.td#0

Four warning groups in one hunk, joined by nothing but their position in the
file: the base facility's `contract-violation`, P3400's
`contract-invalid-label-facet`, and P3595's `contract-configuration`.

The fourth, `ContractWarning`, is the umbrella `-Wcontracts` and **names
`ContractConfiguration` in its member list**, so it cannot be declared
before P3595 exists.  It therefore travels with the P3595 piece rather than
with the base facility, even though the umbrella is not itself a P3595
concept -- the reference is what fixes the order.

- +/^\/\/ Warnings about an assertion-control object that nearly provides a facet\.$/
- +/^\/\/ Warnings about problems in the P3595 contract configuration\.$/

## clang/include/clang/Basic/DiagnosticParseKinds.td#0

Four features' parse diagnostics in a row.  The first three are flag gates:
P3098's for postcondition captures, P4283's for requires clauses, and
P3400's for assertion-control labels.  Each says the name of the very flag
its own commit adds, so a gate landing before its feature would name a flag
that does not exist yet.  The tail
(`warn_contract_on_parameter_declarator`, `err_contract_on_non_function`,
`err_contract_on_invalid_declaration`) is the base facility diagnosing a
contract in a place no feature makes legal.

- +/^\/\/ P4283 requires clause diagnostics$/
- +/^\/\/ P3400 assertion-control label diagnostics$/
- +/^def warn_contract_on_parameter_declarator : Warning</

## clang/lib/Sema/TreeTransform.h#2

`TransformContractStmt` is one 206-line function that instantiates every
part of a contract, so the base facility owns most of it.  P4283's share is
self-contained: declaring `RequiresClause`, transforming it, and discarding
the whole contract when a non-dependent clause is unsatisfied.

The tail after the cut is base again, because `RebuildContractStmt` is the
common exit.  **It reads `RequiresClause`, which the P4283 piece declares**,
so the base piece standing alone needs that argument stubbed as `nullptr`;
that is recorded as the compile gap of `1080-p2900-base-sema-templates`.

Not cut here, and owed: the same function's P3400 label transform (the
`EnterExpressionEvaluationContext ConstCtx` block that transforms
`S->getLabelExpr()`) is still inside the base piece.  It is inert without
P3400 -- the label expression is simply absent -- so it is imprecision
rather than breakage.

- +/^  Expr \*RequiresClause = nullptr;$/
- +/^  return getDerived\(\)\.RebuildContractStmt\($/

## clang/lib/Frontend/InitPreprocessor.cpp#0

Eleven feature-test macros in one hunk, one per paper.  Section 3.2's rule
is that a per-paper feature-test macro rides with the commit implementing
that paper, so this is cut once per macro.

Piece 0 is not purely base: `__cpp_contracts` is defined by an if/else whose
first arm raises the value to `202606L` when P3097 is on.  The two arms are
one statement and cannot be divided, so the P3097 bump stays with the base
facility, where it is inert until P3097 turns the LangOpt on.

The last piece is P3100's: `__clang_contracts_allow_assume` reflects
`-fcontracts-allow-assume`, which exists only because P3100 can route a
check to the assume semantic.

- +/^  if \(LangOpts\.ContractsP3099\)$/
- +/^  if \(LangOpts\.ContractsP3098\)$/
- +/^  if \(LangOpts\.ContractsP3290\)$/
- +/^  if \(LangOpts\.ContractsP3400\)$/
- +/^  if \(LangOpts\.ContractsP4283\)$/
- +/^  if \(LangOpts\.ContractsP3100\)$/
- +/^  if \(LangOpts\.ContractsP4298\)$/
- +/^  if \(LangOpts\.ContractsP4301\)$/
- +/^  if \(LangOpts\.ContractOpts\.AllowAssume\)$/

## clang/lib/AST/ExprConstant.cpp#13

`HandleFunctionCall`'s prologue runs the preconditions and then opens the
P3098 capture scope.  The two are adjacent and in one segment, but only the
first belongs to the base facility: binding a postcondition capture in the
call frame is P3098's, and so is the `CallScopeRAII` that keeps it alive
past the body's own block scopes.

Cut at the P3098 comment, so `4000-p3098` carries its own entry sequencing
and `1090-p2900-base-sema-constexpr` keeps only the precondition call.

- +/^  \/\/ P3098: postcondition captures must be destroyed/

## clang/lib/AST/ExprConstant.cpp#16

The same division at the other end of the function: tearing the capture
scope down is P3098's, returning the contract result is the base's.

The removed line goes with piece 1, which is what the `-` spec is for.
Upstream's `return ESR == ESR_Returned;` is replaced by the base's
`return ContractsOK;`, so the deletion has to travel with the line that
replaces it -- left on piece 0 the base commit would carry both returns,
with upstream's unreachable below the new one.

- +/^  return ContractsOK;$/ -/^  return ESR == ESR_Returned;$/
