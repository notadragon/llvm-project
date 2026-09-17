// Test that the violation comment contains the predicate text.
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t && %t

#include <contracts.h>
#include <string.h>

static int handler_called = 0;

void handle_contract_violation(const contract_violation_t *cv) {
  const char *comment = stdc_contract_violation_comment(cv);
  if (!comment)
    __builtin_abort();
  if (!strstr(comment, "x > 0"))
    __builtin_abort();
  handler_called++;
}

int guarded(int x) _Pre(x > 0) {
  return x;
}

int main(void) {
  guarded(-1);
  if (handler_called != 1)
    __builtin_abort();

  return 0;
}
