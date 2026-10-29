// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3: a predicate's calls make a proof conditional only if, once
// the proof shadow is optimized, the function called is not known to only
// read memory, return and not throw (by its attributes, or, for an inline
// function, whose attributes LLVM does not infer, by its body).  So an
// inline member function not declared pure (S::size, Vec::size, the shape
// of std::vector::size) keeps the proof sound; a call to an external
// function (even one the proof never evaluates) or a volatile access makes
// it conditional.  Both suppress the diagnostic; the remark tells them
// apart.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-pure-call.C
// (which uses std::vector; this test has no standard library)

int sink(int);
extern int log_call();
volatile int vol;

struct S {
  int n;
  int size() const { return n; }
};

int member(const S &s) {
  if (s.size() > 2)
    contract_assert(s.size() > 1);  // expected-remark {{proof shadow: proven, sound}}
  return sink(s.n);
}

struct Vec {
  int *b, *e;
  decltype(sizeof 0) size() const { return e - b; }
  int &front() const { return *b; }
};

int vector_size(const Vec &v) {
  if (v.size() > 2)
    contract_assert(v.size() > 1);  // expected-remark {{proof shadow: proven, sound}}
  return sink(v.front());
}

int impure(int x) {
  if (x > 0)
    contract_assert(x > 0 || log_call());  // expected-remark {{proof shadow: proven, conditional}}
  return sink(x);
}

int volatile_read(int x) {
  if (x > 0)
    contract_assert(x > 0 || vol);  // expected-remark {{proof shadow: proven, conditional}}
  return sink(x);
}
