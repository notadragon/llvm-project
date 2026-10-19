// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3099 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: %t

// P3400: the compile-time facets may be consteval member functions -- they
// are only ever called during translation.  A label whose compute_semantic,
// compute_comment and compute_message are all consteval applies all three:
// at run time (enforce downgraded to observe, so the handler sees the
// computed comment and message and execution continues) and during constant
// evaluation (a second label's consteval compute_semantic yields ignore, so
// a false predicate does not stop the constant).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-consteval-facets.C

#include <contracts>
#include <cstring>

using std::contracts::evaluation_semantic;

struct ce_t {
  using assertion_control_object = ce_t;
  consteval evaluation_semantic compute_semantic(evaluation_semantic) const {
    return evaluation_semantic::observe;
  }
  consteval const char *compute_comment(const char *) const {
    return "computed comment";
  }
  consteval const char *compute_message(const char *) const {
    return "computed message";
  }
};
constexpr ce_t ce{};

struct ce_ignore_t {
  using assertion_control_object = ce_ignore_t;
  static consteval evaluation_semantic compute_semantic(evaluation_semantic) {
    return evaluation_semantic::ignore;
  }
};
constexpr ce_ignore_t ce_ignore{};

static int violations;
static evaluation_semantic sem;
static const char *comment, *message;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++violations;
  sem = v.semantic();
  comment = v.comment();
  message = v.message();
}

void f(int x) pre<ce>(x > 0, "written") {}

constexpr int g(int x) pre<ce_ignore>(x > 0) { return x; }
static_assert(g(-1) == -1);

int main() {
  f(0);
  if (violations != 1 || sem != evaluation_semantic::observe ||
      std::strcmp(comment, "computed comment") != 0 ||
      std::strcmp(message, "computed message") != 0)
    __builtin_abort();
  volatile int m = -1;
  if (g(m) != -1 || violations != 1)
    __builtin_abort();
}
