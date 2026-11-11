// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-group-evaluation-semantic=safety:observe \
// RUN:   -fcontract-group-evaluation-semantic=perf:ignore %libcxx_flags -o %t
// RUN: %t

// P3400: multiple -fcontract-group-evaluation-semantic flags and prefix
// matching on the '.' delimiter.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-group-multi.C)

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

void f_safety(int x) pre<"safety"group>(x > 0) { }
void f_safety_mem(int x) pre<"safety.memory"group>(x > 0) { }
void f_safety_deep(int x) pre<"safety.memory.bounds"group>(x > 0) { }
void f_perf(int x) pre<"perf"group>(x > 0) { }
void f_perf_cache(int x) pre<"perf.cache"group>(x > 0) { }
// "safetyx" does NOT prefix-match "safety" (no '.' after "safety").
void f_safetyx(int x) pre<"safetyx"group>(x > 0) { }

int main() {
  f_safety(-1);
  if (violations != 1) __builtin_abort();
  f_safety_mem(-1);
  if (violations != 2) __builtin_abort();
  f_safety_deep(-1);
  if (violations != 3) __builtin_abort();
  f_perf(-1);       // ignore
  if (violations != 3) __builtin_abort();
  f_perf_cache(-1); // ignore via prefix
  if (violations != 3) __builtin_abort();
  f_safetyx(1);     // default enforce, non-violating
  if (violations != 3) __builtin_abort();
}
