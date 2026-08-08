// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 %libcxx_flags -o %t
// RUN: %t

// Verify lexical ordering: preconditions and capture inits are
// interleaved in declaration order.

#include <cassert>
#include <cstdio>

int order_idx = 0;
int order[10] = {};

void record(int id) { order[order_idx++] = id; }

int f(int x)
  pre ((record(1), true))
  post [a = (record(2), x)] (a >= 0)
  pre ((record(3), true))
  post [b = (record(4), x)] (b >= 0)
{
  return x;
}

int main() {
  order_idx = 0;
  f(1);

  // Lexical order: pre, capture-init, pre, capture-init
  assert(order[0] == 1);
  assert(order[1] == 2);
  assert(order[2] == 3);
  assert(order[3] == 4);

  std::printf("PASS\n");
}
