// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t -Xclang -verify
// RUN: not --crash %t | FileCheck %s
// REQUIRES: contracts-libcxx

// D4405, E198 (d): diagnose-then-enforce is the standard diagnose label
// combined with a label whose compute_semantic returns enforce (written
// here, not in <contracts>).  The combination leaves diagnose allowed, so
// pre<diagnose | enforced> warns at compile time that it cannot be proven,
// and its runtime evaluation is enforce whatever the build configures --
// here observe: f(-1) is a violation and the program terminates after the
// handler returns.  (p4405-std-labels-diagnose-enforced-ignore-run.cpp:
// built to ignore; p4405-std-labels.cpp: diagnose | always_enforce and
// diagnose | terminating are not diagnosed.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-std-labels-diagnose-enforced-observe-run.C

#include <contracts>
#include <cstdio>

using std::contracts::evaluation_semantic;
using std::contracts::labels::operator|;

struct enforced_t {
  using assertion_control_object = enforced_t;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return evaluation_semantic::enforce;
  }
};
constexpr enforced_t enforced{};

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("violation: %s, %s\n", v.comment(),
              v.semantic() == evaluation_semantic::enforce ? "enforce"
                                                           : "other");
  std::fflush(stdout);
}

[[gnu::noinline]] void f(int x) pre<diagnose | enforced>(x > 0) {} // expected-note {{the predicate of the contract assertion}}

int main(int argc, char **) {
  f(argc - 2); // expected-warning {{contract assertion cannot be proven}}
  std::puts("after");
}

// CHECK: violation: x > 0, enforce
// CHECK-NOT: after
