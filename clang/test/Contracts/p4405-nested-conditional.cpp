// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-prove-assume -fcontract-evaluation-semantic=observe -fcontract-configuration-file=%S/p4405-nested-conditional.json -triple x86_64-unknown-linux-gnu -O2 -Rcontract-proof -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-prove-assume -fcontract-evaluation-semantic=observe -fcontract-configuration-file=%S/p4405-nested-conditional.json -triple x86_64-unknown-linux-gnu -O0 -Rcontract-proof -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405, notadragon_wg21 DECISIONS.md E169: a nested precondition proven
// only conditionally -- here relying on an earlier assertion that is only
// observed -- makes the outer assertion conditional, so that
// -fcontracts-prove-assume does not turn it into an assumption (no
// "; assumed" in the remark).  Only the outer assertion (by its line, in the
// JSON file) and g's precondition are analyzed (with prove); g's
// precondition is also analyzed at its call in the outer's check (observe),
// which the observed assertion proves conditionally too.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-nested-conditional.C
// (gnu_gcc, PC1-GCC).  Line-sensitive: the JSON file names line 31.

namespace pp {
int g(int x) pre(x != 0);
int g(int) { return 1; }
}

// The remarks:
// the outer assertion's own result, and with the precondition nested in it;
// the nested precondition's; the precondition's at the call in the check.

int f(int y) {
  contract_assert(y != 0);
  // expected-remark@+3 {{proof shadow: proven, conditional; with those nested in it: proven, conditional}}
  // expected-remark@+2 {{proof shadow: nested: proven, conditional}}
  // expected-remark-re@+1 {{proof shadow: proven, conditional{{$}}}}
  contract_assert(pp::g(y) == 1);
  return y;
}
