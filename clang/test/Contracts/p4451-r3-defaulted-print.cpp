// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4451 -ast-print %s > %t.cpp
// RUN: FileCheck --input-file=%t.cpp %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4451 -fsyntax-only %t.cpp
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4451 -ast-dump %s | FileCheck --check-prefix=DUMP %s

// P4451R3: an assertion-control-specifier keeps no expression, so -ast-print
// prints it with the qualified name of the label it names, defaulted as
// well as enforced, however it was spelled, and what it prints parses back
// to the same assertions; -ast-dump marks each.  The declarations below
// stand in for <contracts>'s: the check compares what the specifier finds
// with what qualified lookup in std finds.

namespace std::contracts::controls {
inline namespace __p4451 {
struct __defaulted_label_t {};
struct __enforced_label_t {};
inline constexpr __defaulted_label_t defaulted{};
inline constexpr __enforced_label_t enforced{};
}
}

namespace scl = std::contracts::controls;

int d(int x) pre<scl::defaulted>(x > 0);
// CHECK: int d(int x) pre<std::contracts::controls::defaulted>(x > 0);
int e(int x) pre<scl::enforced>(x > 0);
// CHECK: int e(int x) pre<std::contracts::controls::enforced>(x > 0);
int n(int x) pre(x > 0);
// CHECK: int n(int x) pre(x > 0);
void a(int x) {
  using namespace std::contracts::controls;
  contract_assert<defaulted>(x > 0);
}
// CHECK: contract_assert<std::contracts::controls::defaulted>(x > 0);

// DUMP: FunctionDecl {{.*}} d 'int (int)'
// DUMP: ContractStmt {{.*}} pre enforce defaulted{{$}}
// DUMP: FunctionDecl {{.*}} e 'int (int)'
// DUMP: ContractStmt {{.*}} pre enforce terminating{{$}}
// DUMP: FunctionDecl {{.*}} n 'int (int)'
// DUMP: ContractStmt {{.*}} pre enforce{{$}}
// DUMP: FunctionDecl {{.*}} a 'void (int)'
// DUMP: ContractStmt {{.*}} contract_assert enforce defaulted{{$}}
