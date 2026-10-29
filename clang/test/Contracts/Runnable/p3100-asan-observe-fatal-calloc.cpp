// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:noexcept_observe \
// RUN:   %libcxx_flags -o %t && not %t

// With the address check routed as noexcept_observe, a report ASan builds as
// fatal -- its caller cannot continue past it -- runs the handler and then
// terminates, instead of returning.  Without the fix:
// A calloc-overflow report returns into calloc, which allocates the wrapped
// size (C requires a null result).
// Correct: the program terminates non-zero with at most one handler call.
// A second handler call exits 0.
//
// Mirror: gcc/testsuite/g++.dg/asan/p3100-asan-observe-fatal-calloc.C

#include <contracts>
#include <cstdlib>

static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  if (++calls > 1)
    std::_Exit(0);
}

int main() {
  volatile std::size_t n = std::size_t(1) << 33, s = std::size_t(1) << 31;
  void *volatile p = std::calloc(n, s);  // volatile: the call is not elided
  std::free(p);
  if (!p)
    return 1; // null is also correct
  return 0;
}
