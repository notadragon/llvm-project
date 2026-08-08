// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-null-deref-throw-nonnoexcept.json %libcxx_flags -o %t && %t

// P3100: null dereference with a THROWING handler in a non-noexcept function --
// under observe and enforce the handler throws before the dereference, so the
// exception propagates out and is caught by the caller.  (Clang supports
// throwing enforce/observe for this middle-end check; GCC defers them.)

#include <contracts>
#include <cstdlib>

struct E { int tag; };
static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++calls;
  throw E{7};
}

namespace obs_ns { int load(int *p) { return *p; } }   // observe
namespace enf_ns { int load(int *p) { return *p; } }   // enforce

int main() {
  int *p = nullptr;
  int caught = 0;
  try { obs_ns::load(p); } catch (E& e) { if (e.tag == 7) ++caught; }
  try { enf_ns::load(p); } catch (E& e) { if (e.tag == 7) ++caught; }
  if (caught != 2) std::abort();
  if (calls != 2) std::abort();
}
