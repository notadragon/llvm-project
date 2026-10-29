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

## clang/include/clang/Basic/TokenKinds.def#1

One hunk declares three features' keywords in a row: the base facility's
`contract_assert` and its `__contract_assert` alias, P3400's
`contract_control`, and P4299's C spellings `_Pre` / `_Post` /
`_ContractAssert`.  Only adjacency joins them.

- +/^KEYWORD\(contract_control /
- +/^\/\/ C contracts keywords \(P4299\)$/

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

## clang/lib/Sema/TreeTransform.h#3

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
- +/^  StmtResult Result = getDerived\(\)\.RebuildContractStmt\($/

## clang/lib/Frontend/InitPreprocessor.cpp#0

Twelve feature-test macros in one hunk, one per paper.  Section 3.2's rule
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
- +/^  if \(LangOpts\.ContractsP4405\)$/
- +/^  if \(LangOpts\.ContractOpts\.AllowAssume\)$/


## clang/lib/Frontend/InitPreprocessor.cpp#1

The C side of the predefined contracts macros: P3100's
`__clang_contracts_allow_assume` for P4299 C contracts, then P4299's own
`__clang_contracts_p4299`, which rides with the P4299 commit.

- +/^  \/\/ P4299's keywords need no header/
## clang/lib/AST/ExprConstant.cpp#21

`HandleFunctionCall`'s prologue runs the preconditions and then opens the
P3098 capture scope.  The two are adjacent and in one segment, but only the
first belongs to the base facility: binding a postcondition capture in the
call frame is P3098's, and so is the `CallScopeRAII` that keeps it alive
past the body's own block scopes.

Cut at the P3098 comment, so `4000-p3098` carries its own entry sequencing
and `1090-p2900-base-sema-constexpr` keeps only the precondition call.

- +/^  \/\/ P3098: postcondition captures must be destroyed/

## clang/lib/AST/ExprConstant.cpp#24

The same division at the other end of the function: tearing the capture
scope down is P3098's, returning the contract result is the base's.

The removed line goes with piece 1, which is what the `-` spec is for.
Upstream's `return ESR == ESR_Returned;` is replaced by the base's
`return ContractsOK;`, so the deletion has to travel with the line that
replaces it -- left on piece 0 the base commit would carry both returns,
with upstream's unreachable below the new one.

- +/^  return ContractsOK;$/ -/^  return ESR == ESR_Returned;$/

## clang/lib/AST/StmtCXX.cpp#1.5

`getUserMessage` falls back to `[[clang::contract_message]]` when the
assertion has no message of its own.  The attribute is `2400-p3595`'s, so
the fallback is too; the base facility's message is the syntactic one.

- +/^  if \(auto \*A = getAttr<ContractMessageAttr>\(\)\)$/
- +/^  return \{\};$/

## clang/lib/AST/StmtCXX.cpp#1.14

`getConfigGroups` falls back to a `[[clang::contract_group]]` group for
the configuration query.  The attribute is `2400-p3595`'s; without it the
query has no groups.

- +/^  if \(auto \*A = getAttr<ContractGroupAttr>\(\)\)$/
- +/^  return Storage;$/

## clang/lib/AST/ASTContext.cpp#1.2

The violation object of a contract carries a message when the assertion
has one; `[[clang::contract_message]]` is one, and is `2400-p3595`'s.

- +/^  if \(CS->getAttr<ContractMessageAttr>\(\)\)$/
- +/^  if \(HasMessage\) \{$/

## clang/lib/Sema/SemaContract.cpp#0.42

`ActOnContractAssert` rejects a syntactic message together with
`[[clang::contract_message]]`, a check that belongs with the attribute in
`2400-p3595`.

- +/^  if \(MessageExpr\) \{$/
- +/^  StmtResult Res =$/

## clang/lib/Sema/SemaContract.cpp#0.53

Two declarations' contracts differ in their message when one has
`[[clang::contract_message]]` and the other does not; that test is
`2400-p3595`'s, with the attribute.

- +/^        if \(C->getAttr<ContractMessageAttr>\(\)\)$/
- +/^        return C->hasMessage\(\);$/

## clang/lib/CodeGen/CGCall.cpp#3

`EmitPostContracts` is the base facility's, but its tail makes P3100's
`[[noreturn]]` check, which a `[[noreturn]]` function that returns owes
after its postconditions have been evaluated
(`6380-p3100-check-noreturn`), and its head holds the object that, on the
way out, makes the obligations of a virtual function's override pairs
(`4940-p4405-override`).

- +/^  \/\/ D4405: then the obligations of a virtual function's override pairs\.$/
- +/^  if \(!FD \|\| !FD->hasContracts\(\) \|\| structorVariantDelegates\(\)\)$/
- +/^  \/\/ P3100: a \[\[noreturn\]\] function's return is checked after its$/
- +/^  return RV;$/

## clang/lib/CodeGen/CGClass.cpp#0

`structorVariantDelegates` is the base facility's; the early exit for the
override shadow of a complete-object destructor, which runs the whole
destructor instead of calling the base-object one, is
`4940-p4405-override`'s.

- +/^      \/\/ D4405: an override shadow runs the whole destructor\.$/
- +/^      return !isa_and_nonnull<CXXTryStmt>\(Dtor->getBody\(\)\);$/

## clang/lib/CodeGen/CGCXX.cpp#1

`TryEmitBaseDestructorAsAlias` refuses a destructor with contract
assertions, whose body evaluates them (the base facility's, CLANG-869),
and, under D4405, one overriding a destructor with contract assertions,
whose override pairs are analyzed in its own body (`4940-p4405-override`).

- +/^  \/\/ D4405: a virtual destructor overriding one with contract assertions is$/

## clang/lib/CodeGen/CGExprCXX.cpp#2

Two lexical P3100 checks of a member function called on `*this`, one
statement each at the head of `EmitCXXMemberOrOperatorMemberCallExpr`: in
a constructor's or destructor's function-try-block handler
(`6390-p3100-check-handler-member`), and before the bases are initialized
(`6400-p3100-check-base-init-mem-fun`).

- +/^  \/\/ P3100: a member function of \*this called before the bases are$/

## clang/lib/CodeGen/CGExprCXX.cpp#7

The member call's type check in `EmitCXXMemberOrOperatorMemberCallExpr`:
the P3100 null check of an object converted to a base class, made by the
conversion, drops the null check that follows
(`6110-p3100-check-null-and-alignment`), right before the check's gate
widened for P3100 (`3000-p3097`).

- +/^  if \(sanitizePerformTypeCheck\(\) \|\| getLangOpts\(\)\.ContractsP3100\)$/ -/^  if \(sanitizePerformTypeCheck\(\)\)$/

## clang/lib/CodeGen/CGExprCXX.cpp#22

The virtual destructor call of a delete-expression goes through P3097's
contract wrapper when the destructor's interface contracts are evaluated
there (`3000-p3097`); ahead of it, the analysis evaluations of the
preconditions of the destructor named are placed at the call
(`4930-p4405-analysis`).  The removed lines, the plain call the wrapper
replaces, stay with the first piece.

- +/^          \/\/ D4405: the analysis evaluations of the destructor named\.$/
- +/^          CGF\.EmitVirtualDtorCallWithContracts\($/

## clang/lib/CodeGen/CGContracts.cpp#0.112

`emitCheckForSemantic`, the per-semantic check body, is the base
facility's.  The proof shadow's hook at the top, which emits an earlier
evaluation's assumption in a shadow, is `4920-p4405-shadow`'s; the
assertion that a runtime evaluation is never `prove` or `diagnose` (D4405:
those apply to the analysis evaluation, placed on its own) is
`4930-p4405-analysis`'s.

- +/^  \/\/ P4405 proving hook: in a proof shadow, an assumption\.$/
- +/^  \/\/ P3100 "assume" deliberately emits no check today, exactly like "ignore"\.$/
- +/^  \/\/ D4405: a runtime evaluation is never prove or diagnose, which apply to$/
- +/^  const auto Style = \[&\]\(\) \{$/

## clang/lib/CodeGen/CGContracts.cpp#0.94

`EmitContractStmtAsFullStmt` is the base facility's; ahead of a
postcondition's own analysis evaluation, the hook placing a virtual
function's override pairs' obligations of it is `4940-p4405-override`'s.

- +/^  \/\/ D4405: a virtual function's postcondition is analyzed first for each$/
- +/^  if \(isContractAnalyzedHere\(S\)\) \{$/

## clang/lib/CodeGen/CGContracts.cpp#0.0

The file's header and includes are the base facility's; the include of the
P4405 proving interface is `4900-p4405`'s.

- +/^#include "ContractProve\.h"$/
- +/^#include "TargetInfo\.h"$/

## clang/lib/CodeGen/CGContracts.cpp#0.120

`noteContractEvaluation`, the refinable-body note, is the base facility's:
every contract evaluation marks its function.  The early exit for an
evaluation a P3400 label settles is `4230-p3400-facet-allowed-semantics`'s.

- +/^  \/\/ P3400: a settled evaluation leaves the same choice in every copy\.$/
- +/^  CurFnHoldsUnsettledContract = true;$/

## clang/lib/CodeGen/CMakeLists.txt#1

The P4405 proving interface's source (`4900-p4405`), the folding
strategy's (`4910-p4405-fold`), the override shadows'
(`4940-p4405-override`) and the proof shadow's (`4920-p4405-shadow`), in
the list's alphabetical order.

- +/^  ContractProveFold\.cpp$/
- +/^  ContractProveOverride\.cpp$/
- +/^  ContractProveShadow\.cpp$/

## clang/lib/CodeGen/ContractProve.h#0.15

The folding strategy's declaration is `4910-p4405-fold`'s, whole: cut at
its declaration only so that `4900-p4405`'s `#0` leaves it alone.

- +/^extern const ContractProofStrategy ContractProveFoldStrategy;$/

## clang/lib/CodeGen/ContractProve.h#0.16

The proof shadow strategy's declaration is `4920-p4405-shadow`'s, whole:
cut at its declaration only so that `4900-p4405`'s `#0` leaves it alone.

- +/^extern const ContractProofStrategy ContractProveShadowStrategy;$/

## clang/lib/CodeGen/ContractProve.h#0.17

The override shadows' strategy's declaration is `4940-p4405-override`'s;
the end of the header, `4900-p4405`'s.

- +/^} \/\/ namespace CodeGen$/

## clang/lib/CodeGen/ContractProve.cpp#0.2

The registry is `4900-p4405`'s, null-terminated so that it is valid with
no strategy; the folding strategy's entry is `4910-p4405-fold`'s, the proof
shadow's `4920-p4405-shadow`'s, the override shadows'
`4940-p4405-override`'s.

- +/^    &ContractProveFoldStrategy,$/
- +/^    &ContractProveShadowStrategy,$/
- +/^    &ContractProveOverrideStrategy,$/
- +/^    nullptr,$/

## clang/lib/CodeGen/CodeGenFunction.h#2

The contract emission state is the base facility's; the proof shadow's
state and helpers in the middle of it are `4920-p4405-shadow`'s, but for
the two hooks of the override shadows (`4940-p4405-override`).

- +/^  \/\/\/ P4405 proof shadows \(ContractProveShadow\.cpp\): set while a proof shadow,$/
- +/^  \/\/\/ D4405 \(ContractProveOverride\.cpp\): at the entry of a virtual function's$/
- +/^  \/\/\/ D4405: whether the analysis evaluation of \\p S is placed where \\p S is$/
- +/^  CurrentContractInfo \*CurContract\(\);$/

## clang/lib/CodeGen/CodeGenModule.h#5.1

`diagnoseUnprovenContract` is `4900-p4405`'s; the proof shadow's
declarations after it, `4920-p4405-shadow`'s.

- +/^  \/\/\/ P4405 proof shadows \(ContractProveShadow\.cpp\): the strategy's state,$/

## clang/lib/CodeGen/CodeGenModule.h#5.2

The set of evaluations already diagnosed is `4900-p4405`'s; the proof
shadow's state after it, `4920-p4405-shadow`'s.

- +/^  \/\/\/ P4405: the proof shadow strategy's state\.$/

## clang/lib/CodeGen/CodeGenFunction.cpp#8

`GenerateCode`'s evaluation of the preconditions and postcondition
captures after the prologue is the base facility's; the hook ahead of it
that makes a virtual function's override pairs' obligations at entry is
`4940-p4405-override`'s.

- +/^  \/\/ Emit preconditions and postcondition capture initializations in$/

## clang/docs/UsersManual.md#0.51

The analysis evaluation's section is `4930-p4405-analysis`'s; the
`-fcontracts-prove-override` option at its end, written as a list so that
no line of it starts an anchor piece of its own, is `4940-p4405-override`'s.

- +/^:::\{option\} -fcontracts-prove-override$/
