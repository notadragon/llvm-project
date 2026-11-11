// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-allow-assume -fcontract-evaluation-semantic=ignore %libcxx_flags -O2 -S -emit-llvm -o /dev/null -Rcontract-proof -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405R0, "Previous Assertions and Undefined Behavior" (DECISIONS.md
// E214): an obligation's predicate that calls, out of line, a replaceable
// function whose code holds an unsettled contract evaluation makes the
// proof conditional -- not diagnosed, never assumed -- however free of side
// effects this translation unit's copy is: another translation unit's copy,
// the one the program may run, may evaluate it with observe, whose
// violation handler may have side effects.  Every evaluation here is
// ignore (unsettled) but the prove ones, and each prove predicate holds
// whatever its call returns.
//
// * replaceable: a noinline inline function holding an ignored
//   contract_assert: conditional, not assumed.
// * internal: the same as a static function, an exact definition: judged
//   by its body (no check), sound and assumed.
// * plain: a noinline inline function with no contract evaluation: judged
//   by its body, sound and assumed.
// * via_internal: a static function calling the replaceable one out of
//   line: conditional, not assumed.
// * inlined: an always_inline function holding an ignored contract_assert,
//   inlined into the shadow: decided by the inlining rule, which keeps a
//   proof sound only by making the real code's own calls inline the same
//   copy -- and the real code makes no call of it but the predicate's --
//   so conditional, not assumed (as before E214).
//
// Before E214, replaceable and via_internal were sound and assumed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-unsettled-callee.C

#include <contracts>
using namespace std::contracts::labels;

int sink(int);

[[gnu::noinline]] inline bool checked(int x) {
  contract_assert(x != 5);
  return x > 0;
}

[[gnu::noinline]] static bool checked_internal(int x) {
  contract_assert(x != 5);
  return x > 0;
}

[[gnu::noinline]] inline bool plain(int x) { return x > 0; }

[[gnu::noinline]] static bool via_checked(int x) { return checked(x); }

[[gnu::always_inline]] inline bool checked_inlined(int x) {
  contract_assert(x != 5);
  return x > 0;
}

int replaceable(int x) {
  if (x > 0)
    contract_assert<prove>(checked(x) || x > 0); // expected-remark-re {{proof shadow: proven, conditional{{$}}}}
  return sink(x);
}

int internal(int x) {
  if (x > 0)
    contract_assert<prove>(checked_internal(x) || x > 0); // expected-remark {{proof shadow: proven, sound; assumed}}
  return sink(x);
}

int plain_callee(int x) {
  if (x > 0)
    contract_assert<prove>(plain(x) || x > 0); // expected-remark {{proof shadow: proven, sound; assumed}}
  return sink(x);
}

int via_internal(int x) {
  if (x > 0)
    contract_assert<prove>(via_checked(x) || x > 0); // expected-remark-re {{proof shadow: proven, conditional{{$}}}}
  return sink(x);
}

int inlined(int x) {
  if (x > 0)
    contract_assert<prove>(checked_inlined(x) || x > 0); // expected-remark-re {{proof shadow: proven, conditional{{$}}}}
  return sink(x);
}
