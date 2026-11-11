// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
// RUN: cd %t
// RUN: %clang_cc1 -std=c++20 -Wno-experimental-header-units \
// RUN:   -emit-header-unit -xc++-user-header hu.h -o hu.pcm
// RUN: %clang_cc1 -std=c++20 -Wno-experimental-header-units \
// RUN:   -fmodule-file=hu.pcm -fsyntax-only -verify use.cpp

// D3400R6 [module.interface]: every using-directive in a header unit is
// exported, whether at global scope or nested in a namespace, so an
// importer's unqualified and qualified lookup see the nominated namespace.
// The nested directive is in its namespace only, so the name it nominates
// is not an ordinary global name.  (HU survey case 2.)
//
// Mirror: gcc/testsuite/g++.dg/modules/using-directive-hu-1_a.H and _b.C

//--- hu.h
namespace hu1_n {
inline int hu1_v = 1;
}
using namespace hu1_n;

namespace hu1_a {
namespace hu1_m {
inline int hu1_w = 2;
}
using namespace hu1_m;
} // namespace hu1_a

//--- use.cpp
import "hu.h";

// The global directive: unqualified lookup.
int g1() { return hu1_v; }

// The nested directive: qualified lookup into hu1_a ...
int g2() { return hu1_a::hu1_w; }

// ... and unqualified lookup from within hu1_a.
namespace hu1_a {
int g3() { return hu1_w; }
} // namespace hu1_a

// Not an ordinary global name: the nested directive is in hu1_a only.
int g4() { return hu1_w; } // expected-error {{use of undeclared identifier 'hu1_w'}}
