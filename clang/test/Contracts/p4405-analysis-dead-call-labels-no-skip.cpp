// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-prove-optimize=0 -fno-contract-analysis-skip-dead-calls %libcxx_flags -ferror-limit=0 -O0 -S -emit-llvm -o %t.ll -Xclang -verify=expected,o0 %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fno-contract-analysis-skip-dead-calls %libcxx_flags -ferror-limit=0 -O2 -S -emit-llvm -o %t.ll -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P4405, notadragon_wg21 DECISIONS.md E169 (13), (14), (18), E195: as
// p4405-analysis-dead-call-labels.cpp with
// -fno-contract-analysis-skip-dead-calls: every call is analyzed.  The
// `l*' and `b*' calls are emitted (code generation keeps a substatement
// holding a label), so they are diagnosed as with the option, the code
// before the label in `l6' and `l8' "always false" at proof level 0 and proven never
// reached by the proof shadow at -O2.  The `s*' calls are never emitted:
// each is analyzed where it is, decided at once by folding (no proof
// shadow holds code never emitted, CodeGenFunction::
// analyzeContractDeadCalls), so "always false" at both levels.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-dead-call-labels-no-skip-O0.C
// (and -no-skip-O2.C), where GCC emits the `s*' calls and its proof shadow
// proves them never reached at -O2.

#include <contracts>
using namespace std::contracts::labels;

void g (int x) pre<always_prove> (x != 0);  // expected-note 23 {{predicate}} o0-note 2 {{predicate}}

// Reached through a label.
void l1 (int c) { if (c) goto L; if (0) { L: g (0); } }  // expected-error {{always false}}
void l2 (int c) { if (c) goto L; if (1) {} else { L: g (0); } }  // expected-error {{always false}}
void l3 (int c) { switch (c) { case 0: if (0) { case 1: g (0); } } }  // expected-error {{always false}}
void l4 (int c) { switch (c) { case 0: if (0) { default: g (0); } } }  // expected-error {{always false}}
template <int N> void l5 (int c) { if (c) goto L; if (N) { L: g (0); } }  // expected-error {{always false}}
template void l5<0> (int);  // expected-note {{in instantiation}}
void l6 (int c)
{
  if (c)
    goto L;
  if (0)
    {
      g (0);  // o0-error {{always false}}
    L:
      g (0);  // expected-error {{always false}}
    }
}
void l7 (int c) { if (c) goto L; if (0) { while (c) { L: g (0); } } }  // expected-error {{always false}}
void l7b (int c) { if (c) goto L; if (0) { while (c-- > 1) { L:; } g (0); } }  // expected-error {{always false}}
void l7c (int c) { if (c) goto L; if (0) { do { L: g (0); } while (0); } }  // expected-error {{always false}}
template <int N> void l8 (int c)
{
  if (c)
    goto L;
  if (N)
    {
      g (0);  // o0-error {{always false}}
    L:
      g (0);  // expected-error {{always false}}
    }
}
template void l8<0> (int);  // expected-note {{in instantiation}}
template <int N> void l9 (int c) { if (c) goto L; if (N) { while (c) { L: g (0); } } }  // expected-error {{always false}}
template void l9<0> (int);  // expected-note {{in instantiation}}

// GCC-994's shapes: the branch a constant condition never selects, entered
// at a label, falls through to what follows the `if'.
void b1 (int c) { if (c) goto L; if (1) return; else { L:; } g (0); }  // expected-error {{always false}}
void b2 (int c) { if (c) goto L; if (0) { L:; } else return; g (0); }  // expected-error {{always false}}
void b3 (int c) { switch (c) { case 0: if (1) return; else { case 1:; } g (0); } }  // expected-error {{always false}}
void b4 (int c) { if (c) goto L; if (1) { if (1) return; else { L:; } } g (0); }  // expected-error {{always false}}
void b5 (int c) { if (c) goto L; if (0) { L:; } else throw 1; g (0); }  // expected-error {{always false}}
template <int N> void b6 (int c) { if (c) goto L; if (N) return; else { L:; } g (0); }  // expected-error {{always false}}
template void b6<1> (int);  // expected-note {{in instantiation}}

// Never reached: never emitted, analyzed at once.
void s1 (int c) { if (0) { switch (c) { case 1: g (0); default: break; } } }  // expected-error {{always false}}
void s2 (int c) { if (0) { while (c) { if (c > 1) break; g (0); } } }  // expected-error {{always false}}
void s3 (int c) { if (0) { for (;;) { if (c) continue; g (0); } } }  // expected-error {{always false}}
void s4 (int c) { switch (c) { case 0: if (0) { switch (c) { case 1: g (0); } } } }  // expected-error {{always false}}
void s5 (int c) { if (1) return; else { switch (c) { case 1:; } } g (0); }  // expected-error {{always false}}
void s6 (int c) { if (1) return; else {} g (0); }  // expected-error {{always false}}
