// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-assume-attr-pure.json %libcxx_flags -o %t && %t

// P3100: whether the checking semantics are available for a configured
// [[assume]] depends on the PREDICATE.  A side-effect-free predicate -- here a
// call to a [[gnu::pure]] function -- can be evaluated, so observe checks it.
// An opaque function call may have side effects, so it is NOT checkable: the
// configured checking semantic clamps to ignore and the predicate is never
// evaluated.  (GCC mirror: g++.dg/contracts/cpp26/p3100-assume-attr-pure.C)

#include <contracts>
#include <cstdlib>

namespace cs = std::contracts;
static int calls = 0;
static bool all_implicit = true;
void handle_contract_violation(const cs::contract_violation &v) {
  ++calls;
  if (v.kind() != cs::assertion_kind::implicit)
    all_implicit = false;
}

[[gnu::pure]] bool pure_pred(int x);      // pure -> checkable
bool opaque_pred(int x);                  // opaque -> not checkable
static int opaque_side = 0;
bool opaque_pred(int x) { ++opaque_side; return x > 0; }   // has a side effect
[[gnu::pure]] bool pure_pred(int x) { return x > 0; }

namespace obs_pure   { int f(int x) { [[assume(pure_pred(x))]];   return x; } }
namespace obs_opaque { int f(int x) { [[assume(opaque_pred(x))]]; return x; } }

int main() {
  // pure predicate + observe: CHECKED -> false fires the handler.
  obs_pure::f(-1);
  if (calls != 1 || !all_implicit) std::abort();

  // opaque predicate + observe: clamps to ignore -> no check, predicate not
  // evaluated (no side effect).
  obs_opaque::f(-1);
  if (calls != 1) std::abort();
  if (opaque_side != 0) std::abort();
}
