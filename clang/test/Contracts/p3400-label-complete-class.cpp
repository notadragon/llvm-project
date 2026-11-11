// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=ignore %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P3400: a function contract specifier is a complete-class context
// ([class.mem]), so a member function's assertion-control label sees members
// declared after it, as its predicate does; and a function's parameters are
// in scope in the whole specifier, label included.  Each label is
// std::contracts::labels::terminating, so constant evaluation with the
// configured ignore still finds a false predicate.  (Clang parses a
// member's whole specifier, label included, when the class is complete.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-label-complete-class.C
// (GCC-960)

#include <contracts>  // `terminating`

struct S {
  static constexpr int f(int x) pre<lab>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
  static constexpr int g(int x) pre<lab>(x > 0);
  static constexpr int h(int x) post<lab>(r : r > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
  static constexpr int t(int x) pre<lab_t{}>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
  static constexpr auto lab = std::contracts::labels::terminating;
  using lab_t = decltype(std::contracts::labels::terminating);
};

// The redeclaration's label names the same member.
constexpr int S::g(int x) pre<lab>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}

constexpr int a = S::f(1);
constexpr int b = S::f(-1); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
constexpr int c = S::g(-1); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
constexpr int d = S::h(-1); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
constexpr int e = S::t(-1); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}

// A nested class's member sees the enclosing class complete too.
struct O {
  struct I {
    static constexpr int f(int x) pre<olab>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
  };
  static constexpr auto olab = std::contracts::labels::terminating;
};
constexpr int o = O::I::f(-1); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}

// A class template's member, at instantiation.
template <typename T>
struct TS {
  static constexpr T f(T x) pre<lab>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
  static constexpr auto lab = std::contracts::labels::terminating;
};
constexpr int ts = TS<int>::f(-1); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}

// A label ending in a template-argument-list, closed by `>>`.
template <typename T>
constexpr auto vlab = std::contracts::labels::terminating;
template <typename T> struct box {};

struct V {
  static constexpr int f(int x) pre<vlab<int>>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
  static constexpr int g(int x) pre<vlab<box<int>>>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
};
constexpr int v1 = V::f(-1); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
constexpr int v2 = V::g(-1); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}

// A namespace-scope function's label sees its parameters: the parameter
// hides the label of the same name.
constexpr auto plab = std::contracts::labels::terminating;
void p(int plab) pre<plab>(plab > 0); // expected-error {{value of type 'int' is not a valid assertion-control label}}
