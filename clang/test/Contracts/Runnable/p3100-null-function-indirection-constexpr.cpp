// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t && %t
// P3100: with ub:expr.unary.dereference.nullptr configured, the check of a
// call through `*fp` (CLANG-755, CLANG-757) leaves constant evaluation
// unchanged: a call through a pointer to a constexpr function, directly,
// through `static_cast<F &>` or a conditional, is a constant expression and
// a constexpr variable initialized by one is constant-initialized (a call
// through a null pointer is rejected, as unconfigured: the control of
// OpenBugs/constexpr-null-function-indirection.cpp).  At run time the calls
// report nothing.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-function-indirection-constexpr.C.)

#include <contracts>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &) { ++reports; }

using F = int();
constexpr int seven() { return 7; }
constexpr int call(F *fp) { return (*fp)(); }
constexpr int call_cast(F *fp) { return static_cast<F &>(*fp)(); }
constexpr int call_cond(F *fp, F *gp, bool b) { return (b ? *fp : *gp)(); }
static_assert(call(seven) == 7);
static_assert(call_cast(seven) == 7);
static_assert(call_cond(seven, nullptr, true) == 7);
constexpr int v = call(seven);
constexpr int w = call_cast(seven);

int main() {
  int r = call(seven) + call_cast(seven) + call_cond(nullptr, seven, false);
  static int s = call(seven);
  if (r != 21 || s != 7 || v != 7 || w != 7 || reports != 0)
    __builtin_abort();
}
