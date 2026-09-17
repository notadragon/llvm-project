// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-div-by-zero-throw-nonnoexcept.json %libcxx_flags -o %t && %t

// P3100: divide-by-zero with a THROWING handler in a non-noexcept function --
// the exception propagates out and is caught by the caller (observe + enforce).

#include <contracts>
#include <cstdlib>

struct E { int tag; };
static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++calls;
  throw E{7};
}

namespace obs_ns { int divi(int a, int b) { return a / b; } }   // observe
namespace enf_ns { int divi(int a, int b) { return a / b; } }   // enforce

int main() {
  int caught = 0;
  try { obs_ns::divi(1, 0); } catch (E& e) { if (e.tag == 7) ++caught; }
  try { enf_ns::divi(1, 0); } catch (E& e) { if (e.tag == 7) ++caught; }
  if (caught != 2) std::abort();
  if (calls != 2) std::abort();
}
