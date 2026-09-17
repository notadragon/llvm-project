// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-configuration-file=%S/p3595-config-template.json %libcxx_flags -o %t && %t

// P3595 x templates: a configuration entry matching a function template's
// contract location applies to every instantiation.  The template's
// precondition line is configured to observe while the base semantic is enforce.
// (GCC mirror: p3595-config-template.C)

#include <contracts>
#include <cstdio>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

template <typename T>
T f(T x)
  pre(x > T{})   // configured to observe
{ return x; }

int main() {
  violations = 0;
  (void) f(-1);
  (void) f(-1.0);
  if (violations != 2) __builtin_abort();
  std::printf("PASS\n");
}
