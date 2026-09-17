---
id: 4230-p3400-facet-allowed-semantics
subject: '[clang][contracts] the allowed_semantics facet'
depends: [4200-p3400-core]
regenerates: []
fixes: []
---

## Rationale
P3400's `allowed_semantics` facet: a label may restrict which evaluation
semantics its contracts are permitted to have.

A contract with no label allows every semantic; a label that provides the
facet narrows the set, and a configuration or command-line option that asks
for a semantic outside it is adjusted rather than obeyed.  The mask is
computed in Sema (`extractAllowedMask`) and consumed when
`ContractOptions.cpp` resolves the semantic.

**The one rule worth stating: a non-const member is not a facet.**  The
comment in `extractAllowedMask` records what happens when that is not
enforced -- a label the library says has no `allowed_semantics` facet at
all gets its semantic set narrowed anyway, and a bare label stops agreeing
with its combined form.  Facet lookup is `const`-qualified everywhere for
this reason, and it is the same reason `dropInaccessibleFacetCandidates`
is called here: a member this context cannot name is not a facet either,
and must not be an error.

An empty allowed set is diagnosed in Sema before resolution, so
`ContractOptions.cpp` may assume every mask it sees contains at least one
semantic.

## Compile gap
Needs `4200-p3400-core` for the facet-lookup engine and `2400-p3595` for
the resolution path the mask constrains; both precede it.

None outward.

**Behaviour trap:** a label whose `allowed_semantics` is not consulted
produces no diagnostic -- the contract simply gets whatever semantic was
configured, which is the answer the label existed to prevent.  The tests
here check the adjusted semantic, not the absence of an error, because
there is no error to check for.

## Contents

- clang/lib/Basic/ContractOptions.cpp : @semanticLevel, @semanticAtLevel, /ContractOptions::clampToAllowed/
- clang/lib/Sema/SemaContract.cpp : @extractAllowedMask
- clang/test/Contracts/Runnable/allowed-mask-quick-enforce.cpp : *
- clang/test/Contracts/Runnable/p3400-facet-allowed-const.cpp : *
- clang/test/Contracts/p3400-facet-allowed-error.cpp : *
- clang/test/Contracts/p3400-facet-allowed-noexcept.cpp : *
- clang/test/Contracts/p3400-facet-allowed.cpp : *
- clang/test/Contracts/p3400-label-stateful-allowed.cpp : *
