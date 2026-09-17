// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 %libcxx_flags -o %t
// RUN: %t

#include <cassert>
#include <cstdio>

int side_effect_counter = 0;

int get_and_inc() { return side_effect_counter++; }

// Init-capture from expression — capture holds call-time value
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

// Capture does not constify — verified at sema level (not runtime testable with ++)
// int f6(int x) post [old = x] (old++ >= 0) { return x; }

int main() {
  assert(f1(5) == 6);
  assert(f2(10) == 11);

  side_effect_counter = 0;
  f3(0);
  assert(side_effect_counter == 2);

  assert(f4(3, 4) == 7);
  assert(f5() == 100);
  std::printf("PASS\n");
}
