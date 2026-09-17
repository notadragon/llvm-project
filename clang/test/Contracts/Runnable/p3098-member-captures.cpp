// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3098: Postcondition captures on member functions (inline class body).

#include <contracts>
#include <cstdio>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

static int counter = 0;

struct Widget {
  // Inline member with init-capture
  int next() post [old_c = counter] (r: r == old_c + 1) {
    return ++counter;
  }

  // Inline member with parameter capture
  int add(int x) post [x] (r: r >= x) {
    return counter += x;
  }
};

int main() {
  Widget w;

  counter = 0;
  violation_count = 0;
  int r = w.next();  // old_c=0, returns 1, 1==0+1 -> true
  if (violation_count != 0) __builtin_abort();
  if (r != 1) __builtin_abort();

  violation_count = 0;
  r = w.add(5);  // x=5, counter becomes 6, 6>=5 -> true
  if (violation_count != 0) __builtin_abort();
  if (r != 6) __builtin_abort();

  std::printf("PASS\n");
}
