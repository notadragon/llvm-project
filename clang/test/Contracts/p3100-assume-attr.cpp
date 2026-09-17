// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-assume-attr.json %libcxx_flags -o %t && %t

// P3100: the [[assume]] attribute is a configurable implicit contract assertion
// (group ub:dcl.attr.assume.false).  For a side-effect-free predicate every
// semantic is available: observe reports (assertion_kind::implicit) then
// continues; ignore drops the assumption; the default is assume (no check).
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assume-attr.C)

#include <contracts>
#include <cstdlib>
#include <cstring>

namespace cs = std::contracts;
static int calls = 0;
static bool all_implicit = true;
static const char *last_comment = nullptr;
void handle_contract_violation(const cs::contract_violation &v) {
  ++calls;
  if (v.kind() != cs::assertion_kind::implicit)
    all_implicit = false;
  last_comment = v.comment();
}

namespace obs_ns { int f(int x) { [[assume(x > 0)]]; return x; } }   // observe
namespace ign_ns { int f(int x) { [[assume(x > 0)]]; return x; } }   // ignore
int def_f(int x) { [[assume(x > 0)]]; return x; }                    // assume

int main() {
  // observe: predicate false -> handler runs, then continues (returns x).
  if (obs_ns::f(-1) != -1) std::abort();
  if (calls != 1 || !all_implicit) std::abort();
  if (std::strcmp(last_comment, "assumed condition is false") != 0) std::abort();

  // observe: predicate true -> no handler.
  if (obs_ns::f(5) != 5) std::abort();
  if (calls != 1) std::abort();

  // ignore: dropped, no check even when the predicate is false.
  if (ign_ns::f(-1) != -1) std::abort();
  if (calls != 1) std::abort();

  // assume (builtin default): no runtime check (call with a true predicate).
  if (def_f(5) != 5) std::abort();
  if (calls != 1) std::abort();
}
