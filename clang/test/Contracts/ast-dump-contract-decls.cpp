// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -ast-dump %s | FileCheck %s --check-prefix=TEXT
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -ast-dump=json %s | FileCheck %s --check-prefix=JSON

// The text and JSON AST dumps of a function contract specifier sequence: the
// ContractSpecifierDecl, each ContractStmt, a postcondition's captures (one an
// init-capture, one a capture of a parameter) and its result names.
//
// No GCC mirror: Clang AST dump formats.

int f(int x) pre(x > 0) post [old = x, x] (r : r > old) post(q : q > 0) { return x; }

// TEXT:      ContractSpecifierDecl {{.*}} pre 1 post 2 captures{{$}}
// TEXT-NEXT: ContractStmt {{.*}} pre enforce{{$}}
// TEXT:      ContractStmt {{.*}} post enforce{{$}}
// TEXT-NEXT: DeclStmt
// TEXT-NEXT: PostconditionCaptureDecl {{.*}} used old 'int' cinit{{$}}
// TEXT:      PostconditionCaptureDecl {{.*}} x 'int' cinit parameter_capture{{$}}
// TEXT:      ResultNameDecl {{.*}} r 'int'{{$}}
// TEXT:      ContractStmt {{.*}} post enforce{{$}}
// TEXT-NEXT: DeclStmt
// TEXT-NEXT: ResultNameDecl {{.*}} q 'int'{{$}}

// JSON:      "kind": "ContractSpecifierDecl",
// JSON:      "numPreconditions": 1,
// JSON-NEXT: "numPostconditions": 2,
// JSON-NEXT: "hasCaptures": true,
// JSON:      "kind": "ContractStmt",
// JSON:      "contractKind": "pre",
// JSON-NEXT: "semantic": "enforce"
// JSON:      "kind": "ContractStmt",
// JSON:      "contractKind": "post",
// JSON-NEXT: "semantic": "enforce",
// JSON-NEXT: "hasResultName": true
// JSON:      "kind": "PostconditionCaptureDecl",
// JSON:      "name": "old",
// JSON:      "kind": "PostconditionCaptureDecl",
// JSON:      "name": "x",
// JSON:      "isParameterCapture": true,
// JSON:      "kind": "ResultNameDecl",
// JSON:      "name": "r",
// JSON-NEXT: "type": {
// JSON-NEXT:   "qualType": "int"
// JSON-NEXT: }
// JSON-NEXT: }{{,?$}}
// JSON:      "kind": "ResultNameDecl",
// JSON:      "name": "q",
// JSON-NEXT: "type": {
// JSON-NEXT:   "qualType": "int"
// JSON-NEXT: }
// JSON-NEXT: }{{,?$}}
