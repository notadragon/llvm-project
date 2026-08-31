// The AST-shape half of the runtime test in
// Runnable/contract-dependent-predicate-temporary.cpp: a contract whose
// predicate is type-dependent and holds a temporary with a non-trivial
// destructor must still be a plain ContractStmt in the specifier.
//
// It used to come back from ActOnFinishFullStmt wrapped in a GNU statement-
// expression -- the printer showed `[[({ pre(...) })]]` -- because the
// enclosing cleanup state was still dirty, and the wrapper was then stored as
// a bogus ContractStmt by ActionResult::getAs's static_cast.  Printing the
// specifier's source range crashed outright, so this test also pins that
// -ast-print gets through the file at all.
//
// RUN: %clang_cc1 -std=c++26 -fcontracts -ast-print %s | FileCheck %s

struct Guard {
  long V;
  Guard(long X);
  ~Guard();
};
bool operator<=(const Guard &A, long B);

struct S {
  template <class T>
  void f(const T &V) pre(Guard(10) <= V);

  template <class T>
  T g(const T &V) post(R : Guard(10) <= R);
};

// `[[` is FileCheck's variable syntax, so the doubled brackets the printer
// emits around a contract specifier have to be escaped.
// CHECK: template <class T> void f(const T &V) {{\[\[}}pre(Guard(10) <= V)]];
// CHECK: template <class T> T g(const T &V) {{\[\[}}post(Guard(10) <= R)]];

// The statement-expression wrapper must not appear anywhere.
// CHECK-NOT: ({
