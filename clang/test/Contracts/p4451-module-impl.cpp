// DEFINE: %{v} = -fcontracts-p4451
// DEFINE: %{p} = p4451
// DEFINE: %{cxx} = %clangxx -std=c++26 %{v} %libcxx_flags
// DEFINE: %{check} = %{cxx} -Wno-reserved-module-identifier --precompile %libcxx_moduledir/std.cppm -o %t/std.pcm && \
// DEFINE:   %{cxx} -fmodule-file=std=%t/std.pcm --precompile %t/m.cppm -o %t/m.pcm && \
// DEFINE:   %{cxx} -fmodule-file=std=%t/std.pcm -fmodule-file=p4451m=%t/m.pcm -fsyntax-only -Xclang -verify %t/impl.cpp && \
// DEFINE:   %{cxx} -fmodule-file=std=%t/std.pcm -fmodule-file=p4451m=%t/m.pcm -fsyntax-only -Xclang -verify=%{p} %t/use.cpp
//
// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %{check}
// REDEFINE: %{v} = -fcontracts-p3400
// REDEFINE: %{p} = p3400
// RUN: %{check}
// REDEFINE: %{v} = -fcontracts-p3400 -fcontracts-p4451
// RUN: %{check}
// REQUIRES: contracts-libcxx, libcxx-std-module

// P4451: a module interface that imports std without exporting it.  Its
// implementation unit sees the declaration of
// std::contracts::controls::enforced; an importer does not, so its
// assertion-control-specifier, unqualified or qualified, is rejected.  Under -fcontracts-p4451 alone,
// -fcontracts-p3400 alone (the label is undeclared) and both.
//
// Mirror: gcc/testsuite/g++.dg/modules/contract-p4451-impl_{a,b,c}.C (and
// their -p3400 and -both variants)

//--- m.cppm
export module p4451m;
import std;

export int f(int);

//--- impl.cpp
// expected-no-diagnostics
module p4451m;

static int g(int x) pre<std::contracts::controls::enforced>(x > 0) { return x; }

int f(int x) {
  using namespace std::contracts::controls;
  contract_assert<enforced>(x > 0);
  return g(x);
}

//--- use.cpp
import p4451m;

void h(int x) pre<enforced>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}} p4451-note {{'<contracts>' declares them}} p3400-error {{use of undeclared identifier 'enforced'}}
void k(int x) pre<std::contracts::controls::enforced>(x > 0); // p4451-error {{must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}} p4451-note {{'<contracts>' declares them}} p3400-error {{missing '#include <contracts>'; 'enforced' must be declared before it is used}}
// p3400-note@*:* {{declaration here is not visible}}
