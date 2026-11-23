// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097: Interface postconditions checked after virtual dispatch.

#include <contracts>
#include <cstring>
#include <cstdio>

static int log_idx = 0;
static const char* log_comments[10];

void handle_contract_violation(const std::contracts::contract_violation& v) {
  if (log_idx < 10)
    log_comments[log_idx++] = v.comment();
}

struct Base {
  virtual int f(int x)
    pre(x > 0)
    post(r: r > 100)
  { return x; }
};

struct Derived : Base {
  int f(int x) override
  { return x; }  // no impl contracts
};

int main() {
  Derived d;
  Base& b = d;

  // x=5: interface pre passes (5>0).
  // Derived returns 5, interface post (r>100) fails.
  log_idx = 0;
  b.f(5);
  if (log_idx != 1) __builtin_abort();
  if (std::strcmp(log_comments[0], "r: r > 100") != 0) __builtin_abort();

  // x=-50: interface pre (x>0) fails, body runs, interface post (r>100) fails.
  log_idx = 0;
  b.f(-50);
  if (log_idx != 2) __builtin_abort();
  if (std::strcmp(log_comments[0], "x > 0") != 0) __builtin_abort();
  if (std::strcmp(log_comments[1], "r: r > 100") != 0) __builtin_abort();

  std::printf("PASS\n");
}
