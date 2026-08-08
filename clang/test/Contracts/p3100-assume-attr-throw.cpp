// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-assume-attr-throw.json %libcxx_flags -o %t && %t

// P3100: [[assume]] with a THROWING violation handler in a non-noexcept
// function -- the exception propagates out and is caught by the caller under
// both observe and enforce; and when it unwinds, in-scope automatic objects are
// destroyed (the handler call is a proper unwinding call).
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assume-attr-throw.C)

#include <contracts>
#include <cstdlib>

struct E { int tag; };
struct S { static int dtors; ~S() { ++dtors; } };
int S::dtors = 0;
static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++calls;
  throw E{7};
}

namespace obs_ns    { int f(int x) { [[assume(x > 0)]]; return x; } }   // observe
namespace enf_ns    { int f(int x) { [[assume(x > 0)]]; return x; } }   // enforce
namespace obs_clean { int f(int x) { S s; [[assume(x > 0)]]; return x; } }
namespace enf_clean { int f(int x) { S s; [[assume(x > 0)]]; return x; } }

int main() {
  int caught = 0;
  try { obs_ns::f(-1); } catch (E &e) { if (e.tag == 7) ++caught; }
  try { enf_ns::f(-1); } catch (E &e) { if (e.tag == 7) ++caught; }
  if (caught != 2 || calls != 2) std::abort();

  // A throwing reaction unwinds the scope containing the [[assume]], so the
  // in-scope automatic object's destructor runs -- under observe and enforce.
  try { obs_clean::f(-1); } catch (E &) {}
  if (S::dtors != 1) std::abort();
  try { enf_clean::f(-1); } catch (E &) {}
  if (S::dtors != 2) std::abort();
}
