# CLANG-803: `-fsanitize=null` does not check a downcast of `*p` to a reference with the base at a nonzero offset

**Status:** Open here; open upstream.  The P3100 check on this branch,
CLANG-802, is fixed here (M89-Clang); whether to fix the stock sanitizer
here too was DECISIONS.md E123: deferred by the user 2026-10-18
(E123 (a))
**Component:** clang:codegen
**Upstream Link:** None found (searched 2026-10-17) -- filing owed
**Affects:** measured 2026-10-17 -- 18.1.0, 22.1.6, the stock trunk nightly
(`clang-trunk-20261005`) and this branch's install, at -O0.  GCC reports
every form (gcc-trunk `-fsanitize=null`: "reference binding to null
pointer of type 'struct D2'", "member access within null pointer",
"member call on null pointer").
**D4277R1 category:** `ub:expr.unary.dereference.nullptr` (UBSan's
`-fsanitize=null`)

## Reproducer

[`clang-803-ubsan-null-downcast-ref-offset.cpp`](clang-803-ubsan-null-downcast-ref-offset.cpp),
at `-std=c++17 -fsanitize=null`, run: `bind done`, `addr done` and
`call done` print with no report before them.

## Cause

`static_cast<D2 &>(*p)` is `CXXStaticCastExpr <BaseToDerived>` over `*p`.
`CodeGenFunction::EmitCastLValue` (`CK_BaseToDerived`) forms the derived
object's address, `p` minus the base's offset (`GetAddressOfDerivedClass`
with no null check), and checks that address (`TCK_DowncastReference`); the
use -- a reference binding, a member access, a member call -- checks it
again.  For a base at a nonzero offset the address is not null where `p`
is, so neither check reports.  A base at offset zero is reported ("downcast
of null pointer of type 'D0'", then the use's check).  A fix would check the
operand, `*p`'s address, before converting, as CLANG-748's fix direction
does for an upcast.

## Upstream search (2026-10-17)

GitHub issue search on llvm/llvm-project including closed issues.  Queries:
`ubsan downcast null reference`; `fsanitize=null downcast`; `"downcast of
null pointer"`; `sanitize null static_cast reference multiple
inheritance`; `ubsan null downcast offset`.  None is this defect.

## Draft upstream report (LLVM GitHub; filing owed -- the user files)

**Summary:** `-fsanitize=null` misses a downcast of a null `*p` to a
reference when the base is at a nonzero offset

**Description:**

    struct V { virtual void h() {} int v; };
    struct X { virtual void x() {} long pad; };
    struct D2 : X, V { int d; };          // V at offset 16
    __attribute__((noinline)) int *f(V *p) { return &static_cast<D2 &>(*p).d; }
    int main() { return f(nullptr) != nullptr; }

    $ clang++ -fsanitize=null t.cpp && ./a.out; echo $?
    1

No "downcast of null pointer of type 'D2'" (nor "member access within
null pointer") is reported: the null checks are applied to the derived
object's address, `p - 16`, which is not null.  With `struct D2 : V` (the
base at offset 0) both are reported.  GCC reports it.

## Found by

M89-Clang, checking the provenance of CLANG-802 (the P3100 null check of
the same downcast, which shares `EmitTypeCheck` with `-fsanitize=null`).
