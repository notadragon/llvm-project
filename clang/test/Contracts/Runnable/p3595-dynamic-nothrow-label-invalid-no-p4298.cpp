// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-configuration-file=%S/p3595-dynamic-nothrow-label.json %libcxx_flags -o %t
// RUN: not --crash %t

// P3595, DECISIONS.md E138: without -fcontracts-p4298, an invalid dynamic
// selection under a label whose allowed set holds no throwing semantic
// (neither observe nor enforce) traps as quick_enforce: the violation
// handler is not called and the (true) predicate is not evaluated.  With
// -fcontracts-p4298: p3595-dynamic-nothrow-label-invalid.cpp.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-nothrow-label-invalid-no-p4298.C
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

// Not a semantic: an invalid selection.
evaluation_semantic
p3595_nothrow_sel ()
{
  return evaluation_semantic (42);
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

void f (int x) pre<nothrow_label> (x > 0) { }

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
