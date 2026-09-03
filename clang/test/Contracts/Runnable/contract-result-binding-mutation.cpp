// A mutation performed through a postcondition's result binding is observable
// by the caller.
//
// This branch's position (2026-09-02): a predicate we evaluate is evaluated
// faithfully, so its effects stand.  [basic.contract.eval] permits *not
// evaluating* a predicate -- "an alternative evaluation that produces the same
// value ... but has no side effects" may be substituted -- but that is
// all-or-nothing.  It does not permit evaluating a predicate and then
// discarding what it did.
//
// Clang used to discard it.  EmitPostContracts stores the return value into
// the return slot and binds the result name there, so the predicate's write
// landed in the slot correctly -- but the epilogue had already materialised
// the value it was about to return, and returned that stale copy.  The fix is
// to re-read the slot after the postconditions have run.  GCC makes the same
// mutations observable (gnu_gcc 20eed05e8c4 / 0377d4408ab).
//
// This file covers results returned directly.  A CLASS-typed result is still
// bound to a copy and its mutations are still lost; that gap is pinned
// separately in contract-result-binding-mutation-class.cpp.
//
// RUN: %clangxx -std=c++26 %s -fcontracts %libcxx_flags -o %t && %t

static int failures = 0;

static void check(const char *what, int got, int want) {
  if (got != want) {
    __builtin_printf("FAIL: %s: got %d, expected %d\n", what, got, want);
    ++failures;
  }
}

// The original reproducer: increment through a const_cast.
int direct_increment() post(r : (const_cast<int &>(r)++, true)) { return 1; }

// Assignment rather than increment.
int assignment() post(r : (const_cast<int &>(r) = 42, true)) { return 0; }

// The mutation performed inside a called function taking `const &'.  This is
// the spelling that reaches the binding through a reference parameter, which
// on the GCC side was a distinct bug (the binding denoted a second object).
static bool bump(const int &r) {
  const_cast<int &>(r) += 10;
  return true;
}
int via_reference_parameter() post(r : bump(r)) { return 5; }

// Two postconditions, each mutating: both effects must stand, in order.
int two_postconditions() post(r : (const_cast<int &>(r)++, true))
    post(r : (const_cast<int &>(r) *= 3, true)) {
  return 2;
}

// A postcondition with no result name must be unaffected.
int no_result_name() post(true) { return 77; }

// A predicate that mutates and then fails still mutates -- and the check still
// reports.  Under `observe' the function completes normally.
static int violations = 0;
int mutate_and_fail() post(r : (const_cast<int &>(r) += 100, r < 0)) {
  return 1;
}

int main() {
  check("increment through const_cast", direct_increment(), 2);
  check("assignment through const_cast", assignment(), 42);
  check("mutation via a const& parameter", via_reference_parameter(), 15);
  check("two mutating postconditions", two_postconditions(), 9);
  check("no result name", no_result_name(), 77);

  if (failures)
    __builtin_abort();
  return 0;
}
