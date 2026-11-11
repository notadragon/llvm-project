// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3099 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3099 x templates: a user message on a contract in a function template is
// carried to the handler for each instantiation.  (GCC mirror:
// p3099-message-template.C)

#include <contracts>
#include <cstdio>
#include <cstring>

static const char* last_message = nullptr;
static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  ++violation_count;
  last_message = v.message();
}

template <typename T>
T f(T x) pre(x > T{}, "must be positive") { return x; }

int main() {
  last_message = nullptr;
  violation_count = 0;
  (void) f(-1);
  if (violation_count != 1) __builtin_abort();
  if (!last_message || std::strcmp(last_message, "must be positive") != 0)
    __builtin_abort();

  last_message = nullptr;
  (void) f(-1.0);
  if (violation_count != 2) __builtin_abort();
  if (!last_message || std::strcmp(last_message, "must be positive") != 0)
    __builtin_abort();

  std::printf("PASS\n");
}
