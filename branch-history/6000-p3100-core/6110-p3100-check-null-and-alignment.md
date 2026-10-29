---
id: 6110-p3100-check-null-and-alignment
subject: '[clang][contracts][P3100] implicit checks: null dereference and misalignment'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
Dereferencing a null pointer (`ub:expr.unary.dereference.nullptr`) and
accessing through a misaligned one, as implicit contract assertions.

**Two checks in one commit because they are one code path.**  Both guards
are emitted at the same place in `CGExpr.cpp` -- wherever a pointer is
about to be accessed through -- and both answer the same structural
question about the access.  Splitting them would mean two commits editing
the same twenty call sites.

The shape of each guard is worth reading once, because the rest of the
implicit-check band follows it:

* **`assume` and `ignore` return `false` and emit nothing**, leaving the
  raw access byte-identical to a build without P3100.  That is not an
  optimisation; it is the definition of those semantics.
* **The reaction is a void statement, not a value substitution.**  `*p` is
  an lvalue and there is no defined substitute for it, so unlike
  `6230-p3100-check-flow-off` -- which can supply a return value -- there
  is nothing to continue *with*.  Under `observe` the handler runs and then
  execution proceeds into the real, still-null dereference.

That last point is the one to be sure of before reading the tests:
`observe` here reports and then does the undefined thing anyway.  It buys a
diagnostic, not safety.

39 files, the largest per-check set in the band, because the access shapes
that reach these guards -- member access, array subscript, reference
binding, a `this` that is null, an access through a cast -- each arrive at
`CGExpr.cpp` by a different route.

**Each check reports one leaf of its group**, so that each leaf is
configurable on its own; a configuration naming the group still selects
both, since group matching is by dotted prefix.  A null pointer is
`ub:expr.unary.dereference.nullptr.access` at a load or store, including a
member access that a load or store follows (`p->m` read or written,
`p->arr[i]` and `(p->*pa)[i]` read, `p->*pm` and `(*p).*pm` read:
`P3100MemberAccessed` carries that answer from `EmitCheckedLValue` to the
member access), and
`.indirection` at a reference binding, a member call and a member access
whose address alone is used (`&p->n`, and `&p[i].n`, for which
`P3100SubscriptIndirection` tells the subscript's guard).  **One null
dereference, one report:** a member subobject
(`isP3100CheckedMemberSubobject`: a class member access, a `->*` or `.*`
access, an element of a member array) is not null-checked again where it
is loaded, stored, bound to a reference or called on, since its member
access checked the object pointer.  **A member of a base class** (`p->b`,
`(*p).b`, `p->f()`, `B &r = *p`, `(*p).*pm`) is reached through a
conversion of the object to the base, which for a virtual base loads the
base's offset through the object and for a base at a nonzero offset is not
at a null address; so the object is checked before the conversion, as it
would be unconverted but for null only, and the converted address is not
checked for null again (its alignment, object-size and vptr checks stay;
the vptr check keeps its own null test, `EmitTypeCheck` taking a null
check skipped under P3100 as no guarantee, but for `this` and an alloca):
`EmitCheckedLValue` checks a glvalue conversion's operand itself, and the
other uses -- a member access through a pointer, a member call, a
reference binding, a `.*` call -- name the conversion in a
`P3100BaseConversionUse`, whose operand `EmitPointerWithAlignment` or
`EmitCastLValue` checks.  Every use of a virtual base's member is
`.access`.  **A pointer to function**: a call through `*fp`, `(*fp)()`,
and a reference bound to `*fp` check `fp` under `.indirection`
(`EmitImplicitFunctionIndirectionGuard`, from `EmitCallee` and
`EmitReferenceBindingToExpr`, whose type check skips a glvalue of function
type), also through a cast to a reference, a conditional or a comma
expression yielding `*fp`; a function named directly, `(*fn)()`, is not
checked.  **An indirection whose result is not accessed** is undefined
too, and `.indirection`: `EmitUnaccessedLValue` emits the operand of `&`
and of `std::addressof` and a discarded expression and checks the pointer
when the glvalue is an indirection (`isP3100IndirectionGLValue`: `&*p`,
`*p;`, through a cast to a reference, a conditional or a comma
expression); `&p[i]` checks `p`, as an accessed `p[i]` does; a conversion
of `*p` to a base checks `p` before converting.  **The element's address
checks the pointer:** a use of `p[i]` that null-checks the element's
address -- a reference bound to it, a member function called on it, a
conversion of it to a base -- and every use of an indirection through
pointer arithmetic, `*(p + i)`, constant or not, and through a chain of it,
`*(p + 1 + 2)`, `(p + 1)[i]` (`P3100SubscriptBaseArith`), and through a
conversion of the sum to the base whose member a pointer to member is,
`(p + i)->*pm`, and every use of a downcast of `*p` to a reference,
`static_cast<D &>(*p)`, check `p` instead (`P3100ElementBase`, written by
`EmitArraySubscriptExpr` and `EmitPointerArithmetic`, each sum of a chain
mapped to the one it was formed from, and by `mapP3100ElementBase` for a
conversion's result, read by `EmitImplicitNullDerefGuard` to the innermost
pointer), since `p + i` (the derived object, its base at a nonzero
offset) is not null even where `p` is, and the optimizer
would otherwise fold the check of an inbounds `p + i` into one of `p` at
-O2 only (DECISIONS.md E99 (a), E103 (a)); the downcast's entry is marked,
and the vptr check of the downcast object, the downcast's or a use's,
tests `p` for null where a leaf of the group is checked
(`getP3100DowncastOperand`, `isP3100NullCheckMade`; DECISIONS.md
E116 (a)), rather than read through the object's address and fault before
the report.  **Reading the vtable pointer
is an access** (DECISIONS.md E102 (a)): a call to a virtual function that
is not qualified, devirtualized or not (the leaf follows the source,
DECISIONS.md E110 (a)), checks the object under `.access`
(`P3100VTableRead`, also through a base conversion and for a member subobject's access); a
`dynamic_cast` to a reference type emits its operand with
`EmitP3100VTableReadLValue`, `.access`; a call through a pointer to member
function of a polymorphic class checks the object after the pointer is
evaluated, under the leaf its virtual bit selects
(`CGCXXABI::EmitMemberFunctionPointerIsVirtual`), and so, on a member
subobject (`(p->m.*pmf)()`), is the member access's check of `p`, and,
through `.*` on the object converted to the base whose member `pmf` is
(`((*p).*pmf)()`, `(p[1].*pmf)()`), the conversion's, which
`P3100DeferNull` defers: the member access, subscript or base conversion
it is passed down to records the pointer in `EmitImplicitNullDerefGuard`
instead of checking it; the call's `-fsanitize=vptr` check of the
subobject, which reads through `p`, follows it.  Either check takes the
object's `-fsanitize=null` check with it (`P3100MoveSanitizerNull`,
`P3100DeferredNullCheck::SanPtr`), made by
`EmitP3100MovedSanitizerNullCheck` only where the P3100 check resolves to
`assume` or `ignore`, so the null dereference is reported once
(DECISIONS.md E111 (a)).  `EmitIgnoredMemberBase`
checks the object pointer of a static member, an enumerator or a
pseudo-destructor named through `->` (`sp->sf()`, `sp->k`), and of a
trivial destructor called through a pointer or `*p` (`sp->~S()`,
`(*sp).~S()`), checking a pointer converted to a base before the
conversion.  A trivial assignment called through a pointer,
`sp->operator=(s)`, is a store through it, `.access`, as `*sp = s` is.  The
trivial copy of an empty class, loaded or stored, accesses no object, so its
check is `.indirection`, and so is that of an empty non-virtual base of the
object (`P3100EmptyCopy`), also a store to an element of an array of
them.  A copy of a class member or array element reads it, `.access`,
also under a qualification conversion (`P3100SubscriptUse` tells a
subscript so) or through `std::move`.  A subscript of the array a pointer
to an array points to, `(*pa)[i]`, is an indirection through `pa`,
checked as `p[i]` checks `p`.  A conversion to a virtual base
reads the base's offset through the object, so it is `.access` whatever
its result is used for: the member access, or the subscript, that is its
operand is checked as accessed.  `this`, the
address of an object, an array and a reference are not checked, nor is
anything through `this` (`EmitImplicitNullDerefGuard` returns for
`CXXThisValue`, which in a lambda is the captured `this`, and an element
of `this[i]` is mapped to `this`): a read or write (`(*this).m`,
`this[i].m`, `this->*pm`, a copy of `*this`; DECISIONS.md E100 (a)), a
reference bound to `*this` or a member, a member's address (E106 (a)),
since a null `this` was reported where the member function was called,
and a
glvalue that is already a reference (`isP3100ReferenceGLValue`: a reference
variable, a reference member, a call returning a reference) indirects
through nothing: a binding to it, a conversion of it and an access through
it get no null guard (`P3100SkipNullGuard`), so one null dereference bound
to a reference is reported once.  **A fold
keeps the check:** an evaluation that is not manifestly constant-evaluated
gives up on an indirection through a null pointer
(`refuseP3100NullIndirection` in `ExprConstant.cpp`: `*p`, `p->m`, `p[i]`,
a static member's object expression), so CodeGen does not fold a constexpr
call whose run-time check would fail; manifestly constant evaluation is
unchanged.  Every alignment check is at a typed
access, so it is `ub:basic.align.object.alignment.store`; a creation that
performs no store (`.nostore`) is not checked.  **The required alignment
is the type's, not the address's:** an explicit cast to a pointer type
keeps an alignment derived from a declaration (`(int *)(buf + o)`,
`(int *)&buf[o]`, `buf` a character array: the element's, 1), right for
code generation but not what the access requires; the cast records the
cast-to type's alignment (`P3100CastAlign`), carried to a member or an
element of the object, and the check of the access through that pointer
requires it.  The sanitizer's own check keeps the derived alignment.

## Compile gap
Needs `6000-p3100-core` for `resolveImplicitContractSemantic` and
`emitImplicitGuardReaction`; it precedes this commit.

`CodeGenFunction.h`'s changes are the two `sanitizePerformTypeCheck` gates,
the `P3100SubscriptBaseGuarded`, `P3100MemberAccessed`,
`P3100SubscriptIndirection`, `P3100SubscriptUse`, `P3100NullCheckOnly`,
`P3100SkipNullGuard`, `P3100EmptyCopy`, `P3100DeferNull` and
`P3100MoveSanitizerNull` flags (with `P3100DeferredNullCheck`),
`P3100ElementBase`,
`P3100SubscriptBaseArith`, `isP3100NullCheckedArith`,
`isP3100CheckedMemberSubobject` and `isP3100ReferenceGLValue`;
`P3100BaseConversionUse` and its helpers are declared in a later hunk
claimed by `1100-p2900-base-codegen`, next to the null guard's
declaration, as are `EmitImplicitFunctionIndirectionGuard`,
`EmitImplicitIndirectionGuard`, `EmitUnaccessedLValue`,
`EmitP3100VTableReadLValue`, `EmitP3100MovedSanitizerNullCheck`,
`EmitIgnoredMemberBase` and the two predicates, and the `GuardOnly` and
`DeferNull` fields.
The hooks that call `EmitUnaccessedLValue` and `EmitIgnoredMemberBase` in
`CGExprScalar.cpp` (`&`, a static member's constant) are claimed whole by
`6000-p3100-core`, which therefore uses them before this commit defines
them; so is `EmitPointerArithmetic`, which defines `isP3100NullCheckedArith`
and maps the arithmetic an indirection or subscript goes through for the
null check (`P3100SubscriptBaseArith`, `P3100ElementBase`).

`P3100ElementBase` is declared in the `CodeGenFunction.h` hunk
`6340-p3100-check-pointer-arithmetic` claims (beside `P3100KnownArith`),
and the member-call and pointer-to-member paths in `CGExprCXX.cpp` set
that commit's `P3100IndirectedArith`; both are used here before that
commit declares them.

None outward.

**Behaviour trap:** both guards return a bool saying whether they emitted
anything, and the caller uses it to decide whether to also emit the
sanitizer's own check.  Ignoring the return value gives two reports for one
access under `-fsanitize=null`, which looks like a runtime bug and is a
CodeGen one.

## Contents

- clang/lib/CodeGen/CGContracts.cpp : @EmitImplicitNullDerefGuard, @isP3100NullCheckMade, @EmitP3100MovedSanitizerNullCheck, @isP3100NonNullPointerOperand, @isP3100IndirectionGLValue, @EmitImplicitIndirectionGuard, @EmitImplicitFunctionIndirectionGuard, @EmitImplicitMisalignedGuard
- clang/lib/CodeGen/CGExpr.cpp : /^(?!\s*$)(?!.*\b[PD]4405\b)(?!\s*DiagnoseCallerSideProvingContracts\()/
- clang/lib/CodeGen/CodeGenFunction.h : #3, #5, #6
- clang/lib/CodeGen/CGStmt.cpp : #1
- clang/lib/CodeGen/CGCXXABI.h : *
- clang/lib/CodeGen/ItaniumCXXABI.cpp : *
- clang/lib/CodeGen/CGExprCXX.cpp : #0, #1, #3, #4, #5, #6, #7:0, #8, #13, #14
- clang/lib/CodeGen/CGExprComplex.cpp : #0
- clang/lib/CodeGen/CGBuiltin.cpp : #0
- clang/lib/AST/ExprConstant.cpp : #11, #39, #40, #41, #42, #46, #47, #48, #49, #50
- clang/test/Contracts/Runnable/p3100-align-aligned.cpp : *
- clang/test/Contracts/Runnable/p3100-align-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-align-enforce.json : *
- clang/test/Contracts/Runnable/p3100-align-ignore.cpp : *
- clang/test/Contracts/Runnable/p3100-align-ignore.json : *
- clang/test/Contracts/Runnable/p3100-align-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-align-noexcept-observe.json : *
- clang/test/Contracts/Runnable/p3100-align-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-align-observe.json : *
- clang/test/Contracts/Runnable/p3100-align-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-align-quick.json : *
- clang/test/Contracts/Runnable/p3100-align-throw.cpp : *
- clang/test/Contracts/Runnable/p3100-alignment-report-ondemand.cpp : *
- clang/test/Contracts/Runnable/p3100-alignment-route-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-alignment-route-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-alignment-route-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-null-route-observe.cpp : *
- clang/test/Contracts/p3100-alignment-sanitize-semantic.cpp : *
- clang/test/Contracts/p3100-null-deref-addr-of.cpp : *
- clang/test/Contracts/p3100-null-deref-assume.cpp : *
- clang/test/Contracts/p3100-null-deref-noexcept-enforce.cpp : *
- clang/test/Contracts/p3100-null-deref-noexcept-enforce.json : *
- clang/test/Contracts/p3100-null-deref-noexcept-observe.cpp : *
- clang/test/Contracts/p3100-null-deref-noexcept-observe.json : *
- clang/test/Contracts/p3100-null-deref-observe.json : *
- clang/test/Contracts/p3100-null-deref-quick.cpp : *
- clang/test/Contracts/p3100-null-deref-quick.json : *
- clang/test/Contracts/p3100-null-deref-resolution.cpp : *
- clang/test/Contracts/p3100-null-deref-resolution.json : *
- clang/test/Contracts/p3100-null-deref-throw-cleanup.cpp : *
- clang/test/Contracts/p3100-null-deref-throw-fntryblock.cpp : *
- clang/test/Contracts/p3100-null-deref-throw-noexcept.cpp : *
- clang/test/Contracts/p3100-null-deref-throw-nonnoexcept.cpp : *
- clang/test/Contracts/p3100-null-deref-throw-nonnoexcept.json : *
- clang/test/Contracts/p3100-null-ref-and-call.cpp : *
- clang/test/Contracts/p3100-null-ref-and-call.json : *
- clang/test/Contracts/p3100-null-split.cpp : *
- clang/test/Contracts/p3100-null-split-mptr.cpp : *
- clang/test/Contracts/p3100-null-split-mptr-guarded.cpp : *
- clang/test/Contracts/Runnable/p3100-null-split-mptr-offset.cpp : *
- clang/test/Contracts/Runnable/p3100-null-split-mptr-array.cpp : *
- clang/test/Contracts/Runnable/p3100-null-member-subobject-once.cpp : *
- clang/test/Contracts/p3100-null-member-subobject-once-guarded.cpp : *
- clang/test/Contracts/Runnable/p3100-null-split-dot-mptr.cpp : *
- clang/test/Contracts/Runnable/p3100-null-split-subscript-member.cpp : *
- clang/test/Contracts/Runnable/p3100-null-base-member.cpp : *
- clang/test/Contracts/Runnable/p3100-null-function-indirection.cpp : *
- clang/test/Contracts/p3100-null-function-indirection-codegen.cpp : *
- clang/test/Contracts/Runnable/p3100-null-function-indirection-constexpr.cpp : *
- clang/test/Contracts/Runnable/p3100-null-unaccessed-indirection.cpp : *
- clang/test/Contracts/Runnable/p3100-null-unaccessed-indirection-edges.cpp : *
- clang/test/Contracts/Runnable/p3100-null-trivial-member-call.cpp : *
- clang/test/Contracts/Runnable/p3100-null-empty-class-copy.cpp : *
- clang/test/Contracts/Runnable/p3100-null-empty-class-copy-extra.cpp : *
- clang/test/Contracts/Runnable/p3100-null-binding-read.cpp : *
- clang/test/Contracts/Runnable/p3100-null-reference-rebind.cpp : *
- clang/test/Contracts/Runnable/p3100-null-reference-read.cpp : *
- clang/test/Contracts/Runnable/p3100-null-unaccessed-member.cpp : *
- clang/test/Contracts/Runnable/p3100-null-empty-array-store.cpp : *
- clang/test/Contracts/Runnable/p3100-null-vbase-member-bind.cpp : *
- clang/test/Contracts/Runnable/p3100-null-subscript-copy.cpp : *
- clang/test/Contracts/Runnable/p3100-subscript-copy-bounds.cpp : *
- clang/test/Contracts/p3100-array-bounds-copy-constexpr-base.cpp : *
- clang/test/Contracts/Runnable/p3100-null-constant-subscript.cpp : *
- clang/test/Contracts/Runnable/p3100-null-pointer-arith-constant.cpp : *
- clang/test/Contracts/Runnable/p3100-null-dynamic-cast-reference.cpp : *
- clang/test/Contracts/Runnable/p3100-null-pmf-adjust.cpp : *
- clang/test/Contracts/Runnable/p3100-null-this-read.cpp : *
- clang/test/Contracts/Runnable/p3100-null-pointer-arith-chain.cpp : *
- clang/test/Contracts/Runnable/p3100-null-virtual-call.cpp : *
- clang/test/Contracts/Runnable/p3100-null-this-indirection.cpp : *
- clang/test/Contracts/Runnable/p3100-null-constant-subscript-variable.cpp : *
- clang/test/Contracts/Runnable/p3100-null-virtual-pmf.cpp : *
- clang/test/Contracts/Runnable/p3100-null-virtual-pmf-member.cpp : *
- clang/test/Contracts/Runnable/p3100-null-pmf-member-vptr.cpp : *
- clang/test/Contracts/Runnable/p3100-null-vptr-offset-zero.cpp : *
- clang/test/Contracts/Runnable/p3100-null-ptrmem-base-arith.cpp : *
- clang/test/Contracts/Runnable/p3100-null-downcast-ref-offset.cpp : *
- clang/test/Contracts/Runnable/p3100-null-downcast-ref-offset-vptr.cpp : *
- clang/test/Contracts/Runnable/p3100-null-pmf-base.cpp : *
- clang/test/Contracts/Runnable/p3100-null-member-call-prologue.cpp : *
- clang/test/Contracts/Runnable/p3100-null-downcast-ref-sanitize-null.cpp : *
- clang/test/Contracts/Runnable/p3100-null-sanitize-null-once.cpp : *
- clang/test/Contracts/Runnable/p3100-null-vptr-sanitize-null.cpp : *
- clang/test/Contracts/Runnable/p3100-null-vptr-call.cpp : *
- clang/test/Contracts/Runnable/p3100-null-virtual-call-vbase.cpp : *
- clang/test/Contracts/Runnable/p3100-null-vptr-vbase-subscript.cpp : *
- clang/test/Contracts/Runnable/p3100-handler-effects-ipa.cpp : *
- clang/test/Contracts/Runnable/p3100-null-pmf-sanitize-null.cpp : *
- clang/test/Contracts/Runnable/p3100-null-member-call-class-return.cpp : *
- clang/test/Contracts/Runnable/p3100-align-char-array-cast.cpp : *
- clang/test/Contracts/Runnable/p3100-null-subscript-deref-array.cpp : *
- clang/test/Contracts/Runnable/p3100-null-binding-location.cpp : *
- clang/test/Contracts/Runnable/p3100-null-dynamic-cast-reference-sanitize-null.cpp : *
- clang/test/Contracts/Runnable/p3100-align-binding-arith.cpp : *
- clang/test/Contracts/Runnable/p3100-align-member-call-prologue.cpp : *
- clang/test/Contracts/Runnable/p3100-align-class-copy.h : *
- clang/test/Contracts/Runnable/p3100-align-class-copy.cpp : *
- clang/test/Contracts/Runnable/p3100-align-class-copy-sanitize.cpp : *
- clang/test/Contracts/Runnable/ubsan-ptrmem-null-once.cpp : *
- clang/test/Contracts/Runnable/p3100-null-unaccessed-indirection-fold.cpp : *
- clang/test/Contracts/p3100-null-unaccessed-indirection-codegen.cpp : *
- clang/test/Contracts/p3100-null-unaccessed-indirection-edges-codegen.cpp : *
- clang/test/Contracts/Runnable/p3100-handler-effects-inlined.cpp : *
- clang/test/Contracts/p3100-align-split.cpp : *
