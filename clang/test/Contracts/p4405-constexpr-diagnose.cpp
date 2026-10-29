// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -emit-llvm-only -fexperimental-new-constant-interpreter -verify=expected,bc %s

// P4405: with -fcontract-evaluation-semantic=diagnose, the run-time
// evaluations' diagnose goes to the analysis evaluations, which warn
// (D4405).  Constant evaluation is never analyzed: the diagnose it reaches
// is observe (notadragon_wg21 DECISIONS.md E142), so each false predicate is
// a warning there too, and the constant expression does not fail.  Both
// constant evaluators; the bytecode interpreter repeats an observe warning
// of the static_assert (CLANG-617).
//
// The analysis evaluation of a precondition is made at each call (D4405):
// the run-time call in use() is diagnosed, with a note at the predicate;
// the constant evaluation of f(-1) is not analyzed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-constexpr-diagnose.C
// (a constexpr function is inline, and Clang emits -- and so diagnoses --
// one that is never emitted only when something calls it at run time: use()
// here; notadragon_wg21 DECISIONS.md E130)

constexpr int f(int x)
  pre(x > 0)                    // expected-warning {{contract failed during execution of constexpr function}} expected-note {{the predicate of the contract assertion}} bc-warning {{contract failed during execution of constexpr function}}
{
  contract_assert(x > 0);       // expected-warning {{contract failed during execution of constexpr function}} expected-warning {{contract assertion cannot be proven}} bc-warning {{contract failed during execution of constexpr function}}
  return x;
}

static_assert(f(-1) == -1);

int use(int x) { return f(x); } // expected-warning {{contract assertion cannot be proven}}
