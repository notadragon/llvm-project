---
id: 1060-p2900-base-sema-core
subject: '[clang][contracts][P2900] attach and validate a contract specifier'
depends: []
regenerates: []
fixes: []
coauthors: ['Eric Fiselier <eric@efcs.ca>']
---

## Rationale
Semantic analysis for contracts: attaching a contract-specifier to a
declaration, validating it, and checking that redeclarations agree.

**This commit is larger than the rest of the base facility and could not be
made smaller.**  The plan was to divide `SemaContract.cpp` across the four
Sema commits -- core here, predicate scope in 1070, instantiation in 1080,
constant evaluation in 1090 -- and the facet and configuration work does
come out cleanly.  The base content does not, for a mechanical reason: five
of the file's segments are the bare line `namespace {`, byte-identical to
each other and sharing an anchor, so no selector in the mapping can tell
them apart.  What bounded the split is the addressability of a segment, not
its meaning.

A later pass can fix this by declaring cuts that give those openers
distinguishable neighbours, or by reordering the file so the anonymous
namespaces are not interchangeable.  Neither is worth doing before a
reviewer has said whether the split is wanted.

One file here is not contracts code at all.  This commit widens the upstream
diagnostic `err_disallowed_duplicate_attribute` with a
`%select{declaration|contract specifier}1`, so that a duplicate attribute on
a contract specifier can say so -- and widening a shared diagnostic obliges
every caller to pass the new argument.  `SemaHLSL.cpp`'s
`handleRootSignatureAttr` is one of them.  It travels here rather than with
the contracts code because a commit that changes the format string without
its callers aborts in `Diagnostic::getArgKind` on any translation unit that
reaches them, and the HLSL one is reached by an ordinary repeated
`[[RootSignature]]`.

## Compile gap
None outward: this commit depends on the AST nodes of 1020 and the parser of
1040, both of which precede it.

Inward, several later commits reach into the functions introduced here --
P3400's facet resolution, P3595's configuration lookup and P3098's capture
handling all hang off `BuildContractStmt` and `ActOnContractAssert`.  Those
call sites are in the later commits, not stubbed here.

## Contents
- clang/include/clang/Basic/DiagnosticSemaKinds.td : #0, /cannot appear more than once on a %select/, /\bdef\s+err_contract_on_deleted_or_defaulted\b/, /\bdef\s+err_typecheck_assign_constified\b/, /\bdef\s+select_contract_kind\b/, /\bdef\s+err_contract_violation_handler_invalid\b/, /\bdef\s+err_contract_result_name_shadows_param\b/, /\bdef\s+err_contract_result_name_void_return\b/, /\bdef\s+err_contract_result_name_not_allowed\b/, /\bdef\s+err_constexpr_contract_failure\b/, /\bdef\s+warn_constexpr_contract_failure\b/, /\bdef\s+err_contract_constant_initialization_failed\b/, /\bdef\s+note_initialization_changed_contract_semantic\b/, /\bdef\s+err_function_different_contract_seq\b/, /\bdef\s+note_contract_spec_seq_arity_mismatch\b/, /\bdef\s+note_mismatched_contract\b/, /\bdef\s+note_previous_contracts\b/, /\bdef\s+err_contract_postcondition_parameter_type_invalid\b/, /\bdef\s+note_contract_postcondition_parameter_use\b/, /\bdef\s+err_contract_coroutine_postcondition_param\b/, /\bdef\s+note_contract_coroutine_postcondition_param\b/, /\bdef\s+err_contract_result_name_deduced_non_definition\b/, /\bdef\s+note_contract_function_return_type\b/, /\bdef\s+note_contract_context\b/, /\bdef\s+note_cxx_this_const_in_contract_introduced_here\b/, /\bdef\s+err_keyword_not_allowed_in_contract\b/, /\bdef\s+err_contract_member_access_without_object\b/
- clang/include/clang/Sema/Sema.h : #0
- clang/lib/Sema/CMakeLists.txt : *
- clang/lib/Sema/ScopeInfo.cpp : *
- clang/lib/Sema/SemaContract.cpp : @DiagnoseContractOnDeletedOrDefaulted, @isOwnParameterScope, @isMemberTemplateSpecialization, @ActOnLateParsedFunctionContracts, /SemaContract\.cpp - Semantic Analysis/, @clang, @SemaContractHelper, @namespace, /^  bool Entered = false;$/, /^  const FunctionDecl \*FD;$/, /^  ContractStmt \*CurrentContract = nullptr;$/, /^  DenseSet<ParmVarDecl \*> DiagnosedDecls;$/, /^  struct Use \{$/, /^  explicit CoroutineParamUseFinder/, /^  ParamReferenceChecker\(Sema &S, const FunctionDecl \*FD\)/, @ValueRAII, @ActOnContractAssertCondition, @getContractViolationType, @hasNonTemplateRequiresClause, @BuildContractStmt, @BuildInvalidContractStmt, @SetTransformedContracts, @ActOnContractAssert:0, @ActOnContractAssert:2, @ActOnResultNameDeclarator, @ContractScopeRecord, @ScopeEntry, @walkStartContext, @getScopeEntries, @CheckEquivalentContractSequence:0, @CheckEquivalentContractSequence:2, @CheckContractViolationHandler, @BuildImplicitMemberExprInContract, @getParamNameForDiag, @isOwnValueParmOdrUse, @Derived, @ParamReferenceChecker, @diagnoseRedeclParamConst, @diagnoseParamTypes, @PostconditionParamUseCollector, @CheckPostconditionParamsOnRedecl, @CoroutineParamUseFinder, @diagnoseCoroutinePostconditionParams, @CheckFunctionContracts, @holdsPatternContractSpecifier, @InstantiateContractSpecifier, @ContractSpecifierDecl, @ActOnContractsOnFinishFunctionDecl, @RebuildFunctionContracts, @RebuildContractsWithPlaceholderReturnType, @ActOnContractsOnFinishFunctionBody, @isUsageAcrossContract, @getContractConstification, @DeclContext, @adjustCXXThisTypeForContracts, @isDirectlyInContractPredicate, @getContractScopes, @getCurrentContractEntry, @ContractScopeRAII, @PushContractScope, @PopContractScope, @getStdContractsTagType
- clang/lib/Sema/SemaDecl.cpp : #0, #1, #2, #3, #4, #5, #6, #7, #8, #9, #10, #11, #12, #13, #14, #15, #16, #17, #18, #20
- clang/lib/Sema/SemaDeclAttr.cpp : *
- clang/lib/Sema/SemaHLSL.cpp : *
- clang/lib/Sema/SemaDeclCXX.cpp : @CheckConstexprFunctionStmt, @ActOnFinishDelayedCXXMethodDeclaration, @PushUsingDirective, @SetDeclDeleted
- clang/lib/Sema/SemaExceptionSpec.cpp : /ContractStmtClass/
- clang/lib/Sema/SemaExprMember.cpp : *
- clang/lib/Sema/SemaOverload.cpp : *
- clang/test/Contracts/Sema/basic.contract.eval.cpp : *
- clang/test/Contracts/Sema/decl.contract.func.cpp : *
- clang/test/Contracts/Sema/decl.contract.res.cpp : *
- clang/test/Contracts/constructor-contracts.cpp : *
- clang/test/Contracts/contract-friend-deferred-mismatch.cpp : *
- clang/test/Contracts/contract-invalid-predicate-redecl.cpp : *
- clang/test/Contracts/contract-redecl-mismatch-diagnostics.cpp : *
- clang/test/Contracts/contract-sequence.cpp : *
- clang/test/Contracts/deduce-return-type.cpp : *
- clang/test/Contracts/friend-access-in-predicate.cpp : *
- clang/test/Contracts/predicate-instantiation-in-initializers.cpp : *
- clang/test/Contracts/member-function-contracts.cpp : *
- clang/test/Contracts/nested-class-contracts.cpp : *
- clang/test/Contracts/Sema/over.call.func.cpp : *
- clang/test/Contracts/postcondition-redecl-no-diagnostics.cpp : *
- clang/test/Contracts/postcondition-nonconst-param-redecl.cpp : *
