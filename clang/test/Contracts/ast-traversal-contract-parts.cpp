// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fcontracts-p3099 -fcontracts-p4283 -ast-dump -ast-dump-filter f %s | FileCheck --check-prefix=DUMP %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fcontracts-p3099 -fcontracts-p4283 -ast-list %s | FileCheck --check-prefix=LIST %s

// ASTNodeTraverser and RecursiveASTVisitor reach every part of a
// ContractStmt: the requires-clause, the postcondition captures and the
// message as well as the result name and condition, so -ast-dump shows them
// and a RecursiveASTVisitor client (here -ast-list) sees the capture as it
// sees the lambda init-capture `y`.

int f(int x) post [c = x] (r : r == c, "msg") { return [y = x] { return y; }(); }

template <class T> int fr(T x) pre requires (sizeof(T) > 1) (x > 0);

// DUMP-LABEL: FunctionDecl {{.*}} f 'int (int)'
// DUMP: ContractStmt {{.*}} post
// DUMP-NEXT: DeclStmt
// DUMP-NEXT: PostconditionCaptureDecl {{.*}} c 'int'
// DUMP: ResultNameDecl {{.*}} r
// DUMP: BinaryOperator {{.*}} '=='
// DUMP: StringLiteral {{.*}} "msg"
// DUMP: CompoundStmt

// DUMP-LABEL: FunctionDecl {{.*}} fr 'int (T)'
// DUMP: ContractStmt {{.*}} pre
// DUMP: UnaryExprOrTypeTraitExpr {{.*}} sizeof 'T'
// DUMP: DeclRefExpr {{.*}} 'x'

// LIST: {{^}}f{{$}}
// LIST-DAG: {{^}}c{{$}}
// LIST-DAG: {{^}}y{{$}}
