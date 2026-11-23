// DEFINE: %{cc} = %clangxx -fsyntax-only %s
//
// RUN: %{cc} -std=c++26 -Xclang -verify=off -DVALUE=202611L
// RUN: %{cc} -std=c++2d -Xclang -verify=on -DVALUE=202612L
// RUN: %{cc} -std=gnu++2d -Xclang -verify=on -DVALUE=202612L
// RUN: %{cc} -std=c++2d -fno-contracts-p3097 -Xclang -verify=off -DVALUE=202611L
// RUN: %{cc} -std=c++2d -fno-contracts-p3097 -fcontracts-p3097 -Xclang -verify=on -DVALUE=202612L
// RUN: %{cc} -std=c++2d -fno-contracts-p3097 -fcontracts-p3850 -Xclang -verify=off -DVALUE=202611L
// RUN: %{cc} -std=c++2d -fno-contracts -Xclang -verify=none -DNONE
// RUN: %{cc} -std=c++26 -fcontracts-p3097 -Xclang -verify=on -DVALUE=202612L
// RUN: %{cc} -std=c++23 -fcontracts -Xclang -verify=off -DVALUE=202502L
// RUN: %{cc} -std=c++23 -fcontracts-p3097 -Xclang -verify=on -DVALUE=202612L
//
// RUN: %clang_cc1 -std=c++2d -fcontracts -fsyntax-only -verify=on -DVALUE=202612L %s
// RUN: %clang_cc1 -std=c++2d -fcontracts -fno-contracts-p3097 -fsyntax-only -verify=off -DVALUE=202611L %s
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify=none -DNONE %s

// C++29 adds P3097 (notadragon_wg21 DECISIONS.md E222): with contracts on,
// -std=c++2d, -std=gnu++2d and every later standard imply -fcontracts-p3097,
// as C++26 implies -fcontracts-p4451 (p4451-cxx26.cpp).  Only the language
// mode implies it: -fcontracts with C++26 or earlier does not.  An explicit
// -fno-contracts-p3097 still turns it off, with -fcontracts-p3850 too (which
// then leaves __cpp_contracts at P4451's value), and a later
// -fcontracts-p3097 turns it on again.  -std=c++2d -fno-contracts defines no
// contracts macro.  -cc1 follows the same rule from -fcontracts and the
// language mode, and leaves contracts, so P3097, off without -fcontracts.
//
// __cpp_contracts is the greatest value of the enabled papers: P3097's fixed
// 202612L whenever P3097 is on, in any mode; otherwise P4451R2's 202611L at
// C++26 and later, and P2900's 202502L before.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3097-std-cxx26.C,
// p3097-std-cxx2d.C, p3097-std-gnuxx2d.C, p3097-std-cxx2d-no-p3097.C,
// p3097-std-cxx2d-p3850-no-p3097.C, p3097-std-cxx2d-no-contracts.C,
// p3097-std-cxx26-p3097.C, p3097-std-cxx23.C and p3097-std-cxx23-p3097.C

#ifdef NONE
// none-no-diagnostics
#ifdef __cpp_contracts
#error "contracts are off"
#endif
#else
// on-no-diagnostics
#if __cpp_contracts != VALUE
#error "__cpp_contracts"
#endif

struct Base {
  virtual void f(int x) pre(x > 0); // off-error {{contracts on virtual functions require '-fcontracts-p3097'}}
};
#endif
