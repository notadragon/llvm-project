// DEFINE: %{run} = %clangxx -std=c++26 %s -fcontract-configuration-file=%S/p4451-dynamic.json %libcxx_flags -o %t
//
// RUN: %{run} -fcontracts-p4451 && %t
// RUN: %{run} -fcontracts-p3400 && %t
// RUN: %{run} -fcontracts-p3400 -fcontracts-p4451 && %t

// P4451: a semantic selected at run time (P3595 dynamic selection) is
// adjusted for a terminating contract assertion as for P3400's
// labels::terminating: a selected ignore or observe is evaluated as
// enforce.  Under -fcontracts-p4451 alone, -fcontracts-p3400 alone and
// both.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-run-dynamic.C (and its
// -p3400 and -both variants)

#include <contracts>  // `contract_violation`, `evaluation_semantic`
#include <cstdlib>    // `std::abort`

using std::contracts::evaluation_semantic;

static evaluation_semantic selected;

evaluation_semantic p4451_sel() { return selected; }

static int handled;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.semantic() != evaluation_semantic::enforce)
    std::abort();
  ++handled;
  throw handled;
}

int f(int x) pre<terminating>(x > 0) { return x; }
void g(int x) { contract_assert<terminating>(x > 0); }

int main() {
  for (evaluation_semantic s :
       {evaluation_semantic::ignore, evaluation_semantic::observe,
        evaluation_semantic::enforce}) {
    selected = s;
    try {
      f(-1);
      std::abort();
    } catch (int) {
    }
    try {
      g(-1);
      std::abort();
    } catch (int) {
    }
  }
  if (handled != 6)
    std::abort();
}
