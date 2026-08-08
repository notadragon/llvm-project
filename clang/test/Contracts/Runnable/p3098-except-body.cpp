// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 %libcxx_flags -o %t
// RUN: %t

// When the function body throws, captures are destroyed without
// evaluating postconditions.

#include <cassert>
#include <cstdio>

int ctor_count = 0;
int dtor_count = 0;

struct Tracker {
  int val;
  Tracker(int v) : val(v) { ++ctor_count; }
  Tracker(const Tracker& o) : val(o.val) { ++ctor_count; }
  ~Tracker() { ++dtor_count; }
};

bool postcondition_evaluated = false;

void f(int x)
  post [t = Tracker(x)] ((postcondition_evaluated = true, true))
{
  throw 42;
}

int main() {
  ctor_count = 0;
  dtor_count = 0;
  postcondition_evaluated = false;

  try {
    f(1);
  } catch (int) {
  }

  // Capture was constructed then destroyed, postcondition was NOT evaluated
  assert(ctor_count == 1);
  assert(dtor_count == 1);
  assert(!postcondition_evaluated);

  std::printf("PASS\n");
}
