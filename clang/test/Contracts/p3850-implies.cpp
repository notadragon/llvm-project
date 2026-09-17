// RUN: %clang_cc1 -std=c++26 -fcontracts-p3850 -fsyntax-only -verify %s
// expected-no-diagnostics

// P3850: the umbrella -fcontracts-p3850 flag enables every C++ contract
// sub-feature.  This is the single source of truth for those implications;
// individual feature tests use their own specific -fcontracts-pNNNN flags.
// (P4299 "C++ Contracts for C" is C-only and intentionally not implied here.)
// (GCC mirror: g++.dg/contracts/cpp26/p3850-implies.C, adapted to Clang's
// feature-test macro names -- __clang_contracts_p3100/p3290 vs GCC's __gcc_*.)

// Every sub-feature that predefines a gate/feature-test macro must be on.
#ifndef __cpp_contracts                          // P3097 (base bumped to 202606)
#  error "-fcontracts-p3850 must enable base contracts"
#endif
#ifndef __cpp_contracts_message                  // P3099
#  error "-fcontracts-p3850 must imply -fcontracts-p3099"
#endif
#ifndef __cpp_contracts_labels                   // P3400
#  error "-fcontracts-p3850 must imply -fcontracts-p3400"
#endif
#ifndef __cpp_contracts_postcondition_captures   // P3098
#  error "-fcontracts-p3850 must imply -fcontracts-p3098"
#endif
#ifndef __cpp_contracts_requires                 // P4283
#  error "-fcontracts-p3850 must imply -fcontracts-p4283"
#endif
#ifndef __cpp_contracts_nonthrowing_semantics    // P4298
#  error "-fcontracts-p3850 must imply -fcontracts-p4298"
#endif
#ifndef __cpp_contracts_report                   // P4301
#  error "-fcontracts-p3850 must imply -fcontracts-p4301"
#endif
#ifndef __clang_contracts_p3100                  // P3100
#  error "-fcontracts-p3850 must imply -fcontracts-p3100"
#endif
#ifndef __clang_contracts_p3290                  // P3290
#  error "-fcontracts-p3850 must imply -fcontracts-p3290"
#endif

// P3097 (contracts on virtual functions) predefines no dedicated macro; verify
// by compiling a contract on a virtual function, accepted only under P3097.
struct Base { virtual void f(int x) pre(x > 0) { } virtual ~Base() = default; };

int g(int x) pre(x > 0) { return x; }
