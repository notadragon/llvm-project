# CLANG-19: The contract attributes are undocumented, and the undocumented-list test says so

**Kind:** deferred
**Status:** Open
**Affects:** `clang/test/AST/undocumented-attrs.cpp`, which fails on this
branch in any configuration
**Workaround:** none needed for using the compiler.

## Symptom

`undocumented-attrs.cpp` asks clang-tblgen for the list of attributes with no
`Documentation` in `Attr.td` and FileCheck's it against an expected list in
strict `CHECK-NEXT` order.  This branch adds four attributes with no
documentation, so they appear at the head of the generated list and every
subsequent `CHECK-NEXT` is off by four:

```
ContractEmission
ContractGroup
ContractMessage
ContractSemantic
FormatArg        <-- the first line the test expects to see here
```

## Why it matters

Upstream treats an undocumented attribute as an omission to be fixed, not a
choice: the list in the test is a deliberately curated set of legacy
exceptions, and additions are expected to carry documentation.  Four
undocumented attributes would be raised on the first review of any patch
series that adds them.

The fix is to write `Documentation` entries in
`clang/include/clang/Basic/AttrDocs.td` for the four, which is prose work
about what each attribute means -- worth doing when their semantics are
settled rather than now.

## Why it went unnoticed

The same reason as CLANG-18: `--all` covers `clang/test/Contracts/` only, and
the whole clang suite had never been run against this branch until
`--clang-full` was added on 2026-09-16.

Ours, and there is nothing to file -- the attributes are this branch's.
