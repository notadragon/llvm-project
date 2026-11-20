// RUN: %clangxx -fcontract-evaluation-semantic=observe -std=c++26 %s -fcontracts -fcontracts-p3098 %libcxx_flags -o %t
// RUN: %t

#include <contracts>
#include <cstdio>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

template<typename... Args>
int sum(Args... args)
  post [args...] ((args + ...) > 0)
{ return (args + ...); }

int main() {
  if (sum(1, 2, 3) != 6 || sum(10, 20) != 30 || violations != 0)
    __builtin_abort();
  // Positive control: the folded capture predicate is false, once.
  sum(-1, -2);
  if (violations != 1)
    __builtin_abort();
  std::printf("PASS\n");
}
