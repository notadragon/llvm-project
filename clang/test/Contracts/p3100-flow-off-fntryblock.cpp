// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-fntryblock.json %libcxx_flags -o %t && %t

// P3100: flowing off the end of the try-block body of a *function-try-block* is
// the flow-off-end violation ({stmt.return.flow.off}), and the violation occurs
// inside the function-body scope -- so the function-try-block's own handler can
// catch a throwing observe/enforce handler.  Under ignore the try-body fall-off
// produces a defined return instead (no handler runs).
// (GCC mirror: g++.dg/contracts/cpp26/p3100-flow-off-fntryblock.C)

#include <contracts>
#include <cstdlib>

struct E {};
static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++calls;
  throw E{};
}

namespace obs_ns {                                     // observe
  int f(int x) try { if (x > 0) return x; } catch (E&) { return -1; }
}
namespace enf_ns {                                     // enforce
  int f(int x) try { if (x > 0) return x; } catch (E&) { return -1; }
}
namespace ign_ns {                                     // ignore
  int f(int x) try { if (x > 0) return x; } catch (E&) { return -1; }
}

int main() {
  if (obs_ns::f(-1) != -1) std::abort();
  if (enf_ns::f(-1) != -1) std::abort();
  if (calls != 2) std::abort();

  if (ign_ns::f(-1) != 0) std::abort();   // defined return, no handler
  if (calls != 2) std::abort();

  if (obs_ns::f(7) != 7) std::abort();
  if (enf_ns::f(7) != 7) std::abort();
  if (ign_ns::f(7) != 7) std::abort();
  if (calls != 2) std::abort();
}
