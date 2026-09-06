# CLANG-14: a pure virtual's dependent contract predicate reaches CodeGen

**Status:** Open (defect)
**Kind:** defect
**Component:** CodeGen / Sema (contracts, P3097)
**Upstream Link:** `--` (P3097 virtual contracts are not upstream in Clang;
nothing to file)
**Found:** 2026-09-05, while probing for gaps after CLANG-13 -- specifically by
asking whether a pure virtual's interface contract, which GCC-34's fix had just
made *exist* on the GCC side, actually **fires** at run time.

## Symptom

```
clang/lib/AST/ExprConstant.cpp:22215:
  bool clang::Expr::EvaluateAsInt(...):
  Assertion `!isValueDependent() &&
    "Expression evaluator can't be called on a dependent expression."' failed.
```

## Reproducer

[`clang-14-pure-virtual-dependent-predicate-codegen.cpp`](clang-14-pure-virtual-dependent-predicate-codegen.cpp):

```c++
template <class T> struct A {
  virtual int get() const pre(n >= 0) = 0;
  int n;
};
struct D : A<int> { int get() const override { return n; } };
int main() { D d; A<int> &a = d; return a.get(); }
```

```
clang++ -std=c++26 -fcontracts -fcontracts-p3097 -c clang-14-...cpp
```

## What narrows it

Measured, one variable at a time:

| | result |
|---|---|
| pure virtual, class template, `pre(n >= 0)` | **crash** |
| pure virtual, class template, `pre(sizeof(T) == 4)` | **crash** |
| pure virtual, class template, `pre(f() == 0)` | OK |
| pure virtual, class template, `pre(true)` / `pre(1 == 0)` | OK |
| pure virtual, class template, `post(r: r >= 0)` | OK |
| **non-pure** virtual, class template, `pre(n >= 0)` | OK |
| pure virtual, **non-template** class, `pre(n >= 0)` | OK |

So all three of *pure*, *class template*, and a predicate that is
**value-dependent when parsed** are required. `f() == 0` is a call but not
dependent, and it passes -- which is what separates this from GCC-34, where a
call was enough.

**`-fsyntax-only` does not reproduce it.** The crash is in CodeGen. The
28-shape probe matrix that found CLANG-13 was entirely `-fsyntax-only` and
walked straight past this; it only surfaced when a *runnable* test tried to
prove a pure virtual's interface contract actually fires.

## Characterised further by the matrix (2026-09-06)

`contract-matrix-gen.py`'s `readers` group crosses the reader, the producer and
the predicate kind, and pins the boundary more sharply than the original
reduction did:

| producer | constant | call | dependent | member |
|---|---|---|---|---|
| pure virtual | ok | ok | **crash** | **crash** |
| pure virtual, explicitly instantiated | ok | ok | **crash** | **crash** |
| virtual with a definition | ok | ok | ok | ok |
| virtual defined out-of-line | ok | ok | ok | ok |

All at the codegen phase; every one of these is clean at `-fsyntax-only`.

Two things fall out. Explicit instantiation of the class template does **not**
rescue it -- the member declaration is instantiated but the pure virtual still
has no definition. An out-of-line definition **does**, which is the tell: what
matters is whether any definition is ever instantiated, because that is what
substitutes the contracts. So the condition is "no definition anywhere, plus a
predicate that needs substituting", and `member` (`n >= 0` on a member of the
dependent class) belongs in the trigger set alongside `sizeof (T)`.

## Root cause: SETTLED (2026-09-06)

The stack is unambiguous about the *site*:

```
CodeGenModule::getOrEmitVirtualContractWrapper
  CodeGenFunction::EmitVirtualContractWrapperBody
    CodeGenFunction::EmitContractStmtAsFullStmt
      CodeGenFunction::emitCheckForSemantic
        EmitScalarExpr  ->  ScalarExprEmitter::VisitMemberExpr  ->  assert
```

The P3097 wrapper reads the method's contracts at CodeGen and emits the
predicate. For a pure virtual they are still the pattern's dependent tree, so
this is the same shape as GCC-34 -- a reader that does not need a definition,
looking at contracts nobody substituted -- reached in Clang at a later stage.

**What is measured:**

* `MarkFunctionReferenced` has no pure-virtual early return before the
  `OdrUse == OdrUseContext::Used` branch that calls
  `InstantiateFunctionContractsOnUse`, so the hook is reached for this call.
* Forcing an unambiguous odr-use first (`auto p = &A<int>::get;`) does **not**
  help, nor does an explicit instantiation of the class template, nor giving
  the pure virtual an out-of-line definition.
* A **non-pure** virtual in a class template with no definition anywhere is
  fine. Pure-ness is the discriminator, not absence of a definition.

**Neither guess was right.** Measured with temporary tracing on a release
build -- no debug build needed, contrary to the earlier note here.

`InstantiateFunctionContractsOnUse` is **never called for the pure virtual at
all**. Tracing every entry to it while compiling the reproducer shows only the
constructors; `A<int>::get` never appears. Tracing `MarkFunctionReferenced`
instead shows why:

```
CRASHING (pure):     A<int>::get (pure)  OdrUse=0   <- None
WORKING  (non-pure): A<int>::get         OdrUse=3   <- Used
```

And Clang is **right** to say so. `MarkExprReferenced`'s caller
(`SemaExpr.cpp`, ~21252) passes `MightBeOdrUse = false` for a virtual dispatch
to a pure virtual, quoting the rule in its own comment:

> ... is odr-used, unless it is a pure virtual function and its name is not
> explicitly qualified.

So `OdrUse` is `None`, and the hook -- gated on `OdrUse == OdrUseContext::Used`
-- never runs. The non-pure case only works incidentally: its definition is
odr-used by the vtable, which is a *different* odr-use that happens to
substitute the contracts first.

The gating is what defeats the intent already written into the hook's own
comment: "a virtual function needs this ... so they must exist as a
non-dependent specifier even when the function's own definition is never
instantiated". The definition is genuinely not needed; the **contracts** are,
because P3097 evaluates them in the wrapper around the dispatch.

## There is probably a wording defect behind this

The exception specification of the same pure virtual **is** instantiated and
validated at the same call, in both compilers. Measured with no contracts
involved, plain C++23:

```c++
template <class T> struct A { virtual void g () noexcept (T::nonexistent) = 0; };
void use (A<int> *p) { p->g (); }     // gcc: error   clang: error
void nocall (A<int> *p) { (void) p; } // gcc: silent  clang: silent
```

So it is the *call* that makes the specification needed, and both compilers
agree. The difference is in the wording, not the implementations -- the two
rules use different triggers.

Quoted verbatim from the draft's LaTeX source, `../cplusplus_draft` at
`c5d4aa74` (2026-08-06). Stable names are the durable reference; paragraph
numbers are omitted deliberately, because they drift and nothing here needs
them.

* **[dcl.contract.func]** -- "The function contract assertions of a function
  are considered to be *needed* ([temp.inst]) when: the function is odr-used
  ([basic.def.odr]) **or** the function is defined." Two bullets. No "named in
  an expression".
* **[except.spec]** -- "An exception specification is considered to be *needed*
  when: in an expression, the function is selected by overload resolution
  ([over.match], [over.over]); the function is odr-used ([term.odr.use]); ..."
  Six bullets, and selection in an expression is the **first**, listed
  separately from odr-use.
* **[basic.def.odr]** -- "A virtual member function is odr-used if it is not
  pure. A function is odr-used if it is named by a potentially evaluated
  expression or conversion." And a function is *named by* an expression only
  if "... either it is not a pure virtual function or the expression is an
  *id-expression* naming the function with an explicitly qualified name that
  does not form a pointer to member".

Put together: a pure virtual called by unqualified virtual dispatch is not
*named by* the expression, so it is not odr-used; and [dcl.contract.func]
offers no trigger other than odr-use or definition. So **the contracts of a
pure virtual are never "needed", in any translation unit** -- while a call to
it must still check them at run time. [except.spec] avoids the same trap by
making selection in an expression its own trigger.

Worth raising as a core issue. The obvious repair is to give
[dcl.contract.func] a bullet matching [except.spec]'s first one. Note that
[basic.def.odr] already has the phrase the implementation wants -- "named by a
**potentially evaluated** expression or conversion" -- which is the line to
draw for contracts even though [except.spec] draws a wider one.

The implementation should not wait on that. Whatever the wording ends up
saying, calling a function in a potentially-evaluated expression has to require
its declaration and all of its parts to be valid.

## Proposed fix

Ask the evaluation context directly rather than routing through odr-use, for
virtuals only:

```c++
if (OdrUse == OdrUseContext::Used ||
    (isa<CXXMethodDecl>(Func) && cast<CXXMethodDecl>(Func)->isVirtual() &&
     isOdrUseContext(*this) == OdrUseContext::Used))
  InstantiateFunctionContractsOnUse(Loc, Func);
```

`isOdrUseContext` answers "would this be an odr-use if the definition were
needed" -- which is exactly "named in a potentially-evaluated expression", the
trigger the wording ought to have. It still returns `None` in an unevaluated
operand and `Dependent` in a dependent context.

Note the deliberate asymmetry with the exception specification, which p17 makes
needed even in an unevaluated operand (`noexcept (p->g ())` diagnoses). A
contract is not: nothing evaluates it there, so nothing needs it. Potentially
evaluated is the right line for contracts even though it is not the line for
exception specifications. That matters: the `sfinae` group of the matrix pins
that `decltype`, `sizeof`, `noexcept` and requires-expressions must **not**
instantiate contracts, and this formulation preserves that by construction
rather than by luck.

## Provenance: pre-existing, not the CLANG-13 fix

Measured rather than argued. The pre-CLANG-13-fix sources
(`b9bdd3345c6a^` for `SemaTemplateInstantiateDecl.cpp` and `Sema.h`) were put
back in the working tree, rebuilt in the Space, and given the same reproducer:
**identical assertion**. So the CLANG-13 scope fix and cycle guard neither
caused nor masked it.

## GCC

**Not affected.** GCC compiles all of the crashing shapes, and a runnable
version confirms the pure virtual's interface precondition is genuinely
evaluated through the P3097 wrapper (violation count 1 for a failing value, 0
for a passing one). Note this is only true *after* GCC-34 was fixed the same
day -- before that GCC ICEd on an overlapping but not identical set. The two
compilers were each broken here, in different halves, and the intersection
(`pure + class template + dependent predicate`) is what nobody had a test for.

Mirrored watch tests, opposite expectations as usual:

* `clang/test/Contracts/OpenBugs/pure-virtual-dependent-predicate-codegen.cpp`
  -- `XFAIL: *`, the hand-written reduction.
* `gcc/testsuite/g++.dg/contracts/cpp26/open-bug-pure-virtual-dependent-predicate-codegen.C`
  -- expected pass.
* `clang/test/Contracts/OpenBugs/matrix-readers-openbug-clang-14.cpp` --
  `XFAIL: *`, the four generated cells from the table above.
* `gcc/testsuite/g++.dg/contracts/cpp26/matrix-readers-openbug-clang-14.C` --
  expected pass.

The generated pair regenerates from `contract-matrix-gen.py`, so when this is
fixed the four cells move back into `matrix-readers-p3097-codegen` by rerunning
`emit` rather than by hand-editing.

## Impact

Any P3097 pure virtual in a class template whose predicate mentions a member
or a template parameter. Not reached by BDE today (its `dbg_64_cpp26_contracts`
build is clean on both compilers), and not reached by the contracts test suite,
which is why it survived this long.
