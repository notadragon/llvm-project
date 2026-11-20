// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -triple x86_64-unknown-linux-gnu -emit-llvm-only
// RUN: %{cc1} -O0 -verify=expected,inline %s
// RUN: %{cc1} -O1 -verify=expected,inline %s
// RUN: %{cc1} -O2 -verify=expected,inline %s
// RUN: %{cc1} -O3 -verify=expected,inline %s
// RUN: %{cc1} -O1 -fcontracts-prove-optimize=1 -verify=expected,inline %s
// RUN: %{cc1} -O2 -fcontracts-prove-optimize=1 -verify=expected,inline %s
// RUN: %{cc1} -O0 -fcontracts-prove-optimize=1 -verify=expected,inline %s
// RUN: %{cc1} -O0 -fno-inline-functions -verify=expected,noinline %s
// RUN: %{cc1} -O1 -fno-inline-functions -verify=expected,noinline %s
// RUN: %{cc1} -O2 -fno-inline-functions -verify=expected,noinline %s
// RUN: %{cc1} -O3 -fcontracts-prove-optimize=2 -fno-inline-functions -verify=expected,noinline %s
// RUN: %{cc1} -O0 -finline-hint-functions -verify=expected,noinline %s
// RUN: %{cc1} -O2 -finline-hint-functions -verify=expected,noinline %s
// REQUIRES: x86-registered-target

// D4405's proof optimization level (notadragon_wg21 DECISIONS.md
// E162-E168): a proof at a level other than the build's inlines what the
// level's inliner would, and gets the same results as at the build's own
// level.  add1, an extern function, is inlined into r's proof shadow, and
// r's assertion proven, at -O0 and -O1 proving at level 2 as at -O2, and at
// level 1 in an -O2 or -O0 build as in an -O1 build; with
// -fno-inline-functions or -finline-hint-functions (add1 is not declared
// inline) it is not, at any level.  helper, a large static function called
// once, is inlined into the real q at -O2, but its call in q's shadow is
// a second call, which does not get the inliner's bonus for the last call
// of a static function: q's assertion is not proven at any level, nor at
// the build's own (gnu_gcc proves it at -O2, where its inliner inlines
// helper into all its callers, and, after GCC-999's fix, at every level --
// compilers may prove different things).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove-optimize-called-once.C

int ext(int);

static int helper(int v) {
  int s = v;
  for (int i = 0; i < 4; ++i)
    s += ext(i) * ext(s);
  s += ext(s) + ext(s + 1) + ext(s + 2) + ext(s + 3) + ext(s + 4);
  s += ext(s) + ext(s + 1) + ext(s + 2) + ext(s + 3) + ext(s + 4);
  s += ext(s) + ext(s + 1) + ext(s + 2) + ext(s + 3) + ext(s + 4);
  s += ext(s) + ext(s + 1) + ext(s + 2) + ext(s + 3) + ext(s + 4);
  return v < 0 ? 0 : v + (s & 1);
}

int q(int v) {
  int c = helper(v);
  contract_assert(c != v - 1); // expected-warning {{contract assertion cannot be proven}}
  return c;
}

int add1(int v) { return v + 1; }

int r(int v) {
  int c = add1(v);
  contract_assert(c != v); // noinline-warning {{contract assertion cannot be proven}}
  return c;
}
