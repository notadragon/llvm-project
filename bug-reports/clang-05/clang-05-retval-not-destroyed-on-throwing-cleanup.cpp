// clang-05-retval-not-destroyed-on-throwing-cleanup.cpp              -*-C++-*-
//
// CLANG-5: when a local's destructor throws AFTER the by-value return object
// has been constructed, Clang never destroys the return object.  GCC does.
//
//   clang++ -std=c++17 clang-05-retval-not-destroyed-on-throwing-cleanup.cpp
//   ./a.out; echo $?      -> 1   (constructed once, never destroyed)
//
//   g++     -std=c++17 clang-05-retval-not-destroyed-on-throwing-cleanup.cpp
//   ./a.out; echo $?      -> 0
//
// PLAIN C++17.  No contracts, no `-fcontracts', no P3850 anything -- which is
// why this is filed here rather than being ours to fix.  Verified against
// stock system g++ 13.3.0 (Ubuntu 24.04) as well as our branch GCC, both 0;
// our branch Clang gives 1.
//
// GCC implements this deliberately: `cp/except.cc' carries a whole
// `current_retval_sentinel' mechanism whose stated purpose is to "clean up the
// return value if a local destructor throws".  Clang has no equivalent.
//
// FOUND 2026-08-25 while fixing GCC-5 (a contracts bug in GCC's use of exactly
// that sentinel machinery).  This reproducer is the no-contract control from
// that work, run on Clang for comparison.
//
// THE STANDARD REQUIRES GCC'S BEHAVIOUR -- settled 2026-08-25.  [except.ctor]/2:
//
//     "If an exception is thrown during the destruction of temporaries or
//      local variables for a return statement, the destructor for the
//      returned object (if any) is also invoked."
//
// which is exactly this program.  That sentence exists because the returned
// object is not an object of automatic storage duration in the callee, so the
// paragraph's first sentence does not reach it.  So this is a real bug, not a
// divergence to document, and the report should lead with that quote.
//
// PROVENANCE: MEASURED 2026-09-01, and it is upstream's.  The old note here
// said there was no stock clang on this box and that public Compiler Explorer
// would be needed; that stopped being true once stock release toolchains
// were installed.  Run locally against them:
//
//     stock clang++ 18.1.0, 19.1.0, 20.1.0, 21.1.0, 22.1.6  ->  exit 1 (leaks)
//     stock g++     13.4.0, 14.4.0, 15.3.0, 16.2.0          ->  exit 0 (correct)
//
// RE-RUN 2026-09-02, adding the two trunk nightlies the earlier pass omitted:
//
//     clang++-trunk 24.0.0git 4c176c47d8be         ->  exit 1 (leaks)
//     g++-trunk     17.0.0 20260901 (experimental) ->  exit 0 (correct)
//
// So the bug spans six major Clang versions INCLUDING TODAY'S UPSTREAM TRUNK,
// with five GCC versions as the contrasting control, and it is nothing to do
// with this branch.  The trunk row is the one a filing needs -- "does it still
// reproduce on main?" is the first question upstream asks.
// NOTHING IS OWED BEFORE FILING ANY MORE.
//
// Related but NOT the same defect, and resolved the OTHER way: GCC-6
// (an internal-notes case in the gnu_gcc fork, not migrated to its
// bug-reports/ since it was reclassified as a wording gap needing a CWG
// issue, not a compiler bug) is GCC leaking the return object when a
// contract-violation handler throws out of a postcondition check.  There the
// exception escapes a postcondition assertion, which [except.ctor]/2 does
// not mention and [stmt.return]/5 sequences AFTER local destruction, so
// nothing requires the destructor to run and GCC conforms -- that one is a
// wording gap for CWG, not a compiler bug.  Here the exception escapes a
// local variable's destructor, which the paragraph names.  Clang leaks in
// both, but only this one is Clang's fault.

int live = 0;

struct Counted {
  int v;
  Counted (int x) : v (x) { ++live; }
  Counted (const Counted &o) : v (o.v) { ++live; }
  ~Counted () { --live; }
};

struct ThrowOnDestroy {
  bool armed;
  ~ThrowOnDestroy () noexcept (false) { if (armed) throw 42; }
};

Counted
f (bool arm)
{
  ThrowOnDestroy guard { arm };
  Counted result (7);
  return result;                // built, then `guard' throws on the way out
}

int
main ()
{
  try { f (true); } catch (int) { }
  return live;                  // GCC: 0.  Clang: 1.
}
