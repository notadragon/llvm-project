// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-null-deref-observe.json %libcxx_flags -o %t && %t

// P3100: null dereference with a THROWING handler where the dereference sits in
// a function-try-block's try body -- the site is inside the try scope, so the
// function's own handler catches the throw (no special handling needed).

#include <contracts>
#include <cstdlib>

struct E {};
static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++calls;
  throw E{};
}

int load_it(int *p) try { return *p; } catch (E&) { return -1; }

int main() {
  int *p = nullptr;
  int valid = 42;
  if (load_it(p) != -1) std::abort();     // caught by own handler
  if (calls != 1) std::abort();
  if (load_it(&valid) != 42) std::abort(); // non-null: no violation
  if (calls != 1) std::abort();
}
