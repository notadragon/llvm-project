// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p3400 -fcontract-evaluation-semantic=observe -O1 -fcontract-configuration-file=%S/p3100-implicit-in-labelled-predicate.json %libcxx_flags -o %t && %t

// P3100 x P3400: an assertion-control object belongs to the explicit
// contract assertion it is written on, never to an implicit one -- even one
// inside that assertion's own predicate.  The predicate divides by zero: the
// implicit check takes its semantic from the configuration (observe, with
// the defined result 0), reports through the global handler as
// assertion_kind::implicit, and does not see the label's local handler; the
// explicit precondition, now false, does go to the label, which handles it.
// (E7 of the open-bugs review: "implicit check under a label" is not
// constructible directly, so this pins the nearest constructible shape.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-implicit-in-labelled-predicate.C)

#include <contracts>
#include <cstdlib>

using namespace std::contracts;

static int local_calls = 0, global_calls = 0;
static bool global_saw_implicit = false;

struct local_t {
  using assertion_control_object = local_t;
  violation_handled handle_contract_violation(const contract_violation &v) const {
    ++local_calls;
    if (v.kind() != assertion_kind::pre)
      std::abort();
    return violation_handled::handled;
  }
};
constexpr local_t local{};

void handle_contract_violation(const contract_violation &v) {
  ++global_calls;
  global_saw_implicit = v.kind() == assertion_kind::implicit;
}

__attribute__((noinline)) int opaque(int x) { return x; }

void f(int n, int d) pre<local>(n / d > 0) {}

int main() {
  f(10, opaque(2));
  if (local_calls != 0 || global_calls != 0)
    std::abort();
  f(10, opaque(0));
  if (local_calls != 1 || global_calls != 1 || !global_saw_implicit)
    std::abort();
}
