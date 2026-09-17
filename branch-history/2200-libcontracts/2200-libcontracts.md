---
id: 2200-libcontracts
subject: 'libcontracts: add the contract-violation runtime library'
depends: []
regenerates: []
fixes: []
---

## Rationale
`libcontracts`: the runtime the compiler's violation path actually calls,
and the ABI the two sides agree on.

Everything before this commit emits a call to `__cxa_contract_violation*`
and stops there.  This is the other end of that call -- a small C library,
built as a runtime alongside the others, that receives a violation and
dispatches it to a handler.

**The ABI is the substance, not the code.**  A violation is passed as a
chain of `__cxa_contract_data_block`s, each carrying a descriptor table
naming which fields it holds; `__cxa_find_field` walks the chain looking
for a field id.  That indirection is what makes the ABI extensible without
a version bump: a producer that knows about a field emits it, a consumer
that does not skips it, and neither has to have been compiled against the
other's idea of the layout.  P3099's message, P3400's labels and P4301's
report are all later additions that need no change here because of it.

**It is C, and deliberately so.**  The library is linked into C programs
by P4299 and into C++ programs by everything else, so it cannot depend on
the C++ runtime.  That is also why `libcontracts/` and
`libcxx/src/contracts_abi.cpp` both exist: the C++ side needs the same
facts, and cannot include a `libcontracts/` header from inside libc++.
`libcxx/include/__contracts/abi.h` is that second copy, and the two are
kept byte-identical by hand.  **Do not "fix" the duplication** -- they are
separate build units and the copy is the price.

**`libcontracts.map` is a cross-compiler contract.**  The exported symbol
set and the `LIBCONTRACTS_1.0` version node have to match GCC's build of
the same library, because a program can be compiled by one compiler and
linked against the other's runtime.  The comment at the top of the file
says so, and its glob patterns are what let the per-paper entry points
be added later without editing it again.

`clang/lib/Headers/contracts.h` and `stdcontracts.h` are the freestanding
declarations a translation unit gets without libc++; `Gnu.cpp` is the
driver knowing to link the library; `CodeGenModule.cpp` is the compiler
emitting the descriptor tables the chain walk reads.

## Compile gap
None inward that is not already satisfied: this commit is self-contained C
plus its build files, and `1100-p2900-base-codegen` already emits the calls
it defines.

**It closes a gap rather than opening one.**  `1100-p2900-base-codegen`,
`1160-p2900-base-libcxx` and `1180-p2900-base-tests` each record that they
need a stub definition of the `__cxa_` entry point; this is the commit that
replaces that stub.  `1180-p2900-base-tests` is the one whose tests go from
failing to passing at this point, because observing the handler is what
they do.

Later papers add entry points to this library -- P4299's
`__c_contract_check_*`, P3290's `stdc_handle_*` -- and each rides with its
own paper's commit.  The version script already names them by glob, so no
later commit reopens it.

## Contents
- clang/lib/CodeGen/CodeGenModule.cpp : #4
- clang/lib/Driver/ToolChains/Gnu.cpp : *
- clang/lib/Headers/CMakeLists.txt : *
- clang/lib/Headers/contracts.h : *
- clang/lib/Headers/stdcontracts.h : *
- libcontracts/CMakeLists.txt : *
- libcontracts/accessors.c : *
- libcontracts/c_api.c : *
- libcontracts/contracts-abi.h : *
- libcontracts/dispatch.c : *
- libcontracts/libcontracts.map : *
- libcxx/include/__contracts/abi.h : *
- libcxx/src/contracts_abi.cpp : *
- libcxx/test/CMakeLists.txt : *
- llvm/CMakeLists.txt : *
- llvm/runtimes/CMakeLists.txt : *
- runtimes/CMakeLists.txt : *
