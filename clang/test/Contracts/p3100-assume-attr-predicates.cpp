// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-assume-attr-predicates.json %libcxx_flags -o %t && %t

// P3100: configurable [[assume]] across the full predicate spectrum under the
// non-terminating semantics.  For each predicate kind -- constant true/false, a
// simple expression, a [[gnu::const]] call, a [[gnu::pure]] call, and an opaque
// call -- one namespace is configured "observe" and one "ignore".  A checkable
// (side-effect-free) predicate is evaluated under observe: it reports when false
// and passes when true.  An opaque predicate is not checkable, so a configured
// checking semantic clamps to ignore -- no report and the predicate is never
// evaluated.  ignore never checks for any predicate kind.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assume-attr-predicates.C)

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

[[gnu::const]] bool const_pred(int x) { return x > 0; }   // side-effect-free
static int reads;
[[gnu::pure]] bool pure_pred(int x) { (void)reads; return x > 0; }
static int opaque_side = 0;
bool opaque_pred(int x) { ++opaque_side; return x > 0; }  // NOT side-effect-free

#define MK(ns, pred) namespace ns { int f(int x) { [[assume(pred)]]; return x; } }
MK(t_obs, true)             MK(t_ign, true)
MK(f_obs, false)            MK(f_ign, false)
MK(s_obs, x > 0)            MK(s_ign, x > 0)
MK(c_obs, const_pred(x))    MK(c_ign, const_pred(x))
MK(p_obs, pure_pred(x))     MK(p_ign, pure_pred(x))
MK(o_obs, opaque_pred(x))   MK(o_ign, opaque_pred(x))

int main() {
  // observe: checkable + false -> report; true -> pass; opaque -> clamp.
  t_obs::f(-1);
  if (calls != 0) std::abort();
  f_obs::f(-1);
  s_obs::f(-1);
  c_obs::f(-1);
  p_obs::f(-1);
  if (calls != 4 || !all_implicit) std::abort();
  o_obs::f(-1);
  if (calls != 4) std::abort();
  if (opaque_side != 0) std::abort();

  // ignore: never reports, for any predicate kind.
  t_ign::f(-1); f_ign::f(-1); s_ign::f(-1);
  c_ign::f(-1); p_ign::f(-1); o_ign::f(-1);
  if (calls != 4) std::abort();
  if (opaque_side != 0) std::abort();
}
