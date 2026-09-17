// Documented limitation (mirror of gnu_gcc F31 /
// contract-friend-deferred-mismatch.C): a contract mismatch between two friend
// declarations of the same function -- whose contracts are still deferred at the
// redeclaration-merge point -- is silently accepted rather than diagnosed,
// unlike the ordinary (non-deferred) redeclaration path, which correctly errors
// with "function redeclaration differs in contract specifier sequence".
//
// Both compilers share this limitation for this degenerate two-friend-decls
// construct. This test pins the current (accepting) behavior; if Clang starts
// diagnosing the mismatch the FIXME case below will produce an unexpected
// diagnostic and fail -verify, prompting this test (and note) to be updated.
//
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3850 -fsyntax-only -verify %s
// expected-no-diagnostics

struct C {
  friend int f(int x) pre(x > 0);
  // FIXME: this mismatched contract on a second friend declaration should be
  // diagnosed (as the non-friend redeclaration path is), but the deferred-parse
  // friend path currently accepts it. See gnu_gcc F31.
  friend int f(int x) pre(x < 0);
};
