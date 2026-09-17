---
id: 6130-p3100-check-nonnull-attribute
subject: '[clang][contracts] implicit check: nonnull attribute'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
P3100 coverage for UBSan's `nonnull-attribute` check -- passing a null
pointer to a parameter declared `__attribute__((nonnull))`.

Routing-only.  Resolved to `noexcept_enforce`, the handler runs (kind 7,
semantic 7) and the program terminates.

Paired with `6140-p3100-check-returns-nonnull-attribute`, which is the same
check on the other side of the call; they are separate commits because the
sanitizer treats them as separate checks and `-fsanitize-semantic=` can
configure them independently.

## Compile gap
Needs `6000-p3100-core` for the routing framework and
``6030-p3100-ubsan-runtime`` for the report path; both precede it.

Nothing to stub -- this commit is coverage, not mechanism.

**Behaviour trap:** the routed and unrouted forms of this check are
distinguishable only by *where the text appears*.  A test that merely
asserts the program died passes with routing broken, which is why these
check the handler's own markers.

## Contents

- clang/test/Contracts/Runnable/p3100-nonnull-attribute-route-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-nonnull-attribute-route-observe.cpp : *
