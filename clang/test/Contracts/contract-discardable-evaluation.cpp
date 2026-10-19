// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -Wno-assume -fsyntax-only -verify %s

// Contract reporting must not escape an evaluation whose result is discarded,
// and must not be repeated by one that is not.
//
// The operand of an [[assume]] is not evaluated at all ([dcl.attr.assume]),
// so a contract assertion reached while the evaluator looks at it is not
// evaluated either: nothing it does, and no violation it would detect, is
// observable.  A contract predicate, by contrast, is evaluated once, and only
// its modifications are undone afterwards; a violation a nested contract
// detects during it is real and is reported, once.
//
// Mirror of gnu_gcc's contract-discardable-evaluation.C.

// An [[assume]] whose operand cannot be evaluated without side effects does
// not affect the program.
constexpr bool modifying(int *p) {
  contract_assert((++*p, true));
  return true;
}

constexpr int assume_operand_modifies() {
  int i = 0;
  [[assume(modifying(&i))]]; // operand is not evaluated
  return i;
}
static_assert(assume_operand_modifies() == 0);

// The same through a lambda.
constexpr int assume_operand_modifies_lambda() {
  int i = 0;
  [[assume([q = &i] {
    contract_assert((++*q, true));
    return true;
  }())]];
  return i;
}
static_assert(assume_operand_modifies_lambda() == 0);

// A failing contract inside an unevaluated operand is not reported: the
// operand is not evaluated, so there is no violation.
constexpr bool always_false(int *p) {
  contract_assert(*p > 100);
  return true;
}

constexpr int assume_operand_violates() {
  int i = 0;
  [[assume(always_false(&i))]];
  return i;
}
static_assert(assume_operand_violates() == 0);

// A violation detected by a nested contract during a modifying predicate's
// evaluation is reported exactly once.  obs() is observed, so evaluation
// continues; the predicate's modification is undone, so i ends at 0.
constexpr bool obs(int *p) {
  contract_assert(*p > 100); // expected-warning {{contract failed during execution of constexpr function}}
  return true;
}

constexpr int nested_violation() {
  int i = 0;
  contract_assert(obs(&const_cast<int &>(i)) && (++const_cast<int &>(i), true));
  return i;
}
static_assert(nested_violation() == 0);
