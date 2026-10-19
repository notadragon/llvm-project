// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce \
// RUN:   -Winvalid-constexpr -fsyntax-only -verify=common,checked %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe \
// RUN:   -Winvalid-constexpr -fsyntax-only -verify=common %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore \
// RUN:   -Winvalid-constexpr -fsyntax-only -verify=common %s

// A contract predicate that can never be constant-evaluated makes the call
// unusable in a constant expression, and Expr::isPotentialConstantExpr sees
// that for a postcondition as it does for a precondition.
//
// Scope, so this test is not read as more than it is:
//
//   * -Winvalid-constexpr is required.  Since P2448 the underlying rule is
//     gone in C++23 and later, so Clang skips the check entirely at those
//     dialects unless the warning is asked for.  This is a lint, not a
//     conformance gate.
//   * The semantic is pinned on every RUN line.  Under `enforce` a predicate
//     that cannot be constant-evaluated makes the call non-constant.  Under
//     `observe` it does not: in a manifestly constant-evaluated context such a
//     predicate is a contract violation, and observe reports it and continues
//     ([basic.contract.eval]; DECISIONS.md M1), so the call can still be
//     constant.  `ignore` does not evaluate it at all.  `common` is what holds
//     at every semantic; `checked` is what the predicates add under enforce.
//   * A postcondition is checked whether or not the body can be evaluated
//     with unknown arguments; see the "body not evaluated" group below.
//
// Deliberately NOT mirrored on the GCC side: GCC does not run contract
// predicates through its potential-constant-expression check at all, so this
// widens an existing divergence rather than closing one.  Both are conforming
// -- since P2448 there is no rule left to violate.

int global = 1;
// common-note@-1 1 {{declared here}}
// checked-note@-2 4 {{declared here}}

// -- Control: a non-constant body, no contracts.  Fires at every semantic. --

constexpr int bad_body(int x) { return global + x; }
// common-error@-1 {{constexpr function never produces a constant expression}}
// common-note@-2 {{read of non-const variable 'global' is not allowed in a constant expression}}

// -- Control: the same predicate as a precondition. ------------------------
// A precondition is evaluated before the body, so it is reached whether or not
// the body can be evaluated with unknown arguments.

constexpr int bad_pre(int x) pre(global > 0) { return x; }
// checked-error@-1 {{constexpr function never produces a constant expression}}
// checked-note@-2 {{read of non-const variable 'global' is not allowed in a constant expression}}

// -- The pin: a postcondition on a body that does evaluate. ----------------

constexpr int bad_post() post(global > 0) { return 1; }
// checked-error@-1 {{constexpr function never produces a constant expression}}
// checked-note@-2 {{read of non-const variable 'global' is not allowed in a constant expression}}

// The skip below is per-contract, not per-sequence: the result-name
// postcondition is skipped and the hopeless one is still checked.
constexpr int mixed() post(r: r > 0) post(global > 0) { return 1; }
// checked-error@-1 {{constexpr function never produces a constant expression}}
// checked-note@-2 {{read of non-const variable 'global' is not allowed in a constant expression}}

// -- A postcondition that binds the result name is skipped. ----------------
//
// In this mode the body ran with unknown arguments, so there is no return
// value for the result name to bind to; evaluating the predicate would read
// an unset value.  No diagnostic, even though `r > 0` cannot be decided.

constexpr int result_name() post(r: r > 0) { return 1; }

// -- Body not evaluated. ----------------------------------------------------
//
// isPotentialConstantExpr calls the function with no arguments, so a body that
// reads a parameter stops there, before any postcondition.  The postconditions
// that name neither the result nor a capture are then checked on their own, as
// a precondition is: the same hopeless `global > 0` as bad_post is diagnosed,
// and one that reads a parameter is not (its value is merely unknown).

constexpr int bad_post_unreached(int x) post(global > 0) { return x; }
// checked-error@-1 {{constexpr function never produces a constant expression}}
// checked-note@-2 {{read of non-const variable 'global' is not allowed in a constant expression}}

constexpr int fine_post_param(const int x) post(x > 0) { return x; }

// -- Contracts that are perfectly constant: never diagnosed. ---------------
// The parameter must be const to be named in a postcondition
// ([dcl.contract.func]).

constexpr int fine_pre(int x) pre(x > 0) { return x; }
constexpr int fine_post(const int x) post(r: r == x) { return x; }
constexpr int fine_both(const int x) pre(x > 0) post(r: r == x) { return x; }
