// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-configuration-file=%S/c-p4405-shadow-previous.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3 in C: the proof shadow assumes an earlier observed
// contract assertion (a conditional proof) and an earlier enforced
// precondition (a sound one), as the -Rcontract-proof remark shows.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-shadow-previous.c (GCC
// shows the results in its -fdump-tree-contract_shadow dump)

int sink(int);

int f(int x)
  _Pre(x > 0)                   // line 14: enforce
{
  _ContractAssert(x > 0);       // expected-remark {{proof shadow: proven, sound}}
  return sink(x);
}

int g(int x) {
  _ContractAssert(x > 0);       // line 21: observe
  _ContractAssert(x > 0);       // expected-remark {{proof shadow: proven, conditional}}
  return sink(x);
}
