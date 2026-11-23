// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -Wall -Wextra -fsyntax-only -verify %s
// expected-no-diagnostics

// A contract predicate that modifies an object of the enclosing constant
// evaluation is well formed, and the modification is undone once the
// predicate's value is known (see contract-constexpr-side-effect.cpp), so the
// enclosing evaluation means the same under every evaluation semantic and
// there is nothing to warn about: no diagnostic with -Wall -Wextra, including
// for a contract evaluated many times in one constant evaluation.
//
// Mirror of gnu_gcc's contract-constexpr-side-effect-warn.C (GCC used to warn,
// with -Wcontract-constexpr-side-effect, since removed).

constexpr bool bump(unsigned *p) {
  *p += 1;
  return true;
}
constexpr bool clean(const unsigned *p) { return *p < 1000; }

// Modifies the caller's object, which is then restored.
constexpr unsigned modifies(unsigned *p) {
  contract_assert(bump(p));
  return *p;
}

constexpr unsigned run_modifies() {
  unsigned x = 0;
  return modifies(&x);
}
static_assert(run_modifies() == 0);

// Evaluated many times in one constant evaluation, and undone every time.
constexpr unsigned loop_modifies() {
  unsigned x = 0;
  for (int i = 0; i < 20; ++i)
    modifies(&x);
  return x;
}
static_assert(loop_modifies() == 0);

// The predicate reads but does not modify.
constexpr unsigned reads_only(unsigned *p) {
  contract_assert(clean(p));
  return *p;
}

constexpr unsigned run_reads_only() {
  unsigned x = 7;
  return reads_only(&x);
}
static_assert(run_reads_only() == 7);

// A predicate re-calling a constexpr function that the caller already called
// (see contract-constexpr-repeat-call.cpp).
struct view {
  const char *p;
  unsigned len;
  constexpr bool empty() const { return len == 0; }
  constexpr void advance() {
    ++p;
    --len;
  }
};

constexpr void step(view *v) {
  contract_assert(!v->empty());
  v->advance();
}

constexpr unsigned run_repeat() {
  view v{"xyz", 3};
  unsigned n = 0;
  while (!v.empty()) {
    step(&v);
    ++n;
  }
  return n;
}
static_assert(run_repeat() == 3);

// A modification at run time only, never constant-evaluated.
unsigned runtime_only(unsigned *p) {
  contract_assert(bump(p));
  return *p;
}
