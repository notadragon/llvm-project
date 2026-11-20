// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3098: an exception during predicate evaluation triggers the violation handler
// (evaluation_exception) and the capture is still destroyed; under observe
// execution continues.  (GCC mirror: p3098-except-predicate.C)

#include <contracts>
#include <cstdio>

static int handler_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++handler_count;
}

static int destruct_count = 0;
struct Tracker {
  int id;
  Tracker(int i) : id(i) {}
  Tracker(const Tracker& o) : id(o.id) {}
  ~Tracker() { ++destruct_count; }
};

bool throwing_predicate(int) { throw 99; }

int f(int i) post [t = Tracker(1)] (r: throwing_predicate(r)) { return i; }

int main() {
  destruct_count = 0;
  handler_count = 0;
  int result = f(10);
  if (handler_count != 1) __builtin_abort();
  if (destruct_count != 1) __builtin_abort();  // capture destroyed after throw
  if (result != 10) __builtin_abort();
  std::printf("PASS\n");
}
