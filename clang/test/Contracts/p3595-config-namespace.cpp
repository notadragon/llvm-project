// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-configuration-file=%S/p3595-config-namespace.json %libcxx_flags -o %t && %t

// P3595: test namespace prefix matching in JSON config.
// Config: mylib::internal -> ignore, mylib -> observe, caller -> ignore,
// catch-all -> observe.

#include <contracts>
#include <cstdio>
#include <cstdlib>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

namespace mylib {
  void f(int x) pre(x > 0) { }

  namespace internal {
    void g(int x) pre(x > 0) { }
  }
}

void h(int x) pre(x > 0) { }

int main() {
  // mylib::internal -> ignore: no handler call
  mylib::internal::g(-1);
  if (violations != 0) { printf("FAIL: expected 0, got %d\n", violations); abort(); }

  // mylib -> observe: handler called, continues
  mylib::f(-1);
  if (violations != 1) { printf("FAIL: expected 1, got %d\n", violations); abort(); }

  // global -> observe (catch-all): handler called, continues
  h(-1);
  if (violations != 2) { printf("FAIL: expected 2, got %d\n", violations); abort(); }

  printf("PASS\n");
}
