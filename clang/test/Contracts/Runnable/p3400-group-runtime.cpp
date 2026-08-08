// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=enforce -fcontracts-group-evaluation-semantic=safety:observe && %t 2>&1 | FileCheck %s

// Runtime test: group-based semantic selection via P3595 config.
// Default semantic is enforce; "safety"group overridden to observe.
// An observe-semantic violation in the "safety"group should call
// the handler and continue, while non-grouped contracts enforce.

#include <contracts>
#include <cstdio>
#include <cstdlib>

using namespace std::contracts;
using namespace std::contracts::labels;

int observe_count = 0;
int enforce_count = 0;

void handle_contract_violation(const contract_violation& v) {
  if (v.semantic() == evaluation_semantic::observe) {
    ++observe_count;
    std::fprintf(stderr, "observe: %s\n", v.comment());
  } else if (v.semantic() == evaluation_semantic::enforce) {
    ++enforce_count;
    std::fprintf(stderr, "enforce: %s (will terminate)\n", v.comment());
  }
}

// In group "safety" → observe semantic → violation continues execution
// CHECK: observe:
void checked(int x) pre<"safety"group>(x > 0) {}

int main() {
  checked(-1);
  // CHECK: observe_count=1
  std::fprintf(stderr, "observe_count=%d\n", observe_count);

  // Non-grouped contract with default (enforce) semantic: passes
  auto plain = [](int x) { contract_assert(x > 0); };
  plain(42);

  // CHECK: done
  std::fprintf(stderr, "done\n");
  return 0;
}
