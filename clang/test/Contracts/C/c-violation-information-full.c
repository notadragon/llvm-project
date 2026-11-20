// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe \
// RUN:   -fcontract-violation-information=full %libcxx_flags %s -o %t
// RUN: %t
// REQUIRES: contracts-libcxx, native

// -fcontract-violation-information=full, the default, given explicitly: a
// C contract violation carries the file name, function name, line and
// comment, as do the P3290 convenience macros of <contracts.h>, and the
// compiler does not predefine __clang_contracts_violation_information_none.
// c-violation-information-none.c is the same program under =none.  (Clang
// names the function by its signature, GCC by its identifier, so the
// function name is matched as a substring.)
//
// Mirror, in the gnu_gcc fork:
// gcc/testsuite/gcc.dg/contracts/c-violation-information-full.c.

#ifdef __clang_contracts_violation_information_none
#error "__clang_contracts_violation_information_none is defined"
#endif

#include <contracts.h>
#include <string.h>

static int count;
static int expected_kind;
static const char *expected_function;
static unsigned expected_line;
static const char *expected_comment;

void handle_contract_violation(const contract_violation_t *cv) {
  if (strcmp(stdc_contract_violation_file(cv), __FILE__) != 0 ||
      !strstr(stdc_contract_violation_function(cv), expected_function) ||
      stdc_contract_violation_line(cv) != expected_line ||
      strcmp(stdc_contract_violation_comment(cv), expected_comment) != 0 ||
      stdc_contract_violation_kind(cv) != expected_kind ||
      stdc_contract_violation_semantic(cv) != STDC_CONTRACT_OBSERVE)
    __builtin_abort();
  ++count;
}

int pre_f(int x) _Pre(x > 0) { return x; }           // line 41
int post_f(int x) _Post(r : r > 0) { return x; }     // line 42
void assert_f(int x) { _ContractAssert(x > 0); }     // line 43

static void expect(int kind, const char *function, unsigned line,
                   const char *comment) {
  expected_kind = kind;
  expected_function = function;
  expected_line = line;
  expected_comment = comment;
}

int main(void) {
  expect(STDC_CONTRACT_PRE, "pre_f", 41, "x > 0");
  pre_f(0);
  // Clang's comment for a postcondition names its result ("r : r > 0").
  expect(STDC_CONTRACT_POST, "post_f", 42, "r : r > 0");
  post_f(0);
  expect(STDC_CONTRACT_ASSERT, "assert_f", 43, "x > 0");
  assert_f(0);
  expect(STDC_CONTRACT_MANUAL, "main", __LINE__ + 1, "manual comment");
  stdc_handle_observed_contract_violation("manual comment");
  if (count != 4)
    __builtin_abort();
  return 0;
}
