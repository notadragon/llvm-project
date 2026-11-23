// DEFINE: %{cc} = %clangxx %libcxx_flags -fsyntax-only %s
//
// RUN: %{cc} -std=c++26 -Xclang -verify=on -DON
// RUN: %{cc} -std=gnu++26 -Xclang -verify=on -DON
// RUN: %{cc} -std=c++2c -Xclang -verify=on -DON
// RUN: %{cc} -std=c++2d -fno-contracts-p3097 -Xclang -verify=on -DON
// RUN: %{cc} -std=gnu++2d -fno-contracts-p3097 -Xclang -verify=on -DON
// RUN: %{cc} -std=c++26 -fcontracts -Xclang -verify=on -DON
// RUN: %{cc} -std=c++26 -fno-contracts-p4451 -fcontracts-p4451 -Xclang -verify=on -DON
// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify=on -DON -DSTANDIN %s
//
// RUN: %{cc} -std=c++26 -fno-contracts-p4451 -Xclang -verify=off -DOFF
// RUN: %{cc} -std=c++2d -fno-contracts-p3097 -fcontracts-p4451 -fno-contracts-p4451 -Xclang -verify=off -DOFF
// RUN: %clang_cc1 -std=c++26 -fcontracts -fno-contracts-p4451 -fsyntax-only -verify=off -DOFF -DSTANDIN %s
//
// RUN: %{cc} -std=c++23 -fcontracts -Xclang -verify=off -DOFF -DBELOW
// RUN: %{cc} -std=c++20 -fcontracts -Xclang -verify=off -DOFF -DBELOW
// RUN: %clang_cc1 -std=c++23 -fcontracts -fsyntax-only -verify=off -DOFF -DBELOW -DSTANDIN %s
//
// RUN: %{cc} -std=c++26 -fno-contracts -Xclang -verify=none -DNONE
//
// RUN: %clangxx %libcxx_flags -std=c++26 -fcontract-evaluation-semantic=ignore \
// RUN:   -DON -S -emit-llvm -o - %s | FileCheck --check-prefix=IR %s
//
// RUN: not %{cc} -std=c++26 -fcontracts-p3400 -fno-contracts-p4451 2>&1 \
// RUN:   | FileCheck --check-prefix=ERR %s
// ERR: error: '-fno-contracts-p4451' cannot be combined with '-fcontracts-p3400', which subsumes P4451
//
// REQUIRES: contracts-libcxx

// P4451R2 is part of C++26 (notadragon_wg21 DECISIONS.md E220 (2)):
// -std=c++26, -std=gnu++26 and every later standard imply -fcontracts-p4451
// when contracts are on, as they are by default there.  An explicit
// -fno-contracts-p4451 still turns it off, except with -fcontracts-p3400,
// which subsumes it.  Only the language mode implies it: -fcontracts, or any
// other contracts option, with an earlier standard does not.  -cc1 follows
// the same rule from -fcontracts and the language mode.  The C++29 runs
// turn off P3097, which C++29 implies and which would raise __cpp_contracts
// to 202612L (p3097-cxx29.cpp).
//
// With P4451 on, std::contracts::controls::enforced makes a contract
// assertion terminating -- evaluated with enforce although ignore is
// configured -- and __cpp_contracts and __cpp_lib_contracts carry P4451R2's
// value; without it the specifier points at the option and both keep
// P2900's value.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/ (the GCC E220 C++26-implies-
// P4451 tests)

#ifdef STANDIN
// -cc1 has no library paths: a stand-in for libc++'s P4451 declaration.
namespace std::contracts {
struct __enforced_label_t {};
#ifdef ON
namespace controls::inline __p4451 {
inline constexpr __enforced_label_t enforced{};
}
#endif
}
#else
#include <contracts>
#endif

#ifdef ON
// on-no-diagnostics
#if __cpp_contracts != 202611L
#error "__cpp_contracts"
#endif
#if !(__clang_contracts_p4451 > 0)
#error "__clang_contracts_p4451"
#endif
#if !defined(STANDIN) && __cpp_lib_contracts != 202611L
#error "__cpp_lib_contracts"
#endif
#ifdef __cpp_contracts_labels
#error "P3400 is off"
#endif

void f(int x) pre<std::contracts::controls::enforced>(x > 0);
void g(int x) { contract_assert<std::contracts::controls::enforced>(x > 0); }
int h(int x) post<std::contracts::controls::enforced>(r : r > 0) {
  return x;
}
#endif

#ifdef OFF
#if __cpp_contracts != 202502L
#error "__cpp_contracts"
#endif
#ifdef __clang_contracts_p4451
#error "__clang_contracts_p4451"
#endif
#if !defined(STANDIN) && !defined(BELOW) && __cpp_lib_contracts != 202502L
#error "__cpp_lib_contracts"
#endif

void f(int x) pre<std::contracts::controls::enforced>(x > 0); // off-error {{assertion-control-specifier naming 'enforced' requires '-fcontracts-p4451'}}
#endif

#ifdef NONE
// none-no-diagnostics
#if defined(__cpp_contracts) || defined(__clang_contracts_p4451)
#error "contracts are off"
#endif
#endif

// IR-LABEL: define {{.*}}@_Z1fi(
// IR: call void @__cxa_contract_violation_pre_enforce
#ifdef ON
void f(int x) pre<std::contracts::controls::enforced>(x > 0) {}
#endif
