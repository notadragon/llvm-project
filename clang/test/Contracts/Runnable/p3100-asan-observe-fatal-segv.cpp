// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:noexcept_observe \
// RUN:   %libcxx_flags -o %t && not %t

// With the address check routed as noexcept_observe, a report ASan builds as
// fatal -- its caller cannot continue past it -- runs the handler and then
// terminates, instead of returning.  Without the fix:
// A deadly signal returns into the faulting instruction and re-faults.
// Correct: the program terminates non-zero with at most one handler call.
// A second handler call exits 0.
//
// Mirror: gcc/testsuite/g++.dg/asan/p3100-asan-observe-fatal-segv.C

#include <contracts>
#include <cstdlib>

static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  if (++calls > 1)
    std::_Exit(0);
}

int main() {
  volatile int *p = reinterpret_cast<int *>(0x10);
  *p = 1;
  return 0;
}
