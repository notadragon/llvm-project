// Test _Post with quick_enforce semantic: the program is terminated without
// calling the handler.
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=quick_enforce %libcxx_flags %s -o %t && not --crash %t
// (GCC mirror: gcc.dg/contracts/contracts-post-quick-enforce.c.)

#include <contracts.h>
#include <stdlib.h>

// quick_enforce never calls the handler: a call exits normally, which the
// RUN line's "not --crash" reports as a failure.
void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  _Exit(0);
}

int positive(int x) _Post(r: r > 0) {
  return x;
}

int main(void) {
  positive(-1);
  return 0;
}

// REQUIRES: contracts-libcxx, native
