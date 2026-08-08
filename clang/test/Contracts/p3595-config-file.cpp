// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=ignore -fcontract-configuration-file=%S/p3595-config-file.json %libcxx_flags -o %t && %t

// P3595: test -fcontract-configuration-file= with a JSON file.
// Config file sets:
//   1. group "safety", callee-side -> observe
//   2. caller-side -> ignore
//   3. everything else -> observe
// The catch-all -fcontract-evaluation-semantic=ignore goes last.

#include <contracts>
#include <cstdio>
#include <cstdlib>

static int violations = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

// "safety"group -> observe (handler called, continues)
void f_safety(int x) pre<"safety"group>(x > 0) { }

// No group -> observe (from catch-all in config file)
void f_plain(int x) pre(x > 0) { }

int main() {
  f_safety(-1);
  if (violations != 1) { printf("FAIL: expected 1, got %d\n", violations); abort(); }

  f_plain(-1);
  if (violations != 2) { printf("FAIL: expected 2, got %d\n", violations); abort(); }

  f_safety(1);
  f_plain(1);
  if (violations != 2) { printf("FAIL: expected 2, got %d\n", violations); abort(); }

  printf("PASS\n");
}
