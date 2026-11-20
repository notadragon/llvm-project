// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// P3097 x templates: virtual functions with contract assertions in a CRTP
// base.  The base's interface predicate calls into the derived class through
// the CRTP downcast, and each instantiation's own derived class answers it;
// a virtual call through the base checks that interface predicate around the
// final overrider's own, for each of two derived classes.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3097-crtp.C

#include <contracts>
#include <cstdlib>
#include <cstring>

static int violations;
static const char *where;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++violations;
  where = v.location().function_name();
}

template <class D> struct Base {
  const D &self() const { return static_cast<const D &>(*this); }
  virtual int f(int x) const pre(self().accepts(x))
      post(r : self().accepts(r)) { return x; }
  virtual ~Base() {}
};

struct Pos : Base<Pos> {
  bool accepts(int x) const { return x > 0; }
  int f(int x) const override pre(x < 100) { return x - 1; }
};

struct Neg : Base<Neg> {
  bool accepts(int x) const { return x < 0; }
  int f(int x) const override { return x; }
};

// Clang spells the function "int Base<Pos>::f(int) const".
static bool in_base(const char *fn, const char *d) {
  char want[32] = "Base<";
  std::strcat(want, d);
  std::strcat(want, ">::f");
  return std::strstr(fn, want);
}

template <class D> static int call(const Base<D> &b, int x) {
  violations = 0;
  where = "";
  b.f(x);
  return violations;
}

int main() {
  Pos p;
  Neg n;
  if (call(p, 5) != 0)
    std::abort();
  // Interface pre fails (Pos accepts positives); post 4 > 0 holds.
  if (call(p, 0) != 2 || !in_base(where, "Pos")) // pre and post
    std::abort();
  if (call(p, 1) != 1 || !in_base(where, "Pos")) // post 0
    std::abort();
  if (call(p, 200) != 1 || !std::strstr(where, "Pos::f")) // own pre
    std::abort();
  if (call(n, -5) != 0)
    std::abort();
  if (call(n, 5) != 2 || !in_base(where, "Neg"))
    std::abort();
}
