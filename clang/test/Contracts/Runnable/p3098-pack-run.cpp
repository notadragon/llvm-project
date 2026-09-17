// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 %libcxx_flags -o %t
// RUN: %t

#include <cassert>
#include <cstdio>

template<typename... Args>
int sum(Args... args)
  post [args...] ((args + ...) > 0)
{ return (args + ...); }

int main() {
  assert(sum(1, 2, 3) == 6);
  assert(sum(10, 20) == 30);
  std::printf("PASS\n");
}
