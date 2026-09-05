# CLANG-11: A redeclaration whose parameter type is dependent escapes the postcondition `const` rule

**Kind:** defect
**Status:** Open
**Affects:** `-fcontracts`, C++26 and later; a function template redeclared
with a different top-level `const` on a parameter that a postcondition
predicate odr-uses
**Workaround:** declare the parameter `const` on every declaration, which is
what the rule requires

## Symptom

An ill-formed program is accepted silently:

```cpp
template <typename T> void f (T a) post (a);   // `a` is not const
template <typename T> void f (T const a) { }
template void f<int> (int);                    // accepted
```

[dcl.contract.func] requires the parameter a postcondition predicate odr-uses,
"and the corresponding parameter on all declarations of f", to have `const`
type. On the declaration carrying the contract `a` is `T`, which for
`T = int` is not const.

## Trigger

See `clang-11-postcondition-redecl-dependent-param.cpp` in this directory,
with three controls that all behave correctly today and must keep doing so:

* the same shape with **concrete** types is rejected, so the check does run
  and it is the dependence that defeats it;
* both declarations `const` is well-formed and accepted;
* the declarations disagreeing about writing `const` while `T` is deduced as
  a const type is **well-formed** -- both parameters are const after
  substitution -- and a fix that compares written cv-qualifiers instead of
  substituting would wrongly reject it.

## Why it is open

GCC had the identical bug and was fixed first, per the standing order to fix
GCC before Clang: `gnu_gcc 13da4a8bb32`, tracked as GCC-26 and reported
upstream as [PR127196](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127196).
The Clang mirror was never started.

The diagnosis transfers; the fix probably does not. GCC's problem was that
`duplicate_decls` merges the declarations and the definition's parameters
survive, so the offending declaration no longer existed at instantiation time
-- it had to record the merged-away parameter and substitute it later. Clang's
`ParamReferenceChecker` already walks the finished predicate and its
redeclaration handling is structured differently, the same asymmetry that made
Clang's fix for the discarded-comma-operand bug a fraction of GCC's.

## Notes

Branch-only here: contracts are not upstream in Clang, so there is nothing
upstream to reproduce it against and nothing to file, even though the same
defect is a filed upstream bug on the GCC side.

Not to be confused with what comment 1 of PR127196 raises -- a predicate
naming a parameter only inside `decltype`. That is an unevaluated operand and
therefore not an odr-use, so this rule does not reach it; Clang already gets
that family right, as
[`clang-10-requires-expr-in-contract-ice.md`](clang-10-requires-expr-in-contract-ice.md)
records.
