// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3098: an exception during capture initialization becomes a post_capture /
// evaluation_exception violation; under observe the handler runs, the predicate
// is skipped, and execution continues.  (GCC mirror: p3098-except-init.C)
//
// Clang, fixed: the capture-init exception used to propagate and
// terminate instead of being converted to a violation.  See wg21
// testing-gap-catalogue.md section 10.

#include <contracts>
#include <cstdio>

static int handler_count = 0;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  ++handler_count;
  if (v.kind() != std::contracts::assertion_kind::post_capture) __builtin_abort();
}

struct Bad {
  Bad(int) { throw 42; }
  Bad(const Bad&) { throw 42; }
  ~Bad() { }
};

int f(int i) post [b = Bad(1)] (true) { return i; }

int main() {
  int result = f(10);
  if (handler_count != 1) __builtin_abort();
  if (result != 10) __builtin_abort();
  std::printf("PASS\n");
}
