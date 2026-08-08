// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-throw-nonnoexcept.json %libcxx_flags -o %t && %t

// P3100: a THROWING contract-violation handler at flow-off-end
// ({stmt.return.flow.off}).  In a non-noexcept function the handler-thrown
// exception propagates out of the function under both observe and enforce
// (D3100R8 Option A).  Each call is in a try/catch, so it is caught by the
// caller.  (GCC mirror: g++.dg/contracts/cpp26/p3100-flow-off-throw-nonnoexcept.C)

#include <contracts>
#include <cstdlib>

struct E { int tag; };
static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++calls;
  throw E{7};
}

namespace obs_ns { int f(int x) { if (x > 0) return x; } }   // observe
namespace enf_ns { int f(int x) { if (x > 0) return x; } }   // enforce

int main() {
  int caught = 0;
  try { obs_ns::f(-1); } catch (E& e) { if (e.tag == 7) ++caught; }
  try { enf_ns::f(-1); } catch (E& e) { if (e.tag == 7) ++caught; }
  if (caught != 2) std::abort();
  if (calls != 2) std::abort();
}
