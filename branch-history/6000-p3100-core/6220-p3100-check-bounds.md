---
id: 6220-p3100-check-bounds
subject: '[clang][contracts] implicit check: array bounds'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
An array subscript outside the array's bounds as an implicit contract
assertion.

`EmitImplicitArrayBoundsGuard` is emitted where the bound is statically
known, which is the same restriction UBSan's own `bounds` check operates
under -- neither can say anything about a pointer whose provenance is lost.

Seventeen files: the shapes that carry a known bound (a declared
array, a `std::array`, a member array, a multidimensional one, a VLA with a
computed bound) reach the guard differently, and the guard has to produce
the same answer for all of them.

## Compile gap
Needs `6000-p3100-core`; it precedes this commit.

None outward.

**Behaviour trap:** the guard is emitted only where a bound is known, so a
test that indexes through a decayed pointer silently exercises nothing.
That is a property of the check rather than of this commit, but it is the
reason the test list enumerates shapes rather than operations.

## Contents

- clang/lib/CodeGen/CGContracts.cpp : @EmitImplicitArrayBoundsGuard
- clang/test/Contracts/Runnable/p3100-bounds-route-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-local-bounds-route-observe.cpp : *
- clang/test/Contracts/p3100-array-bounds-codegen.cpp : *
- clang/test/Contracts/p3100-array-bounds-codegen.json : *
- clang/test/Contracts/p3100-array-bounds-enforce.cpp : *
- clang/test/Contracts/p3100-array-bounds-enforce.json : *
- clang/test/Contracts/p3100-array-bounds-noexcept.cpp : *
- clang/test/Contracts/p3100-array-bounds-noexcept.json : *
- clang/test/Contracts/p3100-array-bounds-onepast.cpp : *
- clang/test/Contracts/p3100-array-bounds-onepast.json : *
- clang/test/Contracts/p3100-array-bounds-quick.cpp : *
- clang/test/Contracts/p3100-array-bounds-quick.json : *
- clang/test/Contracts/p3100-array-bounds-throw-nonnoexcept.cpp : *
- clang/test/Contracts/p3100-array-bounds-throw-nonnoexcept.json : *
- clang/test/Contracts/p3100-array-bounds.cpp : *
- clang/test/Contracts/p3100-array-bounds.json : *
