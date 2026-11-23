// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 %libcxx_flags -o %t
// RUN: %t

// P3290: a nullptr comment passed to the API is normalized to "".
// (GCC mirror: p3290-api-nullptr.C)

#include <contracts>
#include <cstdio>
#include <cstring>

static const char* last_comment = "unset";
void handle_contract_violation(const std::contracts::contract_violation& v) {
  last_comment = v.comment();
}

int main() {
  std::contracts::handle_observed_contract_violation(nullptr);
  if (!last_comment || std::strcmp(last_comment, "") != 0) __builtin_abort();
  std::printf("PASS\n");
}
