// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=ignore %libcxx_flags -o %t
// RUN: %t

// Under ignore semantic: no captures constructed or destroyed.

#include <cstdio>
#include <cassert>

int ctor_count = 0;
int dtor_count = 0;

struct Tracker {
  Tracker(int) { ++ctor_count; }
  Tracker(const Tracker&) { ++ctor_count; }
  ~Tracker() { ++dtor_count; }
};

int f(int x) post [t = Tracker(x)] (true) { return x; }

int main() {
  f(42);
  assert(ctor_count == 0);
  assert(dtor_count == 0);
  std::printf("PASS\n");
}
