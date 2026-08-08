// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-array-bounds-throw-nonnoexcept.json %libcxx_flags -o %t && %t

// P3100: out-of-bounds subscript with a THROWING handler in a non-noexcept
// function -- the exception propagates out and is caught by the caller
// (observe + enforce).

#include <contracts>
#include <cstdlib>

struct E { int tag; };
static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++calls;
  throw E{7};
}

int g[4] = { 1, 2, 3, 4 };
namespace obs_ns { int rd(int i) { return g[i]; } }   // observe
namespace enf_ns { int rd(int i) { return g[i]; } }   // enforce

__attribute__((noinline)) int opaque(int x) { return x; }

int main() {
  int caught = 0;
  try { obs_ns::rd(opaque(100)); } catch (E& e) { if (e.tag == 7) ++caught; }
  try { enf_ns::rd(opaque(100)); } catch (E& e) { if (e.tag == 7) ++caught; }
  if (caught != 2) std::abort();
  if (calls != 2) std::abort();
}
