# CLANG-8: `this` incorrectly accepted in the declaration of an explicit-object member function

**Status:** Open
**Component:** clang / Sema
**Upstream Link:** UNKNOWN -- no search for an existing upstream report has
been attempted yet

## Bug Report

[expr.prim.this]/3 forbids `this` "within the declaration" of an
explicit-object member function, but Clang accepts it in both the trailing
return type and the noexcept-specifier, while correctly rejecting the
identical shapes on `static` member functions (the control). Clang is worse
than GCC here: GCC only misses the trailing-return-type row. This is not
contracts-related, and reproduces from Clang 18.1.0 through trunk.

## Reproducer

See [`clang-08-this-in-xobj-declaration.cpp`](clang-08-this-in-xobj-declaration.cpp) in this directory.

## Our Fix

None -- genuinely upstream's. The contracts-specific analog was fixed at the
contracts call site only (not in the shared
`InitCXXThisScopeForDeclaratorIfRelevant` helper), deliberately, so this bug
stays reproducible and untouched on the branch.

## Notes

Shares its root cause with GCC-17 in the `gnu_gcc` fork -- both reports
should be filed together, cross-referencing each other.
