// DEFINE: %{v} = -fcontracts-p4451
// DEFINE: %{p} = p4451
// DEFINE: %{cxx} = %clangxx -std=c++26 %{v} %libcxx_flags
// DEFINE: %{check} = %{cxx} --precompile %t/m.cppm -o %t/m.pcm && \
// DEFINE:   %{cxx} -fmodule-file=p4451g=%t/m.pcm -fsyntax-only -Xclang -verify=%{p} %t/use.cpp
//
// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %{check}
// REQUIRES: contracts-libcxx

// P4451: <contracts> included in a module's global module fragment does not
// make std::contracts::controls::enforced visible to an importer, so name
// lookup from the importer's assertion-control-specifier, unqualified or
// qualified, does not find it (P4451R2).  Under
// -fcontracts-p4451 alone, -fcontracts-p3400 alone (the label is
// undeclared) and both.
//
// Mirror: gcc/testsuite/g++.dg/modules/contract-p4451-gmf_{a,b}.C (and their
// -p3400 and -both variants)

//--- m.cppm
module;
#include <contracts>
export module p4451g;

export int x;

//--- use.cpp
import p4451g;

void h(int x) pre<enforced>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}} p4451-note {{'<contracts>' declares them}} p3400-error {{use of undeclared identifier 'enforced'}}
void k(int x) pre<std::contracts::controls::enforced>(x > 0); // p4451-error {{must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}} p4451-note {{'<contracts>' declares them}} p3400-error {{missing '#include <contracts>'; 'enforced' must be declared before it is used}}
// p3400-note@*:* {{declaration here is not visible}}
