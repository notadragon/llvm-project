// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fno-contract-analysis-skip-dead-calls -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-prove-optimize=0 -fno-contract-analysis-skip-dead-calls -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify=expected,o0 %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-analysis-skip-dead-calls -fno-contract-analysis-skip-dead-calls -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405, notadragon_wg21 DECISIONS.md E169 (13), (14), (18):
// -fno-contract-analysis-skip-dead-calls gives a call that code generation
// does not emit, because it is never evaluated, its analysis evaluation at
// the call all the same, decided at once (folding; no proof shadow holds
// code never emitted): every dead call of p4405-analysis-dead-call.cpp is
// analyzed here, and g3's precondition is always false there, in templates
// and non-templates alike (but for the discarded statement of a template,
// which is never instantiated, `t6', `tn6').  The nested walk of a
// predicate skips such an operand whatever the option says (`h7', `h8';
// folding alone, at proof level 0, cannot prove the assertion of `h7').  The code
// that is emitted, `h4', `th4' and `agg', is decided by the proof shadow
// from -O1, which finds g3's precondition there never reached
// (notadragon_wg21 DECISIONS.md E195: an evaluation never reached is
// proven), whatever the option says; always false at proof level 0.
//
// Differences from gnu_gcc (E169 (18)): `lam', a lambda's body in a dead
// arm, is never emitted here, so its call is not analyzed even so (gnu_gcc
// analyzes it); `a1' and `s1' (a statement after a return, a case of a
// constant switch never reached) are analyzed here as under gnu_gcc, which
// analyzes them whatever the option says.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-dead-call-no-skip.C
// (gnu_gcc, PC4b-GCC); `a1', `s1', `lam', `agg' are Clang's.

int g3(int x) pre(x != 0);      // expected-note 38 {{the predicate of the contract assertion}} o0-note 3 {{the predicate of the contract assertion}}
int g3(int x) { return 10 / x; }
int g4(int x) pre(x != 0);
int g4(int x) { return 10 / x; }

constexpr bool k = false;

// Never evaluated: analyzed all the same.
int h1() { return 1 ? g4(1) : g3(0); }  // expected-error {{contract predicate is always false}}
int h1b() { return 0 ? g3(0) : 2; }  // expected-error {{contract predicate is always false}}
bool h2() { return false && g3(0); }  // expected-error {{contract predicate is always false}}
bool h2b() { return true || g3(0); }  // expected-error {{contract predicate is always false}}
bool h2c() { return 0 && g3(0); }  // expected-error {{contract predicate is always false}}
bool h2d() { return 1 || g3(0); }  // expected-error {{contract predicate is always false}}
int h3(int y) { if (0) return g3(0); return y; }  // expected-error {{contract predicate is always false}}
int h3b(int y) { if (1) return y; else return g3(0); }  // expected-error {{contract predicate is always false}}
int h5() { return k ? g3(0) : 2; }  // expected-error {{contract predicate is always false}}
int h5b(int y) { if (k) return g3(0); return y; }  // expected-error {{contract predicate is always false}}
int h6() { if constexpr (false) return g3(0); return 1; }  // expected-error {{contract predicate is always false}}
int h6b() { if constexpr (true) return 1; else return g3(0); }  // expected-error {{contract predicate is always false}}
bool h9() { return 1 || 0 && g3(0); }  // expected-error {{contract predicate is always false}}
bool h9b() { return true || (false && g3(0)); }  // expected-error {{contract predicate is always false}}
int h9c(int y) { return y ? (0 ? g3(0) : 1) : 2; }  // expected-error {{contract predicate is always false}}
int a1(int y) { return y; g3(0); }  // expected-error {{contract predicate is always false}}
int s1() { switch (1) { case 0: return g3(0); case 1: return 1; } return 0; }  // expected-error {{contract predicate is always false}}
int lam() { return 0 ? [] { return g3(0); }() : 1; }

// Evaluated, or not known never to be: analyzed.
int live1() { return 0 ? 2 : g3(0); }   // expected-error {{contract predicate is always false}}
bool live2() { return true && g3(0); }  // expected-error {{contract predicate is always false}}
bool live3() { return 0 || g3(0); }     // expected-error {{contract predicate is always false}}
int live4(int y) { if (0) return y; else return g3(0); }  // expected-error {{contract predicate is always false}}
int live5(int y) { if (1) return g3(0); return y; }  // expected-error {{contract predicate is always false}}
int live6(int y) { if (0) return y; return g3(0); }  // expected-error {{contract predicate is always false}}
bool live7() { return 0 && g3(0) || g3(0); }  // expected-error 2 {{contract predicate is always false}}
int h4(int y) { return y > 0 && y < 0 ? g3(0) : y; }  // o0-error {{contract predicate is always false}}

struct S { int v; S(int v) : v(v) {} };
S mk(int);
S agg() { return 1 ? S(1) : mk(g3(0)); }  // o0-error {{contract predicate is always false}}

// Predicates: the nested walk, unchanged.
int h7(int y) { contract_assert((1 ? g4(1) : g3(0)) == 10); return y; }  // o0-error {{contract assertion cannot be proven}}
int h8(int y) { contract_assert(false && g3(0)); return y; }  // expected-error {{contract predicate is always false}}

// Templates, decided where they are instantiated: a non-dependent
// condition, and a dependent one.
template <typename T> int t1() { return 1 ? g4(1) : g3(0); }  // expected-error {{contract predicate is always false}}
template <typename T> int t1b() { return 0 ? g3(0) : 2; }  // expected-error {{contract predicate is always false}}
template <typename T> bool t2() { return false && g3(0); }  // expected-error {{contract predicate is always false}}
template <typename T> bool t2b() { return true || g3(0); }  // expected-error {{contract predicate is always false}}
template <typename T> bool t2c() { return 0 && g3(0); }  // expected-error {{contract predicate is always false}}
template <typename T> int t3(int y) { if (0) return g3(0); return y; }  // expected-error {{contract predicate is always false}}
template <typename T> int t3b(int y) { if (1) return y; else return g3(0); }  // expected-error {{contract predicate is always false}}
template <typename T> int t5() { return k ? g3(0) : 2; }  // expected-error {{contract predicate is always false}}
template <typename T> int t6() { if constexpr (false) return g3(0); return 1; }
template <int N> int tn1() { return N ? g4(1) : g3(0); }  // expected-error {{contract predicate is always false}}
template <int N> bool tn2() { return N && g3(0); }  // expected-error {{contract predicate is always false}}
template <int N> int tn3(int y) { if (N) return g3(0); return y; }  // expected-error {{contract predicate is always false}}
template <int N> int tn6() { if constexpr (N) return g3(0); return 1; }
template <typename T> int tlive1() { return 0 ? 2 : g3(0); }  // expected-error {{contract predicate is always false}}
template <int N> bool tlive2() { return N && g3(0); }  // expected-error {{contract predicate is always false}}
template <typename T> int th4(int y) { return y > 0 && y < 0 ? g3(0) : y; }  // o0-error {{contract predicate is always false}}

template int t1<int>();  // expected-note {{in instantiation of function template specialization 't1<int>' requested here}}
template int t1b<int>();  // expected-note {{in instantiation of function template specialization 't1b<int>' requested here}}
template bool t2<int>();  // expected-note {{in instantiation of function template specialization 't2<int>' requested here}}
template bool t2b<int>();  // expected-note {{in instantiation of function template specialization 't2b<int>' requested here}}
template bool t2c<int>();  // expected-note {{in instantiation of function template specialization 't2c<int>' requested here}}
template int t3<int>(int);  // expected-note {{in instantiation of function template specialization 't3<int>' requested here}}
template int t3b<int>(int);  // expected-note {{in instantiation of function template specialization 't3b<int>' requested here}}
template int t5<int>();  // expected-note {{in instantiation of function template specialization 't5<int>' requested here}}
template int t6<int>();
template int tn1<1>();  // expected-note {{in instantiation of function template specialization 'tn1<1>' requested here}}
template bool tn2<0>();  // expected-note {{in instantiation of function template specialization 'tn2<0>' requested here}}
template int tn3<0>(int);  // expected-note {{in instantiation of function template specialization 'tn3<0>' requested here}}
template int tn6<0>();
template int tlive1<int>();  // expected-note {{in instantiation of function template specialization 'tlive1<int>' requested here}}
template bool tlive2<1>();   // expected-note {{in instantiation of function template specialization 'tlive2<1>' requested here}}
template int th4<int>(int);  // o0-note {{in instantiation of function template specialization 'th4<int>' requested here}}
