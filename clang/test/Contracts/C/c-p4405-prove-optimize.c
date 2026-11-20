// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -fcontracts-prove-optimize=0 -emit-llvm-only -verify=lvl0 %s
// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -fcontracts-prove-optimize=1 -emit-llvm-only -verify=lvl1 %s
// RUN: %clang -std=c23 -fcontracts-p4405 -fcontract-evaluation-semantic=prove --target=x86_64-unknown-linux-gnu -O1 -fcontracts-prove-optimize=0 -S -emit-llvm -o %t.ll -Xclang -verify=lvl0 %s
// RUN: %clang -std=c23 -fcontracts-prove-optimize=0 -DNO_CONTRACTS -fsyntax-only %s 2>&1 | FileCheck --check-prefix=NO-EFFECT %s
// RUN: %clang -std=c23 -fcontracts-p4405 -fcontracts-prove-optimize=0 -fsyntax-only %s 2>&1 | FileCheck --allow-empty --check-prefix=QUIET %s
// RUN: not %clang -std=c23 -fcontracts-prove-optimize=4 -DNO_CONTRACTS -fsyntax-only %s 2>&1 | FileCheck --check-prefix=BAD %s
// REQUIRES: x86-registered-target

// D4405's proof optimization level in C (notadragon_wg21 DECISIONS.md
// E162-E168, E205): at -fcontracts-prove-optimize=0 folding is the only
// strategy, so the postcondition c-p4405-shadow-post.c proves in its proof
// shadow is diagnosed, while one that folds is proven; at level 1 the proof
// shadow proves it.  The option is accepted for C, and warns that it has no
// effect without -fcontracts-p4405.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-prove-optimize-0.c,
// c-p4405-prove-optimize-1.c, c-contract-options-no-effect.c,
// c-contract-options-no-effect-contracts-on.c

#ifndef NO_CONTRACTS
int clamped(int v)
  _Post(r: r >= 0 && r <= 9)    // lvl0-error {{contract assertion cannot be proven}}
{
  if (v < 0)
    return 0;
  if (v > 9)
    return 9;
  return v;
}

int folded(int v)
  _Post(1)
{
  return v;
}
#endif

// lvl1-no-diagnostics

// NO-EFFECT: warning: '-fcontracts-prove-optimize=' has no effect without '-fcontracts-p4405'
// NO-EFFECT-NOT: {{warning|error}}:
// QUIET-NOT: {{warning|error}}:
// BAD: error: invalid value '4' in '-fcontracts-prove-optimize=', expected one of: 0 1 2 3 follow
