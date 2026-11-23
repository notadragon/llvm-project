---
id: 0100-upstream-bugfixes
subject: '[clang] upstream fixes found while implementing contracts'
depends: []
regenerates: []
fixes: []
---

## Rationale
Eight upstream defects found while implementing contracts, none
contracts-specific and none ours.  They lead the branch because a
reviewer should be able to take them without taking anything else.

The constant evaluator accepted a derived-to-virtual-base conversion on an
object outside its lifetime.  Computing the virtual base offset consults the
most-derived object's layout, which is a use of the object, so doing it to
one that is deleted or not yet constructed is undefined and cannot be a
constant expression.  `HandleLValueBase` had no liveness check on that path
where the other operations consulting a most-derived type -- member calls,
`dynamic_cast`, `typeid` -- all have one.  GCC already rejects it.  The
conversion gets an access kind of its own, `AK_VirtualBaseConversion`, so
the note names it ("conversion to a virtual base of object outside its
lifetime") rather than borrowing `dynamic_cast`'s.

`ParamReferenceReplacerRAII` did not restore parameters after a coroutine
body.  Its destructor used `DenseMap::insert`, which is a no-op when the key
is already present, and the key always is -- `addCopy` redirected the
parameter by overwriting it in place.  So every parameter stayed pointing at
its coroutine-frame copy for the rest of the function, which is the exact
opposite of what the RAII exists to undo.  Contracts made it visible, by
naming a parameter from a precondition and the frame copy from a
postcondition and getting two different objects for one name, but the fault
is upstream's and predates contracts.

A trailing requires-clause on a typedef of a function type,
`typedef int F(int) requires true;`, was accepted and ignored.
[dcl.decl.general] allows one only on a declarator that declares a
templated function, and `ParseTrailingRequiresClause` diagnosed only a
declarator without a function chunk, which a typedef of function type has;
it now tests the storage class too.  GCC rejects it.

The noexcept operator ignored the elements an initializer list leaves to
its array filler: for `struct A { S s[2]; }` with a potentially-throwing
`S::S()`, `noexcept(A{})` was true.  `Sema::canThrow` treated `InitListExpr`
like any expression its children decide, and the filler is not among
`InitListExpr::children()`; it now has a case of its own that merges the
filler's answer.  GCC is wrong the same way.

libc++'s clang-tidy plugin registered `isExpansionInFileMatching` with an
empty regex for a module partition given no `ExtraHeader`, which a
clang-tidy built with assertions rejects, so the generated std-module test
(`module_std.gen.py`) aborted on its first partition.  The extra-header
matcher is now registered only when the option is given.

libc++'s `std` and `std.compat` modules did not export the using-directive
`<chrono>` declares in `std::chrono` ([time.syn]: `using namespace
literals::chrono_literals;`), so `import std; using namespace std::chrono;
auto x = 1h;` did not compile where the `#include` form does.  `chrono.inc`
re-exported the literal operators but not the directive; it now exports
that too, and `std.compat` follows by re-exporting `std`.

`-ffile-prefix-map` and `-fmacro-prefix-map` remapped `__FILE__` but not
the filenames the sanitizers embed, so the build path stayed in the object
and in the reports.  A UBSan source location took the presumed filename and
applied only `-fsanitize-undefined-strip-path-components=`; it is now
remapped first, as `__FILE__` is.  The ASan pass recorded the module
identifier as the module name of every global and passed the same name to
`__asan_before_dynamic_init`; `AddressSanitizerOptions` gains a
`ModuleName`, which Clang sets to the remapped identifier, so both uses
agree and a translation unit's own dynamic initialization is still not an
initialization-order fiasco.  GCC had the same defect.  How the maps
themselves apply to `__FILE__` is upstream's, and differs from GCC's (the
first of an equal old prefix wins across the two options, no
`-fcanon-prefix-map`); `prefix-map-precedence.c` pins it.

`Block::getMetadataSize` in the bytecode interpreter was a constexpr
function ending in `llvm_unreachable`, which with assertions enabled calls
the non-constexpr `llvm_unreachable_internal`; GCC 8, above LLVM's minimum
host compiler, rejects that in a constexpr function's body, so `clangAST`
did not build with the RHEL 8 system compiler.  Nothing evaluates the
function in a constant expression, so it is no longer constexpr.

## Compile gap
None in the code.  All eight are self-contained fixes to existing upstream
code, and none references anything this branch adds.  The `std`-module
test sits in `clang/test/Contracts/` and uses that directory's
`%libcxx_flags` and `%libcxx_moduledir` substitutions, which a later commit
adds; so do the sanitizer prefix-map tests, the runnable ones with
`%libcxx_flags`.

## Contents
- clang/lib/AST/ExprConstant.cpp : @HandleLValueDirectVirtualBase, @HandleLValueBase, @isModification, @isValidIndeterminateAccess
- clang/lib/CodeGen/CGCoroutine.cpp : #0
- clang/test/SemaCXX/constexpr-vbase-lifetime.cpp : *
- clang/lib/Parse/ParseDeclCXX.cpp : #19
- clang/lib/Sema/SemaExceptionSpec.cpp : /InitListExpr/
- clang/test/SemaCXX/noexcept-array-filler.cpp : *
- libcxx/test/tools/clang_tidy_checks/header_exportable_declarations.cpp : *
- libcxx/modules/std/chrono.inc : *
- clang/test/Contracts/libcxx-std-module-chrono-literals.cpp : *
- clang/test/CXX/dcl.decl/dcl.decl.general/p4-20.cpp : *
- clang/test/CXX/dcl.decl/dcl.decl.general/requires-declarator.cpp : *
- clang/lib/CodeGen/CGExpr.cpp : /\bFilename(String|Buf)\b|// The filename is remapped as __FILE__|// -ffile-prefix-map implies\), then stripped/
- clang/lib/CodeGen/BackendUtil.cpp : #1, #2, #3, #5
- llvm/include/llvm/Transforms/Instrumentation/AddressSanitizer.h : *
- llvm/lib/Transforms/Instrumentation/AddressSanitizer.cpp : *
- clang/test/Contracts/sanitizer-prefix-map-ir.cpp : *
- clang/test/Contracts/Runnable/sanitizer-prefix-map-asan.cpp : *
- clang/test/Contracts/sanitizer-prefix-map-asan-lto.cpp : *
- clang/test/Contracts/Runnable/sanitizer-prefix-map-asan-init-order.cpp : *
- clang/test/Contracts/Runnable/sanitizer-prefix-map-ubsan.cpp : *
- clang/test/Preprocessor/prefix-map-precedence.c : *
- clang/test/CXX/dcl.decl/dcl.decl.general/requires-declarator-effect.cpp : *
- clang/include/clang/Basic/DiagnosticASTKinds.td : #0, #1, #2
- clang/lib/AST/ByteCode/State.h : #0
- clang/lib/AST/ByteCode/Block.h : *
