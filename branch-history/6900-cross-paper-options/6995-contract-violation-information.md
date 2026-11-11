---
id: 6995-contract-violation-information
subject: '[clang][contracts] -fcontract-violation-information=full|none'
depends: [1000-p2900-base-basic, 1020-p2900-base-ast, 1160-p2900-base-libcxx, 2200-libcontracts, 2400-p3595, 2900-umbrella-option, 3300-p3290, 3600-p3099, 4200-p3400-core, 4800-p4298, 5000-p4299, 6000-p3100-core, 6900-cross-paper-options]
regenerates: []
fixes: []
---

## Rationale

A contract-violation record names the file, function, line and column of
the assertion and carries its comment (the predicate's source text) and
its message, so all of that text lands in the object file.
`-fcontract-violation-information=none` keeps it out: every record the
compiler emits for a runtime check then has an empty file name, an empty
function name, line 0, column 0, an empty comment and no message, as for
a contract without one.  The default, `full`, records everything as
before.

It is one option for C, C++, Objective-C and Objective-C++, forwarded by
the driver whatever the other contracts options are, and it covers every
record the compiler builds: `ASTContext::BuildViolationObject` builds the
violation object of preconditions, postconditions, `contract_assert`, C
contract assertions, the implicit (P3100) contract assertions code
generation inserts, and the comment and message a P3400 label computes,
which by then are stored on the contract.  It replaces the values just
before the object is made, so the contract keeps its comment and message
and constant evaluation's diagnostics keep their locations.  The kind,
semantic, detection mode and the label fields of the record are
unchanged.  The `LangOptions.def` entry is `1000-p2900-base-basic`'s, as
every contracts LangOpt is.

The library's own records follow through a predefined macro,
`__clang_contracts_violation_information_none`: under it `<cassert>`'s
contract-checking `assert` (P3290) passes an empty comment and a
default-constructed `std::source_location`, and the P3290 convenience
macros of `<contracts.h>` pass an empty location and comment, still
evaluating a comment argument that has side effects.  The P3290 manual
API, `std::contracts::handle_*_contract_violation`, defaults its location
argument to an empty `source_location()` instead of the caller's; a
location passed explicitly is kept, as is the comment the caller passes.
The default (`std::source_location::current()`) is defined with the API,
in `1160-p2900-base-libcxx`; this commit adds its redefinition.  The
default violation handlers' omission of an empty location part or comment
stays with the handlers (`1160-p2900-base-libcxx`, `2200-libcontracts`).

The tests run the same programs under `none` and under an explicit
`full`, check the default handlers' output under each, scan the assembly
for the predicate text, messages, computed strings, function and `#line`
file names, check that constant evaluation still diagnoses at the
assertion with its message, and check the driver's forwarding and the
predefined macro in each language.  They mirror gnu_gcc's
`6990-contract-violation-information`.

## Compile gap

None.

## Contents

- clang/include/clang/Options/Options.td : /^def fcontract_violation_information_EQ/
- clang/lib/Driver/ToolChains/Clang.cpp : @ConstructJob:1
- clang/lib/Frontend/InitPreprocessor.cpp : #1:2
- clang/lib/AST/ASTContext.cpp : /-fcontract-violation-information=none: the record carries an empty$/
- clang/lib/Headers/contracts.h : #0:1
- libcxx/include/__cassert/assert_contract.h : #0:1
- libcxx/include/contracts : @labels:2
- clang/docs/UsersManual.md : #0
- clang/test/Contracts/contract-violation-information-*.cpp : *
- clang/test/Contracts/contract-violation-information-*.c : *
- clang/test/Contracts/Runnable/contract-violation-information-*.cpp : *
- clang/test/Contracts/Runnable/p3100-violation-information-*.cpp : *
- clang/test/Contracts/Runnable/p3290-api-violation-information-*.cpp : *
- clang/test/Contracts/C/c-violation-information-*.c : *
