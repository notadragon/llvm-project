# CLANG-2: Recovering from an invalid `_Bool` load leaves the invalid bits in place

**Status:** Open (no fix intended here)
**Component:** compiler-rt / UBSan, or codegen for `-fsanitize=bool`
**Upstream Link:** None found. Searched GitHub Issues 2026-09-05, including
closed ones: `"-fsanitize=bool"` (67 hits, none about what -recover leaves
behind), `ubsan bool "invalid value" recover` (none), `sanitize-recover bool`
in title (none). #215798 is the closest and is a different question -- a CHECK
failing at -O0 versus -O2. See "Why this is framed as a divergence" below
for why this may not warrant a report at all
**Affects:** measured 2026-09-05 on our Clang against our GCC; originally
found 2026-07 against system gcc 13.3.0. Plain C, no C++ and no contracts.

## Bug Report

Loading an invalid `_Bool` representation under
`-fsanitize=bool -fsanitize-recover=bool`:

* **GCC** reports, then coerces the value to a valid `false`, so the program
  continues with a defined value.
* **Clang** reports, then continues with the raw invalid bits, which are
  truthy.

Measured output from the reproducer, same source, same flags:

```
clang:  runtime error: load of value 2, which is not a valid value for type '_Bool'
        AFTER r=1
gcc:    runtime error: load of value 2, which is not a valid value for type '_Bool'
        AFTER r=0
```

The diagnostic is the same; the value the program then observes is not.

## Why this is framed as a divergence

`-fsanitize-recover` promises that execution continues after a report, not
that the offending operation acquires a defined substitute, so Clang's
behaviour is defensible on its own terms. What is worth raising is that two
implementations of the same flag silently disagree about the value the program
observes afterwards -- the same class of divergence as the documented
`-fsanitize=bounds` one, where Clang's `bounds` group includes the trap-only
`local-bounds` and so traps regardless of `-fsanitize-recover`.

Anyone reasoning about what a recovered sanitizer check leaves behind -- which
is exactly what routing sanitizer checks into contract violation handlers
requires -- needs to know the two compilers differ here.

## Reproducer

See [`clang-02-ubsan-bool-recover.c`](clang-02-ubsan-bool-recover.c) in this
directory.

```
clang -std=c17 -fsanitize=bool -fsanitize-recover=bool -O0 clang-02-ubsan-bool-recover.c
gcc   -std=c17 -fsanitize=bool -fsanitize-recover=bool -O0 clang-02-ubsan-bool-recover.c
```

## Our Fix

None, and none intended. We resolved it on our side by updating the
`D4277R0-conformance-conv.lval.valid.representation.bool.celink` expectation
to match Clang's behaviour, with a comment recording the divergence.

## Notes

Listed in `../open-issues/README.md` as `Kind: divergence`, because a client
of this branch can observe it even though we do not plan to change it.
