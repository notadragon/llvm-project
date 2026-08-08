// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-null-deref-observe.json %libcxx_flags -o %t && not --crash %t

// P3100: null dereference with a THROWING handler in a NOEXCEPT function --
// under observe the handler throws, the exception reaches the noexcept boundary
// and calls std::terminate.

#include <contracts>

struct E {};
void handle_contract_violation(const std::contracts::contract_violation&) {
  throw E{};
}

int load_it(int *p) noexcept { return *p; }   // observe

int main() {
  int *p = nullptr;
  return load_it(p);
}
