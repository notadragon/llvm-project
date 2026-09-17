---
id: 0100-upstream-bugfixes
subject: '[clang] upstream fixes found while implementing contracts'
depends: []
regenerates: []
fixes: []
---

## Rationale
Two upstream Clang defects found while implementing contracts, neither
contracts-specific and neither ours.  They lead the branch because a
reviewer should be able to take them without taking anything else.

The constant evaluator accepted a derived-to-virtual-base conversion on an
object outside its lifetime.  Computing the virtual base offset consults the
most-derived object's layout, which is a use of the object, so doing it to
one that is deleted or not yet constructed is undefined and cannot be a
constant expression.  `HandleLValueBase` had no liveness check on that path
where the other operations consulting a most-derived type -- member calls,
`dynamic_cast`, `typeid` -- all have one.  GCC already rejects it.

`ParamReferenceReplacerRAII` did not restore parameters after a coroutine
body.  Its destructor used `DenseMap::insert`, which is a no-op when the key
is already present, and the key always is -- `addCopy` redirected the
parameter by overwriting it in place.  So every parameter stayed pointing at
its coroutine-frame copy for the rest of the function, which is the exact
opposite of what the RAII exists to undo.  Contracts made it visible, by
naming a parameter from a precondition and the frame copy from a
postcondition and getting two different objects for one name, but the fault
is upstream's and predates contracts.

## Compile gap
None.  Both are self-contained fixes to existing upstream code, and neither
references anything this branch adds.

## Contents
- clang/lib/AST/ExprConstant.cpp : @HandleLValueDirectVirtualBase, @HandleLValueBase
- clang/lib/CodeGen/CGCoroutine.cpp : #0
- clang/test/SemaCXX/constexpr-vbase-lifetime.cpp : *
