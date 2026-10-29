---
id: 6990-p3100-user-docs
subject: '[clang][contracts][P3100] user documentation for the implicit assertions and -fsanitize-semantic='
depends: [6000-p3100-core, 6060-p3100-msan-runtime, 6110-p3100-check-null-and-alignment, 6410-p3100-check-assign-overlap]
regenerates: []
fixes: []
---

## Rationale
The user documentation of P3100 in Clang's manuals, which GCC has in
`invoke.texi`: a section of `UsersManual.md`, "Implicit contract
assertions for undefined behavior", and a pointer to it from
`UndefinedBehaviorSanitizer.md`.

The section documents `-fcontracts-p3100`; the `ub:` groups the implicit
assertions belong to, their `assume` default and how a configuration
selects a semantic for a group (by `-fcontract-group-evaluation-semantic=`
or a JSON entry, a group name selecting every group that extends it); the
groups Clang checks and their leaves, including the two checks that report
their parent group because the leaf cannot be told at their site; the rule
for the null-pointer leaves (`.access` where an object is read or written,
`.indirection` otherwise, a conversion to a virtual base always `.access`,
each null dereference reported once, a reference not checked again); and
the routing of sanitizer checks to the contract-violation handler by
`-fsanitize-semantic=` -- which checks are routed, the semantics a routed
and a non-routed check take, the semantic one takes from the other
sanitizer options when none is given, `-fsanitize-noncontract-callbacks`,
and the limitation of the checks LLVM passes insert after optimization
(local-bounds, address, memory, thread): after one reports and continues,
the code after it is not guaranteed to see the handler's side effects at
`-O1` and above.

It comes after every check it lists.

## Compile gap
None: documentation only.

## Contents
- clang/docs/UsersManual.md : #1
- clang/docs/UndefinedBehaviorSanitizer.md : *
