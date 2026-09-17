// P3100: interaction of the "assume" semantic with allowed_semantics labels,
// with -fcontracts-allow-assume.  Configured semantic is "assume"; each label
// intersects the allowed set and resolution adjusts into it.
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3100 -fcontracts-allow-assume -fcontract-evaluation-semantic=assume %libcxx_flags -o %t && %t

#include <contracts>
using namespace std::contracts;
using namespace std::contracts::labels;

// Allows ignore + observe (assume is not listed).
struct ig_ob_t {
  using assertion_control_object = ig_ob_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    evaluation_semantic_set(evaluation_semantic::ignore)
    | evaluation_semantic_set(evaluation_semantic::observe);
};
constexpr ig_ob_t ig_ob{};

// Allows observe only.
struct ob_t {
  using assertion_control_object = ob_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    evaluation_semantic_set(evaluation_semantic::observe);
};
constexpr ob_t ob{};

static int side = 0;
static int viol = 0;
bool chk(int x) { ++side; return x > 0; }
void handle_contract_violation(const contract_violation&) { ++viol; }

// assume not in {ignore, observe}, but ignore is -> ignore (no eval, no viol).
void f_ignore(int x) pre<ig_ob>(chk(x)) {}

// assume not in {observe}, ignore not in {observe} -> observe (eval + viol).
void f_observe(int x) pre<ob>(chk(x)) {}

int main() {
  f_ignore(-1);
  if (side != 0 || viol != 0) return 1;
  f_observe(-1);
  if (side != 1 || viol != 1) return 2;
  return 0;
}
