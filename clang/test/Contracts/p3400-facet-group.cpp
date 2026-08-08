// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -fsyntax-only

// identification_label facet: labels with group_names member.
// Verifies parsing and type-level acceptance; runtime config tested separately.

#include <contracts>
using namespace std::contracts;
using namespace std::contracts::labels;

// Basic group label via UDL
void f(int x) pre<"safety"group>(x > 0) {}

// Multiple characters
void g(int x) pre<"std.hardening"group>(x > 0) {}

// Combined: group label | empty
void h(int x) pre<"safety"group | empty_label>(x > 0) {}
void i(int x) pre<empty_label | "safety"group>(x > 0) {}

// Combined: two group labels (concatenation)
void j(int x) pre<"safety"group | "performance"group>(x > 0) {}

// Combined: group label | review (non-group facets preserved)
void k(int x) pre<"safety"group | review>(x > 0) {}

// Custom identification_label type
struct my_groups_t {
  using assertion_control_object = my_groups_t;
  char group_names[2][16] = {"safety", "logging"};
};
constexpr my_groups_t my_groups{};

void l(int x) pre<my_groups>(x > 0) {}

// Combined custom groups with allowed_semantics
struct guarded_t {
  using assertion_control_object = guarded_t;
  char group_names[1][8] = {"audit"};
  static constexpr evaluation_semantic_set allowed_semantics =
    evaluation_semantic_set(evaluation_semantic::observe)
    | evaluation_semantic_set(evaluation_semantic::enforce);
};
constexpr guarded_t guarded{};

void m(int x) pre<guarded>(x > 0) {}

// Combined: custom groups + allowed_semantics + review
void n(int x) pre<guarded | review>(x > 0) {}

// Static assertions to verify group_names member exists on types
static_assert(identification_label<decltype("safety"group)>);
static_assert(!identification_label<empty_label_t>);
static_assert(!identification_label<review_t>);
static_assert(identification_label<my_groups_t>);
static_assert(identification_label<guarded_t>);

// Combined label preserves identification when one side has it
static_assert(identification_label<decltype("safety"group | empty_label)>);
static_assert(identification_label<decltype(empty_label | "safety"group)>);
static_assert(identification_label<decltype("safety"group | "perf"group)>);
static_assert(!identification_label<decltype(empty_label | review)>);
