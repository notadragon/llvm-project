// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-throw-enforce.json %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s --check-prefixes=CHECK,ENFORCE
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-throw-observe.json %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s --check-prefixes=CHECK,OBSERVE

// P3100: a throwing handler at flow-off-end in a noexcept function, under
// enforce and under observe: the exception reaches the noexcept boundary and
// calls std::terminate either way.  The handler prints the semantic it was
// given, which is what tells the two runs apart.

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <exception>

struct E {};
void handle_contract_violation(const std::contracts::contract_violation& v) {
  std::fprintf(stderr, "contract handler ran, semantic %d\n",
               static_cast<int>(v.semantic()));
  throw E{};
}

int f(int x) noexcept { if (x > 0) return x; }   // falls off for x<=0

int main() {
  std::set_terminate([] {
    std::fputs("terminate called\n", stderr);
    std::abort();
  });
  f(-1);
  std::fputs("returned from f\n", stderr);
}

// The handler runs before termination; a native trap would not print this.
// ENFORCE: contract handler ran, semantic 3
// OBSERVE: contract handler ran, semantic 2
// CHECK-NEXT: terminate called
// CHECK-NOT: returned from f

// REQUIRES: contracts-libcxx, native
