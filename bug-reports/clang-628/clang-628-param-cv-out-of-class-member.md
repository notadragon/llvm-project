# CLANG-628: an out-of-class member definition takes its parameters' top-level cv from the in-class declaration

**Kind:** defect (rejects-valid and accepts-invalid)
**Status:** Open -- found 2026-10-02 by the EDG fork (its EDG-51), confirmed
2026-10-06
**Component:** clang / Sema (template instantiation)
**Upstream Link:** [#29713](https://github.com/llvm/llvm-project/issues/29713)
("Class template member function out-of-line definition dependent parameter
uses declaration topmost cv-qualification", open since 2016); duplicate
[#30955](https://github.com/llvm/llvm-project/issues/30955); related
[#59558](https://github.com/llvm/llvm-project/issues/59558) (searched
2026-10-06, see below)
**Affects:** measured 2026-10-06 with `-std=c++20 -fsyntax-only` -- Clang
18.1.0, 22.1.6, trunk `33891afe2ebb` and this branch all reject line A and
accept line B
**Workaround:** spell the parameter's cv-qualifiers the same in both
declarations
**GCC:** right (rejects line B only; 16.2.0 and trunk)

## What is wrong

[`clang-628-param-cv-out-of-class-member.cpp`](clang-628-param-cv-out-of-class-member.cpp):

    template <class T> struct A { void m(const T x); void n(T x); };
    template <class T> void A<T>::m(T x) { x = 1; }        // rejected
    template <class T> void A<T>::n(const T x) { x = 1; }  // accepted
    template struct A<int>;

Top-level cv-qualifiers of a parameter are not part of the function type
([dcl.fct]/5), so in a definition they come from the definition only.  For
a parameter of dependent type in an out-of-class definition of a member of
a class template, Clang's instance uses the in-class declaration's
qualifiers.  The same happens for a member template of a class template
(`C<T>::m(U x)` declared `m(const U x)`), and for the primary definition of
a member some of whose instances are explicitly specialized.  A
non-dependent parameter type is right when the whole function type is
not dependent (Clang's own `SemaTemplate/dependent-type-identity.cpp`,
PR18275), and wrong when another parameter makes it so (#59558's shape).  With `T = const int`
a diagnostic prints the parameter's type as "const const int".

## Cause (ours, investigated 2026-10-06, not fixed)

`Sema::addInstantiatedParametersToScope`
(`clang/lib/Sema/SemaTemplateInstantiateDecl.cpp`), which
`InstantiateFunctionDefinition` calls to map the pattern's parameters onto
the instantiated declaration's, keeps the instantiated declaration's
`ParmVarDecl`s -- made when the class was instantiated, from the in-class
declaration -- and re-types them from the definition only when the
pattern's *function* type is not dependent:

    // If the parameter's type is not dependent, update it to match the type
    // in the pattern. They can differ in top-level cv-qualifiers, and we want
    // the pattern's type here. If the type is dependent, they can't differ,
    // per core issue 1668. ...
    // FIXME: Updating the type to work around this is at best fragile.
    if (!PatternDecl->getType()->isDependentType()) { ... setType(T); }

That is the PR18275 fix (`SemaTemplate/dependent-type-identity.cpp`), and
the comment's premise is what is wrong: a dependent parameter type's
top-level cv can differ between the declarations.  The test is on the whole
function type, so once any parameter is dependent no parameter is re-typed,
and the pack branch below it has the same guard.  A fix would substitute
the pattern parameter's type for every parameter (the code already does
for the non-dependent case), or at least re-apply its top-level cv to the
instantiated parameter's type; "const const int" is the same code adding
the pattern's `const` to a type that already carries the argument's.

## Upstream search (2026-10-06, GitHub issues API, open and closed)

Phrasings: "out-of-line definition parameter const template", "out-of-line
member definition parameter cv", "top-level cv qualifier parameter template
definition", "const parameter out-of-line definition class template",
"parameter const qualifier declaration definition template instantiation",
"cannot assign to variable const out-of-line definition", "dependent
parameter cv-qualification", "topmost cv-qualification", "parameter
top-level const redeclaration template", "const const int parameter".

* [#29713](https://github.com/llvm/llvm-project/issues/29713) (open, 2016,
  bugzilla 30365) -- this bug exactly, both directions, with a
  `typename T::U` parameter.
* [#30955](https://github.com/llvm/llvm-project/issues/30955) (open, 2017,
  bugzilla 31607, `accepts-invalid`) -- the same bug, the accepts-invalid
  direction (`const B* const b` in the definition only, `b` modified): a
  duplicate of #29713 that nobody linked.
* [#59558](https://github.com/llvm/llvm-project/issues/59558) (open, 2022,
  `confirmed`) -- a *non-dependent* `int` parameter of a member whose other
  parameter (`X&&`) makes the function type dependent: the guard above.
  Its reported program is rejected at trunk now (the template definition
  diagnoses the modification of its own non-dependent `const int`), but the
  reverse -- `f(X&&, const int)` declared, `f(X&& u, int x) { x += ...; }`
  defined -- is still rejected at instantiation (measured 2026-10-06 on the
  branch install): a non-dependent parameter takes the declaration's cv too
  whenever another parameter makes the function type dependent.
* Nothing closed covers it; [#75404](https://github.com/llvm/llvm-project/issues/75404)
  (top-level cv in concept subsumption) is unrelated.

Nothing new to file; the member-template, "const const int", #30955-is-a-
duplicate and #59558-is-the-same-guard observations could be added to
#29713 as a comment (the user's call).

## Why it matters here

It bears on contracts' rule that a parameter a postcondition odr-uses be
const in every declaration: the definition's own qualifiers are the ones
that count.
