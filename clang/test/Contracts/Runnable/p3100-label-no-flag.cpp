// P3100: without -fcontracts-allow-assume, a label that lists "assume" in its
// allowed_semantics cannot re-introduce it -- the flag gate is authoritative.
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3100 -fcontract-evaluation-semantic=assume %libcxx_flags -o %t && %t

#include <contracts>
using namespace std::contracts;
using namespace std::contracts::labels;

// Lists observe AND assume, but -fcontracts-allow-assume is off.
struct ob_as_t {
  using assertion_control_object = ob_as_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    evaluation_semantic_set(evaluation_semantic::observe)
    | evaluation_semantic_set(evaluation_semantic::assume);
};
constexpr ob_as_t ob_as{};

static int side = 0;
static int viol = 0;
bool chk(int x) { ++side; return x > 0; }
void handle_contract_violation(const contract_violation&) { ++viol; }

// Flag off -> assume intersected away -> {observe} -> observe.
void f(int x) pre<ob_as>(chk(x)) {}

int main() {
  f(-1);
  if (side != 1 || viol != 1) return 1;   // observe, not assume/ignore
  return 0;
}
