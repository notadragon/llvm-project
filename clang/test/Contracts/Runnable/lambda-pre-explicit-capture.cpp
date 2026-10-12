// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// A precondition on a lambda naming one of the lambda's explicit captures:
// by reference, by copy, and an init-capture; a by-reference capture tracks
// its referent; pre and post together; controls.  GCC ICEd on the
// precondition (PR c++/126038); a regression pin here.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/pr126038.C

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
check_eq (const char *what, int got, int want)
{
  if (got != want)
    {
      std::printf ("FAIL: %s: expected %d, got %d\n", what, want, got);
      ++failures;
    }
}

/* A captured `this' in a lambda predicate: lambda-capture-this-predicate.cpp.  */

int
main ()
{
  int a = 1, b = 2;

  /* GCC's reported case: a by-reference capture named in the
     precondition, with a by-copy capture used in the body.  */
  auto by_ref = [&a, b] (int v) pre (v > a) { return v + b; };
  check_eq ("by-reference capture in a precondition", by_ref (5), 7);
  check ("by-reference capture, precondition satisfied", 0);
  by_ref (0);
  check ("by-reference capture, precondition violated", 1);

  /* A by-copy capture named in the precondition.  */
  auto by_copy = [a] (int v) pre (v > a) { return v; };
  check_eq ("by-copy capture in a precondition", by_copy (5), 5);
  check ("by-copy capture, precondition satisfied", 0);
  by_copy (0);
  check ("by-copy capture, precondition violated", 1);

  /* An init-capture named in the precondition.  */
  auto by_init = [c = 1] (int v) pre (v > c) { return v + c; };
  check_eq ("init-capture in a precondition", by_init (5), 6);
  check ("init-capture, precondition satisfied", 0);
  by_init (0);
  check ("init-capture, precondition violated", 1);

  /* A by-reference capture must see later changes to the captured
     object, in the predicate as much as in the body.  */
  a = 10;
  by_ref (5);
  check ("by-reference capture tracks the referent", 1);
  a = 1;

  /* Both a precondition and a postcondition on the same lambda.  */
  auto both = [a] (int v) pre (v > a) post (r : r > a) { return v + 1; };
  check_eq ("pre and post on one lambda", both (5), 6);
  check ("pre and post on one lambda, both satisfied", 0);

  /* A capture in a postcondition, as a control.  */
  auto post_only = [&a] (int v) post (r : r > a) { return v; };
  check_eq ("capture in a postcondition control", post_only (5), 5);
  check ("capture in a postcondition control, satisfied", 0);
  post_only (0);
  check ("capture in a postcondition control, violated", 1);

  /* A lambda with contracts but no captures at all, as a control.  */
  auto no_cap = [] (int v) pre (v > 0) { return v; };
  check_eq ("no captures control", no_cap (5), 5);
  check ("no captures control, satisfied", 0);

  return failures;
}
