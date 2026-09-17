---
id: 1100-p2900-base-codegen
subject: '[clang][contracts] emit contract checks'
depends: []
regenerates: []
fixes: []
coauthors: ['Eric Fiselier <eric@efcs.ca>']
---

## Rationale
Emitting a contract: `CGContracts.{h,cpp}`, and the hooks in
`CodeGenFunction` that place preconditions, postconditions and
`contract_assert` in a function's IR.

The shape is one check per contract, emitted according to its runtime
semantic: `ignore` emits nothing, `observe` and `enforce` call the
violation handler and then continue or terminate, and `quick_enforce`
traps.  `emitCheckForSemantic` is where that decision becomes IR, and
`EmitContractStmtAsFullStmt` is the entry point for a `contract_assert` in
statement position.

Three things a reviewer should look at:

**The violation object is a constant, built once and shared.**
`BuildContractViolationInfo` asks the AST for the `UnnamedGlobalConstantDecl`
describing the violation -- source location, kind, semantic -- and
`FinishViolationInfo` patches the descriptor-table pointer into field 0.
The patching is what it looks like: the constant is rebuilt with one
operand replaced, because the table it must point at is not known when the
AST builds the object.  Sharing matters -- a contract in a hot function
otherwise costs a fresh global per evaluation site.

**Enforce and trap blocks are shared per function, not per contract.**
`SharedEnforceBlock` and `SharedTrapBlock` are emitted once and branched to
from every check with the same kind, which is why the enforce block is keyed
by contract kind rather than hardcoded: a function with a precondition and
a postcondition reports each as its own kind through one block each.

**`ExitCXXTryStmtWithCatchIR` is new general CodeGen machinery, not
contract-specific.**  A contract whose predicate throws has to be caught
and turned into a violation, which needs a catch-all `try` whose handler is
IR this file generates rather than a `CXXCatchStmt` from the AST.  Nothing
in `CGException.cpp` could do that before.  It asserts its narrow contract
-- exactly one handler, no exception declaration -- and otherwise mirrors
`ExitCXXTryStmt` including the discard-if-nothing-can-throw path.

`CGCall.cpp`'s four changes are all in `EmitFunctionEpilog`: postconditions
run after the return value exists and before the function returns, and that
is the only place that ordering can be expressed.  `CGDebugInfo` and
`CGDecl` are small accommodations so a predicate's code is attributed to
the contract rather than to the line the function happens to start on --
`debug-info.cpp` pins that.

## Compile gap
Needs `1020-p2900-base-ast` for `ContractStmt` and
`ASTContext::BuildViolationObject`, `1060-p2900-base-sema-core` and
`1070-p2900-base-sema-predicate-scope` for a predicate that has been
checked, and `1000-p2900-base-basic` for the semantic enumeration.  All
precede it.

**The violation handler it calls does not exist yet.**
`EmitCxaContractViolationCall` emits a call to the `__cxa_` entry point
that `2200-libcontracts` provides, twelve hundred lines later.  Standing
alone this links only against a stub definition of that entry point;
`2200-libcontracts` replaces it with the real runtime.  The declaration
side needs nothing -- CodeGen emits the prototype itself.

`getOrCreateDescriptorTable` takes a `HasLocalHandler` parameter that is
always `false` here.  The `true` path is P3400's local violation handler,
`4260-p3400-facet-local-violation-handler`; the parameter is present from
the start because threading it through later would touch every caller.

**Behaviour trap:** `EmitContractStmtAsFullStmt` reads
`S.ensureRuntimeSemantic`, which without `2400-p3595` always answers with
the default for the contract's kind.  So a build stopping here evaluates
every contract at its default semantic and silently ignores any
configuration, rather than failing to build.

## Contents
- clang/lib/CodeGen/CGCall.cpp : *
- clang/lib/CodeGen/CGContracts.cpp : /CGContracts\.cpp - Emit LLVM Code/, @clang, @Enforce, @QuickEnforce, @Observe, @Ignore, @NoexceptEnforce, @NoexceptObserve, @PredicateFailed, @ExceptionRaised, @CodeGen, @ContractEmissionStyle, @T, @CurrentContractInfo, @SharedEnforceBlock, /SharedEnforceBlock\(\) = default/, @CreateTrap, @CreateTrapBlock, @SharedTrapBlock, /SharedTrapBlock\(\) = default/, @CGContractData, @CurContract, @CurrentContractRAII, @Create, @BasicBlock, @GetCxaEntryPointName, @getOrCreateDescriptorTable, @EmitCxaContractViolationCall, @FunctionCanThrow, @StmtCanThrow, @EmitContractStmt, @BuildContractCatchAllTry, @Constant, @BuildContractViolationInfo, @EmitContractStmtAsFullStmt, @emitContractDataBlock, @emitCheckForSemantic
- clang/lib/CodeGen/CGContracts.h : *
- clang/lib/CodeGen/CGDebugInfo.cpp : *
- clang/lib/CodeGen/CGDecl.cpp : *
- clang/lib/CodeGen/CGException.cpp : *
- clang/lib/CodeGen/CGStmt.cpp : #0, #1, #3
- clang/lib/CodeGen/CMakeLists.txt : *
- clang/lib/CodeGen/CodeGenFunction.cpp : #0, #1, #2, #3, #5, #7, #10
- clang/lib/CodeGen/CodeGenFunction.h : #0, #1, #2, #3, #4, #7, #8, #9
- clang/lib/CodeGen/CodeGenModule.cpp : #2, #3
- clang/lib/CodeGen/CodeGenModule.h : #2
- clang/test/Contracts/debug-info.cpp : *
- clang/test/Contracts/pure-virtual-dependent-predicate-codegen.cpp : *
