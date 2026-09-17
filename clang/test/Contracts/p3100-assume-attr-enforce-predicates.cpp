// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-assume-attr-enforce-predicates.json %libcxx_flags -o %t && %t

// P3100: configurable [[assume]] across the full predicate spectrum under
// enforce, with a THROWING violation handler so the enforcing reaction can be
// observed and caught.  Each checkable predicate (constant false, a simple
// expression, a [[gnu::const]] call, a [[gnu::pure]] call) throws when false and
// is caught by the caller; a true constant passes; an opaque call is not
// checkable, so enforce clamps to ignore -- no throw and the predicate is never
// evaluated.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assume-attr-enforce-predicates.C)

#include <contracts>
#include <cstdlib>

namespace cs = std::contracts;
struct E {};
static int calls = 0;
static bool all_implicit = true;
void handle_contract_violation(const cs::contract_violation &v) {
  ++calls;
  if (v.kind() != cs::assertion_kind::implicit)
    all_implicit = false;
  throw E{};
}

[[gnu::const]] bool const_pred(int x) { return x > 0; }
static int reads;
[[gnu::pure]] bool pure_pred(int x) { (void)reads; return x > 0; }
static int opaque_side = 0;
bool opaque_pred(int x) { ++opaque_side; return x > 0; }

#define MK(ns, pred) namespace ns { int f(int x) { [[assume(pred)]]; return x; } }
MK(t_enf, true)
MK(f_enf, false)
MK(s_enf, x > 0)
MK(c_enf, const_pred(x))
MK(p_enf, pure_pred(x))
MK(o_enf, opaque_pred(x))

template <class F> static bool threw(F f) {
  try { f(); return false; } catch (E &) { return true; }
}

int main() {
  // true: enforce checks, passes -> no throw.
  if (threw([]{ t_enf::f(-1); })) std::abort();
  if (calls != 0) std::abort();

  // false / simple / const / pure: enforce checks, false -> throw -> caught.
  if (!threw([]{ f_enf::f(-1); })) std::abort();
  if (!threw([]{ s_enf::f(-1); })) std::abort();
  if (!threw([]{ c_enf::f(-1); })) std::abort();
  if (!threw([]{ p_enf::f(-1); })) std::abort();
  if (calls != 4 || !all_implicit) std::abort();

  // opaque: not checkable -> enforce clamps to ignore -> no throw, not evaluated.
  if (threw([]{ o_enf::f(-1); })) std::abort();
  if (calls != 4) std::abort();
  if (opaque_side != 0) std::abort();
}
