# CLANG-882: `import std;` does not bring `std::chrono`'s using-directive for `chrono_literals`

**Status:** Fixed here (M2-Clang, 2026-11-03); open upstream.
**Component:** libc++ (modules)
**Upstream Link:** None found (searched 2026-11-03).  #72427 (closed) is a
different `import std;` literals problem (`std::string_literals` not
exported), fixed in 2023.
**Affects:** measured 2026-11-03 -- libc++ with Clang 18.1.0, 19.1.0,
20.1.0, 21.1.0, 22.1.6, the stock trunk nightly (`clang-trunk`,
f2823c28d856) and this branch (install at 1198e9b1a06f, with and without
`-fcontracts -fcontracts-p3400`).  `libcxx/modules/std/chrono.inc` is
identical on stock trunk and this branch.  The `#include <chrono>` and
`import <chrono>;` (header unit) forms of the same program compile on every
one of them.

## Reproducer

[`clang-882-import-std-chrono-literals.cpp`](clang-882-import-std-chrono-literals.cpp):

    import std;                  // or: import std.compat;
    using namespace std::chrono;
    auto x = 1h;

The file's header comment gives the commands that build `std.pcm` and
`std.compat.pcm` from `<prefix>/share/libc++/v1`.  Both imports fail:

    error: no matching literal operator for call to 'operator""h' with
    argument of type 'unsigned long long' or 'const char *', and no
    matching literal operator template

## The rule it breaks

[time.syn] declares, in `<chrono>`,

    namespace std::chrono {
      using namespace literals::chrono_literals;
    }

so `using namespace std::chrono;` nominates `std::chrono_literals`
transitively and `1h` finds `operator""h`.

D3400R6 (P3400) [std.modules] says the named module `std` "exports
declarations in namespace `std` and all *using-directive*s that are
provided by the importable C++ library headers", and `std.compat` "exports
the same declarations and *using-directive*s as the named module `std`".
So under D3400R6 `import std;` must make that directive visible, and the
program above must compile.  N5050's wording says only "exports
declarations in namespace `std`"; a using-directive is a declaration
inhabiting namespace `std::chrono`, so the current wording arguably
already requires it, and D3400R6 makes that explicit.

## Cause

libc++ builds the `std` module from `export namespace std { ... using
std::X; ... }` blocks in `libcxx/modules/std/*.inc`.  `chrono.inc`
re-exports every `std::chrono` entity and the literal operators in
`std::literals::chrono_literals`, but not the using-directive
`__chrono/duration.h` (and `__chrono/literals.h`) put in `std::chrono`.
A using-declaration list cannot carry a using-directive, and the test that
checks `std` against the headers (`libcxx/test/libcxx/module_std.gen.py`)
compares `using` lists of names, so it does not catch the omission.

This branch fixes it by exporting the directive from `chrono.inc`:

    export namespace std::chrono {
      using namespace literals::chrono_literals;
    } // namespace std::chrono

This branch's `contracts.inc` already exported a directive the same way
(`export using contract_control namespace std::contracts::labels;`), and
Clang carries it to importers.  `std.compat` re-exports `std`, and with it
the directive; no change to `std.compat` is needed.

## Tests

`clang/test/Contracts/libcxx-std-module-chrono-literals.cpp` (the
`XFAIL: *` watch test until the fix), `REQUIRES: libcxx-std-module`
(`std.cppm` in the build tree's `modules/c++/v1`, `%libcxx_moduledir`;
`Contracts/lit.local.cfg`).  It builds `std.pcm` and `std.compat.pcm` and
compiles the reproducer against each.

`verify-cases.txt` rows `CLANG-882a` (`import std;`) and `CLANG-882b`
(`import std.compat;`): setup rows build each compiler's own `std.pcm`
(and `std.compat.pcm`) from its libc++ module sources, then the reproducer
is compiled against them.

`verify.sh` does not measure this case: it has no mode that builds a
module first.  The stock measurement is the commands in the reproducer's
header.

## Draft upstream report

**Title:** [libc++][modules] `import std;` does not export
`std::chrono`'s using-directive for `chrono_literals`

**Body:**

    import std;                  // or: import std.compat;
    using namespace std::chrono;
    auto x = 1h;

fails with

    error: no matching literal operator for call to 'operator""h' with
    argument of type 'unsigned long long' or 'const char *', and no
    matching literal operator template

where `#include <chrono>` and `import <chrono>;` compile.  [time.syn]
declares `namespace std::chrono { using namespace
literals::chrono_literals; }` in `<chrono>`, and `std` exports the
declarations in namespace `std` the importable headers provide
([std.modules]); the using-directive is one of them.
`libcxx/modules/std/chrono.inc` re-exports the literal operators but not
the directive.  Exporting it fixes both modules:

    export namespace std::chrono {
      using namespace literals::chrono_literals;
    } // namespace std::chrono

Reproduced with Clang 18.1.0 through 22.1.6 and trunk.  If [std.modules]
is read as not requiring using-directives to be exported, the omission is
an LWG issue rather than a libc++ bug.

## Found by

The D4451 header-unit divergence survey (HU-Clang), measuring Clang and
libc++ against D3400R6's using-directive rules for header units and the
`std` modules.
