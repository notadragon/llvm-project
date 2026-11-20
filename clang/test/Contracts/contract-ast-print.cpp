// -ast-print must emit a contract specifier as the syntax it was written in,
// so that the printed declaration parses back.  Three things used to make it
// not: the sequence was wrapped in `[[...]]` with no separator between
// specifiers, a postcondition's result-name introducer was dropped, and
// neither a P4283 requires-clause nor P3098 captures were printed at all.
//
// The second RUN line is the real test -- compile what we printed -- and the
// FileCheck pass pins the text, because re-parsing alone would also accept
// output that silently dropped an attribute or a message.
//
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3850 -ast-print %s > %t.cpp
// RUN: FileCheck --input-file=%t.cpp %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3850 -fsyntax-only %t.cpp

int two_pre(int x) pre(x > 0) pre(x < 10);
// CHECK: int two_pre(int x) pre(x > 0) pre(x < 10);

// A postcondition may only name a parameter that is declared const.
int result_name(const int x) post(r : r > x);
// CHECK: int result_name(const int x) post(r : r > x);

int attributed(int x) pre [[clang::contract_group("grp")]] (x != 0);
// `[[` is FileCheck's variable syntax, so the attribute's brackets have to be
// escaped in the expectation.
// CHECK: int attributed(int x) pre {{\[\[}}clang::contract_group("grp")]] (x != 0);

// P3098: an init-capture and a plain capture of a parameter.  In the second,
// `n` in the predicate is the capture rather than the parameter, which is why
// a non-const parameter is well-formed there and not above.
int init_capture(const int n) post [old = n] (old == n) { return n; }
// CHECK: int init_capture(const int n) post [old = n](old == n) {

int param_capture(int n) post [n] (n > 0) { return n; }
// CHECK: int param_capture(int n) post [n](n > 0) {

// P4283: a requires-clause sits between the keyword and the predicate.
template <class T> concept Integral = __is_same(T, int);
template <class T> int constrained(T x) pre requires(Integral<T>) (x > 0);
// CHECK: template <class T> int constrained(T x) pre requires (Integral<T>)(x > 0);

// P3099: a message is part of the predicate's parenthesized form.
void with_message(int x) {
  contract_assert(x > 0, "x must be positive");
}
// CHECK: contract_assert(x > 0, "x must be positive");
