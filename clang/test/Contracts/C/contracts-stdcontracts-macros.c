// The <stdcontracts.h> convenience header maps the lowercase spellings
// pre / post / contract_assert to the _Pre / _Post / _ContractAssert keywords.
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t && %t
// (GCC mirror: gcc.dg/contracts/contracts-stdcontracts-macros.c.)

#include <stdcontracts.h>

static int handler_called = 0;

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  handler_called++;
}

int f(int x) pre(x > 0) post(r: r >= 0) {
  contract_assert(x != 0);
  return x;
}

int main(void) {
  // pre(x > 0): violated (x == -1) -> 1 call.
  // contract_assert(x != 0): holds.
  // post(r: r >= 0): violated (r == -1) -> 1 call.
  f(-1);
  if (handler_called != 2)
    __builtin_abort();
  return 0;
}
