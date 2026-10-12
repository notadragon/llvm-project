# C++ Contracts Implementation (P3850)

This fork implements the C++ Contracts extensions proposed in P3850,
building on Eric Fiselier's contracts-nightly branch.

The majority of the work going into the actual P2900 implementation
was done by Eric Fiselier and Corentin Jabot.  The additions in this
fork are all prototype-quality implementations to gain implementation
experience and explore design alternatives.

A concurrent implementation of this same work in GCC is available at:
https://github.com/notadragon/gnu_gcc (contracts-p3850 branch)

## Compiler Explorer

A build of this branch is available on compiler explorer.  You can use it by
selecting the "x86-64 clang (P3850 contracts)" entry in the compiler dropdown
when editing C++ code at https://godbolt.org/.   

Current nightly build status:
[![Status](https://github.com/compiler-explorer/compiler-workflows/actions/workflows/build-daily-clang_notadragon_contracts_p3850.yml/badge.svg)](https://github.com/compiler-explorer/compiler-workflows/actions/workflows/build-daily-clang_notadragon_contracts_p3850.yml)![Last success](https://img.shields.io/badge/dynamic/json?color=success&label=Last+OK&query=%24.last_success.timestamp&url=https%3A%2F%2Flambda.compiler-explorer.com%2Fcompiler-build%2Fclang_notadragon_contracts_p3850)![Last build](https://img.shields.io/badge/dynamic/json?color=yellow&label=Last+build&query=%24.last_build.timestamp&url=https%3A%2F%2Flambda.compiler-explorer.com%2Fcompiler-build%2Fclang_notadragon_contracts_p3850)

## Branch

- `contracts-p3850` -- Full P3850 implementation

## Implemented Papers --- [P3850R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3850r1.pdf) -- A proposed plan for extending Contracts in C++29

All of the following can be activated with `-fcontracts-p3850`, or `-fcontracts-p####` to enable specific paper support.  (Not all variations have been thoroughly tested).

Some papers were called out as ready in P3850, others have been written and implemented in support of the other papers, and others fall in the "small yet fully formed" category.

- 100% [P2900R14](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2900r14.pdf) -- Contracts for C++
- 100% [P3099R3](https://isocpp.org/files/papers/P3099R3.pdf) -- Contracts for C++: User-defined Diagnostic Messages
- 46% -- [P3100R7](https://isocpp.org/files/papers/P3100R7.pdf) -- A framework for systematically addressing undefined behaviour in the C++ Standard
  - [D4277R0](https://isocpp.org/files/papers/D4277R0.pdf) --- Overview and Implementation Report for P3100 (pending publication)
- 100% [P3290R6](https://isocpp.org/files/papers/P3290R6.pdf) -- Integrating Existing Assertions with Contracts
- 95% [P3400R4](https://isocpp.org/files/papers/P3400R4.pdf) -- Controlling Contract-Assertion Properties
- 100% [P3595R0](https://isocpp.org/files/papers/P3595R0.pdf) -- Configuration of Contract Evaluation Semantics
- 100% [P3098R3](https://isocpp.org/files/papers/P3098R3.pdf) -- Contracts for C++: Postcondition Captures
- 100% [P3097R3](https://isocpp.org/files/papers/P3097R3.pdf) -- Contracts for C++: Support for Virtual Functions
- 100% [P4283R0](https://isocpp.org/files/papers/P4283R0.pdf) -- Requires Clauses for Contract Assertions
- 100% [P4298R0](https://isocpp.org/files/papers/P4298R0.pdf) -- Nonthrowing Evaluation Semantics
- 100% [D4299R0](https://isocpp.org/files/papers/D4299R0.pdf) -- C++ Contracts for C (pending publication)
- 100% [D4301R0](https://isocpp.org/files/papers/D4301R0.pdf) -- Context Reports for the Contract-Violation Handler

## Key features:
- ABI compatability: This compiler is fully ABI-compatible with the corresponding
  clang implementation.

## Known Bugs and Open Issues

Open issues with this branch are tracked in the branch itself so that
consumers have one place to go to see the current state.  Details can be
found in two distinct lists:

- [open-issues/README.md](open-issues/README.md) -- **everything
  contracts-related that is known to be broken on this branch**, whatever
  its origin.  If you encounter issues, double-check here to see if they
  are known.  Some of these are underlying issues that we have deferred,
  others are in progress.
- [bug-reports/README.md](bug-reports/README.md) -- bugs that exist in
  the upstream trunk build of Clang, many of which have been fixed in this
  branch.

Some items that exist upstream which we have not yet fixed are in both
lists.

## Contact

Joshua Berne -- jberne4@bloomberg.net

---

# The LLVM Compiler Infrastructure

[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/llvm/llvm-project/badge)](https://securityscorecards.dev/viewer/?uri=github.com/llvm/llvm-project)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/8273/badge)](https://www.bestpractices.dev/projects/8273)
[![libc++](https://github.com/llvm/llvm-project/actions/workflows/libcxx-pr-conformance-tests.yaml/badge.svg?branch=main&event=schedule)](https://github.com/llvm/llvm-project/actions/workflows/libcxx-pr-conformance-tests.yaml?query=event%3Aschedule)

Welcome to the LLVM project!

This repository contains the source code for LLVM, a toolkit for the
construction of highly optimized compilers, optimizers, and run-time
environments.

The LLVM project has multiple components. The core of the project is
itself called "LLVM". This contains all of the tools, libraries, and header
files needed to process intermediate representations and convert them into
object files. Tools include an assembler, disassembler, bitcode analyzer, and
bitcode optimizer.

C-like languages use the [Clang](https://clang.llvm.org/) frontend. This
component compiles C, C++, Objective-C, and Objective-C++ code into LLVM bitcode
-- and from there into object files, using LLVM.

Other components include:
the [libc++ C++ standard library](https://libcxx.llvm.org),
the [LLD linker](https://lld.llvm.org), and more.

## Getting the Source Code and Building LLVM

Consult the
[Getting Started with LLVM](https://llvm.org/docs/GettingStarted.html#getting-the-source-code-and-building-llvm)
page for information on building and running LLVM.

For information on how to contribute to the LLVM project, please take a look at
the [Contributing to LLVM](https://llvm.org/docs/Contributing.html) guide.

## Getting in touch

Join the [LLVM Discourse forums](https://discourse.llvm.org/), [Discord
chat](https://discord.gg/xS7Z362),
[LLVM Office Hours](https://llvm.org/docs/GettingInvolved.html#office-hours) or
[Regular sync-ups](https://llvm.org/docs/GettingInvolved.html#online-sync-ups).

The LLVM project has adopted a [code of conduct](https://llvm.org/docs/CodeOfConduct.html) for
participants to all modes of communication within the project.
