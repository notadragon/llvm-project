// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-configuration-file=%S/p3595-header-template.json %libcxx_flags -o %t && %t

// P3595 x templates: a template defined in a header and instantiated in the
// translation unit takes the semantic configured for the header's location
// -- where its contract assertions are written -- not the main file's, and so
// differs from a template with the same predicate defined in the main file.
// The header's entry says observe, the catch-all ignore: every header
// instantiation reports its violation, none of the main file's does.
// (GCC mirror: g++.dg/contracts/cpp26/p3595-header-template.C)

#include <contracts>
#include <cstdlib>
#include "p3595-header-template.h"

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

template <class T> T tu_f(T x) pre(x > 0) { return x; }
template <class T> struct TuS { T g(T x) pre(x > 0) { return x; } };

int main() {
  hdr_f(-1);
  hdr_f(-1.0);
  HdrS<int>().g(-1);
  hdr_inline(-1);
  if (violations != 4)
    std::abort();
  tu_f(-1);
  tu_f(-1.0);
  TuS<int>().g(-1);
  if (violations != 4)
    std::abort();
}

// REQUIRES: contracts-libcxx, native
