// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-nodefault.json -fsyntax-only -verify=enforce %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe -fcontract-configuration-file=%S/p3595-dynamic-nodefault.json -fsyntax-only -verify=observe -DOBSERVE %s

// P3595: a "dynamic" entry with no "semantic" never matches during constant
// evaluation (design section 3, half B), and when it is the only entry
// nothing else does either, so constant evaluation uses the command-line
// default: enforce by default (a false predicate is an error), observe
// under -fcontract-evaluation-semantic=observe (a warning, and the call is a
// constant).  p3595-dynamic-constexpr-skip.cpp covers the fall-through to a
// later entry.
// (GCC mirror: g++.dg/contracts/cpp26/p3595-dynamic-nodefault-constexpr.C and
// p3595-dynamic-nodefault-constexpr-observe.C, GCC-201)

constexpr int g(int x) pre(x > 0) { return x; } // enforce-error {{contract failed during execution of constexpr function}} observe-warning {{contract failed during execution of constexpr function}}
constexpr int bad = g(-1); // enforce-error {{constexpr variable 'bad' must be initialized by a constant expression}} enforce-note {{in call to 'g(-1)'}}
constexpr int ok = g(1);
#ifdef OBSERVE
static_assert(bad == -1 && ok == 1);
#endif
