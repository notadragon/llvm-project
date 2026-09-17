---
id: 4280-p3400-facet-queryable
subject: '[clang][contracts] the queryable_label facet'
depends: [4200-p3400-core]
regenerates: []
fixes: []
---

## Rationale
P3400's `query` facet: a label may answer arbitrary key/index lookups from
the violation handler at run time.

Unlike every other facet, this one cannot be resolved at compile time --
the handler decides what to ask for, and it asks while the program is
running.  So the compiler emits a **trampoline**: a small generated
function with the fixed signature
`void *(const void *label, const void *key, size_t index)` that forwards to
the label type's `query` member, adjusting `this` for the base offset.
`getOrCreateQueryTrampoline` emits one per label type and caches it; the
descriptor the violation carries holds a pointer to it.

That is the whole commit, and it is the reason the facet is last in the
P3400 series: it is the only one that needs CodeGen.

The use it exists for is metadata a handler wants but the language has no
vocabulary for -- an owner, a ticket number, a team -- attached to the
label rather than to every contract that uses it.

## Compile gap
Needs `4200-p3400-core` for facet lookup (`findLabelMethod`) and
`1100-p2900-base-codegen` for the descriptor the trampoline pointer is
stored in; both precede it.

`1100-p2900-base-codegen` records that `getOrCreateDescriptorTable` takes a
`HasLocalHandler` parameter that is always false there.  This commit is not
what makes it true -- `4260-p3400-facet-local-violation-handler` is -- but
both are consumers of the same descriptor machinery, which is why that
parameter was threaded through from the start rather than added late.

**Behaviour trap:** a label providing `query` without this commit is a
label whose `query` is never called.  The handler asks and gets nothing
back, with no diagnostic at either end.

## Contents

- clang/lib/CodeGen/CGContracts.cpp : /^getOrCreateQueryTrampoline\(/
- clang/test/Contracts/Runnable/p3400-facet-query-basic.cpp : *
- clang/test/Contracts/Runnable/p3400-facet-query-combined.cpp : *
- clang/test/Contracts/Runnable/p3400-facet-query-no-label.cpp : *
- clang/test/Contracts/Runnable/p3400-facet-query-with-handler.cpp : *
