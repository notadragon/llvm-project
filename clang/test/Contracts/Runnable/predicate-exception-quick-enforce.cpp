// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -o %t
// RUN: %t
// REQUIRES: target={{x86_64-.*-linux.*}}

// An exception from evaluating a predicate is a contract violation, which
// quick_enforce handles by trapping, for a precondition, a contract_assert
// and a postcondition alike: the exception never reaches the caller's
// handler.  A predicate that catches its own exception holds, and a local of
// a function whose postcondition throws is destroyed once.  (EDG M13c2
// shape: codegen/predicate_exception.)
//
// Each trap is caught as SIGILL and resumed with siglongjmp, so one run
// covers every assertion kind.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/predicate-exception-quick-enforce.C
#include <csetjmp>
#include <csignal>

static sigjmp_buf env;
static volatile int throw_at;
static int destroyed;

static void on_trap (int) { siglongjmp (env, 1); }

static bool check (int which)
{
  if (which == throw_at)
    throw 1;
  return true;
}

static bool absorbed ()
{
  try { throw 2; } catch (int) {}
  return true;
}

struct D { ~D () { ++destroyed; } };

int f (int x) pre (check (1)) { return x; }
int g (int x) { contract_assert (check (2)); return x; }
int h (const int x) post (check (3) && absorbed ()) { D d; return x; }

int main ()
{
  std::signal (SIGILL, on_trap);
  int traps = 0;
  for (int which = 0; which <= 3; ++which)
    {
      throw_at = which;
      if (sigsetjmp (env, 1) == 0)
	{
	  try { f (1); g (2); h (3); }
	  catch (...) { __builtin_abort (); }
	  if (which != 0)
	    __builtin_abort ();
	}
      else if (which == 0)
	__builtin_abort ();
      else
	++traps;
    }
  // h's local is destroyed before its postcondition is checked: in the run
  // that throws nowhere and in the run whose postcondition throws.
  if (traps != 3 || destroyed != 2)
    __builtin_abort ();
}
