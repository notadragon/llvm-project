// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_observe %libcxx_flags -o %t
// RUN: %t

// P3098 with P4298: an exception from initializing a postcondition capture
// is a violation of kind post_capture with detection mode
// evaluation_exception.  Under noexcept_observe the handler returns and the
// program continues, without checking the postcondition and without
// destroying the capture, which was never made.  (EDG M13c2 shape:
// p3098/capture_exception.)
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3098-capture-init-throw-no-check.C
#include <contracts>

using namespace std::contracts;

static int reports, checks, destroyed, bodies;

void handle_contract_violation (const contract_violation &v)
{
  if (v.kind () == assertion_kind::post_capture
      && v.semantic () == evaluation_semantic::noexcept_observe
      && v.detection_mode () == detection_mode::evaluation_exception)
    ++reports;
  else
    __builtin_abort ();
}

struct Bad
{
  Bad () {}
  Bad (const Bad &) { throw 1; }
  ~Bad () { ++destroyed; }
};

static bool checked () { ++checks; return true; }

void f (const Bad &b) post [c = b] (checked ()) { ++bodies; }

int main ()
{
  {
    Bad b;
    try { f (b); } catch (...) { __builtin_abort (); }
    if (reports != 1 || checks != 0 || destroyed != 0 || bodies != 1)
      __builtin_abort ();
  }
  if (destroyed != 1)	// b itself
    __builtin_abort ();
}
