// Exercise the contract_violation accessor return values that were previously
// unverified in-tree: semantic, detection_mode, is_terminating, function, line.
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t && %t
// (GCC mirror: gcc.dg/contracts/contracts-accessors.c.)

#include <contracts.h>

static int handler_called = 0;
static int a_semantic = 0;
static int a_detection = 0;
static int a_terminating = -1;
static const char *a_function = 0;
static unsigned a_line = 0;

void handle_contract_violation(const contract_violation_t *cv) {
  handler_called++;
  a_semantic = stdc_contract_violation_semantic(cv);
  a_detection = stdc_contract_violation_detection_mode(cv);
  a_terminating = stdc_contract_violation_is_terminating(cv);
  a_function = stdc_contract_violation_function(cv);
  a_line = stdc_contract_violation_line(cv);
}

int checkme(int x) _Pre(x > 0) { return x; }

int main(void) {
  checkme(-1);
  if (handler_called != 1)
    __builtin_abort();
  if (a_semantic != STDC_CONTRACT_OBSERVE)
    __builtin_abort();
  if (a_detection != STDC_CONTRACT_PREDICATE_FALSE)
    __builtin_abort();
  if (a_terminating != 0) // observe is not a terminating semantic
    __builtin_abort();
  // KNOWN DIVERGENCE: Clang's function() accessor returns the full signature
  // ("int checkme(int)"); GCC returns the bare identifier ("checkme").  Check
  // the name is present as a substring so the test is portable to Clang's form.
  if (a_function == 0 || __builtin_strstr(a_function, "checkme") == 0)
    __builtin_abort();
  if (a_line == 0)
    __builtin_abort();
  return 0;
}
