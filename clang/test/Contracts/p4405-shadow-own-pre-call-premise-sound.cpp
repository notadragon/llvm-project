// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-allow-assume -fcontract-evaluation-semantic=assume %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405R0, "Previous Assertions and Undefined Behavior" (CLANG-918): a
// function's own precondition whose predicate calls a function is a premise
// for its body.  A proof resting on it is conditional -- not diagnosed,
// never assumed -- unless every function the predicate calls is found free
// of side effects, returning and not throwing; then the premise's own
// classification decides, and an assume precondition's is guaranteed.
// Every precondition here is assume, every body evaluation prove, and each
// body evaluation can be proven only from its function's precondition.
//
// * sound_accessor: an inline accessor, free of side effects in fact (not
//   declared so): sound, and assumed in the real code
//   (-fcontracts-prove-assume, implied by -fcontracts-p4405).
// * cond_checked: the accessor has a precondition of its own: conditional,
//   not assumed.  An inline function holding a contract evaluation that is
//   not settled is never found free of side effects: another translation
//   unit's copy, the one the program may run, may evaluate it with observe,
//   whose violation handler may have side effects.
// * cond_opaque: a function only declared here: conditional, not assumed.
// * cond_effects: a callee that stores to a global: conditional.
// * cond_throws: a callee that may throw: conditional.
// * own_effect: a predicate with a side effect of its own (a compound
//   assignment through a mutable member) is not assumed at all, so the body
//   evaluation is not proven.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-own-pre-call-premise-sound.C

#include <contracts>
using namespace std::contracts::labels;

int sink(int);
bool opaque(const int *);
int counter;

struct C {
  int *p;
  int *raw() const { return p; }
  int *get() const pre(p) { return p; }
  int *bump() const { ++counter; return p; }
  int *thr() const { if (!p) throw 1; return p; }
};

int sound_accessor(C &c) pre(c.raw() != nullptr) {
  contract_assert<prove>(c.p != nullptr); // expected-remark {{proof shadow: proven, sound; assumed}}
  return sink(*c.p);
}

int cond_checked(C &c) pre(c.get() != nullptr) {
  contract_assert<prove>(c.p != nullptr); // expected-remark-re {{proof shadow: proven, conditional{{$}}}}
  return sink(*c.p);
}

int cond_opaque(C &c) pre(opaque(c.p) && c.p != nullptr) {
  contract_assert<prove>(c.p != nullptr); // expected-remark-re {{proof shadow: proven, conditional{{$}}}}
  return sink(*c.p);
}

int cond_effects(C &c) pre(c.bump() != nullptr) {
  contract_assert<prove>(c.p != nullptr); // expected-remark-re {{proof shadow: proven, conditional{{$}}}}
  return sink(*c.p);
}

int cond_throws(C &c) pre(c.thr() != nullptr) {
  contract_assert<prove>(c.p != nullptr); // expected-remark-re {{proof shadow: proven, conditional{{$}}}}
  return sink(*c.p);
}

struct M { mutable int k; };

int own_effect(const M &m) pre((m.k += 0) > 0) {
  contract_assert<prove>(m.k > 0); // expected-error {{cannot be proven}} \
                                   // expected-remark {{proof shadow: not proven}}
  return sink(m.k);
}
