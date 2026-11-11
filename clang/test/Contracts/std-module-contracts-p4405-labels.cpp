// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -Wno-unused-command-line-argument -I%S/../../../libcxx/modules --precompile %t/probe.cppm -o %t/probe.pcm
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -Wno-unused-command-line-argument -fmodule-file=probe=%t/probe.pcm -fsyntax-only -Xclang -verify %t/use.cpp
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -Wno-unused-command-line-argument -I%S/../../../libcxx/modules --precompile %t/probe.cppm -o %t/probe-p3400.pcm
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -Wno-unused-command-line-argument -fmodule-file=probe=%t/probe-p3400.pcm -fsyntax-only -Xclang -verify=nop4405 %t/absent.cpp

// The std module's contracts partition (libcxx/modules/std/contracts.inc),
// built with -fcontracts-p3400 -fcontracts-p4405, exports D4405's standard
// labels prove, diagnose, always_prove and always_diagnose (and their
// types), found unqualified in a label through the exported contract_control
// directive as with the #include path (p4405-std-labels.cpp); without
// -fcontracts-p4405 it exports none of them.  As in
// std-module-contracts-labels.cpp, the partition is exported from a small
// module rather than building all of std.
//
// Mirror: gcc/testsuite/g++.dg/modules/std-module-contract-p4405-labels.C

//--- probe.cppm
module;
#include <contracts>
export module probe;
#include "std/contracts.inc"

//--- use.cpp
// expected-no-diagnostics
import probe;

namespace L = std::contracts::labels;
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

constexpr L::prove_t p = L::prove;
constexpr L::diagnose_t d = L::diagnose;
constexpr L::always_prove_t ap = L::always_prove;
constexpr L::always_diagnose_t ad = L::always_diagnose;
static_assert(p.analysis_semantic() == evaluation_semantic::prove);
static_assert(d.analysis_semantic() == evaluation_semantic::diagnose);
static_assert(ap.allowed_semantics ==
              evaluation_semantic_set{evaluation_semantic::prove});
static_assert(ad.allowed_semantics ==
              evaluation_semantic_set{evaluation_semantic::diagnose});
static_assert((L::prove | L::diagnose).analysis_semantic() ==
              evaluation_semantic::prove);

void f(int x) pre<always_prove>(true);
void g(int x) pre<diagnose>(x > 0);
void h(int x) pre<prove | always_diagnose>(x > 0);

//--- absent.cpp
import probe;

namespace L = std::contracts::labels;
constexpr auto ok = L::always_enforce;
constexpr auto a = L::prove; // nop4405-error {{no member named 'prove' in namespace 'std::contracts::labels'}}
