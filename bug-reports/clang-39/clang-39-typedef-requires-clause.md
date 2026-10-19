# CLANG-39: A requires-clause on a typedef of a function type is accepted

**Kind:** defect (accepts-invalid)
**Status:** Fixed here (M5-Clang, 2026-10-06) -- found 2026-09-25 (review
round #12, C-bugtrail#63, B-misc#60)
**Component:** clang / Parse
**Upstream Link:** None found (searched 2026-09-25: GitHub issue search on
llvm/llvm-project for "requires-clause typedef", "typedef requires clause",
"trailing requires clause typedef", "requires clause on typedef"; nothing
relevant)
**Affects:** measured 2026-09-25 with `-std=c++20 -fsyntax-only` -- Clang
18.1.0, 19.1.0, 20.1.0, 21.1.0, 22.1.6, trunk 761561a0b41f, and this branch
all accept line A of the reproducer
**Workaround:** none needed; the code is invalid
**GCC:** rejects it ("requires-clause on typedef"; 16.2.0 and trunk)

## Bug Report

| GitHub field | Value |
|---|---|
| Title | `[clang] Trailing requires-clause on a typedef of a function type is accepted` |
| Labels | `clang:frontend`, `accepts-invalid`, `concepts` |

Attachment: [`clang-39-typedef-requires-clause.cpp`](clang-39-typedef-requires-clause.cpp)

````
[dcl.decl.general]p4: "The optional requires-clause in an init-declarator or
member-declarator shall be present only if the declarator declares a
templated function."  Clang accepts it on a typedef:

  typedef int F(int) requires true;   // accepted, no diagnostic

while diagnosing the same clause on a variable:

  int x requires true;  // error: trailing requires clause can only be used
                        //        when declaring a function

GCC rejects the typedef ("requires-clause on typedef").  Seen with Clang
18.1.0 through trunk (clang -std=c++20 -fsyntax-only).
````

## Notes (ours, not for the report)

`Parser::ParseTrailingRequiresClause` (`clang/lib/Parse/ParseDeclCXX.cpp`)
diagnoses only `!D.isDeclarationOfFunction()`, and
`Declarator::isDeclarationOfFunction` (`clang/lib/Sema/DeclSpec.cpp`) looks
only at the declarator chunks: a typedef's function chunk makes it true.  The
storage class (`SCS_typedef`) is never consulted, and `Sema::ParseTypedefDecl`
then ignores the clause.  The branch already works around the same trap for
contracts in `Parser::ParseContractSpecifierSequence`
(`clang/lib/Parse/ParseContracts.cpp`), whose comment explains it.

Its `verify.sh` case is `CLANG-39` in `verify-cases.txt`.

Fixed on this branch by testing the storage class there too
(`ParseTrailingRequiresClause`); the regression test is the typedef cases
added to `clang/test/CXX/dcl.decl/dcl.decl.general/p4-20.cpp` (a namespace
typedef and a member typedef of a class template), which replaced the
watch test `clang/test/Contracts/OpenBugs/typedef-requires-clause.cpp`.
Mirrored in the GCC fork as an expected pass.
