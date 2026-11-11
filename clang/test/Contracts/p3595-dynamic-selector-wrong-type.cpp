// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-dynamic-selector-wrong-type.json -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-dynamic-selector-wrong-type.json -fsyntax-only -Wno-contract-configuration -verify=quiet %s
// quiet-no-diagnostics

// P3595 / DECISIONS.md E37 (a), CLANG-655: a declaration the translation
// unit makes with the qualified name of a "C++"-linkage dynamic selector in
// the configuration, but not the selector's type
// `std::contracts::evaluation_semantic ()`, is warned about at that
// declaration (-Wcontract-configuration), whether or not a contract in the
// translation unit uses the selector.  A function of that name with
// parameters, a template, or a "C"-linkage selector name is not.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-selector-wrong-type.C
// in the gnu_gcc fork (GCC-672).

namespace std::contracts {
  enum class evaluation_semantic : unsigned short {
    ignore = 1, observe, enforce, quick_enforce
  };
}

namespace defined {
  int sel() { return 3; } // expected-warning {{function 'sel' has the name of the dynamic contract selector 'defined::sel' in the contract configuration, but its type 'int ()' is not 'std::contracts::evaluation_semantic ()'}}
}

namespace declared {
  long sel(); // expected-warning {{function 'sel' has the name of the dynamic contract selector 'declared::sel' in the contract configuration, but its type 'long ()' is not 'std::contracts::evaluation_semantic ()'}}
}

namespace var {
  std::contracts::evaluation_semantic sel; // expected-warning {{variable 'sel' has the name of the dynamic contract selector 'var::sel' in the contract configuration, but it is not a function of type 'std::contracts::evaluation_semantic ()'}}
}

namespace right {
  std::contracts::evaluation_semantic sel();
}

namespace other {
  int sel(int);
  template <class T> int sel();
}

namespace outer {
  inline namespace v1 {
    unsigned short sel(); // expected-warning {{function 'sel' has the name of the dynamic contract selector 'outer::sel' in the contract configuration, but its type 'unsigned short ()' is not 'std::contracts::evaluation_semantic ()'}}
  }
}

int global_sel(); // expected-warning {{function 'global_sel' has the name of the dynamic contract selector 'global_sel' in the contract configuration, but its type 'int ()' is not 'std::contracts::evaluation_semantic ()'}}

extern "C" int c_sel();

namespace redecl {
  int sel();
  int sel() { return 1; } // expected-warning {{function 'sel' has the name of the dynamic contract selector 'redecl::sel' in the contract configuration, but its type 'int ()' is not 'std::contracts::evaluation_semantic ()'}}
}

namespace u1 { void f(int x) pre(x > 0) {} }
namespace u2 { void f(int x) pre(x > 0) {} }
namespace u4 { void f(int x) pre(x > 0) {} }
