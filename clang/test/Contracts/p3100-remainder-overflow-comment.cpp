// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-remainder-overflow-comment.json %libcxx_flags -o %t && %t

// The P3100 checks on `%' name the operation consistently: remainder by zero
// and INT_MIN % -1 both say "remainder".
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-remainder-overflow-comment.C

#include <contracts>
#include <cstring>

static const char *last = "";
void handle_contract_violation(const std::contracts::contract_violation &v) {
  last = v.comment();
}

[[gnu::noinline]] int remi(int a, int b) { return a % b; }

int main() {
  volatile int zero = 0;
  remi(1, zero);
  bool zero_says_rem = std::strstr(last, "remainder") != nullptr;
  last = "";
  remi(-__INT_MAX__ - 1, -1);
  bool ovf_says_rem = std::strstr(last, "remainder") != nullptr;
  if (*last == '\0' || zero_says_rem != ovf_says_rem)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
