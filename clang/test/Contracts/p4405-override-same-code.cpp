// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -fcxx-exceptions -fexceptions -mconstructor-aliases -O2 -discard-value-names -emit-llvm -o %t.override.ll %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -fno-contracts-prove-override -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -fcxx-exceptions -fexceptions -mconstructor-aliases -O2 -discard-value-names -emit-llvm -o %t.nooverride.ll %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=ignore -triple x86_64-unknown-linux-gnu -fcxx-exceptions -fexceptions -mconstructor-aliases -O2 -discard-value-names -emit-llvm -o %t.ignore.ll %s
// RUN: diff %t.override.ll %t.ignore.ll
// RUN: diff %t.nooverride.ll %t.ignore.ll
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -fcxx-exceptions -fexceptions -mconstructor-aliases -O2 -emit-obj -o %t.override.o %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -fno-contracts-prove-override -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -fcxx-exceptions -fexceptions -mconstructor-aliases -O2 -emit-obj -o %t.nooverride.o %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=ignore -triple x86_64-unknown-linux-gnu -fcxx-exceptions -fexceptions -mconstructor-aliases -O2 -emit-obj -o %t.ignore.o %s
// RUN: cmp %t.override.o %t.ignore.o
// RUN: cmp %t.nooverride.o %t.ignore.o
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions": override shadows change nothing in the
// output, and neither do the obligations an overrider takes on without
// them (-fno-contracts-prove-override: B::f's postconditions, with the
// snapshots and conversions they read).  A module whose overriders --
// with and without contract assertions of their own (preconditions and
// postconditions), through a primary, a non-primary and a virtual base,
// with a covariant result, a modified parameter, a loop, a throwing body,
// a class returned by the named return value, and destructors,
// user-provided and implicitly defined, of classes with and without
// virtual bases, with a throwing member destructor and a precondition of
// their own -- are analyzed with diagnose (-fcontracts-prove-assume on, as
// -fcontracts-p4405 implies) is emitted exactly as with every analysis
// evaluation ignore.  (Value names are discarded in the IR compared: a
// real function's -fcontracts-prove-assume regions, removed before it is
// optimized, still take the names they used.)  (Clang's own test; gnu_gcc compares its
// object code by hand, p4405-gcc-implementation.md VF1a/VF1b.)

int sink(int);

struct Pad { int pad[4]; virtual void p(); };

struct B {
  int m;
  virtual int f(int x) pre(x > 0) post(r: r > 0);
  virtual B *self() post(r: r == this);
  virtual int get(const int x) pre(x > 0) post(r: r >= x);
  virtual int loop(int n) pre(n >= 0) post(r: r >= 0);
};

struct V { int v; virtual int g(int x) pre(x > v); };

struct D : Pad, B, virtual V {
  int f(int x) override pre(x >= 0) { return x >= 1 ? x : sink(x); }
  D *self() override { return this; }
  int get(int x) override { int r = x; x = sink(x); return r + (x > 0); }
  int loop(int n) override {
    int s = 0;
    for (int i = 0; i < n; ++i)
      s += sink(i) & 1;
    return s;
  }
  int g(int x) override pre(x > v + 1) { return x - v; }
};

struct E : D {
  int f(int x) override pre(x > 5) { return x; }
};

struct R { R(); R(const R &); ~R(); int v; };

struct P {
  virtual int t(int x) pre(x > 0) post(r: r > 0);
  virtual R make(int x) pre(x > 0) post(r: r.v > 0);
  virtual int own(const int x) pre(x > 0) post(r: r > 0);
  virtual ~P() noexcept(false) post(sink(0) == 0);
};

struct M { ~M() noexcept(false); int v; };

struct Q : P {
  int t(int x) override { if (x > 100) throw 1; return sink(x); }
  R make(int x) override { R r; r.v = sink(x); return r; }
  int own(const int x) override post(r: r >= x) { return x + sink(x); }
  ~Q() override pre(sink(1) > 0) { sink(2); }
};

struct Implicit : P { M m; };
struct VDtor : virtual P { int i; };
struct VDtor2 : VDtor {
  ~VDtor2() override { for (int i = 0; i < sink(3); ++i) sink(i); }
};

void kill(P *p) { delete p; }

void use() { D d; E e; Q q; Implicit i; VDtor v; VDtor2 v2; }
