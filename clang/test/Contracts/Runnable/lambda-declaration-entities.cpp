// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -o %t && %t

// Lambdas in preconditions and postconditions use and capture the function's
// parameters, this, *this and the result name: implicitly and explicitly, by
// copy and by reference, in nested and generic lambdas, on non-member and
// member functions, templates and their instances, and on a lambda's own
// contracts.  Every predicate holds; quick_enforce traps if one is wrongly
// evaluated false.  (EDG-1/2/39 shapes.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/lambda-declaration-entities.C;
// constant evaluation is ../lambda-declaration-entities-constexpr.cpp, the
// diagnostics ../lambda-declaration-entities-errors.cpp.

int f1(int x) pre([&] { return x > 0; }()) { return x; }
int f2(int x) pre([=] { return x > 0; }()) { return x; }
int f3(int x) pre([x] { return x > 0; }()) { return x; }
int f4(int x) pre([&x] { return x > 0; }()) { return x; }
int f5(int x, int y) pre([&, y] { return x < y; }()) { return y - x; }
int f6(int x) pre([=] { return [&] { return x > 0; }(); }()) { return x; }
int f7(int x) pre([x]() mutable { return ++x > 1; }()) { return x; }
int f8(int &x) pre([&] { return x > 0; }()) { return x; }
int f9(int x) pre([&](int k) { return x > k; }(0)) { return x; }
int f10(int x) pre([&](auto k) { return x > k; }(0)) { return x; }

int r1() post(r: [r] { return r > 0; }()) { return 1; }
int r2() post(r: [&r] { return r > 0; }()) { return 1; }
int r3() post(r: [&] { return r > 0; }()) { return 1; }
int r4() post(r: [=] { return r > 0; }()) { return 1; }
int r5() post(r: [&](auto k) { return r + k > 0; }(1)) { return 1; }
int r6(const int x) post(r: [=] { return r > x; }()) { return x + 1; }
int r7(const int x) post(r: [&r, x] { return r > x; }()) { return x + 1; }

struct S {
  int m;
  int g1(int x) pre([this] { return m > 0; }()) { return m + x; }
  int g2(int x) pre([&] { return m > x; }()) { return m - x; }
  int g3(int x) pre([*this] { return m > 0; }()) { return m + x; }
  int g4(int x) pre([this, x] { return m > x; }());
  int g5(const int x) post(r: [&] { return r == m - x; }()) { return m - x; }
  int g6(int x) pre([this](auto k) { return m > k; }(x));
};
int S::g4(int x) { return m - x; }
int S::g6(int x) { return m - x; }

template <class T> T t1(T x) pre([&] { return x > 0; }()) { return x; }
template <class T> T t2(const T x) post(r: [=] { return r == x; }()) {
  return x;
}
template <class T> struct C {
  T m;
  T c1(T x) pre([&] { return m > x; }()) { return m - x; }
  template <class U> T c2(U u) pre([this, u] { return m > u; }()) {
    return m;
  }
};

void lf() {
  auto l = [](int x) pre([&] { return x > 0; }()) {};
  auto l2 = [](int x) pre([x](auto k) { return x > k; }(0)) {};
  l(1);
  l2(1);
}

int main() {
  S s{3};
  C<int> c{3};
  int one = 1;
  f1(1); f2(1); f3(1); f4(1); f5(1, 2); f6(1); f7(1); f8(one); f9(1); f10(1);
  r1(); r2(); r3(); r4(); r5(); r6(1); r7(1);
  s.g1(1); s.g2(1); s.g3(1); s.g4(1); s.g5(1); s.g6(1);
  t1(1); t2(1); c.c1(1); c.c2(1);
  lf();
  return 0;
}
