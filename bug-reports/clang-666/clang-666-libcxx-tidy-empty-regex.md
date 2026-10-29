# CLANG-666: libc++'s clang-tidy plugin asserts on module partitions (empty regex)

**Kind:** defect (crash in a test tool; libc++'s `module_std.gen.py` cannot
run against a clang-tidy built with assertions)
**Status:** Fixed here (M17-Clang, 2026-10-09) -- found 2026-10-09 running
`module_std.gen.py` for CLANG-657
**Component:** libc++ test tools (`libcxx/test/tools/clang_tidy_checks`)
**Upstream Link:** None found (searched 2026-10-09: GitHub issue search on
llvm/llvm-project for "header_exportable_declarations", "module_std
clang-tidy assertion", "libcxx-tidy empty regex"; nothing relevant)
**Affects:** this branch's tree at the 2026-10-06 rebase (`f42afb122770`),
whose `header_exportable_declarations.cpp` matches upstream's; a
clang-tidy built with `LLVM_ENABLE_ASSERTIONS=ON` (as this program's release
tree is)
**Workaround:** a clang-tidy without assertions (upstream's CI uses one)

## What is wrong

The `libcpp-header-exportable-declarations` check registers, for a
`ModulePartition` or `CompatModulePartition`, the matcher
`namedDecl(anyOf(isExpansionInFileMatching(filename_),
isExpansionInFileMatching(extra_header_)))`.  `extra_header_` comes from
the optional `ExtraHeader` option, which `modules.py` passes only for the
few headers that need one, so it is usually empty; a header gets a
never-matching default (`"$^"`), a partition does not.  An empty regex
fails `createAndVerifyRegex`'s `assert(!Regex.empty())`, so with an
assertions-enabled clang-tidy the generated std-module test
(`libcxx/test/libcxx/module_std.gen.py`) aborts on its first partition
(`algorithm`), before it compares anything.

Fixed on this branch by registering the `extra_header_` matcher for a
partition only when the option is given (it cannot take the header's
`"$^"` default: partitions fall through to the module case, which rejects a
non-empty `ExtraHeader`).  No regression test or `verify.sh` case: the
check runs only under clang-tidy with the libcxx-tidy plugin (`REQUIRES:
has-clang-tidy`), which the normal configuration does not build;
`module_std.gen.py` is the test, run with `bin/clang/build.sh
--with-clang-tidy` (notadragon_wg21).
