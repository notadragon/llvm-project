// RUN: %clang_cc1 -std=c++26 -fcxx-exceptions -fexceptions -fsyntax-only -verify %s

// Referring to a non-static member in the handler of a constructor's
// function-try-block is undefined ([except.handle]/10).  GCC's constant
// evaluation accepts it (stock, gnu_gcc GCC-755, deferred), and its watch
// test is the program below.  Clang cannot reach the handler in constant
// evaluation: it does not implement throwing during constant evaluation
// (P3068, "No" in clang/www/cxx_status.html), so the throw itself ends the
// evaluation.  This pins that rejection and Clang's -Wexceptions warning on
// the member reference; when P3068 lands, the handler's use of `x` is what
// must reject the program.  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-function-try-block-handler-member.C

struct S {
  int x;
  constexpr S(bool b) try : x(1) { if (b) throw 0; } // expected-note {{subexpression not valid in a constant expression}}
  catch (...) { int y = x; (void)y; } // expected-warning {{cannot refer to a non-static member from the handler of a constructor function try block}}
};
constexpr bool f() { try { S s(true); } catch (...) {} return true; } // expected-note {{in call to 'S(true)'}}
static_assert(f()); // expected-error {{not an integral constant expression}} \
                    // expected-note {{in call to 'f()'}}
