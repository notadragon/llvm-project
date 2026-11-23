// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/ce-observe-rt-enforce.json -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/ce-observe-rt-enforce.json -fexperimental-new-constant-interpreter -fsyntax-only -verify %s

// Under the observe semantic a violation in constant evaluation is a warning
// and evaluation continues, in the bytecode interpreter as in the default
// evaluator.

constexpr int f(int x) pre(x >= 0) { return x; } // expected-warning {{contract failed during execution of constexpr function}}
constexpr int y = f(-1);
static_assert(y == -1);

constexpr int g(int x) post(r : r > 0) { return x; } // expected-warning {{contract failed during execution of constexpr function}}
constexpr int z = g(-2);

constexpr int h(int x) { contract_assert(x != 3); return x; } // expected-warning {{contract failed during execution of constexpr function}}
constexpr int w = h(3);
