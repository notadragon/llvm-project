// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// P3097: a violation in a virtual call reports the location of the
// assertion that failed.  An interface precondition (checked in the wrapper)
// reports the statically chosen function's declaration, an override's own
// precondition the override's, and an interface postcondition the
// interface's -- not the wrapper, the call site or the other function.  The
// interface's assertions are checked first on entry and last on exit.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3097-violation-location.C

#include <contracts>
#include <cstdlib>
#include <cstring>

struct record { unsigned line; const char *fn; };
static record seen[8];
static int n;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (n < 8) {
    seen[n].line = v.location().line();
    seen[n].fn = v.location().function_name();
  }
  ++n;
}

constexpr unsigned b_pre = __LINE__ + 3;
constexpr unsigned b_post = __LINE__ + 3;
struct B {
  virtual int k(int x) pre(x > 0)
                       post(r : r > 0)
  { return x; }
  virtual ~B() {}
};
constexpr unsigned d_pre = __LINE__ + 2;
struct D : B {
  int k(int x) override pre(x > 1) { return x; }
};

static bool is(int i, unsigned line, const char *fn) {
  return seen[i].line == line && std::strstr(seen[i].fn, fn);
}

int main() {
  D d;
  B *p = &d;
  p->k(0);
  if (n != 3 || !is(0, b_pre, "B::k") || !is(1, d_pre, "D::k") ||
      !is(2, b_post, "B::k"))
    std::abort();
  n = 0;
  p->k(1);
  if (n != 1 || !is(0, d_pre, "D::k"))
    std::abort();
}
