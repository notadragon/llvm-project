# CLANG-881: `-ffile-prefix-map` does not remap the filenames UBSan and ASan embed

**Status:** Fixed here (C1-Clang, 2026-11-05); open upstream.
**Component:** clang:codegen, compiler-rt:asan
**Upstream Link:** None found as an issue (searched 2026-11-01); three open
pull requests address it, none ours:
[#186908](https://github.com/llvm/llvm-project/pull/186908) (adds
`-fsanitize-prefix-map=`, implied by `-ffile-prefix-map`),
[#201803](https://github.com/llvm/llvm-project/pull/201803) (extends
`-ffile-compilation-dir=` to sanitizer metadata instead), and
[#206465](https://github.com/llvm/llvm-project/pull/206465) (the ASan
globals, an RFC from the reporter of GCC's PR125980).
**Affects:** measured 2026-11-01 -- 18.1.0, 22.1.6 and the stock trunk
nightly (`clang-trunk-20261029`) remap `__FILE__` and neither sanitizer
filename.  `-fsanitize-undefined-strip-path-components=` can strip a UBSan
filename's leading components, but nothing remaps it, and nothing touches
ASan's.  GCC has the same defect (GCC-958 in the gnu_gcc fork, fixed
there).

## Reproducer

Two files, each printing `__FILE__` first for comparison.

[`clang-881-ubsan-prefix-map.cpp`](clang-881-ubsan-prefix-map.cpp) names
itself `/src/repro.cpp` with a `#line` directive, so the map does not
depend on where the directory is checked out.  At
`-fsanitize=integer-divide-by-zero -ffile-prefix-map=/src=/mapped`, run:

    __FILE__ is /mapped/repro.cpp
    /src/repro.cpp:4:12: runtime error: division by zero

[`clang-881-asan-prefix-map.cpp`](clang-881-asan-prefix-map.cpp) prints
ASan's description of a global, which a global-buffer-overflow report on
it prints too.  Clang records a global's module name -- the path the file
was compiled as -- not its line, so `#line` does not reach it and the map
has to name the real directory.  At `-fsanitize=address
-ffile-prefix-map=$PWD/clang-881=/mapped`, compiling
`$PWD/clang-881/clang-881-asan-prefix-map.cpp`, run:

    __FILE__ is /mapped/clang-881-asan-prefix-map.cpp
    0x... is located 0 bytes after global variable 'XXX' defined in '<$PWD>/clang-881/clang-881-asan-prefix-map.cpp' (0x...) of size 10

`verify.sh` measures only the UBSan file (CLANG-881): it compiles by
absolute path and normalises away everything up to `bug-reports/`, so no
map it can pass would show the ASan half.  The ASan half is measured by
its watch test, which maps `%S`.

## Cause

`CodeGenFunction::EmitCheckSourceLocation` (`clang/lib/CodeGen/CGExpr.cpp`)
takes the presumed filename and applies only
`-fsanitize-undefined-strip-path-components=`; the ASan pass
(`llvm/lib/Transforms/Instrumentation/AddressSanitizer.cpp`) takes the
module's source filename for a global's descriptor.  Neither consults the
prefix maps that `__FILE__` (`Preprocessor::processPathForFileMacro`) does.

## Fix here

`EmitCheckSourceLocation` remaps the presumed filename through the macro
prefix map (`LangOptions::remapPathPrefix`, as `__FILE__` is) before
stripping path components.  `AddressSanitizerOptions` gains a `ModuleName`,
which the ASan module pass records for globals and passes to
`__asan_before_dynamic_init` in place of the module identifier;
`BackendUtil.cpp` sets it to the remapped identifier, so the two stay equal
and initialization-order checking still recognises the module's own
initializers.  This matches the gnu_gcc fork's fix (GCC-958), which passes
the same names through `remap_macro_filename`.

## Tests

`clang/test/Contracts/Runnable/sanitizer-prefix-map-ubsan.cpp`,
`sanitizer-prefix-map-asan.cpp` and `sanitizer-prefix-map-asan-init-order.cpp`,
and `clang/test/Contracts/sanitizer-prefix-map-ir.cpp` (the IR).

## Found by

Mirroring GCC-958, found surveying which compiler options keep paths and
line numbers out of an object file.
