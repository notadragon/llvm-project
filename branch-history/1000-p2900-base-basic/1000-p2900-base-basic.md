---
id: 1000-p2900-base-basic
subject: '[clang][contracts][P2900] contract options, tokens and diagnostics'
depends: []
regenerates: []
fixes: []
coauthors: ['Eric Fiselier <eric@efcs.ca>']
---

## Rationale
The vocabulary every other contracts commit is written against: the language
options, the token kinds, the contract and semantic enumerations, and the
diagnostic groups.

Nothing here does anything on its own, which is the point -- it is the
header layer that lets the AST, parser, Sema and CodeGen commits that follow
refer to the same types.

One thing in here is worth a reviewer's attention: the
`AllContractSemanticsMask*` constants name P3100's `assume`, P4298's
noexcept-terminating semantics and P4405's `prove` and `diagnose` even
though none of those papers has landed at this point in the series.  They
are masks over the base enumeration, and the comment says explicitly that
the gates are applied elsewhere.  `isProvingContractSemantic` names the
P4405 pair for the places that must treat them as `ignore`, and
`InvalidContractEvaluationSemantic` is what the best fit yields when no
semantic is valid.

## Compile gap
`LANGOPT(Contracts, ...)` defaults to 0, and must: the driver gates on the
language standard and passes `-fno-contracts` below C++26, but a direct
`-cc1` invocation -- which is most of the test suite -- gets no such help.
Defaulting it on enables contracts for every `-cc1` compilation whatever the
standard, and the symptom is an upstream test with no contract syntax in it
suddenly needing `-fno-contracts` on its RUN line.  What turns the option on
is the driver, or the per-paper sub-flag implication in `ParseLangArgs`.

## Contents
- clang/include/clang/Basic/Builtins.td : *
- clang/include/clang/Basic/ContractOptions.h : /C\+\+ Contract Options/, @FirstContractEvaluationSemantic, @LastContractEvaluationSemantic, @isContractEvaluationSemanticValue, @llvm, @clang, @DiagnosticsEngine, @SourceManager, @ContractKind, @NumContractKindBits, @NumContractAllowedMaskB*, @ContractAssertionKind, @ContractEvaluationSema*, @AllContractSemanticsMa*, @gatedContractSemantics*, @contractSemanticFromNa*, @contractSemanticName, @ContractDetectionMode, @FunctionContext, @ContractOptions, @InvalidContractEvaluationSemantic, @ProvingContractSemanticsMask, @isProvingContractSemantic, @contractAnalysisAllowedMask, @contractRuntimeAllowedMask
- clang/include/clang/Basic/DiagnosticGroups.td : #0:0
- clang/include/clang/Basic/IdentifierTable.h : *
- clang/include/clang/Basic/LangOptions.def : *
- clang/include/clang/Basic/LangOptions.h : *
- clang/include/clang/Basic/TokenKinds.def : #0, #1:0
- clang/include/clang/Basic/TokenKinds.h : *
- clang/lib/Basic/CMakeLists.txt : *
- clang/lib/Basic/ContractOptions.cpp : /ContractOptions\.cpp - C\+\+ Contract Options/, @clang, /Delegates to contractSemanticFromName/
- clang/lib/Basic/IdentifierTable.cpp : #0:0, #1, #2, #3, #4, #5
- clang/lib/Frontend/InitPreprocessor.cpp : #0:0
- clang/test/Contracts/lit.local.cfg : *
- clang/test/Contracts/Runnable/lit.local.cfg : *
- clang/test/Contracts/README.txt : *
