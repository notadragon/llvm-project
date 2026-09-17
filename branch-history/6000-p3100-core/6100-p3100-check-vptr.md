---
id: 6100-p3100-check-vptr
subject: '[clang][contracts] implicit check: vptr'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
P3100 coverage for UBSan's `vptr` check -- a virtual call or `dynamic_cast`
on an object whose dynamic type is wrong.

**The routed vptr path is the clearest demonstration of the lazy report
contract**, which is why it is worth having as its own commit even though
it adds no compiler code: on the routed path the sanitizer emits *nothing*
of its own, and the full UBSan diagnostic appears only when the handler
calls `contract_violation::report()`.  The test captures the rendered text
live and asserts it lies strictly between the handler's own markers.

`-fsanitize=vptr` is recoverable by default, so under `-fcontracts-p4298`
the routed semantic resolves to `noexcept_observe`: the handler runs and
the program continues.

## Compile gap
Needs `6000-p3100-core` for the routing framework and
``6030-p3100-ubsan-runtime`` for the report path; both precede it.

Nothing to stub -- this commit is coverage, not mechanism.

**Behaviour trap:** the routed and unrouted forms of this check are
distinguishable only by *where the text appears*.  A test that merely
asserts the program died passes with routing broken, which is why these
check the handler's own markers.

## Contents

- clang/test/Contracts/Runnable/p3100-vptr-report-ondemand.cpp : *
- clang/test/Contracts/Runnable/p3100-vptr-route-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-vptr-route-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-vptr-route-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-vptr-throw-noexcept-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-vptr-throw-noexcept-observe.cpp : *
- clang/test/Contracts/p3100-vptr-sanitize-semantic.cpp : *
