---
id: 4260-p3400-facet-local-violation-handler
subject: '[clang][contracts] the local_violation_label facet and its trampoline'
depends: [4200-p3400-core]
regenerates: []
fixes: []
---

## Rationale
P3400's local violation handler: a label may supply the handler its
contracts report to, instead of the program-wide one.

Like the `query` facet this needs a run-time call into a user type, so it
is emitted as a trampoline and its pointer travels in the violation
descriptor -- which is what `getOrCreateDescriptorTable`'s `HasLocalHandler`
parameter, present but always false since `1100-p2900-base-codegen`, is
for.

**`findLabelMethod` is the part to read, and it exists because CodeGen and
Sema were answering the same question differently.**  Sema decides a facet
is present using ordinary name lookup, which sees inherited members;
CodeGen scanned direct members only.  So a handler inherited from a base
was detected by Sema, no trampoline was built for it, and the facet
silently did nothing.  The replacement mirrors lookup: a direct member
hides an inherited one, and a name found along two independent base paths
is ambiguous and therefore not found.

`LabelMethod` carries a `ThisOffset` for the same reason -- a trampoline
passes the label pointer straight through as `this`, which is only correct
at offset zero, so a facet on a base subobject needs the adjustment to
travel with it.  A facet reachable only through a **virtual** base is
treated as absent: its offset is not a constant the trampoline could apply,
and emitting a wrong one is worse than not emitting the call.

## Compile gap
Needs `4200-p3400-core` for the facet framework and
`1100-p2900-base-codegen` for the descriptor table.  Both precede it.

`4280-p3400-facet-queryable` reuses `findLabelMethod` and the
trampoline shape introduced here; it comes later and depends on this
commit for them.

**Behaviour trap:** a label supplying a local handler without this commit
reports to the global handler instead, silently.  That is the same shape as
every other unimplemented facet, but with a worse consequence -- the
program keeps running and the violation is handled, just by the wrong code.

## Contents

- clang/lib/CodeGen/CGContracts.cpp : @LabelMethod, @findDirectMethod, @CXXMethodDecl, @adjustToBase, /^getOrCreateLocalHandlerTrampoline\(/
- clang/test/Contracts/Runnable/p3400-combine-local-handler.cpp : *
- clang/test/Contracts/Runnable/p3400-facet-local-chain.cpp : *
- clang/test/Contracts/Runnable/p3400-facet-local-exception.cpp : *
- clang/test/Contracts/Runnable/p3400-local-handler-exception-path.cpp : *
- clang/test/Contracts/Runnable/p3400-local-handler-static.cpp : *
- clang/test/Contracts/Runnable/p3400-local-handler.cpp : *
- clang/test/Contracts/p3400-facet-local-handler.cpp : *
- clang/test/Contracts/p3400-local-handler-exception-path-codegen.cpp : *
