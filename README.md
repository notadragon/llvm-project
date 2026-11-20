# C++ Contracts Implementation (P3850)

This fork implements the C++ Contracts extensions proposed in P3850,
building on Eric Fiselier's contracts-nightly branch.

The majority of the work going into the actual P2900 implementation
was done by Eric Fiselier and Corentin Jabot.  The additions in this
fork are all prototype-quality implementations to gain implementation
experience and explore design alternatives.

This branch is part of a larger project to implement Contracts features in
all open-source C++ compilers, and details on the current status and scope
of these changes can be found at https://github.com/notadragon/contracts-p3850.

## Compiler Explorer

A build of this branch is available on compiler explorer.  You can use it by
selecting the "x86-64 clang (P3850 contracts)" entry in the compiler dropdown
when editing C++ code at https://godbolt.org/.

Current nightly build status:
[![Status](https://github.com/compiler-explorer/compiler-workflows/actions/workflows/build-daily-clang_notadragon_contracts_p3850.yml/badge.svg)](https://github.com/compiler-explorer/compiler-workflows/actions/workflows/build-daily-clang_notadragon_contracts_p3850.yml)![Last success](https://img.shields.io/badge/dynamic/json?color=success&label=Last+OK&query=%24.last_success.timestamp&url=https%3A%2F%2Flambda.compiler-explorer.com%2Fcompiler-build%2Fclang_notadragon_contracts_p3850)![Last build](https://img.shields.io/badge/dynamic/json?color=yellow&label=Last+build&query=%24.last_build.timestamp&url=https%3A%2F%2Flambda.compiler-explorer.com%2Fcompiler-build%2Fclang_notadragon_contracts_p3850)

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
