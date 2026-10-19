// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -Wno-assume -fsyntax-only -verify %s
// expected-no-diagnostics

// Composition of [[assume]] and contract assertions during constant
// evaluation:
//
//   * a contract predicate under a checking semantic is evaluated to
//     determine its value, and whatever it modified is then undone
//     ([basic.contract.eval] permits an evaluation without side effects);
//   * an [[assume]] operand is not evaluated ([dcl.attr.assume]), so its side
//     effects must never be observable either.
//
// What must hold is that nesting composes: an inner construct never overrides
// the outer one's rule, and no modification survives either.  Each case
// counts the modifications that survive, so a wrong answer is a wrong number
// rather than a diagnostic.  Deliveries vary between a separate function and a
// lambda.
//
// Mirror of gnu_gcc's contract-assume-nesting-permutations.C.

constexpr bool M(int *p) {
  ++*p;
  return true;
}

constexpr bool in_ctr(int *p) {
  contract_assert(M(p));
  return true;
}
constexpr bool in_asm(int *p) {
  [[assume(M(p))]];
  return true;
}
constexpr bool in_ctr0(int *p) {
  contract_assert(p != nullptr);
  return true;
}

// Outer contract_assert: the predicate is evaluated, and everything it did is
// undone afterwards, whatever is nested inside it -- a nested
// contract_assert's modifications are undone by its own undo and an
// [[assume]]'s are never made.  Hence 0 throughout, silently.  Inside the
// predicate the outer modifications are visible and the nested ones are not,
// which the final i == 2 checks (as a violation, under enforce).

constexpr int ctr_in_ctr_fn() {
  int i = 0;
  contract_assert(M(&const_cast<int &>(i)) && in_ctr(&const_cast<int &>(i)) &&
                  M(&const_cast<int &>(i)) && i == 2);
  return i;
}
static_assert(ctr_in_ctr_fn() == 0);

constexpr int ctr_in_ctr_lambda() {
  int i = 0;
  contract_assert(M(&const_cast<int &>(i)) &&
                  [q = &const_cast<int &>(i)] {
                    contract_assert(M(q));
                    return true;
                  }() &&
                  M(&const_cast<int &>(i)) && i == 2);
  return i;
}
static_assert(ctr_in_ctr_lambda() == 0);

constexpr int asm_in_ctr_fn() {
  int i = 0;
  contract_assert(M(&const_cast<int &>(i)) && in_asm(&const_cast<int &>(i)) &&
                  M(&const_cast<int &>(i)) && i == 2);
  return i;
}
static_assert(asm_in_ctr_fn() == 0);

constexpr int asm_in_ctr_lambda() {
  int i = 0;
  contract_assert(M(&const_cast<int &>(i)) &&
                  [q = &const_cast<int &>(i)] {
                    [[assume(M(q))]];
                    return true;
                  }() &&
                  M(&const_cast<int &>(i)) && i == 2);
  return i;
}
static_assert(asm_in_ctr_lambda() == 0);

// Outer [[assume]]: the operand is not evaluated, so nothing it does is
// observable, whatever is nested inside it and wherever the modification sits
// relative to that nesting.

constexpr int ctr_in_asm_fn() {
  int i = 0;
  [[assume(M(&const_cast<int &>(i)) && in_ctr(&const_cast<int &>(i)) &&
           M(&const_cast<int &>(i)))]];
  return i;
}
static_assert(ctr_in_asm_fn() == 0);

constexpr int ctr_in_asm_lambda() {
  int i = 0;
  [[assume(M(&const_cast<int &>(i)) &&
           [q = &const_cast<int &>(i)] {
             contract_assert(M(q));
             return true;
           }() &&
           M(&const_cast<int &>(i)))]];
  return i;
}
static_assert(ctr_in_asm_lambda() == 0);

constexpr int asm_in_asm_fn() {
  int i = 0;
  [[assume(M(&const_cast<int &>(i)) && in_asm(&const_cast<int &>(i)) &&
           M(&const_cast<int &>(i)))]];
  return i;
}
static_assert(asm_in_asm_fn() == 0);

constexpr int asm_in_asm_lambda() {
  int i = 0;
  [[assume(M(&const_cast<int &>(i)) &&
           [q = &const_cast<int &>(i)] {
             [[assume(M(q))]];
             return true;
           }() &&
           M(&const_cast<int &>(i)))]];
  return i;
}
static_assert(asm_in_asm_lambda() == 0);

// A nested contract that does not itself modify, followed by a modification:
// the enclosing assumption has to survive the inner assertion on its own.
constexpr int nonmod_ctr_then_modify_in_asm() {
  int i = 0;
  [[assume(in_ctr0(&const_cast<int &>(i)) && M(&const_cast<int &>(i)))]];
  return i;
}
static_assert(nonmod_ctr_then_modify_in_asm() == 0);

// A constant, so a wrong answer changes object layout as well.
char probe[nonmod_ctr_then_modify_in_asm() + 1];
static_assert(sizeof(probe) == 1);
