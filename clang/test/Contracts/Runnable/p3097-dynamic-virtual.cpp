// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-configuration-file=%S/p3097-dynamic-virtual.json \
// RUN:   %libcxx_flags -o %t && %t

// P3097 x P3595: callee-side dynamic selection reaches both halves of a
// virtual call.  The statically chosen function's assertions (checked in the
// interface wrapper) and the final overrider's own are configured to
// different dynamic selectors, chosen by namespace, and each selector's
// result is read at run time, per call: switching either one changes which
// of the two preconditions is checked, without recompiling.  A violation
// reports the function whose assertion failed.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3097-dynamic-virtual.C

#include <contracts>
#include <cstdlib>
#include <cstring>

using std::contracts::evaluation_semantic;

static evaluation_semantic iface_sem, impl_sem;
namespace sel {
evaluation_semantic iface() { return iface_sem; }
evaluation_semantic impl() { return impl_sem; }
} // namespace sel

static int violations;
static const char *where;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++violations;
  where = v.location().function_name();
}

namespace iface {
struct B {
  virtual int f(int x) pre(x > 0) { return x; }
  virtual ~B() {}
};
} // namespace iface
namespace impl {
struct D : iface::B {
  int f(int x) override pre(x > 1) { return x; }
};
} // namespace impl

static int run(evaluation_semantic i, evaluation_semantic d, int x) {
  iface_sem = i;
  impl_sem = d;
  violations = 0;
  where = "";
  impl::D obj;
  iface::B *p = &obj;
  p->f(x);
  return violations;
}

int main() {
  const auto ign = evaluation_semantic::ignore;
  const auto obs = evaluation_semantic::observe;
  if (run(ign, ign, 0) != 0)
    std::abort();
  if (run(obs, ign, 0) != 1 || !std::strstr(where, "B::f"))
    std::abort();
  if (run(ign, obs, 0) != 1 || !std::strstr(where, "D::f"))
    std::abort();
  if (run(obs, obs, 0) != 2)
    std::abort();
  // x == 1 satisfies only the interface's precondition.
  if (run(obs, obs, 1) != 1 || !std::strstr(where, "D::f"))
    std::abort();
}
