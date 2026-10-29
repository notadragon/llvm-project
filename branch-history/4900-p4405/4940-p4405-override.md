---
id: 4940-p4405-override
subject: '[clang][contracts][P4405] override shadows of virtual functions'
depends: [4930-p4405-analysis]
regenerates: []
fixes: []
---

## Rationale
D4405R0, "Virtual Functions".  A call through a base class `B` evaluates
the contract assertions of `B::f`, which the caller sees, and those of the
final overrider `D::f`, which it does not, so no caller can analyze whether
`D::f` honors the contract it overrides.  For each function `B::f` that a
virtual function `D::f` defined in the translation unit overrides, directly
or indirectly, the analysis evaluations for a call through `B::f` -- of
`D::f`'s preconditions at entry, and at each return of `D::f`'s
postconditions, each ahead of its own check, and of `B::f`'s
postconditions, after all of `D::f`'s checks, each with that assertion's
own analysis semantic, resolved as for any other -- are made in an override
shadow of `D::f` that takes `B::f`'s preconditions as given.  An unproven
one is diagnosed at `D::f`'s definition, "contract assertion cannot be
proven for a call through 'B::f'" (or "contract predicate is always false
for a call through 'B::f'"), with a note at the predicate.  A `B::f`
without contract assertions gives a pair with nothing given, so `D::f`'s
preconditions must hold for any argument.

`-fcontracts-prove-override` (implied by `-fcontracts-p4405`;
`-fno-contracts-prove-override` on either side of it) selects the strategy,
`ContractProveOverrideStrategy` in `CodeGen/ContractProveOverride.cpp`, the
registry's third entry.  `GenerateCode`, after the prologue, calls
`emitContractOverrideEntry`: in the real function of a virtual method that
is not pure and not an `available_externally` copy, it
walks `overridden_methods()` recursively, makes one override pair per
overridden function with an obligation (`ContractOverridePair`, the
interface's: `D::f`, `B::f`, the derived-to-base path from its last virtual
base on, as Sema builds a cast's, `B::f`'s name, and an expression standing
for `D::f`'s definition that identifies the pair's evaluations), and offers
each obligation at entry with the pair; one the strategy defers queues the
pair's override shadow, `<name>.contract.override.<n>`, which
`4920-p4405-shadow`'s
machinery emits through `GenerateCode` and decides like any proof shadow.
There the same hook emits the pair's premises -- `B::f`'s preconditions, as
`if (!pred) unreachable`, `B::f`'s parameters `D::f`'s by position and
`this` the `B` subobject, not one with side effects, guaranteed only when
the precondition's label settles it (every caller evaluates it, in any
translation unit) -- and then the obligations.  Override shadows are made
when proof shadows could be (an optimized module, the same exclusions) and
the function can have one.  While they can be (C++ with
`-fcontracts-p3097`), the strategy takes every diagnostic decided at once,
as the proof shadows do, so that the back end issues all of them in order
and an error issued during code generation does not keep it from deciding
the override shadows.  Clang generates only the functions a translation
unit emits, so an inline overrider whose vtable is emitted elsewhere is
never analyzed.

At each return, `EmitContractStmtAsFullStmt` offers each of `D::f`'s
postconditions for each pair ahead of its own analysis evaluation
(`emitContractOverrideOwnPost`), and `EmitPostContracts`, on its way out,
`B::f`'s postconditions (`emitContractOverridePosts`): in the pair's
override shadow they read `B::f`'s parameters as `D::f`'s -- a by-value one
`D::f` receives as its own object from a copy taken at entry, since the
P3097 wrapper checks the postcondition with its own copy --, `this` as the
`B` subobject converted at entry, and the result as `D::f`'s, converted to
`B::f`'s return type for a covariant one (null-checked for a pointer).  A
postcondition with captures is decided at once, as its own analysis
evaluation is.

Virtual destructors override too, implicitly defined ones included.  Only
the variant a delete through a base runs whole is analyzed: the
base-object destructor of a class without virtual bases, and the
complete-object destructor of one with virtual bases (not of an abstract
class, whose complete-object destructor is a trap), whose override shadow
runs the whole destructor instead of calling the base-object one
(`isContractOverrideWholeDestructor`, consulted by `structorVariantDelegates`
and `EmitDestructorBody`), so that the overridden destructor's
postconditions are proven after every base is destroyed; the deleting
destructor holds nothing.  A base-object destructor overriding one with
contract assertions is never made an alias of its base's, which would leave
nothing to analyze.  A base class occurring as several subobjects gives one
pair per subobject when `B::f`'s contract assertions name `this` (each reads
a different object), one per overridden function otherwise.

Without override shadows -- the flag off, or none can be made -- the same
obligations are `D::f`'s own, with nothing given: its preconditions are
settled only by folding, and `B::f`'s postconditions are obligations at
each return of `D::f` (`emitContractOverridePosts`, from
`EmitPostContracts`), proven in its proof shadow, with `B::f`'s parameters
`D::f`'s (a by-value one `D::f` receives as its own object read from a
copy taken at entry, since the P3097 wrapper checks the postcondition with
its own copy), `this` the `B` subobject and a covariant result converted to
`B::f`'s return type.  The real function's code is unchanged in both modes
(`p4405-override-same-code.cpp` compares the IR and the object with every
analysis evaluation `ignore`).

## Compile gap
None.  The strategy uses `4900-p4405`'s proving interface (the override
pair, and the query's override fields) and `4920-p4405-shadow`'s shadows,
premises, bindings and snapshots.  As for the other strategies, the
`LangOptions.def` entry is `1000-p2900-base-basic`'s, the implication by
`-fcontracts-p4405` and its round trip in `CompilerInvocation.cpp`
`2400-p3595`'s, and the "for a call through" form of the diagnostics
`2400-p3595`'s, with them.

**Behaviour gap: the driver does not pass the flag on before
`6900-cross-paper-options`**, whose contracts block in `Clang.cpp` forwards
it; until then only a `-cc1` command line sees it.  The `-###` lines of
`p4405-override-nooverride.cpp` and the tests that use the driver
(`p4405-override-{labels,premise,assume}.cpp`) cannot pass before `6900`.

## Contents
- clang/include/clang/Options/Options.td : /^defm contracts_prove_override : /
- clang/lib/CodeGen/CMakeLists.txt : #1:2
- clang/lib/CodeGen/ContractProve.h : @ContractProveOverrideStrategy:0
- clang/lib/CodeGen/ContractProve.cpp : @ContractProofStrategies:3
- clang/lib/CodeGen/ContractProveOverride.cpp : *
- clang/lib/CodeGen/CodeGenFunction.h : #2:2
- clang/lib/CodeGen/CodeGenFunction.cpp : #8:0
- clang/lib/CodeGen/CGCall.cpp : #3:1
- clang/lib/CodeGen/CGContracts.cpp : @EmitContractStmtAsFullStmt:1
- clang/lib/CodeGen/CGClass.cpp : #0:1, /^    \/\/ D4405: an override shadow runs the whole destructor\.$/
- clang/lib/CodeGen/CGCXX.cpp : /^static bool overridesDestructorWithContracts\(/, #1:1
- clang/docs/UsersManual.md : #0:1
- clang/test/Contracts/p4405-override-*.cpp : *
