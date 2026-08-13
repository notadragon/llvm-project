// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// Without -fcontracts-p3400, label syntax is rejected.

struct L { using assertion_control_object = L; };
constexpr L l{};

// The label is still parsed so that we can point at the missing flag and then
// recover to the predicate, rather than derailing the whole declaration.
// Mirrors GCC's "assertion-control labels require %<-fcontracts-p3400%>".
void f(int x) pre<l>(x > 0) {} // expected-error {{assertion-control labels require '-fcontracts-p3400'}}

// The same must hold for a late-parsed contract on a member function.  That
// path caches tokens before re-parsing them; if it skips the label, the cached
// stream starts at '<' and the re-parse emits the same cascade of unrelated
// errors this diagnostic exists to replace.
struct S {
  void g(int x) pre<l>(x > 0) {} // expected-error {{assertion-control labels require '-fcontracts-p3400'}}
};
