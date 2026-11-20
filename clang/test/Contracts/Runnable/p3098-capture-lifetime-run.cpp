// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -o %t
// RUN: %t

// P3098: when a postcondition capture is made and destroyed, relative to
// the function's locals, its checks and its exceptions.  A local is
// destroyed, then the postcondition is checked, then the capture is
// destroyed; captures are made in declaration order with the
// preconditions; a capture is destroyed once when an exception leaves the
// body, including from a function-try-block's handler that rethrows, and
// once after the check when that handler returns.  (EDG M13c2 shapes:
// p3098/captures_run, capture_unwind, openbugs/edg-86-capture-unwind.)
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3098-capture-lifetime-run.C
#include <cstring>

// One letter per event: C capture copied, D capture destroyed, L local
// destroyed, K postcondition checked, H handler entered, X exception caught
// by the caller; 1, 2 preconditions, c capture made, p postcondition.
static char events[128];
static int nevents;
static void say (char c) { events[nevents++] = c; }
static bool note (char c) { say (c); return true; }

struct Box
{
  int v;
  Box (int x) : v (x) {}
  Box (const Box &o) : v (o.v) { say ('C'); }
  ~Box () { say ('D'); }
};
struct Local { ~Local () { say ('L'); } };
static bool may_throw (int v) { if (v < 0) throw 2; return true; }

int grow (Box &b) post [old = b] (note ('K') && b.v > old.v)
{
  Local l;
  b.v += 1;
  return b.v;
}

void interleave (const int x)
  pre (note ('1'))
  post [c = (note ('c'), x)] (note ('p') && c == x)
  pre (note ('2'))
{}

void f (const Box &b, const int n) post [c = b] (note ('K') && c.v == 1)
{
  Local l;
  if (n == 1)
    throw 1;
}

// A capture whose initializer might throw.
void g (const Box &b, const int n) post [c = b, d = may_throw (n)] (d)
{
  if (n == 1)
    throw 1;
}

// A function-try-block whose handler rethrows, and one whose handler returns.
void h (const Box &b, const int n) post [c = b] (note ('K') && c.v == 1)
try { if (n > 0) throw 1; }
catch (int) { say ('H'); if (n == 1) throw; }

static bool expect (const char *s)
{
  bool ok = nevents == (int) std::strlen (s)
	    && std::memcmp (events, s, nevents) == 0;
  nevents = 0;
  return ok;
}

int main ()
{
  Box grown (1);
  grow (grown);
  if (!expect ("CLKD")) __builtin_abort ();
  interleave (3);
  if (!expect ("1c2p")) __builtin_abort ();

  Box b (1);
  try { f (b, 1); } catch (int) { say ('X'); }
  if (!expect ("CLDX")) __builtin_abort ();
  f (b, 2);
  if (!expect ("CLKD")) __builtin_abort ();
  try { g (b, 1); } catch (int) { say ('X'); }
  if (!expect ("CDX")) __builtin_abort ();
  g (b, 0);
  if (!expect ("CD")) __builtin_abort ();
  try { h (b, 1); } catch (int) { say ('X'); }
  if (!expect ("CHDX")) __builtin_abort ();
  h (b, 2);
  if (!expect ("CHKD")) __builtin_abort ();
}
