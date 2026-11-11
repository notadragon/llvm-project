// DEFINE: %{v} = -fcontracts-p4451
// DEFINE: %{p} = p4451
// DEFINE: %{cxx} = %clangxx -std=c++26 %{v} %libcxx_flags
// DEFINE: %{check} = %{cxx} --precompile %t/m.cppm -o %t/m.pcm && \
// DEFINE:   %{cxx} -fmodule-file=p4451g=%t/m.pcm -fsyntax-only -Xclang -verify=%{p} %t/use.cpp
//
// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %{check}
// REDEFINE: %{v} = -fcontracts-p3400
// REDEFINE: %{p} = p3400
// RUN: %{check}
// REDEFINE: %{v} = -fcontracts-p3400 -fcontracts-p4451
// RUN: %{check}
// REQUIRES: contracts-libcxx

// P4451: <contracts> included in a module's global module fragment does not
// make std::contracts::labels::terminating visible to an importer, so it
// does not precede the importer's assertion-control-specifier.  Under
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

void h(int x) pre<terminating>(x > 0); // p4451-error {{no declaration of 'std::contracts::labels::terminating' precedes this assertion-control-specifier}} p4451-note {{declared by <contracts>}} p3400-error {{use of undeclared identifier 'terminating'}}
