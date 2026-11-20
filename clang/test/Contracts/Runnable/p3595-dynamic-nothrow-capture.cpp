// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontracts-p3400 -fcontracts-p4298 \
// RUN:   -fcontract-configuration-file=%S/p3595-dynamic-nothrow-label.json %libcxx_flags -o %t
// RUN: %t

// P3595, P3098, DECISIONS.md E137 and E138: a postcondition with captures
// whose semantic a dynamic selector chooses, under a label whose allowed
// set holds no throwing semantic (neither observe nor enforce): with
// -fcontracts-p4298 an invalid selection is reported, without initializing
// the captures or evaluating the (true) predicate, as a noexcept_enforce
// violation.  (Clang chooses the semantic once, where the captures would be
// initialized, and the postcondition's check reports the invalid choice;
// gnu_gcc reports it where the captures are initialized.)  The violation
// handler throws; the program must terminate, never reach the catch.
// Without -fcontracts-p4298: p3595-dynamic-nothrow-capture-no-p4298.cpp.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-nothrow-capture.C
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

int f (const int i) post<nothrow_label> [c = i] (c == i) { return i; }

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
