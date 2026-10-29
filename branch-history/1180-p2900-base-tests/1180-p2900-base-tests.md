---
id: 1180-p2900-base-tests
subject: '[clang][contracts][P2900] base-facility tests spanning several components'
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
contract that is silently not evaluated.  A predicate in a template that
never runs, a result binding that names a copy so a mutation
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
* **templates**, which is where the base facility's instantiation paths
  were thinnest.  (The lambda tests are `1170-p2900-base-lambdas`'s.)

`contract-declarator-positions.cpp` is the fourth, and the clearest example
of why these tests run rather than compile.  A
function-contract-specifier-seq follows the *complete* declarator, so it
belongs to the function that declarator declares however complicated the
declarator is -- including when the return type contains a parameter list of
its own, as in `int (*b1(int i))(int) pre(i > 0)` and its trailing-return
spellings.  Two distinct things can go wrong there and a compile-only test
cannot tell them apart: the contract can bind to an abstract declarator that
never becomes a function and be discarded in silence, or it can reach the
right function while its predicate is parsed in the return type's parameter
scope, so a name in it resolves to a parameter that has no instance at the
call.  The `d1`/`d2` pair separates them -- the function and its return type
each declare a parameter spelled `i`, of different types, and the two rows
invert the same predicate, so a wrong binding fails both and a dropped
contract fails `d2` alone.  Clang is correct on all of it; the file is the
pin.

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
- clang/test/Contracts/Runnable/contract-declarator-positions.cpp : *
- clang/test/Contracts/Runnable/contract-dependent-predicate-temporary.cpp : *
- clang/test/Contracts/Runnable/contract-result-binding-identity.cpp : *
- clang/test/Contracts/Runnable/contract-result-binding-mutation-class.cpp : *
- clang/test/Contracts/Runnable/contract-result-binding-mutation.cpp : *
- clang/test/Contracts/Sema/postcondition-result-name-return-type.cpp : *
- clang/test/Contracts/Runnable/contract-retval-destroyed-on-unwind.cpp : *
- clang/test/Contracts/Runnable/contracts-runtime.h : *
- clang/test/Contracts/Runnable/contracts-support.h : *
- clang/test/Contracts/Runnable/coroutine-contract-parameters.cpp : *
- clang/test/Contracts/Runnable/decl.contracts.res.cpp : *
- clang/test/Contracts/Runnable/deducing-this-runtime.cpp : *
- clang/test/Contracts/Runnable/evaluation-order.cpp : *
- clang/test/Contracts/Runnable/function-try-block-no-catch.cpp : *
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
- clang/test/Contracts/postcondition-undeduced-result.cpp : *
- clang/test/Contracts/Runnable/constify-requires-expr.cpp : *
- clang/test/Contracts/ast-print-contract-assert-layout.cpp : *
- clang/test/Contracts/astimporter-contracts.cpp : *
- clang/test/Contracts/block-scope-decl-then-def-contract.cpp : *
- clang/test/Contracts/block-scope-first-decl-contract.cpp : *
- clang/test/Contracts/clang-cl-std-latest.cpp : *
- clang/test/Contracts/constexpr-ctor-contracts.cpp : *
- clang/test/Contracts/constexpr-dtor-contracts.cpp : *
- clang/test/Contracts/constexpr-observe-fold-drops-runtime-check.cpp : *
- clang/test/Contracts/constexpr-observe-nonconstant-predicate.cpp : *
- clang/test/Contracts/constexpr-post-reference-result.cpp : *
- clang/test/Contracts/constexpr-result-name-subobject.cpp : *
- clang/test/Contracts/constify-ref-nttp.cpp : *
- clang/test/Contracts/constify-structured-binding.cpp : *
- clang/test/Contracts/contract-assert-cxx98.cpp : *
- clang/test/Contracts/contract-array-ref-capture-constify.cpp : *
- clang/test/Contracts/contract-assert-vla-implicit-capture.cpp : *
- clang/test/Contracts/contract-classtemplate-pack-member-fold.cpp : *
- clang/test/Contracts/contract-classtemplate-pack-member-fold-constexpr.cpp : *
- clang/test/Contracts/contract-comment-blank-line.cpp : *
- clang/test/Contracts/contract-comment-macro.cpp : *
- clang/test/Contracts/contract-comment-macro.h : *
- clang/test/Contracts/contract-copy-capture-constify.cpp : *
- clang/test/Contracts/contract-enclosing-lambda-capture-constify.cpp : *
- clang/test/Contracts/contract-explicit-copy-capture.cpp : *
- clang/test/Contracts/contract-on-deleted-or-defaulted.cpp : *
- clang/test/Contracts/contract-ref-capture-constify.cpp : *
- clang/test/Contracts/contract-repeated-location.cpp : *
- clang/test/Contracts/contract-repeated-location-constexpr.cpp : *
- clang/test/Contracts/ctor-contracts-no-ctor-aliases.cpp : *
- clang/test/Contracts/ctor-pre-qualified-member-rejected.cpp : *
- clang/test/Contracts/ctor-pre-static-overload.cpp : *
- clang/test/Contracts/ctor-pre-unevaluated-member.cpp : *
- clang/test/Contracts/default-handler-demangle-leak.cpp : *
- clang/test/Contracts/default-outofline-contract.cpp : *
- clang/test/Contracts/deleted-violation-handler.cpp : *
- clang/test/Contracts/dtor-contracts-base-alias.cpp : *
- clang/test/Contracts/dtor-contracts-no-ctor-aliases.cpp : *
- clang/test/Contracts/dtor-redecl-contracts-not-compared.cpp : *
- clang/test/Contracts/explicit-spec-contract-dropped.cpp : *
- clang/test/Contracts/explicit-spec-contract-mismatch.cpp : *
- clang/test/Contracts/explicit-spec-member-template-contracts.cpp : *
- clang/test/Contracts/friend-contract-class-scope.cpp : *
- clang/test/Contracts/Runnable/explicit-spec-decl-renamed-params.cpp : *
- clang/test/Contracts/Runnable/contracts-tmpl-spec2.cpp : *
- clang/test/Contracts/Runnable/contracts-friend1.cpp : *
- clang/test/Contracts/extension-contract-assert-without-contracts.cpp : *
- clang/test/Contracts/extension-pre-without-contracts.cpp : *
- clang/test/Contracts/fno-contracts-driver-link.cpp : *
- clang/test/Contracts/hidden-friend-template-pre.cpp : *
- clang/test/Contracts/module-inline-checked-contract.cpp : *
- clang/test/Contracts/module-template-contract-export.cpp : *
- clang/test/Contracts/nested-assert-in-stmt-expr.cpp : *
- clang/test/Contracts/noexcept-violation-handler.cpp : *
- clang/test/Contracts/observe-warning-unused-var-eval.cpp : *
- clang/test/Contracts/odr-hash-contracts.cpp : *
- clang/test/Contracts/pch-checked-contract.cpp : *
- clang/test/Contracts/pch-contract-bits.cpp : *
- clang/test/Contracts/pch-contract-predicate-can-throw.cpp : *
- clang/test/Contracts/pch-contract-semantic.cpp : *
- clang/test/Contracts/post-odr-use-template-shapes.cpp : *
- clang/test/Contracts/postcondition-const-function-pointer-param.cpp : *
- clang/test/Contracts/postcondition-function-param-reason.cpp : *
- clang/test/Contracts/postcondition-outer-local.cpp : *
- clang/test/Contracts/predicate-throw-constructor.cpp : *
- clang/test/Contracts/predicate-throw-delete.cpp : *
- clang/test/Contracts/noexcept-delete-null.cpp : *
- clang/test/Contracts/predicate-throw-array-filler.cpp : *
- clang/test/Contracts/predicate-throw-default-arg.cpp : *
- clang/test/Contracts/predicate-throw-new-array.cpp : *
- clang/test/Contracts/redecl-condition-folded.cpp : *
- clang/test/Contracts/redecl-post-param-const-class-template.cpp : *
- clang/test/Contracts/redecl-post-param-const.cpp : *
- clang/test/Contracts/redecl-result-name-presence.cpp : *
- clang/test/Contracts/result-name-attributes.cpp : *
- clang/test/Contracts/result-name-decl-printed-twice.cpp : *
- clang/test/Contracts/result-name-explicit-capture.cpp : *
- clang/test/Contracts/result-name-shadows-parameter.cpp : *
- clang/test/Contracts/stmt-expr-local-constified.cpp : *
- clang/test/Contracts/violation-handler-cv-param.cpp : *
- clang/test/Contracts/Runnable/pch-contracts-runtime.cpp : *
- clang/test/Contracts/Runnable/predicate-exception-quick-enforce.cpp : *
- clang/test/Contracts/contract-quick-enforce-throwing-predicate-uninit.cpp : *
- clang/test/Contracts/Runnable/result-name-deduced.cpp : *
- clang/test/Contracts/Runnable/this-param-out-of-class.cpp : *
- clang/test/Contracts/constexpr-object-lifetime.cpp : *
- clang/test/Contracts/constexpr-postcondition-param-address.cpp : *
- clang/test/Contracts/pch-contracts-constexpr.cpp : *
- clang/test/Contracts/postcondition-params-template-redecl.cpp : *
- clang/test/Contracts/postcondition-typeid-polymorphic-param.cpp : *
- clang/test/Contracts/predicate-await-forms.cpp : *
- clang/test/Contracts/result-name-deduced.cpp : *
- clang/test/Contracts/result-name-init-capture.cpp : *
- clang/test/Contracts/typeid-polymorphic-capture.cpp : *
- clang/test/Contracts/nested-copy-capture-in-predicate.cpp : *
- clang/test/Contracts/result-name-underscore.cpp : *
- clang/test/Contracts/contract-unexpanded-pack.cpp : *
- clang/test/Contracts/pre-without-parenthesis.cpp : *
- clang/test/Contracts/postcondition-discarded-member-access.cpp : *
- clang/test/Contracts/postcondition-discarded-member-template.cpp : *
- clang/test/Contracts/postcondition-unnamed-param-diagnostic.cpp : *
- clang/test/Contracts/postcondition-unnamed-param-definition.cpp : *
- clang/test/Contracts/qualified-friend-member-contract-mismatch.cpp : *
- clang/test/Contracts/violation-handler-signature-wording.cpp : *
- clang/test/Contracts/Runnable/contract-retval-destroyed-on-unwind-coro.cpp : *
- clang/test/Contracts/Runnable/post-result-address-register-returned.cpp : *
