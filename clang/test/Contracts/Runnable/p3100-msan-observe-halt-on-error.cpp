// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=memory -fsanitize-semantic=memory:noexcept_observe \
// RUN:   %libcxx_flags -o %t && env MSAN_OPTIONS=halt_on_error=1 %t

// With the memory check routed as noexcept_observe, MSan continues after the
// handler even under MSAN_OPTIONS=halt_on_error=1: a routed report decides
// continue-vs-terminate from the semantic alone, as ASan, UBSan and TSan do.
//
// No GCC mirror: GCC has no MSan.

#include <contracts>

static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++calls;
}

__attribute__((noinline)) int trigger(int x) {
  int y;
  return y + (x - x); // indeterminate value
}

int main() {
  volatile int seed = 0;
  int v = trigger(seed);
  if (v) // MSan checks the value here
    seed = 1;
  return calls == 1 ? 0 : 1;
}
