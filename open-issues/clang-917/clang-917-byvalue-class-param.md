# CLANG-917: a precondition on a by-value class-type parameter is never proven at a call

**Status:** open
**Kind:** defect
**Upstream:** `--` (the D4405 proofs are ours: `-fcontracts-p4405` exists
only on this branch)
**Measured:** 2026-11-09, this branch at `c7e0b969329e` (the install,
release build with assertions)
**Watch test:** `clang/test/Contracts/OpenBugs/p4405-shadow-byvalue-class-param.cpp`
**Found by:** D4405 research (a by-value parameter case); gnu_gcc and EDG
(through edg-gxx) behave the same way

## Symptom

```c++
#include <contracts>
using namespace std::contracts::labels;
struct T { int c; };
int byval(T t)        pre<always_prove>(t.c == 1);
int byref(const T &t) pre<always_prove>(t.c == 1);
int scalar(int c)     pre<always_prove>(c == 1);
int f1() { T a = {1}; return byval(a); }   // cannot be proven
int f2() { return byval(T{1}); }           // cannot be proven
int f3() { T a = {1}; return byref(a); }   // proven
int f4() { return scalar(1); }             // proven
```

`clang++ -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405
-stdlib=libc++ -O2 -S`: both calls of `byval` are reported "contract
assertion cannot be proven"; the calls of `byref` and `scalar`, whose
preconditions say the same of the same value, are proven.  Likewise at
-O0, -O1 and -O3.

## Expected

Proven: the argument is a constant whose member `c` is 1, and the
parameter is a copy of it.

## Cause

A caller-side precondition is deferred to the caller's proof shadow only
if `canSubstituteCallerSide` (`clang/lib/CodeGen/ContractProveShadow.cpp`)
accepts it: each parameter the predicate names must be a reference bound
to a glvalue argument or have scalar evaluation kind
(`hasScalarEvaluationKind`).  A class-type parameter passed by value is
neither, so `proveByShadow` returns `Unproven`, and folding
(`ContractProveFold.cpp`) does not see through `T{1}`'s member either; the
obligation is decided at once as unproven.

## Fix scope

Let `canSubstituteCallerSide` accept a by-value class-type parameter whose
argument has no side effects, and have `CallerSideBinding::bind` give the
parameter an object in the shadow: a temporary initialized from the
argument as the call initializes the parameter (the copy or move
constructor, or the prvalue itself), destroyed at the end of the
predicate's scope -- or, for a trivially copyable type, the argument's
glvalue directly.  The constructor and destructor calls then count as the
predicate's calls (judged as side-effect free or not by
`ContractShadowDecide.cpp`, as for any other call in a predicate).  Local
to `ContractProveShadow.cpp`; moderate.
