// A contract on a non-defining declaration (a prototype, as in a header) must
// not leak into an *unrelated* contract-free definition (D4299).  Regression:
// Clang previously asserted (assert(false) in ParseDeclGroup) on any contract
// attached to a prototype; now the prototype's contract is parsed and applies
// only to its own function.
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t && %t
// (GCC mirror: gcc.dg/contracts/contracts-proto-leak.c.)
//
// KNOWN DIVERGENCE: GCC (P4299 N-1) does not apply a prototype's contract to the
// definition at all, so its test expects foo(-1) to be silent.  Clang applies a
// contract declared on a prior declaration to the definition (per P2900
// cross-declaration semantics), so foo(-1) here DOES fire.  The shared,
// essential guarantee is that the contract does not leak into the unrelated bar.

#include <contracts.h>

static int handler_called = 0;

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  handler_called++;
}

// Prototype carrying a precondition.
int foo(int x) _Pre(x > 0);

// A contract-free definition that must NOT inherit foo's precondition.
int bar(int x) { return x; }

// The real definition of foo, without its own contract specifier.
int foo(int x) { return x; }

int main(void) {
  // No leak into the unrelated bar: a "bad" argument triggers nothing.
  bar(-1);
  if (handler_called != 0)
    __builtin_abort();

  // Clang applies foo's prototype precondition to its definition -> fires once.
  foo(-1);
  if (handler_called != 1)
    __builtin_abort();

  return 0;
}
