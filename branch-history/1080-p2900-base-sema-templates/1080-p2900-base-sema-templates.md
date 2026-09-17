---
id: 1080-p2900-base-sema-templates
subject: '[clang][contracts] instantiate contracts in templates and lambdas'
depends: []
regenerates: []
fixes: []
coauthors: ['Eric Fiselier <eric@efcs.ca>']
---

## Rationale
Instantiating a contract into a template specialization, and carrying one
onto a lambda's call operator.

`TransformContractStmt` is the bulk of it: substituting the predicate, the
result name, the captures, the label and the attributes, and rebuilding the
statement.  The lambda half is smaller and was a real defect -- a
`pre` or `post` on a lambda inside a template was silently dropped, because
`TransformLambdaExpr` builds a fresh call operator and nothing copied the
contract specifier onto it.  No diagnostic, no violation, no crash: the
contract simply was not checked.

## Compile gap
`TransformContractStmt` is claimed here in two pieces with P4283's
requires-clause handling cut out between them, because that handling is
self-contained.  **The second piece calls `RebuildContractStmt` passing
`RequiresClause`, which the P4283 piece declares**, so this commit standing
alone needs that argument stubbed as `nullptr`.  4600-p4283 overwrites the
stub.

The same function's P3400 label transform is NOT cut out and remains here.
It is inert without P3400 -- there is no label expression to transform -- so
it is imprecision in the mapping rather than a compile or behaviour gap.

## Contents
- clang/lib/Sema/SemaTemplateInstantiate.cpp : *
- clang/lib/Sema/SemaTemplateInstantiateDecl.cpp : *
- clang/lib/Sema/TreeTransform.h : #0, #1, #2:0, #2:2, #3, #4, #5, #6, #7, #8, #9
- clang/test/Contracts/Sema/contract-unexpanded-pack.cpp : *
- clang/test/Contracts/Sema/deducing-this-postcondition-param-dependent.cpp : *
- clang/test/Contracts/Sema/postcondition-pack-const.cpp : *
- clang/test/Contracts/Sema/postcondition-pack-empty.cpp : *
- clang/test/Contracts/Sema/postcondition-redecl-dependent-param.cpp : *
- clang/test/Contracts/constify-template-params.cpp : *
- clang/test/Contracts/contract-dependent-predicate-temporary-ast.cpp : *
- clang/test/Contracts/contract-instantiate-on-use.cpp : *
- clang/test/Contracts/lambda-in-generic-lambda-smoke.cpp : *
- clang/test/Contracts/lambda-templ.cpp : *
- clang/test/Contracts/sfinae-test.cpp : *
- clang/test/Contracts/template-contract-diagnostics.cpp : *
- clang/test/Contracts/variadic-contracts.cpp : *
