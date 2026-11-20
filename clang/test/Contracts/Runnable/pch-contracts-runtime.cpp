// RUN: rm -rf %t && split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -stdlib=libc++ -x c++-header %t/h.h -o %t/h.pch
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -include-pch %t/h.pch -DVIOLATE=0 %t/main.cpp %libcxx_flags -o %t/a0
// RUN: %t/a0 | FileCheck %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -include-pch %t/h.pch -DVIOLATE=1 %t/main.cpp %libcxx_flags -o %t/a1
// RUN: not --crash %t/a1
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -include-pch %t/h.pch -DVIOLATE=2 %t/main.cpp %libcxx_flags -o %t/a2
// RUN: not --crash %t/a2
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -include-pch %t/h.pch -DVIOLATE=3 %t/main.cpp %libcxx_flags -o %t/a3
// RUN: not --crash %t/a3
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -include-pch %t/h.pch -DVIOLATE=4 %t/main.cpp %libcxx_flags -o %t/a4
// RUN: not --crash %t/a4
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -include-pch %t/h.pch -DVIOLATE=5 %t/main.cpp %libcxx_flags -o %t/a5
// RUN: not --crash %t/a5
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -include-pch %t/h.pch -DVIOLATE=6 %t/main.cpp %libcxx_flags -o %t/a6
// RUN: not --crash %t/a6
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -include-pch %t/h.pch -DVIOLATE=7 %t/main.cpp %libcxx_flags -o %t/a7
// RUN: not --crash %t/a7

// Contract assertions declared in a precompiled header are checked at run
// time under quick_enforce: a function's precondition and postcondition, an
// inline function's contract_assert, a function template's precondition
// instantiated only after the PCH, a member function's and a lambda's, and
// eight more function templates (so that losing a template's contracts
// cannot pass by chance).  VIOLATE=0 satisfies every check; each other value
// fails one, which traps.  (EDG shape: pch/runtime.)
//
// GCC: gcc/testsuite/g++.dg/pch/contract-runtime.C, which compares the
// assembly built with and without the PCH (GCC-646).

// CHECK: end

//--- h.h
extern "C" int puts(const char *);
int f(int x) pre(x > 0) post(r: r > 1) { return x; }
inline void g(int x) { contract_assert(x != 3); }
template <class T> T t(T x) pre(x > 0) { return x; }
struct M { int m(int x) pre(x > 0) { return x; } };
inline auto lam = [](int x) pre(x > 0) { return x; };
template <class T> T t0(T x) pre(x > 0) post(r: r > 0) { return x; }
template <class T> T t1(T x) pre(x > 1) post(r: r > 1) { return x; }
template <class T> T t2(T x) pre(x > 2) post(r: r > 2) { return x; }
template <class T> T t3(T x) pre(x > 3) post(r: r > 3) { return x; }
template <class T> T t4(T x) pre(x > 4) post(r: r > 4) { return x; }
template <class T> T t5(T x) pre(x > 5) post(r: r > 5) { return x; }
template <class T> T t6(T x) pre(x > 6) post(r: r > 6) { return x; }
template <class T> T t7(T x) pre(x > 7) post(r: r > 7) { return x; }

//--- main.cpp
int main() {
  if (VIOLATE == 0) { f(2); g(0); t(1); M().m(1); lam(1); }
  if (VIOLATE == 1) f(0);         // precondition
  if (VIOLATE == 2) f(1);         // postcondition
  if (VIOLATE == 3) g(3);         // contract_assert
  if (VIOLATE == 4) t(0L);        // template's precondition
  if (VIOLATE == 5) M().m(0);     // member's precondition
  if (VIOLATE == 6) lam(0);       // lambda's precondition
  if (VIOLATE == 0)
    t0(1), t1(2), t2(3), t3(4), t4(5), t5(6), t6(7), t7(8);
  if (VIOLATE == 7) t7(7);        // a later template's precondition
  puts("end");
}
