// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -fcontract-configuration-file=%S/p4405-nested-diagnose.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify=expected,o2 %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -fcontract-configuration-file=%S/p4405-nested-diagnose.json -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify=expected,o2 %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -fcontract-configuration-file=%S/p4405-nested-diagnose.json -triple x86_64-unknown-linux-gnu -O2 -Rcontract-proof -emit-llvm-only -verify=expected,o2,rem %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -fcontract-configuration-file=%S/p4405-nested-diagnose.json -triple x86_64-unknown-linux-gnu -O0 -Rcontract-proof -emit-llvm-only -verify=expected,o2,rem %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontracts-prove-optimize=0 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -fcontract-configuration-file=%S/p4405-nested-diagnose.json -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify=expected,o0,noshadow %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fno-contracts-prove-override -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -fcontract-configuration-file=%S/p4405-nested-diagnose.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify=expected,no,noshadow %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fno-contracts-prove-override -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -fcontract-configuration-file=%S/p4405-nested-diagnose.json -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify=expected,no,noshadow %s
// REQUIRES: x86-registered-target

// P4405, notadragon_wg21 DECISIONS.md E169 (8), (16): an override shadow's
// evaluations -- D::f proving D::f's preconditions and postconditions and
// B::f's postconditions, B::f's preconditions assumed (D4405R0, "Virtual
// Functions") -- get the nested evaluations of the preconditions of the
// calls in their predicates, decided and reported per E169, in the
// override shadow of their pair: a nested prove precondition not proven
// makes the override obligation unproven ("... for a call through
// 'B::f'", at D::f's definition, E155) with a note at it (D2, unprovable;
// D3, always false, by folding); B::f's precondition proves one (D1, D5,
// conditionally, E151); a nested diagnose one is warned at its call in
// B::f's or D::f's predicate, naming the pair, and only makes its outer
// conditional (D4, D7).  D6::f's postcondition is diagnosed in both its
// override shadow and its own body.  The -Rcontract-proof remarks show
// E169 (12) in an override shadow: the outer's own result stays sound
// beside a failed nested one (gnu_gcc's dump scans).
//
// -O0, folding alone (o0): no override shadow, so every override
// obligation calling chk or dd::dchk with an unknown argument is unproven,
// noting chk's precondition; the nested diagnose one is still warned once
// at its call for its pair.  D6::f's postcondition has no override
// obligation without override shadows; its ordinary analysis is unproven.
// -fno-contracts-prove-override (no, DECISIONS.md E157): D::f's
// preconditions are decided at once by folding and B::f's postconditions
// in D::f's ordinary proof shadow, with no premise from B::f's
// preconditions, so D1's and D5's nested chk calls are unproven too.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-nested.C,
// p4405-override-nested-O0.C and p4405-override-nested-nooverride.C
// (gnu_gcc, PC4-GCC).

int chk(int v) pre(v != 0);     // expected-note 2 {{precondition of a call in the predicate cannot be proven}} \
                                // o2-note {{precondition of a call in the predicate cannot be proven}} \
                                // noshadow-note 2 {{precondition of a call in the predicate cannot be proven}} \
                                // expected-note {{precondition of a call in the predicate is always false}}
int chk(int) { return 1; }
namespace dd {
int dchk(int v) pre(v != 0);    // expected-note 2 {{the predicate of the contract assertion}}
int dchk(int) { return 1; }
}

// Provable: B1::f's precondition makes x, hence r, nonzero.
struct B1 {
  virtual int f(int x) pre(x > 0)
    post(r: chk(r) == 1);       // noshadow-note {{the predicate of the contract assertion}} \
                                // rem-remark {{override shadow: nested: proven, conditional}}
};
struct D1 : B1 {
  int f(int x) override         // noshadow-error {{contract assertion cannot be proven for a call through 'B1::f'}} \
                                // rem-remark {{override shadow: proven, conditional; with those nested in it: proven, conditional}}
  { return x; }
};

// Unprovable: nothing is known of r.
struct B2 {
  virtual int f(int x)
    post(r: chk(r) == 1);       // expected-note {{the predicate of the contract assertion}} \
                                // rem-remark {{override shadow: nested: not proven}}
};
struct D2 : B2 {
  int f(int x) override         // expected-error {{contract assertion cannot be proven for a call through 'B2::f'}} \
                                // rem-remark {{override shadow: proven, sound; with those nested in it: not proven}}
  { return x; }
};

// Always false: chk(0).
struct B3 {
  virtual int f(int x)
    post(r: chk(0) == 1);       // expected-note {{the predicate of the contract assertion}}
};
struct D3 : B3 {
  int f(int x) override         // expected-error {{contract assertion cannot be proven for a call through 'B3::f'}} \
                                // rem-remark {{override shadow: proven, sound; with those nested in it: not proven}}
  { return x; }
};

// Diagnose: dd::dchk's precondition, unprovable, warned at its call in
// B4::f's postcondition, naming the pair; the outer is not affected
// (beyond being conditional).
struct B4 {
  virtual int f(int x)
    post(r: dd::dchk(r) == 1);  // expected-warning {{contract assertion cannot be proven for a call through 'B4::f'}} \
                                // o0-note {{the predicate of the contract assertion}} \
                                // rem-remark {{override shadow: nested: not proven}}
};
struct D4 : B4 {
  int f(int x) override         // o0-error {{contract assertion cannot be proven for a call through 'B4::f'}} \
                                // rem-remark {{override shadow: proven, sound; with those nested in it: proven, conditional}}
  { return x; }
};

// D5::f's own precondition, whose nested one is provable only through
// B5::f's precondition.
struct B5 { virtual int f(int x) pre(x > 0); };
struct D5 : B5 {
  int f(int x) override         // noshadow-error {{contract assertion cannot be proven for a call through 'B5::f'}} \
                                // rem-remark {{override shadow: proven, conditional; with those nested in it: proven, conditional}}
    pre(chk(x) == 1)            // noshadow-note {{the predicate of the contract assertion}} \
                                // rem-remark {{override shadow: nested: proven, conditional}}
  { return x; }
};

// D6::f's own postcondition, unprovable in the override shadow and in
// D6::f's body.
struct B6 { virtual int f(int x); };
struct D6 : B6 {
  int f(int x) override         // o2-error {{contract assertion cannot be proven for a call through 'B6::f'}} \
                                // rem-remark {{override shadow: proven, sound; with those nested in it: not proven}}
    post(r: chk(r - 1) == 1)    // expected-error {{contract assertion cannot be proven}} \
                                // o2-note {{the predicate of the contract assertion}} \
                                // rem-remark {{override shadow: nested: not proven}} \
                                // rem-remark {{proof shadow: nested: not proven}} \
                                // rem-remark {{proof shadow: proven, sound; with those nested in it: not proven}}
  { return x; }
};

// D7::f's own precondition with a nested diagnose one, not provable under
// B7::f's precondition either.
struct B7 { virtual int f(int x) pre(x > 0); };
struct D7 : B7 {
  int f(int x) override         // noshadow-error {{contract assertion cannot be proven for a call through 'B7::f'}} \
                                // rem-remark {{override shadow: proven, conditional; with those nested in it: proven, conditional}}
    pre(dd::dchk(x - 1) == 1)   // expected-warning {{contract assertion cannot be proven for a call through 'B7::f'}} \
                                // noshadow-note {{the predicate of the contract assertion}} \
                                // rem-remark {{override shadow: nested: not proven}}
  { return x; }
};

void *make(int i) {
  switch (i) {
  case 1: return new D1;
  case 2: return new D2;
  case 3: return new D3;
  case 4: return new D4;
  case 5: return new D5;
  case 6: return new D6;
  default: return new D7;
  }
}
