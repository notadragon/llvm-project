// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -Wno-shift-count-overflow -fcontract-configuration-file=%S/p3100-shift-oob-throw-nonnoexcept.json %libcxx_flags -o %t && %t

// P3100: shift-out-of-range with a throwing handler in a non-noexcept function
// -- propagates out and is caught by the caller, under observe and enforce.

#include <contracts>
#include <cstdlib>

struct E { int tag; };
static int calls = 0;
void handle_contract_violation (const std::contracts::contract_violation&) {
  ++calls;
  throw E{7};
}

namespace obs_ns { int shl (int a, int b) { return a << b; } }   // observe
namespace enf_ns { int shl (int a, int b) { return a << b; } }   // enforce

int main () {
  int caught = 0;
  try { obs_ns::shl (1, 100); } catch (E& e) { if (e.tag == 7) ++caught; }
  try { enf_ns::shl (1, 100); } catch (E& e) { if (e.tag == 7) ++caught; }
  if (caught != 2) std::abort ();
  if (calls != 2) std::abort ();
}
