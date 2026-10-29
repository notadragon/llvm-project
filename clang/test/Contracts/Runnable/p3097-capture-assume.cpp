// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fcontracts-p3400 -fcontracts-p3100 -fcontracts-allow-assume \
// RUN:   -fcontract-evaluation-semantic=assume %libcxx_flags -o %t
// RUN: %t

// P3097+P3098+P3400+P3100: an allowed_semantics label admitting 'assume' on a
// capturing interface postcondition of a virtual function, with base semantic
// 'assume'.  assume lowers to ignore, and the ignore semantic gates the whole
// postcondition as a unit: the capture is NOT constructed and the predicate is
// not evaluated.  (GCC mirror: g++.dg/contracts/cpp26/p3097-capture-assume.C)

#include <contracts>
#include <cstdio>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct allow_assume_t {
  using assertion_control_object = allow_assume_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::assume, evaluation_semantic::observe};
};
constexpr allow_assume_t aa{};

static int ctor_count = 0;
struct Tracker {
  Tracker() { ++ctor_count; }
  Tracker(const Tracker&) { ++ctor_count; }
  ~Tracker() { }
};
static Tracker g_tracker;

static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

struct Base {
  virtual int f()
    post<aa> [t = g_tracker] (r: r > 0)
  { return 1; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int f() override { return -1; }  // would violate r > 0 if the post were live
};

int main() {
  Derived d;
  Base& b = d;
  ctor_count = 0;
  violation_count = 0;

  int r = b.f();
  if (r != -1) __builtin_abort();
  if (ctor_count != 0) __builtin_abort();      // capture not constructed
  if (violation_count != 0) __builtin_abort(); // predicate not evaluated

  std::printf("PASS\n");
}
