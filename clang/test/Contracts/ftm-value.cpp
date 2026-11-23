// The contracts feature-test macro has the expected value under -fcontracts:
// at C++26, which implies P4451, P4451R2's 202611L (notadragon_wg21
// DECISIONS.md E220 (2)); with -fno-contracts-p4451, P2900's 202502L.  At
// C++29, which implies P3097 too, P3097's 202612L (E222 (2), (3)).
// (GCC asserts __cpp_contracts >= 202502L in several cpp26 tests.)
// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify -DVALUE=202611L %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fno-contracts-p4451 -fsyntax-only -verify -DVALUE=202502L %s
// Also at the next standard: the whole contracts suite otherwise pins
// -std=c++26 and nothing would notice a regression at a later one.  See
// contracts-std-versions.cpp, which covers the driver's side of that.
// RUN: %clang_cc1 -std=c++2d -fcontracts -fsyntax-only -verify -DVALUE=202612L %s
// RUN: %clang_cc1 -std=c++2d -fcontracts -fno-contracts-p4451 -fsyntax-only -verify -DVALUE=202612L %s
// RUN: %clang_cc1 -std=c++2d -fcontracts -fno-contracts-p3097 -fsyntax-only -verify -DVALUE=202611L %s
// RUN: %clang_cc1 -std=c++2d -fcontracts -fno-contracts-p3097 -fno-contracts-p4451 -fsyntax-only -verify -DVALUE=202502L %s
// expected-no-diagnostics

#if !defined(__cpp_contracts)
#error "__cpp_contracts must be defined with -fcontracts"
#endif
static_assert(__cpp_contracts == VALUE, "unexpected __cpp_contracts value");
