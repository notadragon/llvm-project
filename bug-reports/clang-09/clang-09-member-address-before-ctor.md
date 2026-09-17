# CLANG-9: constexpr evaluation accepts forming a member's address before its non-trivial constructor begins (non-virtual-base and direct-member shapes)

**Status:** Open
**Component:** clang / constexpr
**Upstream Link:** [#211286](https://github.com/llvm/llvm-project/issues/211286) -- **PARTIAL, not this bug.** "Calling
member function before base subobject initialization in constant evaluation
not rejected", open and `confirmed`. Same family (a subobject used before its
initialization is not diagnosed in constant evaluation) but a different shape:
it CALLS a member function where this one FORMS AN ADDRESS.

Note the same report was filed against **both** trackers with identical text --
it is [PR126357](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126357) on the
GCC side, linked the same way from GCC-2 there. Filing this one should cite
it as related rather than duplicate it. Searched 2026-09-05; nothing covering
the address-formation shape was found
been attempted yet

## Bug Report

[class.cdtor]/1 makes referring to any non-static member or base of an
object with a non-trivial constructor, before that constructor begins,
undefined -- with no carve-out for virtual versus non-virtual bases and
none distinguishing forming an address from reading. CLANG-1 in this
directory covers one shape of this defect: a virtual-base member, which
this branch now rejects (CLANG-1's fix); upstream Clang still accepts it.
Two more shapes of the same defect remain accepted:

- (b) a non-virtual base member of a class with a non-trivial constructor,
  address formed in an enclosing constructor's init-list before the member
  is built.
- (c) a direct member of a class with a non-trivial constructor, address
  formed in an enclosing constructor's init-list before the member is
  built.

Both are undefined for the same reason as the virtual-base shape, but
Clang (like GCC) currently accepts them.

**Versions.** Measured 2026-09-04 with `-std=c++26 -fsyntax-only` against
the reproducer below:

| Clang version | Result |
|---|---|
| 18.1.0 | ACCEPTED (bug) |
| 19.1.0 | ACCEPTED (bug) |
| 20.1.0 | ACCEPTED (bug) |
| 21.1.0 | ACCEPTED (bug) |
| 22.1.6 | ACCEPTED (bug) |
| trunk | ACCEPTED (bug) |

Both shapes (b) and (c) are accepted, unchanged, across every version
tested including today's trunk.

## Reproducer

See [`clang-09-member-address-before-ctor.cpp`](clang-09-member-address-before-ctor.cpp) in this directory.

## Our Fix

None -- deliberately. This is a core-language constant-evaluator change
owed to the compiler, not something to patch on this branch. GCC shares
this defect (tracked as GCC-2 in the `gnu_gcc` fork), and the report to
file covers both compilers together.

## Notes

This is the still-open remainder of the same defect whose row (a)
(virtual-base member) is CLANG-1, already fixed here. GCC's counterpart,
covering all four rows of the same table, is tracked as GCC-2 in the
`gnu_gcc` fork.
