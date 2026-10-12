// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
// RUN: cd %t
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -Wno-experimental-header-units \
// RUN:   -emit-header-unit -xc++-user-header cc.h -o cc.pcm
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -Wno-experimental-header-units \
// RUN:   -fmodule-file=cc.pcm -fsyntax-only -verify use.cpp

// P3400: a header unit carries a `using contract_control namespace` directive
// to its importer, where the short name resolves inside an assertion-control
// expression and does not leak into ordinary lookup.
//
// Mirror of gnu_gcc's g++.dg/modules/contract-control-2_a.H and _b.C.

//--- cc.h
namespace g10ns {
struct my_label_t {
  using assertion_control_object = my_label_t;
};
inline constexpr my_label_t my_label{};
} // namespace g10ns

using contract_control namespace g10ns;

//--- use.cpp
import "cc.h";

void f(int x) pre<my_label>(x > 0) {}

// Still not an ordinary name.
auto leak = my_label; // expected-error {{use of undeclared identifier 'my_label'; did you mean 'g10ns::my_label'?}}
// expected-note@cc.h:5 {{'g10ns::my_label' declared here}}
