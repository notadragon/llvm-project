// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-invalid-value-throw-nonnoexcept.json %libcxx_flags -o %t && %t

// P3100: invalid value load with a THROWING handler in a non-noexcept function
// -- the exception propagates out of the load and is caught by the caller
// (observe + enforce).

#include <contracts>
#include <cstring>
#include <cstdlib>

struct E { int tag; };
static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++calls;
  throw E{7};
}

namespace obs_ns { bool load(const bool* p) { return *p; } }   // observe
namespace enf_ns { bool load(const bool* p) { return *p; } }   // enforce

int main() {
  unsigned char c = 4; bool b; std::memcpy(&b, &c, 1);
  int caught = 0;
  try { obs_ns::load(&b); } catch (E& e) { if (e.tag == 7) ++caught; }
  try { enf_ns::load(&b); } catch (E& e) { if (e.tag == 7) ++caught; }
  if (caught != 2) std::abort();
  if (calls != 2) std::abort();
}
