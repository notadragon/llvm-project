// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t && %t

// A contract condition that calls a constexpr function already called earlier
// in the same constant evaluation must still be constant.
//
// Mirror of gnu_gcc's contract-constexpr-repeat-call.C.  There, a contract
// condition is evaluated under a "modifiable tracker" so that a predicate
// cannot modify objects of the enclosing evaluation, and membership of the
// modifiable set was decided by whether the object was already a key in the
// value map.  Retiring a VAR_/PARM_/RESULT_DECL leaves it in that map marked
// dead rather than removing it, so a constexpr function called once, returned
// from, and then called again from inside a contract condition was refused
// permission to write its own result, and the condition came out non-constant:
//
//   error: contract condition is not constant
//
// Clang does not reproduce it -- it has no such tracker -- so this is carried
// over as a guard.  Nothing here is about labels; the labelled case is only
// the shape the bug was reported from.
//
// Found by the BDE contracts integration, where a formatter's parse loop tests
// spec.empty() and then calls a helper asserting !spec.empty().

#include <contracts>

using std::contracts::evaluation_semantic;

struct label_enforce {
  using assertion_control_object = label_enforce;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return evaluation_semantic::enforce;
  }
};

struct label_observe {
  using assertion_control_object = label_observe;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return evaluation_semantic::observe;
  }
};

struct view {
  const char *p;
  unsigned len;
  constexpr bool empty() const { return len == 0; }
  constexpr char front() const { return *p; }
  constexpr void advance() { ++p; --len; }
};

// contract_assert: the caller's loop calls empty(), then this calls it again
// from the condition.
constexpr unsigned step_assert(view *v) {
  contract_assert(!v->empty());
  unsigned c = v->front() == 'x' ? 1u : 0u;
  v->advance();
  return c;
}

// Same, via a precondition.
constexpr unsigned step_pre(view *v) pre(!v->empty()) {
  unsigned c = v->front() == 'x' ? 1u : 0u;
  v->advance();
  return c;
}

// Same, via a postcondition, whose condition also calls empty() after the body
// already has.  A value parameter named in a postcondition must be const.
constexpr unsigned step_post(view *const v)
    post(r : r <= 1u && (v->empty() || !v->empty())) {
  unsigned c = v->front() == 'x' ? 1u : 0u;
  v->advance();
  return c;
}

// Labelled forms.  The observe one produced a spurious warning rather than an
// error under GCC, so it is worth its own case.
constexpr unsigned step_labelled(view *v) {
  contract_assert<label_enforce{}>(!v->empty());
  unsigned c = v->front() == 'x' ? 1u : 0u;
  v->advance();
  return c;
}

constexpr unsigned step_labelled_observe(view *v) {
  contract_assert<label_observe{}>(!v->empty());
  unsigned c = v->front() == 'x' ? 1u : 0u;
  v->advance();
  return c;
}

// The driver: empty() is called in the loop condition, so by the time the
// contract's condition calls it the callee's result is already known to the
// evaluation.
template <unsigned (*STEP)(view *)>
constexpr unsigned count(const char *s, unsigned n) {
  view v{s, n};
  unsigned total = 0;
  while (!v.empty())
    total += STEP(&v);
  return total;
}

static_assert(count<step_assert>("xyx", 3) == 2);
static_assert(count<step_pre>("xyx", 3) == 2);
static_assert(count<step_post>("xyx", 3) == 2);
static_assert(count<step_labelled>("xyx", 3) == 2);
static_assert(count<step_labelled_observe>("xyx", 3) == 2);

// A nested call one level deeper, so the reused result belongs to a frame
// further out than the condition's own caller.
constexpr bool outer_empty(const view *v) { return v->empty(); }

constexpr unsigned step_nested(view *v) {
  contract_assert(!outer_empty(v));
  v->advance();
  return 1;
}

constexpr unsigned count_nested(const char *s, unsigned n) {
  view v{s, n};
  unsigned total = 0;
  while (!outer_empty(&v))
    total += step_nested(&v);
  return total;
}

static_assert(count_nested("xyx", 3) == 3);

int main() {
  // The same functions at run time, to confirm ordinary code generation is
  // undisturbed.
  if (count<step_assert>("xyx", 3) != 2)
    __builtin_abort();
  if (count<step_labelled>("xyx", 3) != 2)
    __builtin_abort();
  if (count_nested("xyx", 3) != 3)
    __builtin_abort();
  return 0;
}
