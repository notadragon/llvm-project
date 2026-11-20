// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-prove-optimize=0 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify=expected,o0 %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-analysis-skip-dead-calls -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// RUN: %clang -### -std=c++26 -fcontracts-p4405 -fcontracts-prove-optimize=0 -fno-contract-analysis-skip-dead-calls -c %s 2>&1 | FileCheck --check-prefix=DRIVER %s
// RUN: %clang -### -std=c++26 -fcontracts-p4405 -fcontracts-prove-optimize=0 -fno-contract-analysis-skip-dead-calls -fcontract-analysis-skip-dead-calls -c %s 2>&1 | FileCheck --check-prefix=DRIVER-ON %s
// REQUIRES: x86-registered-target

// P4405, notadragon_wg21 DECISIONS.md E169 (13), (14), (18): a call that
// is never evaluated has no analysis evaluation at the call under
// -fcontract-analysis-skip-dead-calls, which -fcontracts-p4405 implies.
// Clang's code generation never emits such code, and the analysis
// evaluations at calls are made as calls are emitted, so what is skipped
// is what code generation leaves out because it folds a condition
// (ConstantFoldsToSimpleInteger, the constant evaluator in folding mode):
// an operand of `?:' that the condition never selects, the second operand
// of `&&' or `||' behind a constant, a substatement of `if' that a
// constant condition never selects, a discarded statement, a case of a
// `switch' on a constant that is never reached, and a statement after a
// return (unreachable, no label) -- at every -O level.  `1 ?' and `0 &&'
// count as `true ?' and `false &&' do, and so does a `constexpr'
// variable; a template decides where it is instantiated, as a
// non-template does (`t*').  The operand that is evaluated is analyzed
// (`live*'), and so is any call behind a condition that does not fold
// (`h4').  The nested walk of a predicate skips such an operand whatever
// the option says (`h7', `h8': no note for g3's precondition; folding
// alone, at proof level 0, cannot prove the assertion of `h7').
// -fno-contract-analysis-skip-dead-calls: p4405-analysis-dead-call-no-skip.cpp.
//
// Differences from gnu_gcc, which decides by constant evaluation where a
// call is built (E169 (18): each compiler's own permission):
// - `a1', `s1': a statement after a return and a case of a constant
//   switch never reached are skipped here; gnu_gcc analyzes them.
// - `lam': a lambda's body in a dead arm is never emitted here, so its
//   calls are not analyzed (under -fno- either); gnu_gcc analyzes a
//   lambda's body as a function of its own.
// - `agg': the dead arm of a `?:' of class type is emitted all the same
//   (CGExprAgg does not fold), so g3's precondition there is always false
//   at proof level 0; from -O1 the proof shadow finds it never reached, as it does
//   `h4' and `th4' (notadragon_wg21 DECISIONS.md E195: an evaluation never
//   reached is proven).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-dead-call.C
// (gnu_gcc, PC4b-GCC); `a1', `s1', `lam', `agg' are Clang's.

int g3(int x) pre(x != 0);      // expected-note 9 {{the predicate of the contract assertion}} o0-note 3 {{the predicate of the contract assertion}}
int g3(int x) { return 10 / x; }
int g4(int x) pre(x != 0);
int g4(int x) { return 10 / x; }

constexpr bool k = false;

// Never evaluated: not analyzed.
int h1() { return 1 ? g4(1) : g3(0); }
int h1b() { return 0 ? g3(0) : 2; }
bool h2() { return false && g3(0); }
bool h2b() { return true || g3(0); }
bool h2c() { return 0 && g3(0); }
bool h2d() { return 1 || g3(0); }
int h3(int y) { if (0) return g3(0); return y; }
int h3b(int y) { if (1) return y; else return g3(0); }
int h5() { return k ? g3(0) : 2; }
int h5b(int y) { if (k) return g3(0); return y; }
int h6() { if constexpr (false) return g3(0); return 1; }
int h6b() { if constexpr (true) return 1; else return g3(0); }
bool h9() { return 1 || 0 && g3(0); }
bool h9b() { return true || (false && g3(0)); }
int h9c(int y) { return y ? (0 ? g3(0) : 1) : 2; }
int a1(int y) { return y; g3(0); }
int s1() { switch (1) { case 0: return g3(0); case 1: return 1; } return 0; }
int lam() { return 0 ? [] { return g3(0); }() : 1; }

// Evaluated, or not known never to be: analyzed.
int live1() { return 0 ? 2 : g3(0); }   // expected-error {{contract predicate is always false}}
bool live2() { return true && g3(0); }  // expected-error {{contract predicate is always false}}
bool live3() { return 0 || g3(0); }     // expected-error {{contract predicate is always false}}
int live4(int y) { if (0) return y; else return g3(0); }  // expected-error {{contract predicate is always false}}
int live5(int y) { if (1) return g3(0); return y; }  // expected-error {{contract predicate is always false}}
int live6(int y) { if (0) return y; return g3(0); }  // expected-error {{contract predicate is always false}}
bool live7() { return 0 && g3(0) || g3(0); }  // expected-error {{contract predicate is always false}}
int h4(int y) { return y > 0 && y < 0 ? g3(0) : y; }  // o0-error {{contract predicate is always false}}

struct S { int v; S(int v) : v(v) {} };
S mk(int);
S agg() { return 1 ? S(1) : mk(g3(0)); }  // o0-error {{contract predicate is always false}}

// Predicates: the nested walk, unchanged.
int h7(int y) { contract_assert((1 ? g4(1) : g3(0)) == 10); return y; }  // o0-error {{contract assertion cannot be proven}}
int h8(int y) { contract_assert(false && g3(0)); return y; }  // expected-error {{contract predicate is always false}}

// Templates, decided where they are instantiated: a non-dependent
// condition, and a dependent one.
template <typename T> int t1() { return 1 ? g4(1) : g3(0); }
template <typename T> int t1b() { return 0 ? g3(0) : 2; }
template <typename T> bool t2() { return false && g3(0); }
template <typename T> bool t2b() { return true || g3(0); }
template <typename T> bool t2c() { return 0 && g3(0); }
template <typename T> int t3(int y) { if (0) return g3(0); return y; }
template <typename T> int t3b(int y) { if (1) return y; else return g3(0); }
template <typename T> int t5() { return k ? g3(0) : 2; }
template <typename T> int t6() { if constexpr (false) return g3(0); return 1; }
template <int N> int tn1() { return N ? g4(1) : g3(0); }
template <int N> bool tn2() { return N && g3(0); }
template <int N> int tn3(int y) { if (N) return g3(0); return y; }
template <int N> int tn6() { if constexpr (N) return g3(0); return 1; }
template <typename T> int tlive1() { return 0 ? 2 : g3(0); }  // expected-error {{contract predicate is always false}}
template <int N> bool tlive2() { return N && g3(0); }  // expected-error {{contract predicate is always false}}
template <typename T> int th4(int y) { return y > 0 && y < 0 ? g3(0) : y; }  // o0-error {{contract predicate is always false}}

template int t1<int>();
template int t1b<int>();
template bool t2<int>();
template bool t2b<int>();
template bool t2c<int>();
template int t3<int>(int);
template int t3b<int>(int);
template int t5<int>();
template int t6<int>();
template int tn1<1>();
template bool tn2<0>();
template int tn3<0>(int);
template int tn6<0>();
template int tlive1<int>();  // expected-note {{in instantiation of function template specialization 'tlive1<int>' requested here}}
template bool tlive2<1>();   // expected-note {{in instantiation of function template specialization 'tlive2<1>' requested here}}
template int th4<int>(int);  // o0-note {{in instantiation of function template specialization 'th4<int>' requested here}}

// The driver passes the flag on, the last one winning.
// DRIVER: "-fno-contract-analysis-skip-dead-calls"
// DRIVER-ON: "-fcontract-analysis-skip-dead-calls"
// DRIVER-ON-NOT: "-fno-contract-analysis-skip-dead-calls"
