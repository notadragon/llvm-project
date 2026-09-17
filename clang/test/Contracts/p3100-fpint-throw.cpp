// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-fpint-throw-nonnoexcept.json %libcxx_flags -o %t && %t

// P3100: float-cast out of range with a throwing handler in a non-noexcept
// function -- propagates out and is caught by the caller, under observe and
// enforce.

#include <contracts>
#include <cstdlib>

struct E { int tag; };
static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++calls;
  throw E{7};
}

namespace obs_ns { int fi(double x) { return (int) x; } }   // observe
namespace enf_ns { int fi(double x) { return (int) x; } }   // enforce

int main() {
  int caught = 0;
  try { obs_ns::fi(1e30); } catch (E& e) { if (e.tag == 7) ++caught; }
  try { enf_ns::fi(1e30); } catch (E& e) { if (e.tag == 7) ++caught; }
  if (caught != 2) std::abort();
  if (calls != 2) std::abort();
}
