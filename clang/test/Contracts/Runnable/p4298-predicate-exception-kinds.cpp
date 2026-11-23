// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_observe %libcxx_flags -o %t
// RUN: %t

// P4298: under noexcept_observe an exception from evaluating a predicate is
// reported to the violation handler with detection mode
// evaluation_exception, for a precondition, a contract_assert and a
// postcondition alike; the handler returns and the program continues, and
// the exception never reaches the caller.  (EDG M13c2 shape:
// p4298cgen/predicate_exception.)
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p4298-predicate-exception-kinds.C
#include <contracts>

using namespace std::contracts;

static assertion_kind kinds[8];
static int reports;

void handle_contract_violation (const contract_violation &v)
{
  if (v.semantic () != evaluation_semantic::noexcept_observe
      || v.detection_mode () != detection_mode::evaluation_exception
      || reports == 8)
    __builtin_abort ();
  kinds[reports++] = v.kind ();
}

static bool boom () { throw 3; }

int f (int x) pre (boom ()) { return x; }
int g (int x) { contract_assert (boom ()); return x; }
int h (const int x) post (boom ()) { return x; }

int main ()
{
  int sum = 0;
  try { sum = f (1) + g (2) + h (3); }
  catch (...) { __builtin_abort (); }
  if (sum != 6 || reports != 3
      || kinds[0] != assertion_kind::pre
      || kinds[1] != assertion_kind::assert
      || kinds[2] != assertion_kind::post)
    __builtin_abort ();
}
