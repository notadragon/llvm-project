// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4298 \
// RUN:   -fcontract-configuration-file=%S/p3595-dynamic-nothrow-compute.json %libcxx_flags -o %t
// RUN: %t

// P3595, DECISIONS.md E137 and E138: under a label whose allowed set holds
// no throwing semantic (neither observe nor enforce), a dynamic selection
// brings none in, also through the label's compute_semantic: the selected
// quick_enforce is computed to observe, which the label does not allow, so
// the selection is invalid -- as the static path rejects the same result --
// and is reported, without evaluating the (true) predicate, as a
// noexcept_enforce violation.  The violation handler throws; the program
// must terminate, never reach the catch.  Without -fcontracts-p4298:
// p3595-dynamic-nothrow-label-compute-no-p4298.cpp.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-nothrow-label-compute.C
#include <contracts>
#include <cstdlib>
#include <exception>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct review_nothrow_t {
  using assertion_control_object = review_nothrow_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::noexcept_observe,
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

static int handled;
static evaluation_semantic seen;
static std::contracts::detection_mode mode;

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  ++handled;
  seen = v.semantic ();
  mode = v.detection_mode ();
  throw 42;
}

static void
on_terminate ()
{
  if (handled == 1 && seen == evaluation_semantic::noexcept_enforce
      && mode == std::contracts::detection_mode::predicate_false)
    std::_Exit (0);
  std::abort ();
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
      std::abort ();	// no exception may leave the check
    }
  std::abort ();
}
