// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o %t.shadow.ll %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fno-contracts-prove-shadow -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o %t.noshadow.ll %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o %t.ignore.ll %s
// RUN: diff %t.shadow.ll %t.noshadow.ll
// RUN: diff %t.shadow.ll %t.ignore.ll
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-obj -o %t.shadow.o %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -triple x86_64-unknown-linux-gnu -O2 -emit-obj -o %t.ignore.o %s
// RUN: cmp %t.shadow.o %t.ignore.o
// REQUIRES: x86-registered-target

// P4405 stage 3: the proof shadows change nothing in the output.  Without
// -fcontracts-prove-assume, a module whose evaluations are decided in proof
// shadows (some proven, some not, a predicate-only function, a static
// helper only a caller-side precondition's predicate and the real code
// share, a constructor's two variants) is optimized and emitted exactly as
// without the shadows, and exactly as with every evaluation ignore: the
// shadows are optimized in a module of their own, so the real functions'
// inlining and propagation see only the real code.  (Clang's own test.)

inline bool in_range(int i, int n) { return i >= 0 && i < n; }
static int helper(int v) { return v * 3 + 1; }

int lookup(const int *a, int i, int n)
  pre(in_range(i, n))
{
  contract_assert(i < n);
  return a[i];
}

static int scaled(int v)
  pre(helper(v) > 0)
{
  return helper(v) * 2;
}

int caller(int v) {
  if (v > 0)
    return scaled(v);
  return 0;
}

struct B { int b; B(int x) : b(x) {} };
struct D : virtual B {
  D(int x) : B(x) { contract_assert(x != 0); }
};
D make(int x) { return D(x); }

int loop(const char *s) {
  int n = 0;
  for (int i = 0; s[i]; ++i) {
    contract_assert(i >= 0);
    ++n;
  }
  return n;
}
