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
// (These assignments used to compile because Clang constified automatic
// storage alone; see Sema/contract-predicate-constify-storage.cpp.)  The
// static locals BELOW are declared inside the predicate, so they are not
// constified and `z = x` stays fine.
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
  assert(fz == nullptr);
  assert(f(i) == 101);
  assert(fz != nullptr);
  assert(gz<int> == nullptr);
  assert(gz<long> == nullptr);
  g(42);
  assert(gz<int> != nullptr);
  assert(gz<long> == nullptr);
  assert(*gz<int> == 42);

  A a;
  assert(gz<A> == nullptr);
  a.f(42);
  assert(gz<A> != nullptr);
  assert(gz<A>->z == 0);


}