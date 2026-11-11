// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// A postcondition that needs the address of the result binding when the
// result is returned in a register (an empty class, a scalar, a small POD),
// and in memory (a large POD, and [dcl.contract.res] Example 2's class with
// a non-trivial copy constructor, for which &r must be the returned object).
// The empty-class half of Example 2 "can fail" and is not asserted on.  GCC
// ICEd on the empty class (PR c++/125574); a regression pin here.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/pr125574.C

#include <contracts>
#include <cstdio>

static int viol = 0;
static int failures = 0;
void handle_contract_violation (const std::contracts::contract_violation&)
{ ++viol; }

static void
check (const char *what, int want)
{
  if (viol != want)
    {
      std::printf ("FAIL: %s: expected %d violations, got %d\n",
		   what, want, viol);
      ++failures;
    }
  viol = 0;
}

static void
check_true (const char *what, bool ok)
{
  if (!ok)
    {
      std::printf ("FAIL: %s\n", what);
      ++failures;
    }
}

/* An empty class: returned in a register, so the result binding needs a
   temporary before its address can be taken.  */
struct Empty { int tag () const { return 7; } };

static int seen = -1;
static bool
look (const Empty &e)
{
  seen = e.tag ();
  return true;
}

static bool
refuse (const Empty &)
{
  return false;
}

/* Binding the result to a reference parameter (GCC's ICE).  */
Empty by_ref () post (r : look (r)) { return {}; }

/* A member call on the result needs the address just the same.  */
Empty by_member () post (r : r.tag () == 7) { return {}; }

/* GCC's reported shape: two postconditions, only one addressing r.  */
Empty two_posts () post (r : true) post (r : look (r)) { return {}; }

/* The check must be real, not vacuously true.  */
Empty violates () post (r : refuse (r)) { return {}; }

/* Controls.  A scalar and a small POD are also register-returned; a
   large POD is returned in memory.  */
struct SmallPod { int a, b; };
struct BigPod { int a[8]; };

static bool ok_int (const int &i) { return i == 3; }
static bool ok_small (const SmallPod &p) { return p.a == 1 && p.b == 2; }
static bool ok_big (const BigPod &p) { return p.a[0] == 5; }

int scalar () post (r : ok_int (r)) { return 3; }
SmallPod small_pod () post (r : ok_small (r)) { return { 1, 2 }; }
BigPod big_pod () post (r : ok_big (r)) { return { { 5 } }; }

/* [dcl.contract.res] Example 2.  A class with a non-trivial copy
   constructor is returned in memory, so no temporary is introduced and the
   check is required to succeed.  The example's other half -- the empty
   class -- is deliberately not asserted on: the standard says that check
   "can fail" for a register-returned result, for which an
   implementation may introduce a temporary.  */
struct NonTrivial
{
  NonTrivial () { }
  NonTrivial (const NonTrivial &) { }
};

static bool addr_matched = false;

/* Recorded from a called function: a predicate constifies everything it
   names, so it cannot assign to addr_matched itself.  */
static bool
record_addr (const NonTrivial *got, const NonTrivial *want)
{
  addr_matched = (got == want);
  return true;
}

NonTrivial
in_memory (NonTrivial *const ptr)
  post (r : record_addr (&r, ptr))
{
  return {};
}

int
main ()
{
  Empty e = by_ref ();
  (void) e;
  check ("empty class bound to a reference parameter", 0);
  check_true ("predicate saw the returned object", seen == 7);

  Empty m = by_member ();
  (void) m;
  check ("member call on the result binding", 0);

  seen = -1;
  Empty t = two_posts ();
  (void) t;
  check ("two postconditions, the second addressing r", 0);
  check_true ("second postcondition saw the returned object", seen == 7);

  Empty v = violates ();
  (void) v;
  check ("empty-class postcondition that fails", 1);

  if (scalar () != 3) __builtin_abort ();
  check ("scalar control", 0);
  SmallPod sp = small_pod ();
  if (sp.a != 1 || sp.b != 2) __builtin_abort ();
  check ("small POD control", 0);
  BigPod bp = big_pod ();
  if (bp.a[0] != 5) __builtin_abort ();
  check ("large POD control", 0);

  NonTrivial nt = in_memory (&nt);
  (void) nt;
  check ("[dcl.contract.res] Example 2, returned in memory", 0);
  check_true ("Example 2: &r == ptr for a memory-returned result",
	      addr_matched);

  return failures;
}
