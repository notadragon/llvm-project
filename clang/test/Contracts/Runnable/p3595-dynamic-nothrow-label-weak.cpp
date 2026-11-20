// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4298 \
// RUN:   -fcontract-configuration-file=%S/p3595-dynamic-nothrow-label.json %libcxx_flags -o %t
// RUN: %t

// P3595, DECISIONS.md E137: under a label whose allowed set holds no
// throwing semantic (neither observe nor enforce), a dynamic selection
// brings none in, so no exception leaves the check.  With no selector of
// its own the program uses the weak default, which returns the configured
// semantic, adjusted to the label by the check.  The violation handler
// throws; the program must terminate, never reach the catch.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-nothrow-label-weak.C
#include <contracts>
#include <cstdlib>
#include <exception>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct nothrow_t {
  using assertion_control_object = nothrow_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::noexcept_enforce, evaluation_semantic::quick_enforce};
};
constexpr nothrow_t nothrow_label{};

// No selector: the weak default returns the configured enforce, which the
// check adjusts to the label (noexcept_enforce), as the static path does.

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

void f (int x) pre<nothrow_label> (x > 0) { }

int main ()
{
  std::set_terminate (on_terminate);
  try
    {
      f (-1);
    }
  catch (...)
    {
      std::abort ();	// no exception may leave the check
    }
  std::abort ();
}
