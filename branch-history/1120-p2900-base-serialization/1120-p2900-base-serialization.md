---
id: 1120-p2900-base-serialization
subject: '[clang][contracts] serialize contracts for modules and PCH'
depends: []
regenerates: []
fixes: []
coauthors: ['Eric Fiselier <eric@efcs.ca>']
---

## Rationale
Reading and writing contracts in a PCH or a module: the four new record
kinds, and the `FunctionDecl` bit that says a function has contracts.

Mechanically routine -- a `Visit` method per node on each side -- and the
three places it is not are the places to read:

**`ContractSpecifierDecl` writes its contract count *before* `VisitDecl`.**
It is a trailing-object node, so `ReadDeclRecord` needs the count to size
the allocation before the declaration exists, which is earlier than the
fields `VisitDecl` writes.  Written in the natural order the reader takes
`NumContracts` from whatever field happens to sit at that offset, and the
record layout silently desynchronises from there.  The comment in the
writer says so; the ordering is load-bearing.

**`ResultNameDecl` is serialized through `VisitValueDecl`, not
`VisitNamedDecl`.**  It is a `ValueDecl`, and going through the narrower
base drops its type -- which survives the round trip as a null and then
crashes template instantiation of the postcondition after a module import,
a long way from the writer that caused it.

**`DeclaratorDecl`'s `hasExtInfo()` bit moves into a `BitsPacker`.**  That
is upstream's format, not ours; it is here because the contract bit added
to `FunctionDecl` pushed the neighbouring fields past where the unpacked
form was still convenient.

`clang/test/Modules/contracts.cppm` is the round-trip test and is the real
coverage for all of it: everything here is invisible until something is
written out and read back.

## Compile gap
Needs `1020-p2900-base-ast` for every node it serializes and
`1060-p2900-base-sema-core` for the contract-specifier attachment that
gives a `FunctionDecl` something to write.  Both precede it.

None outward.  Nothing later in the series calls into this commit; later
papers add fields to the nodes and those fields are serialized by the
commit that adds them.

**Behaviour trap:** omitting this commit does not break a build and does
not break a non-modular compile.  It breaks exactly one thing -- a contract
that crosses a module or PCH boundary -- and the failure is a null
dereference at the read, not a diagnostic.  A test suite without a
`.cppm` in it reports nothing.

## Contents
- clang/include/clang/Serialization/ASTBitCodes.h : *
- clang/lib/Serialization/ASTCommon.cpp : *
- clang/lib/Serialization/ASTReaderDecl.cpp : *
- clang/lib/Serialization/ASTReaderStmt.cpp : *
- clang/lib/Serialization/ASTWriterDecl.cpp : *
- clang/lib/Serialization/ASTWriterStmt.cpp : *
- clang/test/Modules/contracts.cppm : *
