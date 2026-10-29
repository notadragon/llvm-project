// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-configuration-file=%S/p3595-dynamic-nothrow-compute-no-p4298.json %libcxx_flags -o %t
// RUN: not --crash %t

// P3595, DECISIONS.md E138: without -fcontracts-p4298, a compute_semantic
// result outside the allowed set is an invalid dynamic selection, as the
// static path rejects it, and under a label whose allowed set holds no
// throwing semantic it traps as quick_enforce.  The label allows ignore,
// noexcept_observe and quick_enforce (without the flag, ignore and
// quick_enforce) and computes quick_enforce to observe; the configured
// ignore is the compile-time semantic, and the selector returns
// quick_enforce.  The violation handler is not called and the (true)
// predicate is not evaluated.  With -fcontracts-p4298:
// p3595-dynamic-nothrow-label-compute.cpp.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-nothrow-label-compute-no-p4298.C
#include <contracts>
#include <cstdlib>
#include <exception>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct review_nothrow_t {
  using assertion_control_object = review_nothrow_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::ignore, evaluation_semantic::noexcept_observe,
     evaluation_semantic::quick_enforce};
  constexpr evaluation_semantic
  compute_semantic (evaluation_semantic s) const
  {
    return (s == evaluation_semantic::quick_enforce
	    ? evaluation_semantic::observe : s);
  }
};
constexpr review_nothrow_t review_nothrow{};

evaluation_semantic
p3595_nothrow_compute_sel ()
{
  return evaluation_semantic::quick_enforce;
}

// quick_enforce calls no handler: a call is a failure, and exiting normally
// is how a dg-shouldfail test reports one.  So is an exception reaching the
// catch, a terminate, or returning from f.
void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::_Exit (0);
}

[[noreturn]] static void
on_terminate ()
{
  std::_Exit (0);
}

void f (int x) pre<review_nothrow> (x > 0) { }

int main ()
{
  std::set_terminate (on_terminate);
  try
    {
      f (1);
    }
  catch (...)
    {
      std::_Exit (0);
    }
  return 0;
}
