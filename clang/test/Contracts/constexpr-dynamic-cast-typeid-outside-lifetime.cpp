// RUN: %clangxx -std=c++26 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// typeid and dynamic_cast of a polymorphic object outside its lifetime are
// undefined ([basic.life]/7), so they are not core constant expressions,
// whether or not the result is used.  Clang rejects all three; GCC accepts
// the first two and evaluates the third's cast to a null pointer (stock,
// gnu_gcc GCC-752, deferred), and its watch test is the program below.  Not
// contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-dynamic-cast-typeid-outside-lifetime.C

#include <memory>
#include <typeinfo>

struct B { constexpr virtual ~B() {} };
struct D : B {};

constexpr int t1() {
  D d; // expected-note {{declared here}}
  B &br = d;
  d.~D();
  (void)typeid(br); // expected-note {{typeid applied to object outside its lifetime}}
  std::construct_at(&d);
  return 1;
}
static_assert(t1() == 1); // expected-error {{not an integral constant expression}} \
                          // expected-note {{in call to 't1()'}}

constexpr int t2() {
  std::allocator<D> a;
  D *p = a.allocate(1); // expected-note {{heap allocation performed here}}
  B *bp = std::construct_at(p);
  std::destroy_at(p);
  (void)dynamic_cast<D *>(bp); // expected-note {{dynamic_cast of object outside its lifetime}}
  a.deallocate(p, 1);
  return 1;
}
static_assert(t2() == 1); // expected-error {{not an integral constant expression}} \
                          // expected-note {{in call to 't2()'}}

// The result used.
constexpr int t3() {
  std::allocator<D> a;
  D *p = a.allocate(1); // expected-note {{heap allocation performed here}}
  B *bp = std::construct_at(p);
  std::destroy_at(p);
  bool r = dynamic_cast<D *>(bp) != nullptr; // expected-note {{dynamic_cast of object outside its lifetime}}
  a.deallocate(p, 1);
  return r;
}
static_assert(t3() == 0); // expected-error {{not an integral constant expression}} \
                          // expected-note {{in call to 't3()'}}
