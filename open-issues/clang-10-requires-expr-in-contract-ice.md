# CLANG-10: A parameter named inside a `requires`-expression in a contract predicate crashes the compiler

**Kind:** defect
**Status:** Open
**Affects:** `-fcontracts`, C++26 and later; any `pre` or `post` whose
predicate contains a requires-expression that names a parameter of the
function. Const-qualification of the parameter makes no difference.
**Workaround:** hoist the requirement out of the predicate -- into a named
concept, a constraint on the function, or a `constexpr bool` -- and name that
from the contract instead.

## Symptom

The compiler aborts with an assertion failure rather than diagnosing anything:

```
SemaContract.cpp:1680: ScopeWalker::nextFuncScope():
  Assertion `FunctionScopeIndex < FunctionScopes.size()' failed.
```

An assertionless build will not stop there, so the behaviour in a release
compiler is unpredictable rather than absent.

## Trigger

```cpp
void f (int p) pre (requires { +p; }) { }
```

See `clang-10-requires-expr-in-contract-ice.cpp` in this directory. It carries
two controls that bound the problem, and both of them compile:

* a requires-expression naming a **global** instead of a parameter, and
* the **same parameter** named outside a requires-expression.

So this is the parameter lookup, not requires-expressions in general, and not
the postcondition `const` rule.

## Why it is open

Found 2026-09-05, not yet investigated. The stack points at
`Sema::isUsageAcrossContract` -> `getInterveningScopeEntries` ->
`ScopeWalker`, which walks outward from the use to the contract looking for
intervening scopes. A requires-expression introduces a scope that walk does
not expect, and the index runs past the end of `FunctionScopes`.

## Notes

Branch-only: contracts are not upstream in Clang, so there is nothing upstream
to reproduce this against and nothing to file.

Found while writing the Clang mirror of a GCC fix
(`gnu_gcc e9b7222a73f`, "a parameter in an unevaluated operand is not
odr-used"). That mirror test is **not written yet** precisely because of this
crash: it would have to omit the requires-expression shape, and the Clang
contracts suite has no XFAILs worth introducing one into. Fixing this unblocks
the mirror test, which should then cover `decltype`, `decltype((p))`,
`sizeof`, `noexcept` and a requires-expression in one file.

Clang is otherwise correct on that whole family -- it accepts every shape the
GCC test pins, dependent or not, including a coroutine.
