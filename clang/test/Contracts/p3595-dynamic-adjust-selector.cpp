// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3098 -fcontract-configuration-file=%S/p3595-dynamic-adjust-selector.json %libcxx_flags -o %t && %t

// A P3595 dynamic selector may return any evaluation semantic.  One that is
// not allowed is adjusted to the allowed set as D3595R1 ("Adjusting a
// Semantic to the Allowed Set") describes, rather than being an invalid
// selection (notadragon_wg21 DECISIONS-resolved.md E66): without
// -fcontracts-allow-assume, assume is adjusted to ignore, and without
// -fcontracts-p4298 noexcept_observe and noexcept_enforce are adjusted to
// observe and enforce.  A label's allowed set takes part: under a label
// allowing only observe and enforce, assume is adjusted upward to observe.
// A postcondition with captures adjusts the value it chooses once the same
// way.  The C form is C/c-dynamic-selection-adjust.c.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-adjust-selector.C
// in the gnu_gcc fork.

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

static evaluation_semantic chosen;
extern "C" evaluation_semantic adjust_sel() { return chosen; }

static int evaluated, observed, inits;
static bool exit_on_enforce;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.semantic() == evaluation_semantic::observe)
    ++observed;
  else if (v.semantic() == evaluation_semantic::enforce && exit_on_enforce)
    std::_Exit(0); // adjusted from noexcept_enforce, as expected
  else
    __builtin_abort();
}

struct observe_or_enforce_t {
  using assertion_control_object = observe_or_enforce_t;
  static constexpr evaluation_semantic_set allowed_semantics = {
      evaluation_semantic::observe, evaluation_semantic::enforce};
};
constexpr observe_or_enforce_t observe_or_enforce{};

struct G {
  G() { ++inits; }
  G(const G &) { ++inits; }
};

bool chk(int x) {
  ++evaluated;
  return x > 0;
}

void f(int x) pre(chk(x)) {}
void lf(int x) pre<observe_or_enforce>(chk(x)) {}
int pf(const int x) post[g = G()](chk(x)) { return x; }

static void expect(int e, int o, int i) {
  if (evaluated != e || observed != o || inits != i)
    __builtin_abort();
}

int main() {
  chosen = evaluation_semantic::assume;
  f(-1);
  expect(0, 0, 0); // assume -> ignore: not evaluated
  lf(-1);
  expect(1, 1, 0); // assume -> observe under the label
  pf(-1);
  expect(1, 1, 0); // assume -> ignore: no capture, no check

  chosen = evaluation_semantic::noexcept_observe;
  f(-1);
  expect(2, 2, 0); // noexcept_observe -> observe
  lf(-1);
  expect(3, 3, 0); // likewise under the label
  pf(-1);
  expect(4, 4, 1); // likewise for the captures

  chosen = evaluation_semantic::noexcept_enforce;
  exit_on_enforce = true;
  f(-1); // noexcept_enforce -> enforce: the handler exits
  __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
