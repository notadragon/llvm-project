---
id: 6996-contracts-option-docs
subject: '[clang][contracts] user documentation for the contracts option rule and -fcontracts-p4451'
depends: [4930-p4405-analysis, 5400-p4451, 6900-cross-paper-options, 6990-p3100-user-docs, 6995-contract-violation-information]
regenerates: []
fixes: []
---

## Rationale
A section of `UsersManual.md`, "Contracts extension options", in the
chapter on language-independent features, since the option rule covers C
as well as C++.

"How the contracts options combine" documents the rule
`handleContractsOptionRule` implements: in C++ every per-paper option
turns `-fcontracts` on and is an error with an explicit, final
`-fno-contracts`; the options are additive, so `-fno-contracts-p3850`
never turns off one given explicitly; P4451 is on when it, P3400 or the
umbrella is on, and `-fno-contracts-p4451` with P3400 on is an error; in C
`-fcontracts-p3100`, `-fcontracts-p4298`, `-fcontracts-p4405` and the
umbrella relate to `-fcontracts-p4299` in the same way, and the other
per-paper options, `-fcontracts` and the two C++-only non-paper options
are C++ only; and which options warn that they have no
effect, the negative forms never.

"Terminating contract assertions" documents `-fcontracts-p4451`: the
`< terminating >` specifier and where it is written, the semantics a
terminating assertion takes (configured, dynamically selected, in constant
evaluation, the constant-initialization exception, `prove` under P4405),
the two lookup requirements, redeclaration matching, the diagnostics
without P3400 or without the option, P3400's subsumption, and the
feature-test macros and libc++ declarations.

"Running a program that uses contracts" documents the run-time
requirement of `libcontracts.so.1`: a program linked with contracts on
(and, through libc++'s linker script, every `-stdlib=libc++` program)
needs it, and when Clang is installed outside the system's library path
`-frtlib-add-rpath`, `-Wl,-rpath` or `LD_LIBRARY_PATH` supplies its
directory, as for the installation's `libc++.so.1`.

It comes after the options it documents and the sections it refers to.

## Compile gap
None: documentation only.

## Contents
- clang/docs/UsersManual.md : #1
