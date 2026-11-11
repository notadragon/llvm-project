// DEFINE: %{run} = %clangxx -std=c++26 %s -fcontracts-p3097 \
// DEFINE:   -fcontract-evaluation-semantic=ignore \
// DEFINE:   -fcontract-configuration-file=%S/p4451-callerside-virtual.json \
// DEFINE:   %libcxx_flags -o %t
//
// RUN: %{run} -fcontracts-p4451 && %t
// RUN: %{run} -fcontracts-p3400 && %t
// RUN: %{run} -fcontracts-p3400 -fcontracts-p4451 && %t

// P3097, notadragon_wg21 DECISIONS.md E180 (5): the checks a virtual call
// makes of the contract assertions of the function it names (the interface
// wrapper) never add ignore to their allowed sets: they use the callee-side
// semantics, so with ignore configured everywhere -- also for caller-side
// checks (p4451-callerside-virtual.json) -- B::f's terminating precondition
// #1 and postcondition #3 are still enforced around D::f, and only the
// ordinary #2 is skipped.  Sequence: 1, D::f's body, 3.  Under
// -fcontracts-p4451 alone, -fcontracts-p3400 alone (the P3400 label) and
// both.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-callerside-virtual.C
// (and its -p3400 and -both variants)

#include <contracts>
#include <cstdlib>
#include <cstring>

static char trace[16];
static int evaluations, target;

static bool check(int id) {
  std::size_t n = std::strlen(trace);
  trace[n] = '0' + id;
  trace[n + 1] = '\0';
  return ++evaluations != target;
}

struct violation {};

void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.semantic() != std::contracts::evaluation_semantic::enforce)
    std::abort();
  std::strcat(trace, "v");
  throw violation{};
}

struct B {
  virtual void f()
    pre<terminating>(check(1))
    pre(check(2))
    post<terminating>(check(3));
};

void B::f() {}

struct D : B {
  void f() override { std::strcat(trace, "|"); }
};

static void run(B &b, int n, const char *seq) {
  trace[0] = '\0';
  evaluations = 0;
  target = n;
  try {
    b.f();
  } catch (violation) {
  }
  if (std::strcmp(trace, seq) != 0)
    std::abort();
}

int main() {
  D d;
  run(d, 0, "1|3");
  run(d, 1, "1v");
  run(d, 2, "1|3v");
}
