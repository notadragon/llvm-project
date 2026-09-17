---
id: 6200-p3100-check-invalid-value
subject: '[clang][contracts] implicit check: invalid value'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
Loading a value with an invalid representation -- a `bool` that is neither
`0` nor `1`, an enumeration outside its range -- as an implicit contract
assertion.

`EmitImplicitInvalidValueGuard` is emitted at the load, which is the only
place the question can be asked: by the time the value is used it has
already been assumed valid by whatever consumed it.

Fourteen files, because "invalid representation" covers several
unrelated type categories and each reaches the guard from a different
conversion path.

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
