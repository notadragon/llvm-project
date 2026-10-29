// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 -fcontract-analysis-semantic=ignore %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("Taking the Address of a Function", DECISIONS.md E144 (a)): a
// function passed to a library facility taking a callable by forwarding
// reference -- std::invoke, std::function, std::thread, std::bind -- or by
// value is bound to a reference or decayed to a pointer, and is called only
// inside the library, where it is no longer named, so those calls have no
// analysis evaluation: binding the reference counts as taking the address.
// A precondition that allows no runtime semantic then cannot be proven for
// those calls: an error for always_prove, a -Wcontract-unproven warning
// for always_diagnose; nothing for one not analyzed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-indirect-forward.C

#include <contracts>
#include <functional>
#include <thread>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

void fp(int x) pre<always_prove>(x > 0);  // expected-note 4 {{precondition here}}
void fd(int x) pre<always_diagnose>(x > 0);  // expected-note 6 {{precondition here}}
void fn(int x) pre(x > 0);

struct S {
  void mp(int x) pre<always_prove>(x > 0);  // expected-note {{precondition here}}
  void md(int x) pre<always_diagnose>(x > 0);  // expected-note {{precondition here}}
};

template <typename F, typename... A>
void my_invoke(F &&f, A &&...a) { f(static_cast<A &&>(a)...); }
template <typename F>
void by_value(F f) { f(1); }

void
use(S &s)
{
  std::invoke(fp, 1);  // expected-error {{cannot be proven for a call through the address of 'fp'}}
  std::invoke(fd, 1);  // expected-warning {{contract assertion cannot be proven for a call through the address of 'fd'}}
  std::invoke(fn, 1);
  std::function<void(int)> g1 = fp;  // expected-error {{cannot be proven for a call through the address}}
  std::function<void(int)> g2 = fd;  // expected-warning {{cannot be proven for a call through the address}}
  std::function<void(int)> g3 = fn;
  std::thread t1(fd, 1);  // expected-warning {{cannot be proven for a call through the address}}
  t1.join();
  std::thread t2(fp, 1);  // expected-error {{cannot be proven for a call through the address}}
  t2.join();
  my_invoke(fp, 1);  // expected-error {{cannot be proven for a call through the address}}
  my_invoke(fd, 1);  // expected-warning {{cannot be proven for a call through the address}}
  by_value(fd);  // expected-warning {{cannot be proven for a call through the address}}
  by_value(fn);
  std::invoke(&S::mp, s, 1);  // expected-error {{cannot be proven for a call through the address}}
  std::invoke(&S::md, s, 1);  // expected-warning {{cannot be proven for a call through the address}}
  auto b = std::bind(fd, 1);  // expected-warning {{cannot be proven for a call through the address}}
  b();
  g1(1); g2(1); g3(1);
}
