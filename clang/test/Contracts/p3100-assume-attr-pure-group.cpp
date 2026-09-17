// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-assume-attr-pure-group.json %libcxx_flags -o %t && %t

// P3100: the checkable/pure subset of [[assume]] is separately matchable by
// the qualified group ub:dcl.attr.assume.false.pure (distinct from the
// unqualified ub:dcl.attr.assume.false covering the opaque case).  A config
// rule naming ONLY the qualified group, with no namespace/site restriction,
// must apply to the pure call site and NOT to the opaque one -- unlike
// p3100-assume-attr-pure.cpp, which discriminates the two by namespace, this
// test discriminates purely by group id.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assume-attr-pure-group.C)

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

[[gnu::pure]] bool pure_pred(int x);      // pure -> checkable -> .pure group
bool opaque_pred(int x);                  // opaque -> unqualified group
static int opaque_side = 0;
bool opaque_pred(int x) { ++opaque_side; return x > 0; }   // has a side effect
[[gnu::pure]] bool pure_pred(int x) { return x > 0; }

int f_pure(int x) { [[assume(pure_pred(x))]]; return x; }
int f_opaque(int x) { [[assume(opaque_pred(x))]]; return x; }

int main() {
  // The config below only names the .pure group, so only this call is
  // checked: false fires the handler.
  f_pure(-1);
  if (calls != 1 || !all_implicit) std::abort();

  // The opaque call reports the unqualified group, which the .pure-only rule
  // does not match, so it keeps the default "assume" semantic: no check, no
  // handler call, and (since assume never evaluates its operand) no side
  // effect either.
  f_opaque(-1);
  if (calls != 1) std::abort();
  if (opaque_side != 0) std::abort();
}
