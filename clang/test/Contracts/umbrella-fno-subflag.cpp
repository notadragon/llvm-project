// RUN: %clang_cc1 -std=c++26 -fcontracts-p3850 -fno-contracts-p3400 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fno-contracts-p3400 -fcontracts-p3850 -fsyntax-only -verify %s

// An explicit -fno-contracts-pNNNN wins over the -fcontracts-p3850 umbrella,
// in either order: P3400 is off, the label is rejected and
// __cpp_contracts_labels is not defined.  The umbrella still implies P4451
// (DECISIONS.md E175), whose assertion-control-specifier must name
// std::contracts::controls::enforced (P4451R2).  The driver half is
// umbrella-fno-subflag-driver.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/umbrella-fno-subflag.C

struct L { using assertion_control_object = L; };
constexpr L lbl{}; // #lbl
int f(int x) pre<lbl>(x > 0) { return x; } // expected-error {{assertion-control-specifier must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds another declaration}} expected-note@#lbl {{'lbl' declared here}} expected-note {{'<contracts>' declares them}}

#if defined(__cpp_contracts_labels)
#error "__cpp_contracts_labels defined with -fno-contracts-p3400"
#endif
#if !defined(__clang_contracts_p4451)
#error "__clang_contracts_p4451 not defined with -fcontracts-p3850"
#endif
