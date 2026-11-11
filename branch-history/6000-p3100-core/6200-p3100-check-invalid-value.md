---
id: 6200-p3100-check-invalid-value
subject: '[clang][contracts][P3100] implicit check: invalid value'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
Loading a value with an invalid representation -- a `bool` that is neither
`0` nor `1`, an enumeration outside its range -- as an implicit contract
assertion of `ub:conv.lval.valid.representation`, reported as its leaf
`.bool` or `.enum` by the type loaded.

`EmitImplicitInvalidValueGuard` is emitted at the load, which is the only
place the question can be asked: by the time the value is used it has
already been assumed valid by whatever consumed it.

Fourteen files, because "invalid representation" covers several
unrelated type categories and each reaches the guard from a different
conversion path.

The tests `p3100-ub-leaves*` configure each leaf of this check and of the
shift, division and signed-overflow checks on its own, and the names these
checks reported before their leaves as the groups selecting them
(`ub:conv.lval.valid.representation.bool.enum` is a combined group, see
`2400-p3595`); `p3100-precedence-leaf.cpp` shows the routed sanitizer
checking what an unconfigured leaf leaves.  They are here, after all four
checks.

## Compile gap
Needs `6000-p3100-core`; it precedes this commit.

Related to `6310-p3100-check-enum-cast`, which is the *cast* form of the
same question -- this one is about loading a value that is already wrong,
that one about producing one.  They are separate checks in UBSan's taxonomy
and separately configurable, so they are separate commits.

**Behaviour trap:** an unguarded invalid load is not merely unreported --
the optimiser is entitled to use the value's declared range, so the
observable behaviour can differ arbitrarily from what the source says.
Omitting this commit does not leave the program unchanged; it leaves it
undefined, as before.

## Contents

- clang/lib/CodeGen/CGContracts.cpp : /EmitImplicitInvalidValueGuard/
- clang/test/Contracts/Runnable/p3100-bool-route-observe.cpp : *
- clang/test/Contracts/p3100-invalid-value-codegen.cpp : *
- clang/test/Contracts/p3100-invalid-value-codegen.json : *
- clang/test/Contracts/p3100-invalid-value-enforce.cpp : *
- clang/test/Contracts/p3100-invalid-value-enforce.json : *
- clang/test/Contracts/p3100-invalid-value-noexcept.cpp : *
- clang/test/Contracts/p3100-invalid-value-noexcept.json : *
- clang/test/Contracts/p3100-invalid-value-quick.cpp : *
- clang/test/Contracts/p3100-invalid-value-quick.json : *
- clang/test/Contracts/p3100-invalid-value-throw-nonnoexcept.cpp : *
- clang/test/Contracts/p3100-invalid-value-throw-nonnoexcept.json : *
- clang/test/Contracts/p3100-invalid-value.cpp : *
- clang/test/Contracts/p3100-invalid-value.json : *
- clang/test/Contracts/Runnable/p3100-ub-leaves.h : *
- clang/test/Contracts/Runnable/p3100-ub-leaves.cpp : *
- clang/test/Contracts/Runnable/p3100-ub-leaves-select.cpp : *
- clang/test/Contracts/Runnable/p3100-ub-leaves-groups.cpp : *
- clang/test/Contracts/Runnable/p3100-ub-leaves-groups.json : *
- clang/test/Contracts/Runnable/p3100-precedence-leaf.cpp : *
