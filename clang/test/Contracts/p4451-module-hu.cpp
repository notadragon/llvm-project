// DEFINE: %{v} = -fcontracts-p4451
// DEFINE: %{cxx} = %clangxx -std=c++26 %{v} %libcxx_flags
// DEFINE: %{check} = %{cxx} -w -fmodule-header=system -xc++-system-header contracts -o %t/contracts.pcm && \
// DEFINE:   %{cxx} -Wno-experimental-header-units -fmodule-file=%t/contracts.pcm -fsyntax-only -Xclang -verify %s
//
// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: %{check}
// REDEFINE: %{v} = -fcontracts-p3400
// RUN: %{check}
// REDEFINE: %{v} = -fcontracts-p3400 -fcontracts-p4451
// RUN: %{check}
// REQUIRES: contracts-libcxx

// expected-no-diagnostics

// P4451: <contracts> as a header unit declares
// std::contracts::controls::enforced for its importers, so after `import
// <contracts>;` assertion-control-specifiers can name it (qualified, through
// an alias and through a using-directive).  Under -fcontracts-p4451 alone,
// -fcontracts-p3400 alone and both.
//
// Mirror: gcc/testsuite/g++.dg/modules/contract-p4451-hu_a.X / _b.C (and
// their -p3400 and -both variants)

import <contracts>;

namespace scl = std::contracts::controls;

int f(int x) pre<std::contracts::controls::enforced>(x > 0)
    post<scl::enforced>(r : r > 0) {
  using namespace std::contracts::controls;
  contract_assert<enforced>(x > 1);
  return x;
}
