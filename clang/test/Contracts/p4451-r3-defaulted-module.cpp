// DEFINE: %{v} = -fcontracts-p4451
// DEFINE: %{p} = p4451
// DEFINE: %{cxx} = %clangxx -std=c++26 %{v} %libcxx_flags
// DEFINE: %{check} = %{cxx} -Wno-reserved-module-identifier --precompile %libcxx_moduledir/std.cppm -o %t/std.pcm && \
// DEFINE:   %{cxx} -fmodule-file=std=%t/std.pcm --precompile %t/m.cppm -o %t/m.pcm && \
// DEFINE:   %{cxx} -fmodule-file=std=%t/std.pcm -fmodule-file=p4451d=%t/m.pcm -fsyntax-only -Xclang -verify=expected,%{p} %t/impl.cpp
//
// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %{check}
// REQUIRES: contracts-libcxx, libcxx-std-module

// P4451R3: the contract assertions of functions a module interface declares
// keep which label their specifier names through the BMI, so its
// implementation unit's definitions are matched against them: a definition
// naming defaulted again, however spelled, matches (f), one with no
// specifier (g) or naming enforced (h) does not, nor does one naming
// defaulted where the interface has none (k).  `import std;` declares
// defaulted (u).  Under -fcontracts-p4451 alone, -fcontracts-p3400 alone
// (the label is an expression the BMI keeps) and both.

//--- m.cppm
export module p4451d;
import std;

namespace scl = std::contracts::controls;

export int f(int x) pre<scl::defaulted>(x > 0);
export int g(int x) pre<scl::defaulted>(x > 0);
export int h(int x) pre<std::contracts::controls::defaulted>(x > 0);
export int k(int x) pre(x > 0);
export int u(int x) pre<std::contracts::controls::defaulted>(x > 0);

//--- impl.cpp
module p4451d;

int f(int x) pre<std::contracts::controls::defaulted>(x > 0) { return x; }
int g(int x) pre(x > 0) { return x; } // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}
int h(int x) pre<std::contracts::controls::enforced>(x > 0) { return x; } // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}
int k(int x) pre<scl::defaulted>(x > 0) { return x; } // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}
int u(int x) {
  using namespace std::contracts::controls;
  contract_assert<defaulted>(x > 0);
  return x;
}

// p4451-note@m.cppm:7 {{contract previously specified with an assertion-control-specifier naming 'std::contracts::controls::defaulted'}}
// p4451-note@m.cppm:8 {{contract previously specified with an assertion-control-specifier naming 'std::contracts::controls::defaulted'}}
// p4451-note@m.cppm:9 {{contract previously specified without an assertion-control-specifier}}
// p3400-note@m.cppm:7 {{contract previously specified with a different assertion-control label}}
// p3400-note@m.cppm:8 {{contract previously specified with a different assertion-control label}}
// p3400-note@m.cppm:9 {{contract previously specified with a different assertion-control label}}
