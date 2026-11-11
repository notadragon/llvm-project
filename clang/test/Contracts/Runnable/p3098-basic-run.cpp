// RUN: %clangxx -fcontract-evaluation-semantic=enforce -std=c++26 %s -fcontracts -fcontracts-p3098 %libcxx_flags -o %t
// RUN: %t

#include <cstdio>

int side_effect_counter = 0;

int get_and_inc() { return side_effect_counter++; }

// Init-capture from expression -- capture holds call-time value
int f1(int x) post [old = x] (old >= 0) { return x + 1; }

// By-copy parameter capture
int f2(int x) post [x] (x >= 0) { return x + 1; }

// Capture evaluates at call time, not return time
int f3(int x) post [old = get_and_inc()] (old == 0) {
  get_and_inc(); // side_effect_counter is now 2
  return x;
}

// Multiple captures
int f4(int a, int b) post [a, b] (a + b > 0) { return a + b; }

// Init-capture from literal
int f5() post [x = 42] (x == 42) { return 100; }

// Capture does not constify -- verified at sema level (not runtime testable)
// int f6(int x) post [old = x] (old++ >= 0) { return x; }

int main() {
  if (!(f1(5) == 6)) __builtin_abort();
  if (!(f2(10) == 11)) __builtin_abort();

  side_effect_counter = 0;
  f3(0);
  if (!(side_effect_counter == 2)) __builtin_abort();

  if (!(f4(3, 4) == 7)) __builtin_abort();
  if (!(f5() == 100)) __builtin_abort();
  std::printf("PASS\n");
}
