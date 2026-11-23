// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3099 -fsyntax-only -verify %s

// P3099: the diagnostic message appears in a constant-evaluation contract
// violation.  (GCC mirror: g++.dg/contracts/cpp26/p3099-message-constexpr.C;
// GCC and Clang phrase the diagnostic differently but both include the text.)

constexpr int f(int x) pre(x > 0, "must be positive") { // expected-error {{contract failed during execution of constexpr function: must be positive}}
    return x;
}

constexpr int y = f(-1); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'f(-1)'}}
