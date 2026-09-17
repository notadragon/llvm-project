---
id: 1160-p2900-base-libcxx
subject: '[libc++][contracts] <contracts> and std::contracts::contract_violation'
depends: []
regenerates: []
fixes: []
coauthors: ['Eric Fiselier <eric@efcs.ca>']
---

## Rationale
The library half of C++26 contracts: `<contracts>`,
`std::contracts::contract_violation`, the default violation handler in
`libcxx/src/contracts.cpp`, and everything needed for a new header to be a
real libc++ header.

"Everything needed" is most of the file count and is not boilerplate.  A
header that is not registered in `include/CMakeLists.txt`, the
`module.modulemap.in`, the `std` module's source list, the transitive-include
CSVs and the generated-test scripts is a header the suite silently does not
test, and the failure is silence rather than an error.  Two of them are
easy to miss and both are load-bearing here: `contracts.inc` must be in
`LIBCXX_MODULE_STD_SOURCES` or `import std;` exports no contracts entities
at all, and `assert.h` is a header this commit ADDS, so it needs the same
registration every other libc++ header has.  That is why the file list is
long.

Three things worth a reviewer's attention:

**`source_location::__create_from_pointer` is a hack and is labelled one.**
`contract_violation` has to hand back a `std::source_location` built from
the ABI's own layout-compatible struct, and there is no supported way to do
that.  The comment says as much.  It is here rather than in a refactoring
band because it is an accommodation the base facility needs, not a
restructuring -- GCC's `2600-refactoring-after-libcontracts` has no
counterpart on this side for exactly that reason.

**`assert.h` deliberately has no include guard.**  It is the P3290
integration point: `assert` has to be redefinable according to the current
`NDEBUG` on every inclusion, which is what the C standard requires of
`<assert.h>` and what an include guard would break.  It `#include_next`s
the C library's version first so the integration redefines a definition
that is already in place.

**The test-suite parameter is two separate concepts and keeping them
separate was the bug fix.**  `use-contracts` means "compile the whole suite
with contracts enforced"; the `contracts` *feature*, probed independently
in `features/misc.py`, means "this compiler supports contracts".  Bundling
them meant that turning the suite-wide flags off also skipped the only two
tests that exercise `<contracts>`.  Both comments in `params.py` exist to
stop that being re-merged.

## Compile gap
Needs `1000-p2900-base-basic` for the semantic enumeration the library
mirrors, and is compiled by a compiler that already has
`1100-p2900-base-codegen`; both precede it.

**The violation handler's entry point is not here.**  `contracts.cpp`
defines the *default* handler and the `contract_violation` accessors; the
`__cxa_` entry point the compiler emits a call to, and the ABI header the
two sides agree on, are `2200-libcontracts`.  Standing alone, this links
only if that entry point is stubbed, and `2200-libcontracts` replaces the
stub.

`<contracts>`'s `contract_violation` here exposes the base accessors only.
P3099's `comment()`, P3400's label queries and P4301's `report()` are each
added by their own paper's commit, so the class grows over the series --
which is also why `libcxx/include/contracts` appears in five entries rather
than one.

**Behaviour trap:** with the entry point stubbed, a violated contract calls
a stub and returns.  Every `-fsyntax-only` test still passes, every
`Runnable` test still links, and a precondition that must terminate the
program silently does not.

## Contents
- clang/test/Contracts/Runnable/library-compiler-interface.cpp : *
- clang/test/Contracts/default-handler-noexcept.cpp : *
- libcxx/include/CMakeLists.txt : *
- libcxx/include/assert.h : *
- libcxx/include/contracts : /#ifndef _LIBCPP_CONTRACTS/, @contracts, @char, @assertion_kind, @__UINT16_TYPE__, @evaluation_semantic, @detection_mode, @evaluation_semantic_set, @public, @handled, @contract_violation, /__cxa_contract_data_block\* __chain_/, @invoke_default_contract_violation_handler, @__handle_manual_contract_violation, /__can_throw=\*\/true/, @__attribute__
- libcxx/include/module.modulemap.in : *
- libcxx/include/source_location : *
- libcxx/include/version : *
- libcxx/modules/CMakeLists.txt : *
- libcxx/modules/std.cppm.in : *
- libcxx/modules/std/contracts.inc : *
- libcxx/src/CMakeLists.txt : *
- libcxx/src/contracts.cpp : *
- libcxx/test/extensions/libcxx/no_assert_include.gen.py : *
- libcxx/test/libcxx/module_std.gen.py : *
- libcxx/test/libcxx/transitive_includes/cxx26.csv : *
- libcxx/test/libcxx/transitive_includes/cxx29.csv : *
- libcxx/test/std/contracts/enums.compile.pass.cpp : *
- libcxx/test/std/contracts/violation_handler.pass.cpp : *
- libcxx/test/std/header_inclusions.gen.py : *
- libcxx/utils/libcxx/test/features/misc.py : *
- libcxx/utils/libcxx/test/params.py : *
