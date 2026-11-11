// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe -fcontracts-p3100 -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero:observe -ffile-prefix-map==FILE-PREFIX %libcxx_flags -o %t && %t
// REQUIRES: contracts-libcxx, native

// -ffile-prefix-map remaps the file name a contract violation reports through
// contract_violation::location(), as it does __FILE__ and the file name of a
// std::source_location: for a precondition, a postcondition, a
// contract_assert and an implicit (P3100) contract assertion.
// contract-location-macro-prefix-map.cpp does the same with
// -fmacro-prefix-map.
//
// Mirror, in the gnu_gcc fork:
// gcc/testsuite/g++.dg/contracts/cpp26/contract-location-file-prefix-map.C.

#include <contracts>
#include <cstring>

static int count;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  const char *file = v.location().file_name();
  if (std::strncmp(file, "FILE-PREFIX", 11) != 0 ||
      std::strcmp(file, __FILE__) != 0)
    __builtin_abort();
  ++count;
}

int pre_f(int x) pre(x > 0) { return x; }
int post_f(int x) post(r : r > 0) { return x; }
void assert_f(int x) { contract_assert(x > 0); }
int div_f(int a, int b) { return a / b; }

int main() {
  pre_f(0);
  post_f(0);
  assert_f(0);
  div_f(1, 0);
  if (count != 4)
    __builtin_abort();
}
