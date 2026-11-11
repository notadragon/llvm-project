// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontract-configuration-file=%S/p3100-noexcept-operator.json \
// RUN:   -fsyntax-only -verify %s

// P3100: an implicit contract assertion must never change the result of the
// noexcept operator, whatever evaluation semantic is selected for it.  Clang
// builds the implicit UB guards in CodeGen, so it was already correct here; this
// test locks the behavior in and mirrors the GCC test that drove the fix
// (g++.dg/contracts/cpp26/p3100-noexcept-operator.C).  The config assigns a
// different checking semantic to each check to prove the transparency is
// semantic-independent.

// expected-no-diagnostics

int    g_arr[3] = { };

// ---- front-end-style checks (these carried the GCC bug) ----
void div_zero (int d)       { static_assert (noexcept (6 / d)); }     // enforce
void mod_zero (int d)       { static_assert (noexcept (6 % d)); }     // enforce
void div_ovf  (int a, int b){ static_assert (noexcept (a / b)); }     // quick_enforce
void shl      (int x, int s){ static_assert (noexcept (x << s)); }    // noexcept_enforce
void shr      (int x, int s){ static_assert (noexcept (x >> s)); }    // noexcept_enforce
void fcast    (double f)    { static_assert (noexcept (int (f))); }   // noexcept_observe

// ---- other implicit checks, locked in ----
void add_ovf  (int a, int x){ static_assert (noexcept (a + x)); }
void sub_ovf  (int a, int x){ static_assert (noexcept (a - x)); }
void mul_ovf  (int a, int x){ static_assert (noexcept (a * x)); }
void bounds   (int i)       { static_assert (noexcept (g_arr[i])); }
void deref    (int *p)      { static_assert (noexcept (*p)); }

// ---- negative controls: transparency must not over-suppress real throwers ----
int thrower ();                                  // potentially throwing
void nc1 ()                 { static_assert (!noexcept (thrower ())); }
void nc2 (int d)            { static_assert (!noexcept (thrower () / d)); }
