// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-fntryblock-observe.json %libcxx_flags -o %t && %t

// P3100: the flow-off check appears in TWO places for a function-try-block.
// When the try body falls off, the handler catches the (observe) throw; if the
// handler then runs off its own end without returning, that is again flow-off
// UB, guarded by the check after the whole construct -- which throws and
// propagates out of f (no enclosing handler catches it).

#include <contracts>
#include <cstdlib>

struct E {};
static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++calls;
  throw E{};
}

int f(int x) try { if (x > 0) return x; } catch (E&) { /* no return */ }

int main() {
  bool caught = false;
  try { f(-1); } catch (E&) { caught = true; }
  if (!caught) std::abort();
  if (calls != 2) std::abort();   // inside check + after-construct check
}
