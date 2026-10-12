// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -o %t
// RUN: %t | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -o %t1 -DVIOLATE=1
// RUN: not --crash %t1
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -o %t2 -DVIOLATE=2
// RUN: not --crash %t2
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -o %t3 -DVIOLATE=3
// RUN: not --crash %t3
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -o %t4 -DVIOLATE=4
// RUN: not --crash %t4

// A generic lambda's own preconditions and postconditions name its captures
// (by copy, by reference, an init-capture, this and *this, a pack), checked
// in each instance -- which may be instantiated after the enclosing function
// is done (mk) -- and in a lambda in a template; a captureless generic
// lambda converted to a function pointer keeps its precondition.  Each
// VIOLATE case breaks one of them.  (A trailing return type is in
// ../generic-lambda-pre-capture.cpp.)
//
// Mirror of the EDG fork's codegen/generic_lambda_own_contracts test.  No
// GCC mirror: GCC ICEs on every generic lambda whose precondition names a
// capture (GCC-633, GCC-634).

extern "C" int puts(const char *);
int f() {
  int t = 1;
  auto l1 = [=](auto u) pre(u > t) { return u + t; };
  auto l2 = [&](const auto u) pre(u > t) post(u > 0) { return u + t; };
  auto l3 = [c = t](auto u) pre(u > c) { return u + c; };
  auto l4 = [t]<class T>(T u) pre(u > t) post(r: r > t) { return u + t; };
  auto l5 = [=](const auto u) post(u > t) { return u + t; };
  auto l6 = [=](auto a) {
    return [=](auto b) pre(b > a) pre(b > t) { return a + b + t; }(a + 1);
  };
  auto l7 = [=](auto... us) pre(((us > t) && ...)) { return (us + ... + t); };
  return l1(2) + l1(2L) + l2(2) + l3(2) + l4(2) + l5(2) + l6(2) + l7(2, 3);
}
auto mk(int t) { return [=](auto u) pre(u > t) { return u + t; }; }
struct S {
  int k = 1;
  int h() { auto l = [this](auto u) pre(u > k) { return u + k; }; return l(5); }
  int h2() { auto l = [*this](auto u) pre(u > k) { return u + k; }; return l(5); }
};
template <class T> struct C {
  T k = 1;
  int h(T v) {
    auto l = [=, this](auto u) pre(u > v + k) { return u + v + k; };
    return l(5);
  }
};
template <class T> T g(T x) {
  T t = x;
  auto l = [=](auto u) pre(u > t) { return u + t; };
  return l(2);
}
template <class... Ts> int p(Ts... ts) {
  auto l = [=](int u) pre(((u > ts) && ...)) { return (u + ... + ts); };
  return l(5);
}
constexpr int k() {
  int lo = 1;
  auto l = [lo](auto b) pre(b > lo) post(r: r > lo) { return b; };
  return l(2);
}
static_assert(k() == 2);
int main() {
  if (f() != 3 + 3 + 3 + 3 + 3 + 3 + 6 + 6) { puts("f"); return 1; }
  if (mk(1)(2) != 3) { puts("mk"); return 1; }
  S s;
  if (s.h() != 6 || s.h2() != 6) { puts("S"); return 1; }
  C<int> c;
  if (c.h(1) != 7) { puts("C"); return 1; }
  if (g(1) != 3 || p(1, 2) != 8) { puts("g"); return 1; }
  int (*fp)(int) = [](auto x) pre(x > 0) { return x; };
  if (fp(1) != 1) { puts("fp"); return 1; }
#if VIOLATE == 1
  mk(5)(2);
#elif VIOLATE == 2
  { int t = 3; auto l = [&](const auto u) post(u > t) { t = 5; return u; };
    l(4); }
#elif VIOLATE == 3
  c.h(5);
#elif VIOLATE == 4
  fp(0);
#endif
  puts("end");
  return 0;
}
// CHECK: end
