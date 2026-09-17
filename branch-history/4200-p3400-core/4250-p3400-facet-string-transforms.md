---
id: 4250-p3400-facet-string-transforms
subject: '[clang][contracts] the comment and message facets'
depends: [4200-p3400-core]
regenerates: []
fixes: []
---

## Rationale
P3400's `compute_comment` and `compute_message` facets: a label may rewrite
the text a violation reports.

**Test-only in the compiler's terms.**  The mechanism is entirely
`4200-p3400-core`'s facet-call engine plus the library's label types, so
this commit adds no compiler source at all -- what it adds is the coverage
that the two string facets are actually wired through, which nothing else
exercises.

The cases the tests pin are the ones where the answer is not obvious: a
facet that receives a null comment because none was supplied, a facet that
replaces a message the contract did supply, `fixed_message_label_t`, which
always answers the same string, and the pass-through case, where a facet
that returns its argument unchanged must be indistinguishable from no facet
at all.  Redaction is the motivating use -- the reason to transform a
message is usually to remove something from it.

## Compile gap
Needs `4200-p3400-core` for the facet-call engine and the library's label
types; it precedes this commit.

Nothing to stub in either direction: no compiler source changes here.

**Behaviour trap:** none -- but the reason there is no source change is
worth keeping.  These facets work because `applyLabelFacets` asks for every
facet it knows about, including these two, from `4200-p3400-core` onwards.
If a reviewer drops this commit they lose only the proof.

## Contents

- clang/test/Contracts/Runnable/p3400-comment-runtime.cpp : *
- clang/test/Contracts/Runnable/p3400-message-runtime.cpp : *
- clang/test/Contracts/p3400-facet-comment.cpp : *
- clang/test/Contracts/p3400-facet-message.cpp : *
