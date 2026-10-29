---
id: 1170-p2900-base-lambdas
subject: '[clang][contracts][P2900] contracts and lambdas'
depends: []
regenerates: []
fixes: []
---

## Rationale
Everything in the base facility that is about lambdas, in one commit, so
that it can be reviewed -- and posted -- on its own.  P2900 touches
lambdas in two directions, and both are here.

**A lambda can have contracts.**  `[](int x) pre(x > 0) { ... }` parses a
contract-specifier-seq after the lambda declarator and attaches it to the
call operator (`ParseExprCXX.cpp`, `ActOnStartOfLambdaDefinition`), and
template instantiation carries the specifiers across to the new call
operator, which `CreateLambdaCallOperator` builds without them
(`TransformLambdaExpr`).

**A contract can contain a lambda, and what the lambda captures is
const.**  [expr.prim.lambda.capture] makes an entity captured by reference
from inside a contract assertion const in the lambda, as the predicate's
own references to it are.  Sema records on each capture whether it was
made across a contract (`sema::Capture`, and `LambdaCapture` in the AST,
serialized), and gives such a capture by reference a reference-to-const
member and initializer (`BuildCaptureField`, `BuildCaptureInit`); the
capture walk in `tryCaptureVariable` decides it, also for a capture
rebuilt by instantiation or by `RebuildLambdaScopeInfo`.  `*this` captured
by copy is the exception: it is a new object, so its constness comes from
the lambda's own call operator.  A result name is a local entity, so a
lambda in a postcondition captures it like a variable, and a capture-default
is allowed where the lambda's innermost enclosing scope is a contract
assertion.  An implicit capture odr-used only in contract assertions is
ill-formed, which `LambdaCaptureChecker` diagnoses once the lambda is
complete (`-fno-contract-lambda-capture-restrictions` turns it off).

The `LANGOPT` for that flag is in `1000-p2900-base-basic`'s block of
contract language options, and the capture import in `ASTImporter.cpp` is
in `1020-p2900-base-ast` with the rest of that file.

## Compile gap
Needs the contract scope and constification of `1070-p2900-base-sema-predicate-scope`
(`getContractConstification`, `isInContract`) and the instantiation of
`1080-p2900-base-sema-templates`; both precede it.  `1020-p2900-base-ast`'s
`ASTImporter.cpp` copies `LambdaCapture`'s contract bit, which this commit
adds, and `1070`'s `getCurrentThisType` computes the uncontracted `this`
type that only this commit's `*this`-capture path reads.

None outward.

## Contents
- clang/include/clang/AST/Decl.h : #16, #17, #18
- clang/include/clang/AST/LambdaCapture.h : *
- clang/include/clang/Options/Options.td : @restrictions
- clang/include/clang/Sema/ScopeInfo.h : #2, #3, #4, #5, #6, #7, #8, #9
- clang/include/clang/Sema/Sema.h : #5, #6, #7
- clang/lib/Parse/ParseExprCXX.cpp : *
- clang/lib/Sema/SemaContract.cpp : @CaptureUsage, @LambdaCaptureChecker, @CheckLambdaCapturesForContracts, /^  LambdaCaptureChecker\(Sema &S, LambdaExpr \*LE\)/, /^  bool shouldVisitLambdaBody\(\) const/, /^  static void Check\(Sema &Actions, LambdaExpr \*LE\)/
- clang/lib/Sema/SemaDecl.cpp : #19
- clang/lib/Sema/SemaExpr.cpp : #5, #6, #7, #8, #9, #10, #14, #19, #20, #21, #22, #23, #24, #25, #26, #27, #28, #29, #30, #31, #32, #33, #34, #35, #36, #37, #38, #39, #40, #41, #42, #43, #44, #45, #46, #47, #48, #49, #50, #51
- clang/lib/Sema/SemaExprCXX.cpp : #0, #1, #3, #4, #5, #6, #7
- clang/lib/Sema/SemaLambda.cpp : *
- clang/lib/Sema/SemaTemplateInstantiateDecl.cpp : #9
- clang/lib/Sema/TreeTransform.h : #4, #5, #6, #7
- clang/lib/Serialization/ASTReaderDecl.cpp : #5, #7, #10
- clang/lib/Serialization/ASTWriter.cpp : *
- clang/lib/Serialization/ASTWriterDecl.cpp : #4, #5
- clang/test/Contracts/Runnable/contract-outlined-lambda-in-template.cpp : #0
- clang/test/Contracts/Runnable/generic-lambda-own-contracts.cpp : #0
- clang/test/Contracts/Runnable/lambda-capture-contract-in-template.cpp : #0
- clang/test/Contracts/Runnable/lambda-capture-in-contract.cpp : #0
- clang/test/Contracts/Runnable/lambda-capture-this-predicate.cpp : #0
- clang/test/Contracts/Runnable/lambda-contract-in-template.cpp : #0
- clang/test/Contracts/Runnable/lambda-declaration-entities.cpp : #0
- clang/test/Contracts/Runnable/lambda-in-postcondition-result-name.cpp : #0
- clang/test/Contracts/Runnable/lambda-pre-explicit-capture.cpp : #0
- clang/test/Contracts/Runnable/lambda-test.cpp : #0
- clang/test/Contracts/Runnable/predicate-lambda-pack-capture.cpp : #0
- clang/test/Contracts/Runnable/stmt-expr-local-generic-lambda.cpp : #0
- clang/test/Contracts/Sema/contract-predicate-constify-in-lambda.cpp : #0
- clang/test/Contracts/Sema/lambda-vla-ref-capture-template.cpp : #0
- clang/test/Contracts/Sema/postcondition-lambda-param.cpp : #0
- clang/test/Contracts/constexpr-result-name-in-lambda.cpp : #0
- clang/test/Contracts/contract-assert-in-lambda-in-template.cpp : #0
- clang/test/Contracts/contract-assert-lambda-cleanups.cpp : #0
- clang/test/Contracts/contract-lambda-capture-constify.cpp : #0
- clang/test/Contracts/contract-lambda-declaration-gc.cpp : #0
- clang/test/Contracts/ctor-pre-member-in-lambda.cpp : #0
- clang/test/Contracts/decl-contract-lambda-default-arg.cpp : #0
- clang/test/Contracts/generic-lambda-contract-capture.cpp : #0
- clang/test/Contracts/generic-lambda-free-function-capture.cpp : #0
- clang/test/Contracts/generic-lambda-modifies-param.cpp : #0
- clang/test/Contracts/lambda-capture-constify-generic-function.cpp : #0
- clang/test/Contracts/lambda-capture-in-contract-error.cpp : #0
- clang/test/Contracts/lambda-capture-scope-diagnostics.cpp : #0
- clang/test/Contracts/lambda-contract-diagnostics.cpp : #0
- clang/test/Contracts/lambda-contract-only-capture-constant-template.cpp : #0
- clang/test/Contracts/lambda-contract-only-capture-constant.cpp : #0
- clang/test/Contracts/lambda-contract-only-capture-edges.cpp : #0
- clang/test/Contracts/lambda-contract-only-capture-pack.cpp : #0
- clang/test/Contracts/lambda-contract-only-capture.cpp : #0
- clang/test/Contracts/lambda-declaration-entities-constexpr.cpp : #0
- clang/test/Contracts/lambda-declaration-entities-errors.cpp : #0
- clang/test/Contracts/lambda-capture-constified-by-own-contract.cpp : *
- clang/test/Contracts/lambda-in-generic-lambda-smoke.cpp : #0
- clang/test/Contracts/lambda-modifies-param-in-pre.cpp : #0
- clang/test/Contracts/lambda-this-modifies-object.cpp : #0
- clang/test/Contracts/local-class-in-contract-lambda.cpp : #0
- clang/test/Contracts/nested-lambda-contract-capture-depth.cpp : #0
- clang/test/Contracts/nsdmi-lambda-contract-this.cpp : #0
- clang/test/Contracts/postcondition-param-lambda.cpp : #0
- clang/test/Contracts/predicate-lambda-pack-capture.cpp : #0
- clang/test/Contracts/redecl-contract-lambda-template.cpp : #0
- clang/test/Contracts/redecl-omit-contract-lambda.cpp : #0
- clang/test/Contracts/redecl-unnamed-param-lambda-capture.cpp : #0
- clang/test/Contracts/result-binding-lambda-capture.cpp : #0
- clang/test/Contracts/result-binding-lambda-explicit-capture.cpp : #0
- clang/test/Contracts/result-binding-lambda-ref-constify.cpp : #0
- clang/test/Contracts/result-name-lambda-diag.cpp : #0
- clang/test/Contracts/result-name-lambda-enclosing-scope.cpp : #0
- clang/test/Contracts/result-name-shadows-lambda-capture.cpp : #0
- clang/test/Contracts/stmt-expr-local-generic-lambda.cpp : #0
- clang/test/Contracts/stmt-expr-local-lambda-deduced-post.cpp : #0
- clang/test/Contracts/template-decl-contract-lambda.cpp : #0
- clang/include/clang/Basic/DiagnosticSemaKinds.td : /\bdef\s+err_contract_result_name_shadows_capture\b/, /\bdef\s+err_contract_lambda_implicit_capture_only\b/, /\bdef\s+note_contract_lambda_implicit_capture_usage\b/, /\bdef\s+note_contract_lambda_implicit_capture_explicit\b/, /\bdef\s+err_contract_lambda_decl_ref_not_modifiable_lvalue\b/
