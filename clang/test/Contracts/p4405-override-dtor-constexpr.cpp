// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -fcontract-evaluation-semantic=observe -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -fno-contracts-prove-callee-premises -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions", DECISIONS.md E152: a constexpr virtual
// destructor, user-provided or implicitly defined, gets its override
// shadows like any other, and constant evaluation of it is unaffected:
// nothing of an override shadow is in the destructor's code.
//
// -fno-contracts-prove-callee-premises: with the callee premise copies
// (D4405, DECISIONS.md E199 (b)) the base destructor's own observed
// postcondition, which the override shadow runs before the pair's obligation
// of it, would be a premise for that obligation, proving it conditionally.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-dtor-constexpr.C

int g;

struct CB {
  constexpr virtual ~CB()
    post(__builtin_is_constant_evaluated() || g == 1)  // expected-error {{contract assertion cannot be proven}} expected-note {{the predicate of the contract assertion}}
  {}
};

struct CD : CB {
  constexpr ~CD() override
  { if !consteval { g = 1; } }
};

struct CE : CB {                // expected-error {{cannot be proven for a call through 'CB::~CB'}}
  int i = 0;
};

constexpr bool destroy() {
  CB *p = new CD;
  delete p;
  p = new CE;
  delete p;
  { CD d; CE e; }
  return true;
}

static_assert(destroy());

void use() { CD d; CE e; }
