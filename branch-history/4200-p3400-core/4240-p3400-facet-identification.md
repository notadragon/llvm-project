---
id: 4240-p3400-facet-identification
subject: '[clang][contracts][P3400] the identification facet and group names'
depends: [4200-p3400-core]
regenerates: []
fixes: []
---

## Rationale
P3400's identification facets: a label can name the groups its contracts
belong to, which is what `-fcontract-group-evaluation-semantic=` and
P3595's `group` matching then act on.

`extractGroupNames` reads the label's `group_names` member.  The
significant decision is in the comment beside it: the label expression is
**constant-evaluated as a whole** to get its value, rather than the member
being read through a pointer at run time.  A run-time read would imply a
label's group membership can change during execution and have an effect,
which it cannot -- group membership is a compile-time property, and P3400's
own example spells it that way.

That is not a theoretical preference.  The recorded consequence of getting
it wrong is that a label's group-based configuration is never applied at
all: `-fcontract-group-evaluation-semantic=safety:observe` leaves a
`safety`-labelled contract at its default.

Same two lookup rules as the other facets -- `const`-qualified, and an
inaccessible member is not a facet rather than an error.

`group_names` is read as P3400 [support.contract.control.ident] has it: a
range whose elements convert to `string_view`.  `extractGroupNames` walks it
as a range-based for would -- `begin`/`end` as members or by
argument-dependent lookup, the iterator incremented until it equals the
end -- converting each element to `std::string_view`, or to `const char*`
when `<string_view>` is not declared, so a span, an `initializer_list` or a
user-defined range is read in full.  It is read by constant-evaluating
calls to three reader functions built for it -- a constexpr function taking
`group_names` by reference, with the iterator in a local -- each walking
the range once: one counts the names, stopping one past the cap, one returns
their lengths and one all their characters, so the reading costs time
linear in the size of the names (notadragon_wg21 DECISIONS.md E190).  A
name is read within the full-expression that dereferences the iterator, so
an iterator may return its names by value.  Each name is read whole: a
`string_view` name is its `size()` bytes, NUL bytes included
(notadragon_wg21 DECISIONS.md E191 (a)), a `const char*` name the bytes
before its NUL; the configuration matches it hierarchically on all of its
bytes, so `"safety\0x"sv` is not the `safety` group and `"safety.x\0y"sv` is
within it.

A `group_names` that is not such a range gives the label no groups, as an
absent one does.  One that is, but whose reading is not a constant
expression -- undefined behaviour in its `begin`, a name that is not a
constant, the constexpr step limit -- is an error, "group_names facet of
assertion-control object T must be usable in a constant expression",
followed by the evaluator's notes; and a `group_names` of more than 4096
names, range or array, is an error, counted before any name is read
(notadragon_wg21 DECISIONS.md E189 (a), E187 (a), E190):
otherwise the label would silently be in no group, and its assertions
configured as unlabelled.  The diagnostics are issued after the reading's
SFINAE trap.

## Compile gap
Needs `4200-p3400-core` for the facet engine, and `2400-p3595` for the
group matching that consumes the names; both precede it.

None outward.

**Behaviour trap:** exactly the one above.  Missing group names are
indistinguishable from a label that declares none, so the contract is
configured as if unlabelled and nothing says so.

## Contents

- clang/lib/Sema/SemaContract.cpp : @extractGroupNames, @GroupNamesReader, /^  static constexpr uint64_t MaxNames = 4096;$/
- clang/include/clang/Basic/DiagnosticSemaKinds.td : /\bdef\s+err_contract_group_names_too_many\b/
- clang/test/Contracts/p3400-group-names-not-constant.cpp : *
- clang/test/Contracts/p3400-group-names-steps-limit.cpp : *
- clang/test/Contracts/p3400-group-names-too-many.cpp : *
- clang/test/Contracts/p3400-group-names-too-many-string-view.cpp : *
- clang/test/Contracts/p3400-group-names-read-in-full.cpp : *
- clang/test/Contracts/p3400-group-names-string-view-nul.cpp : *
- clang/test/Contracts/Runnable/p3400-group-names-string-view-nul-run.cpp : *
- clang/test/Contracts/p3400-group-names-bytes.cpp : *
- clang/test/Contracts/p3400-group-names-bytes.json : *
- clang/test/Contracts/p3400-group-names-reported-once.cpp : *
- clang/test/Contracts/Runnable/p3400-facet-group-const.cpp : *
- clang/test/Contracts/Runnable/p3400-group-basic.cpp : *
- clang/test/Contracts/Runnable/p3400-group-combined.cpp : *
- clang/test/Contracts/Runnable/p3400-group-multi.cpp : *
- clang/test/Contracts/Runnable/p3400-group-postcondition.cpp : *
- clang/test/Contracts/Runnable/p3400-group-runtime.cpp : *
- clang/test/Contracts/Runnable/p3400-group-with-facets.cpp : *
- clang/test/Contracts/p3400-facet-group.cpp : *
- clang/test/Contracts/p3400-group-config.cpp : *
- clang/test/Contracts/p3400-identification-label-range.cpp : *
- clang/test/Contracts/p3400-identification-label-range-no-header.cpp : *
- clang/test/Contracts/p3400-identification-label-string-view-range.cpp : *
- clang/test/Contracts/p3400-combined-label-nesting.cpp : *
- clang/test/Contracts/Runnable/p3400-identification-label-range-run.cpp : *
- clang/test/Contracts/Runnable/p3400-identification-label-range-run.json : *
- clang/test/Contracts/Runnable/p3400-identification-label-range-no-header-run.cpp : *
- clang/test/Contracts/Runnable/p3400-identification-label-string-view-groups.cpp : *
- clang/test/Contracts/Runnable/p3400-identification-label-pointer-array-combine.cpp : *
- libcxx/test/std/contracts/labels_identification_range.compile.pass.cpp : *
