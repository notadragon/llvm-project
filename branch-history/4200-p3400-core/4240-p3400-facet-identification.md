---
id: 4240-p3400-facet-identification
subject: '[clang][contracts] the identification facet and group names'
depends: [4200-p3400-core]
regenerates: []
fixes: []
---

## Rationale
P3400's identification facets: a label can name the groups its contracts
belong to, which is what `-fcontract-group-evaluation-semantic=` and
P3595's `group` matching then act on.

`extractGroupNames` reads the label's `group_names` member.  The
significant decision is in the comment beside it: the label expression is
**constant-evaluated as a whole** to get its value, rather than the member
being read through a pointer at run time.  A run-time read would imply a
label's group membership can change during execution and have an effect,
which it cannot -- group membership is a compile-time property, and P3400's
own example spells it that way.

That is not a theoretical preference.  The recorded consequence of getting
it wrong is that a label's group-based configuration is never applied at
all: `-fcontract-group-evaluation-semantic=safety:observe` leaves a
`safety`-labelled contract at its default.

Same two lookup rules as the other facets -- `const`-qualified, and an
inaccessible member is not a facet rather than an error.

## Compile gap
Needs `4200-p3400-core` for the facet engine, and `2400-p3595` for the
group matching that consumes the names; both precede it.

None outward.

**Behaviour trap:** exactly the one above.  Missing group names are
indistinguishable from a label that declares none, so the contract is
configured as if unlabelled and nothing says so.

## Contents

- clang/lib/Sema/SemaContract.cpp : @decodeGroupNameArray, @extractGroupNames
- clang/test/Contracts/Runnable/p3400-facet-group-const.cpp : *
- clang/test/Contracts/Runnable/p3400-group-basic.cpp : *
- clang/test/Contracts/Runnable/p3400-group-combined.cpp : *
- clang/test/Contracts/Runnable/p3400-group-multi.cpp : *
- clang/test/Contracts/Runnable/p3400-group-postcondition.cpp : *
- clang/test/Contracts/Runnable/p3400-group-runtime.cpp : *
- clang/test/Contracts/Runnable/p3400-group-with-facets.cpp : *
- clang/test/Contracts/p3400-facet-group.cpp : *
- clang/test/Contracts/p3400-group-config.cpp : *
