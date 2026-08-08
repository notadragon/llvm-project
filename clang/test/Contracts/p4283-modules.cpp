// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
//
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4283 \
// RUN:   %t/mod.cppm -emit-module-interface -o %t/mod.pcm
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4283 \
// RUN:   -fprebuilt-module-path=%t %t/use.cpp -fsyntax-only -verify
// XFAIL: *
// Pre-existing: ContractSpecifierDecl deserialization crash in module context.

// P4283: Serialization round-trip for contracts with requires clauses.

//--- mod.cppm
export module mod;

export template<typename T>
concept Integral = __is_integral(T);

export template<typename T>
T safe_divide(T a, T b)
  pre requires(Integral<T>) (b != 0)
{ return b != T{} ? a / b : T{}; }

export template<typename T>
T identity(const T x)
  post requires(Integral<T>) (r: r == x)
{ return x; }

//--- use.cpp
// expected-no-diagnostics
import mod;

// Instantiate with int — constraint satisfied, contracts kept.
template int safe_divide<int>(int, int);
template int identity<int>(int);

// Instantiate with double — constraint not satisfied, contracts discarded.
template double safe_divide<double>(double, double);
template double identity<double>(double);
