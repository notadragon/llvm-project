// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -fcontract-evaluation-semantic=observe -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -fcontract-evaluation-semantic=observe -triple x86_64-unknown-linux-gnu -O2 -mconstructor-aliases -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions", DECISIONS.md E152: a virtual destructor
// overrides the virtual destructors of its bases, and gets override shadows
// like any other overrider -- an implicitly defined one too.  The body
// analyzed is what the destructor runs: its own statements, then the
// destruction of its members and bases, so the overridden destructor's
// postconditions are proven after it has run.  One analysis per source
// destructor: the deleting destructor only calls the complete one, which,
// in a class without virtual bases, calls the base-object one (or is an
// alias of it), which is analyzed; in a class with virtual bases the
// complete-object destructor is, which is what a delete through a base
// class runs (the base-object destructor does not destroy the virtual
// base), and its override shadow runs the whole destructor.  With
// -mconstructor-aliases, a base-object destructor that only calls its base's
// is not made an alias of it when it overrides a destructor with contract
// assertions, so that it is analyzed all the same.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-dtor.C
// (gnu_gcc counts its override shadows and their variants in its dumps;
// here each obligation's remark, and V1, V2 and W are proven only if their
// complete-object destructors are what is analyzed.  The runtime
// evaluations are observe, so that no check makes the postcondition that
// follows it true; Clang's shadows take an observed evaluation as an
// assumption that is not guaranteed, which makes a proof relying on code
// holding one conditional -- B3::~B3's own, and V1's, V2's and W's, which
// run it -- as does relying on G::~G's code, which another translation
// unit's copy may refine, in H::~H's)

int g;

struct B {
  virtual ~B()
    post(g == 1);               // expected-error {{contract assertion cannot be proven}} expected-remark {{proof shadow: not proven}} expected-note 2 {{the predicate of the contract assertion}}
};

B::~B() {}

// User-provided: proven from its own body.
struct D : B {
  ~D() override { g = 1; }      // expected-remark {{override shadow: proven, sound}}
};

// User-provided: unproven.
struct E : B {
  ~E() override {}              // expected-error {{cannot be proven for a call through 'B::~B'}} expected-remark {{override shadow: not proven}}
};

// Implicitly defined: unproven, diagnosed at the class.
struct F : B {                  // expected-error {{cannot be proven for a call through 'B::~B'}} expected-remark {{override shadow: not proven}}
  int y;
};

// Implicitly defined: proven by a member's destructor, which runs before
// B::~B.
struct Setter { ~Setter() { g = 1; } };
struct G : B {                  // expected-remark {{override shadow: proven, sound}}
  Setter s;
};

// An indirect base: H::~H overrides B::~B through G::~G.
struct H : G {                  // expected-remark {{override shadow: proven, conditional}}
};

// A destructor's own precondition, settled by folding (B::~B has none:
// DECISIONS.md E156); its observed runtime evaluation is an assumption that
// is not guaranteed.
struct P : B {
  ~P() override pre(true) { g = 1; }  // expected-remark {{override shadow: proven, conditional}}
};

// A virtual base: the complete-object destructor runs B3::~B3, which
// establishes its postcondition itself; the base-object destructor does
// not, and is never the target of a delete through B3.
int g3;
struct B3 {
  virtual ~B3()
    post(g3 == 1)               // expected-remark {{proof shadow: proven, conditional}}
  { g3 = 1; }
};
struct V1 : virtual B3 {        // expected-remark {{override shadow: proven, conditional}}
};
struct V2 : virtual B3 {
  ~V2() override { g3 = 2; }    // expected-remark {{override shadow: proven, conditional}}
};
struct W : V1, V2 {             // expected-remark {{override shadow: proven, conditional}}
};

void kill(B *p) { delete p; }
void kill3(B3 *p) { delete p; }
void use() { D d; E e; F f; G gg; H h; P p; V1 v1; V2 v2; W w; }
