---
id: 1180-p2900-base-tests
subject: '[clang][contracts] base-facility tests spanning several components'
depends: []
regenerates: []
fixes: []
coauthors: ['Eric Fiselier <eric@efcs.ca>']
---

## Rationale
The end-to-end tests for the base facility -- the ones that compile, link,
run, and check what actually happened.

Every commit from `1000-p2900-base-basic` to `1160-p2900-base-libcxx`
carries its own tests, and those are almost all `-fsyntax-only` and
`-verify`: they check that the right thing is diagnosed.  Not one of them
can tell whether a precondition was *evaluated*.  These can, and they are a
separate commit because they need the whole facility -- a parser, Sema,
CodeGen and a runtime to call -- so there is no earlier commit they could
travel with.

They are also the only tests that can catch a whole class of failure: a
contract that is silently not evaluated.  A predicate on a lambda inside a
template that never runs, a result binding that names a copy so a mutation
through it is lost, a coroutine postcondition that reads a destroyed frame
-- each produces no diagnostic and no crash, and each is invisible to a
`-verify` test.  Only counting what the handler saw distinguishes them from
correct behaviour.

The shape to know before reading them: four shared headers do the work.
`contracts.h` and `contracts-runtime.h` give a test a violation handler
that records rather than terminates, `event_sequence.h` makes evaluation
*order* assertable, and `source-location.h` checks the location a violation
reports.  A test that only asserted "the program aborted" would pass for
the wrong reason most of the time, which is why so few of them do that.

Three clusters are worth a reviewer's attention, each covering a question
whose answer is not obvious from the code:

* **result bindings** -- identity, scalar mutation, class mutation and a
  reference return.  The question each answers is whether the name in the
  postcondition denotes the object being returned or a copy of it.
* **exceptions out of a predicate or out of a body** -- the throwing
  handler, the escaping postcondition, the destroyed return value on
  unwind, the function-try-block with no catch, `noexcept` termination.
* **lambdas and templates**, which is where the base facility's
  instantiation paths were thinnest.

## Compile gap
Depends on the whole of `1000`-`1160`.  Nothing to stub: these are test
files, and nothing in the compiler refers to them.

**The one thing that is not obvious:** they need a violation handler to
call, so they depend on `2200-libcontracts`'s entry point existing -- and
`2200-libcontracts` comes **after** this commit.  Standing alone, the same
stub that `1100-p2900-base-codegen` and `1160-p2900-base-libcxx` describe
is what these link against, and with a stub they all fail, because
observing the handler is the entire point of the commit.

This is the one place in the base series where "independently buildable"
and "independently *passing*" come apart, and the ordering is deliberate:
putting `2200-libcontracts` ahead of this commit would place the runtime
before the feature that needs it, which reads worse than a test commit
waiting one commit for its runtime.

## Contents
- clang/test/Contracts/Runnable/breathing-test.cpp : *
- clang/test/Contracts/Runnable/contract-assert-handler-throw-catchable.cpp : *
- clang/test/Contracts/Runnable/contract-classtemplate-outofline.cpp : *
- clang/test/Contracts/Runnable/contract-dependent-predicate-temporary.cpp : *
- clang/test/Contracts/Runnable/contract-exceptions.cpp : *
- clang/test/Contracts/Runnable/contract-result-binding-identity.cpp : *
- clang/test/Contracts/Runnable/contract-result-binding-mutation-class.cpp : *
- clang/test/Contracts/Runnable/contract-result-binding-mutation.cpp : *
- clang/test/Contracts/Runnable/contract-result-name.cpp : *
- clang/test/Contracts/Runnable/contract-retval-destroyed-on-unwind.cpp : *
- clang/test/Contracts/Runnable/contracts-runtime.h : *
- clang/test/Contracts/Runnable/contracts.h : *
- clang/test/Contracts/Runnable/coroutine-contract-parameters.cpp : *
- clang/test/Contracts/Runnable/decl.contracts.res.cpp : *
- clang/test/Contracts/Runnable/deducing-this-runtime.cpp : *
- clang/test/Contracts/Runnable/evaluation-order.cpp : *
- clang/test/Contracts/Runnable/event_sequence.h : *
- clang/test/Contracts/Runnable/function-try-block-no-catch.cpp : *
- clang/test/Contracts/Runnable/lambda-capture-contract-in-template.cpp : *
- clang/test/Contracts/Runnable/lambda-capture-in-contract.cpp : *
- clang/test/Contracts/Runnable/lambda-contract-in-template.cpp : *
- clang/test/Contracts/Runnable/lambda-in-postcondition-result-name.cpp : *
- clang/test/Contracts/Runnable/lambda-test.cpp : *
- clang/test/Contracts/Runnable/my_assert.h : *
- clang/test/Contracts/Runnable/noexceptions-contracts.cpp : *
- clang/test/Contracts/Runnable/postcondition-result-name-mangling.cpp : *
- clang/test/Contracts/Runnable/postcondition-throw-escapes-try.cpp : *
- clang/test/Contracts/Runnable/postcondition-throw-noexcept-terminate.cpp : *
- clang/test/Contracts/Runnable/result-name-reference-return.cpp : *
- clang/test/Contracts/Runnable/smf-handler-throws-terminate.cpp : *
- clang/test/Contracts/Runnable/source-location.h : *
- clang/test/Contracts/Runnable/violation-comment-kind.cpp : *
- clang/test/Contracts/Runnable/violation-handler-throws-noexcept.cpp : *
- clang/test/Contracts/Runnable/violation-handler-throws-observe.cpp : *
- clang/test/Contracts/Runnable/violation-template-function-name.cpp : *
- clang/test/Contracts/Runnable/void-postcondition-explicit-return-test.cpp : *
- clang/test/Contracts/Runnable/void-postcondition-test.cpp : *
