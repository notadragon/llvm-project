// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -ast-dump %s | FileCheck %s

// A contract assertion's source range ends at its closing parenthesis, not at
// the end of its last sub-statement: `pre(x > 0   )' and
// `contract_assert(x > 0   );' end at the `)', and a postcondition with a
// capture ends there too, not inside the capture list.
//
// No GCC mirror: Clang AST source ranges have no GCC counterpart.

void f(int x) pre(x > 0   ) {
  contract_assert(x > 0   );
}

int g(const int x) post [c = x] (c > 0   ) { return x; }

// CHECK: ContractStmt {{.*}} <col:15, col:27> pre
// CHECK: ContractStmt {{.*}} <line:11:3, col:27> contract_assert
// CHECK: ContractStmt {{.*}} <col:20, col:42> post
