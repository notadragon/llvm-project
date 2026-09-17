// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: %t

// P3098: postcondition captures on noexcept functions.
// (GCC mirror: g++.dg/contracts/cpp26/p3098-noexcept.C)

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

int main() {
  if (f(10) != 11) __builtin_abort();
  if (g(3, 4) != 7) __builtin_abort();
  h(5);
}
