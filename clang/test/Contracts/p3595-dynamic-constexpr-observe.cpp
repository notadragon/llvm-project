// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fcontract-configuration-file=%S/p3595-dynamic-constexpr-observe.json %libcxx_flags -fsyntax-only -Xclang -verify %s

// P3595: a dynamic entry's selector is never consulted during constant
// evaluation; its "semantic" is what constant evaluation uses.  Here that
// semantic is observe: a false predicate is a warning, not an error, and the
// call is still a constant expression -- whatever the selector would return
// at run time (enforce here, which would make it an error), and whatever the
// command line says.  p3595-dynamic-constexpr.cpp covers an "enforce"
// semantic, p3595-dynamic-nodefault-constexpr.cpp an entry with no semantic.
// (GCC mirror: g++.dg/contracts/cpp26/p3595-dynamic-constexpr-observe.C)

#include <contracts>

std::contracts::evaluation_semantic p3595_sel_ceo() {
  return std::contracts::evaluation_semantic::enforce;
}

constexpr int g(int x) pre(x > 0) { return x; } // expected-warning {{contract failed during execution of constexpr function}}
constexpr int bad = g(-1);
constexpr int ok = g(1);
static_assert(bad == -1 && ok == 1);

// REQUIRES: contracts-libcxx
