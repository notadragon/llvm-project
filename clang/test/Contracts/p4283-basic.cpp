// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4283 -fsyntax-only -verify %s
// expected-no-diagnostics

// P4283: requires clause on pre/post/contract_assert — parsing.

template<typename T>
concept Integral = __is_integral(T);

template<typename T>
concept SignedIntegral = Integral<T> && __is_signed(T);

template<typename T>
T safe_divide(T a, T b)
  pre requires(Integral<T>) (b != 0)
{ return b != T{} ? a / b : T{}; }

template<typename T>
T identity(const T x)
  post requires(Integral<T>) (r: r == x)
{ return x; }

template<typename T>
void check_positive(T x) {
  contract_assert requires(SignedIntegral<T>) (x > 0);
}

// Explicit instantiations to verify template machinery works.
template int safe_divide<int>(int, int);
template double safe_divide<double>(double, double);
template int identity<int>(int);
