# CLANG-5: return object not destroyed when a throwing cleanup unwinds a `return` statement

**Status:** Open (deliberately unfixed)
**Component:** CodeGen / EH
**Upstream Link:** UNKNOWN -- no search for an existing upstream report has
been attempted yet

## Bug Report

When an exception is thrown while destroying a local during a `return`
statement, [except.ctor]/2 requires the already-constructed return object's
destructor to also run. Clang never runs it, leaking it in all three return
forms (named local, prvalue, copy-from-reference) at both `-O0` and `-O2`.
GCC has been correct for 5+ releases, and [except.ctor]/2's own normative
example is structurally identical to the reproducer. This is not
contracts-dependent -- it reproduces in plain C++17.

The following is the complete ready-to-file draft (title and body) for this
report:

> **Title:** Returned object is not destroyed when a local's destructor throws
> ([except.ctor]/2)
>
> **Body:**
>
> > When an exception is thrown while destroying local variables for a `return`
> > statement, [except.ctor]/2 requires the returned object's destructor to be
> > invoked.  Clang never runs it, so the returned object is leaked.  GCC has
> > done this correctly for at least five releases.
> >
> > ```c++
> > int live = 0;
> > struct Counted {
> >   Counted() { ++live; }
> >   Counted(const Counted&) { ++live; }
> >   ~Counted() { --live; }
> > };
> > struct Throw { ~Throw() noexcept(false) { throw 42; } };
> >
> > Counted f() { Throw g; Counted r; return r; }
> >
> > int main() {
> >   try { f(); } catch (int) {}
> >   return live;          // expected 0; clang gives 1
> > }
> > ```
> >
> > `clang++ -std=c++17 t.cpp && ./a.out; echo $?` prints `1`.  The same program
> > built with g++ prints `0`.
> >
> > This is not an elision artifact and not optimization-dependent.  All three
> > return forms leak, at `-O0` and `-O2` alike: a named local (NRVO candidate),
> > a prvalue (guaranteed elision in C++17), and a copy from a reference where no
> > elision is possible.  [attach `clang-05b-retval-leak-all-return-forms.cpp`,
> > which reports all three in one run as a bitmask]
> >
> > **What the standard requires** -- [except.ctor]/2:
> >
> > > Each object with automatic storage duration is destroyed if it has been
> > > constructed, but not yet destroyed, since the try block was entered.  **If
> > > an exception is thrown during the destruction of temporaries or local
> > > variables for a `return` statement, the destructor for the returned object
> > > (if any) is also invoked.**
> >
> > The second sentence exists precisely because the returned object is not an
> > object of automatic storage duration in the callee, so the first sentence
> > does not reach it.  The paragraph's own example is this program:
> >
> > ```c++
> > struct A { };
> > struct Y { ~Y() noexcept(false) { throw 0; } };
> > A f() { try { A a; Y y; A b; return {}; } catch (...) { } return {}; }
> > ```
> >
> > > "Next, the local variable `y` is destroyed, causing stack unwinding,
> > > **resulting in the destruction of the returned object**, followed by the
> > > destruction of the local variable `a`."
> >
> > **Versions.**  Reproduces on clang 18.1.0, 19.1.0, 20.1.0, 21.1.0, 22.1.6 and
> > trunk (24.0.0git `4c176c47d8be`).  Does not reproduce on gcc 13.4.0, 14.4.0,
> > 15.3.0, 16.2.0, or trunk (17.0.0 20260901).
> >
> > **Note for whoever picks this up.**  The obvious fix is not free.  GCC handles
> > it with the `current_retval_sentinel` machinery in `cp/except.cc`, created by
> > `maybe_set_retval_sentinel` only when `cp_function_chain->throwing_cleanup` is
> > set or there is an NRV candidate -- i.e. decided *after* the body is parsed.
> > Clang's `StartFunction` runs before body emission, and a cleanup pushed at the
> > `return` statement lands innermost, below the local cleanups that actually
> > throw, so it never fires.  Making it body-wide works, but
> > `EHScopeStack::requiresLandingPad()` returns true for any cleanup on the
> > stack, so every function returning a class with a non-trivial destructor gains
> > a personality function, an exception slot and invokes -- measured on
> > `clang/test/CodeGenCXX/trivial_abi.cpp`, an ordinary `Small t; return t;`
> > picked up EH scaffolding it previously had none of.

## Reproducer

See
[`clang-05-retval-not-destroyed-on-throwing-cleanup.cpp`](clang-05-retval-not-destroyed-on-throwing-cleanup.cpp)
and
[`clang-05b-retval-leak-all-return-forms.cpp`](clang-05b-retval-leak-all-return-forms.cpp)
in this directory.

## Our Fix

None yet. A body-wide cleanup is the correct fix, but
`EHScopeStack::requiresLandingPad()` treats any cleanup on the stack as
requiring a landing pad, so a body-wide cleanup would add EH scaffolding
(personality function, landing pads) to every function returning a class
with a non-trivial destructor -- measured on
`clang/test/CodeGenCXX/trivial_abi.cpp`. This cost was judged too broad to
pay for a general fix, so the general case is deliberately scoped out and
left open here.

Only the unrelated contracts-specific half of this problem (a violation
handler throwing out of a postcondition) was fixed, at commit
`667256c8dd6f`, via a different code path (the epilogue) that doesn't pay
this cost.
