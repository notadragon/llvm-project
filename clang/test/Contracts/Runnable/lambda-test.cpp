// RUN: %clangxx -std=c++26 %s -fcontracts %libcxx_flags -o %t -fcontract-evaluation-semantic=observe
// RUN: %t
// RUN: %clangxx -std=c++26 %s -fcontracts %libcxx_flags -o %t -fcontract-evaluation-semantic=enforce
// RUN: %t

#include "my_assert.h"
#include <contracts>

// The recording pointers are written through calls rather than assigned to
// directly: [expr.prim.id.unqual]/3+d const-qualifies every variable a
// predicate names, whatever its storage duration, so `fz = &z` inside the
// predicate is ill-formed -- see the paper's own example, whose `++n` on a
// namespace-scope variable inside a predicate lambda is marked an error.
// (Constifying automatic storage alone would let these assignments compile;
// see Sema/contract-predicate-constify-storage.cpp.)  The static locals BELOW
// are declared inside the predicate, so they are not constified and `z = x`
// stays fine.
const int *fz = nullptr;
static bool note_f(const int *p) { fz = p; return true; }

constexpr int f(int x) pre([x=x](int y) { static int z(0);  z = x; note_f(&z); return y > x; }(1000)) {
  return x;
}

template <class T>
const T* gz = nullptr;

template <class T>
bool note_g(const T *p) { gz<T> = p; return true; }

template <class T>
constexpr T g(T x) pre([x=x](T y) { static T z(0); z = x;  note_g<T>(&z); return y > x; }(1000)) {
  return x;
}
template int g(int);
template long g(long);

struct A {
  constexpr A() : z(0) {}

  int f(int x) pre([=,this](int y) { static A a; note_g<A>(&a); a.z = z; return y > x; }(1000)) {
    return x;
  }

  int z = 0;
};


struct B {
  constexpr B() : z(0) {}

  int f(int x) pre([z=z]() { static B a; note_g<B>(&a); a.z = z; return true; }()) {
    return x;
  }

  int z = 0;
};

int main() {
  int i = 101;
  long l = 42;
  if (!(fz == nullptr)) __builtin_abort();
  if (!(f(i) == 101)) __builtin_abort();
  if (!(fz != nullptr)) __builtin_abort();
  if (!(gz<int> == nullptr)) __builtin_abort();
  if (!(gz<long> == nullptr)) __builtin_abort();
  g(42);
  if (!(gz<int> != nullptr)) __builtin_abort();
  if (!(gz<long> == nullptr)) __builtin_abort();
  if (!(*gz<int> == 42)) __builtin_abort();

  A a;
  if (!(gz<A> == nullptr)) __builtin_abort();
  a.f(42);
  if (!(gz<A> != nullptr)) __builtin_abort();
  if (!(gz<A>->z == 0)) __builtin_abort();


}
