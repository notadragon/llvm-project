// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -Wno-assume -fsyntax-only -verify %s
// expected-no-diagnostics

// Undoing a contract predicate's modifications must survive nesting.
//
// A contract predicate that calls a constexpr function containing its own
// contract assertion, or an [[assume]], starts a second evaluation of a
// predicate or an assumption inside the first.  The inner one must restore
// what it saved and leave the outer one in charge, so that the outer
// predicate's modification is still undone (and silently) whether it comes
// before or after the nested construct.
//
// Mirror of gnu_gcc's contract-constexpr-side-effect-nested.C.

constexpr bool bump(unsigned *p) {
  *p += 1;
  return true;
}
constexpr bool clean(const unsigned *p) { return *p < 1000; }

// An inner contract assertion.
constexpr bool inner_contract(const unsigned *p) {
  contract_assert(clean(p));
  return true;
}

// An inner [[assume]].
constexpr bool inner_assume(const unsigned *p) {
  [[assume(*p < 1000)]];
  return true;
}

// The control, with no nesting.
constexpr unsigned plain(unsigned *p) {
  contract_assert(bump(p));
  return *p;
}

// Nested contract assertion before the modification.
constexpr unsigned after_contract(unsigned *p) {
  contract_assert(inner_contract(p) && bump(p));
  return *p;
}

// Nested [[assume]] before the modification.
constexpr unsigned after_assume(unsigned *p) {
  contract_assert(inner_assume(p) && bump(p));
  return *p;
}

// Two frames deep, and the modification after both.
constexpr bool two_deep(const unsigned *p) {
  contract_assert(inner_contract(p));
  return true;
}

constexpr unsigned after_two(unsigned *p) {
  contract_assert(two_deep(p) && bump(p));
  return *p;
}

// The modification before the nested contract, whose own predicate must see
// it (and whose undo must not undo it early: the outer predicate reads it
// again afterwards).
constexpr bool inner_sees_one(const unsigned *p) {
  contract_assert(*p == 1);
  return true;
}

constexpr unsigned before_contract(unsigned *p) {
  contract_assert(bump(p) && inner_sees_one(p) && *p == 1);
  return *p;
}

// The predicate reads but does not modify.
constexpr unsigned reads_only(unsigned *p) {
  contract_assert(inner_contract(p) && clean(p));
  return *p;
}

constexpr unsigned run_plain() { unsigned x = 0; return plain(&x); }
constexpr unsigned run_after_contract() { unsigned x = 0; return after_contract(&x); }
constexpr unsigned run_after_assume() { unsigned x = 0; return after_assume(&x); }
constexpr unsigned run_after_two() { unsigned x = 0; return after_two(&x); }
constexpr unsigned run_before_contract() { unsigned x = 0; return before_contract(&x); }
constexpr unsigned run_reads_only() { unsigned x = 7; return reads_only(&x); }

static_assert(run_plain() == 0);
static_assert(run_after_contract() == 0);
static_assert(run_after_assume() == 0);
static_assert(run_after_two() == 0);
static_assert(run_before_contract() == 0);
static_assert(run_reads_only() == 7);
