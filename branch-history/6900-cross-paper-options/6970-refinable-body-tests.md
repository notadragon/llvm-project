---
id: 6970-refinable-body-tests
subject: '[clang][contracts] refinable bodies: tests across semantics, labels, P3097, P3100 and C'
depends: [1100-p2900-base-codegen, 2400-p3595, 3000-p3097, 4200-p3400-core, 4230-p3400-facet-allowed-semantics, 4800-p4298, 5000-p4299, 6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
The tests of the refinable-body rule: the evaluation semantic of a contract
assertion may differ between translation units, also between the copies of
one inline function, template instantiation or C99 inline definition, so
what one copy's body shows -- here, that it cannot throw -- must not reach
callers that may run another copy.  The code is `1100-p2900-base-codegen`'s
guard in `GenerateCode` (no `TryMarkNoThrow` for a function whose
definition is not exact and which holds an unsettled contract evaluation),
`4230-p3400-facet-allowed-semantics`'s settledness from a label, and
`3000-p3097`'s marking of a caller whose P3097 evaluations live in its own
TU.  The tests name the semantics a configuration chooses (`2400-p3595`),
P4298's semantics, labels, P3097, P3100 and C, so they come after the last
of them, as gnu_gcc's `6970-refinable-body-fork-tests`.

* `refinable-body-nounwind.cpp` (callers of inline, template, constructor,
  member and lambda functions keep their landing pads; static, external
  and contract-free controls lose them), `refinable-body-enforce.cpp`
  (direction A, which LLVM already gets right: no fact from an inexact
  definition), `refinable-body-label.cpp`, `refinable-body-p3097.cpp`,
  `refinable-body-p3100.cpp` (implicit checks never mark),
  `C/c-refinable-body.c`.
* Multi-TU run tests (split-file): `Runnable/ignore-ipa-replaceable-*.cpp`,
  the mirrors of gnu_gcc's GCC-896 tests (CLANG-824),
  `Runnable/refinable-fork-{sem,label,wrapper}.cpp`, and
  `Runnable/refinable-body-p3097-unit-config.cpp`: DECISIONS.md E136's
  shape in Clang, a P3097 wrapper in one unit and none in another, which
  cannot go wrong (a caller found nounwind without the wrapper calls a
  noexcept function, whose checks terminate in the wrapper too), so
  gnu_gcc's GCC-904 has no Clang form.

## Compile gap
None: tests only.

## Contents
- clang/test/Contracts/refinable-body-*.cpp : *
- clang/test/Contracts/C/c-refinable-body.c : *
- clang/test/Contracts/Runnable/ignore-ipa-replaceable-*.cpp : *
- clang/test/Contracts/Runnable/refinable-fork-*.cpp : *
- clang/test/Contracts/Runnable/refinable-body-*.cpp : *
