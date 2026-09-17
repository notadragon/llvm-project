---
id: 1060-p2900-base-sema-core
subject: '[clang][contracts] attach and validate a contract specifier'
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
- clang/include/clang/Basic/DiagnosticSemaKinds.td : #0, /\bdef\s+err_typecheck_assign_constified\b/, /\bdef\s+err_disallow_incompatible_attribute\b/, /\bdef\s+select_contract_kind\b/, /\bdef\s+err_contract_violation_handler_invalid\b/, /\bdef\s+err_result_name_shadows_param\b/, /\bdef\s+err_void_result_name\b/, /\bdef\s+err_deduced_auto_result_name_without_body\b/, /\bdef\s+err_result_name_not_allowed\b/, /\bdef\s+err_contract_violation\b/, /\bdef\s+err_contract_requirement_failed\b/, /\bdef\s+warn_contract_requirement_failed\b/, /\bdef\s+err_constexpr_contract_failure\b/, /\bdef\s+warn_constexpr_contract_failure\b/, /\bdef\s+err_initialization_of_constant_initialized_variable_failed\b/, /\bdef\s+note_initialization_changed_contract_semantic\b/, /\bdef\s+err_function_different_contract_seq\b/, /\bdef\s+note_contract_spec_seq_arity_mismatch\b/, /\bdef\s+note_previous_contract_spec_seq\b/, /\bdef\s+note_mismatched_contract\b/, /\bdef\s+note_previous_contracts\b/, /\bdef\s+err_contract_attr_bool_arg_not_constant\b/, /\bdef\s+warn_contract_always_evaluates_to\b/, /\bdef\s+err_contract_postcondition_parameter_type_invalid\b/, /\bdef\s+err_contract_coroutine_postcondition_param\b/, /\bdef\s+note_contract_coroutine_postcondition_param\b/, /\bdef\s+note_parameter_with_name\b/, /\bdef\s+note_parameter_with_name_and_type\b/, /\bdef\s+err_lambda_implicit_capture_in_contracts_only\b/, /\bdef\s+note_lambda_implicit_capture_in_contract_usage\b/, /\bdef\s+note_lambda_implicit_capture_in_contracts_only\b/, /\bdef\s+err_auto_result_name_on_non_def_decl\b/, /\bdef\s+note_function_return_type\b/, /\bdef\s+note_contract_context\b/, /\bdef\s+note_cxx_this_const_in_contract_introduced_here\b/, /\bdef\s+err_keyword_not_allowed_in_contract\b/, /\bdef\s+err_lambda_decl_ref_not_modifiable_lvalue_contract\b/, /\bdef\s+err_contract_message_and_attribute\b/, /\bdef\s+err_contract_member_access_without_object\b/
- clang/include/clang/Sema/Sema.h : #0, #1, #3
- clang/lib/Sema/CMakeLists.txt : *
- clang/lib/Sema/ScopeInfo.cpp : *
- clang/lib/Sema/SemaContract.cpp : /SemaContract\.cpp - Semantic Analysis/, @clang, @SemaContractHelper, @namespace, @private, @ValueRAII, @public, @RebuildAutoResultName, @ActOnContractAssertCondition, @getContractViolationType, @BuildContractStmt, @ActOnContractAssert, @ActOnResultNameDeclarator, @ContractScopeRecord, @ScopeEntry, @ScopeWalker, @getScopeEntries, @getInterveningScopeEntries, @getInterveningContractEntry, @CheckEquivalentContractSequence, @ParamReferenceChecker, @diagnoseRedeclParamConst, @diagnoseParamTypes, @CoroutineParamUseFinder, @diagnoseCoroutinePostconditionParams, @CheckFunctionContracts, @holdsPatternContractSpecifier, @InstantiateContractSpecifier, @ContractSpecifierDecl, @ActOnContractsOnFinishFunctionDecl, @RebuildFunctionContracts, @RebuildContractsWithPlaceholderReturnType, @ActOnContractsOnFinishFunctionBody, @isUsageAcrossContract, @getContractConstification, @DeclContext, @adjustCXXThisTypeForContracts, @CaptureUsage, @LambdaCaptureChecker, @CheckLambdaCapturesForContracts, @getFunctionScopeIndexForDeclaration, @isContractAssertionContext, @getAllContractScopes, @getContractScopes, @FunctionScopeInfo, @getCurrentContractEntry, @WalkUpContractScopesTest, @ContractScopeRAII, @PushContractScope, @PopContractScope, @SourceLocation
- clang/lib/Sema/SemaDecl.cpp : *
- clang/lib/Sema/SemaDeclAttr.cpp : *
- clang/lib/Sema/SemaHLSL.cpp : *
- clang/lib/Sema/SemaDeclCXX.cpp : *
- clang/lib/Sema/SemaExceptionSpec.cpp : *
- clang/lib/Sema/SemaExprMember.cpp : *
- clang/lib/Sema/SemaInit.cpp : *
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
- clang/test/Contracts/friendship.cpp : *
- clang/test/Contracts/initialization.cpp : *
- clang/test/Contracts/inline-methods.cpp : *
- clang/test/Contracts/nested-class-contracts.cpp : *
- clang/test/Contracts/over.call.func.p3.1.cpp : *
- clang/test/Contracts/postcondition-redecl-no-diagnostics.cpp : *
