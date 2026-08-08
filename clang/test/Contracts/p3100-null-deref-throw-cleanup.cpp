// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-null-deref-observe.json %libcxx_flags -o %t && %t

// P3100: when a throwing null-dereference handler unwinds, the destructors of
// in-scope automatic variables must run -- the handler call is a proper
// unwinding call (an invoke with a landing pad).  observe semantic.

#include <contracts>
#include <cstdlib>

struct E {};
struct S { static int dtors; ~S() { ++dtors; } };
int S::dtors = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  throw E{};
}

int load_it(int *p) { return *p; }

int main() {
  int *p = nullptr;
  bool caught = false;
  try {
    S s;
    int i = load_it(p);   // observe handler throws -> unwind must destroy s
    (void)i;
  } catch (E&) {
    caught = true;
  }
  if (!caught) std::abort();
  if (S::dtors != 1) std::abort();
}
