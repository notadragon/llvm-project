// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3098: postcondition captures on noexcept functions.
// (GCC mirror: g++.dg/contracts/cpp26/p3098-noexcept.C)

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

int f(int i) noexcept
  post [old_i = i] (r: r > old_i)
{
  return i + 1;
}

int g(int a, int b) noexcept
  post [a, b] (r: r == a + b)
{
  int result = a + b;
  a = 0;
  b = 0;
  return result;
}

void h(int i) noexcept
  post [old_i = i] (old_i > 0)
{
}

// Positive control: the postcondition fails, once.
int bad(int i) noexcept
  post [old_i = i] (r: r > old_i)
{
  return i;
}

int main() {
  if (f(10) != 11) __builtin_abort();
  if (g(3, 4) != 7) __builtin_abort();
  h(5);
  if (violations != 0) __builtin_abort();
  bad(1);
  if (violations != 1) __builtin_abort();
}
