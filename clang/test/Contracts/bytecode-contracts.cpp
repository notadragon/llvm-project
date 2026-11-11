// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fexperimental-new-constant-interpreter -fsyntax-only -verify %s

// The bytecode constant interpreter evaluates contract assertions as the
// default evaluator does: a contract_assert, preconditions, and
// postconditions with a result name (of scalar, class and reference type) and
// with captures.  Both RUN lines share one set of expectations.

constexpr int ca(int x) { contract_assert(x > 1); return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int ca_ok = ca(5);
constexpr int ca_bad = ca(0); // expected-error {{constexpr variable 'ca_bad' must be initialized by a constant expression}} expected-note {{in call to 'ca(0)'}}

constexpr int f(int x) pre(x > 10) { return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int f_ok = f(11);
constexpr int c = f(5); // expected-error {{constexpr variable 'c' must be initialized by a constant expression}} expected-note {{in call to 'f(5)'}}

constexpr int g(int x) post(r : r > 10) { return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int g_ok = g(11);
constexpr int d = g(5); // expected-error {{constexpr variable 'd' must be initialized by a constant expression}} expected-note {{in call to 'g(5)'}}

// Several returns, each through the postconditions.
constexpr int abs_(const int x) post(r : r >= 0) post(r2 : r2 == x || r2 == -x) {
  if (x < 0)
    return -x;
  return x;
}
static_assert(abs_(-3) == 3 && abs_(4) == 4);

// A void function flows off its end into its postconditions.
constexpr void v(const int x) post(x > 10) {} // expected-error {{contract failed during execution of constexpr function}}
constexpr int use_v(int x) { v(x); return x; } // expected-note {{in call to 'v(5)'}}
constexpr int v_ok = use_v(11);
constexpr int v_bad = use_v(5); // expected-error {{constexpr variable 'v_bad' must be initialized by a constant expression}} expected-note {{in call to 'use_v(5)'}}

// A class result: the result name denotes the returned object.
struct S { int v; };
constexpr S mk(int x) post(r : r.v > 10) { return S{x}; } // expected-error {{contract failed during execution of constexpr function}}
constexpr S mk_ok = mk(11);
constexpr S mk_bad = mk(5); // expected-error {{constexpr variable 'mk_bad' must be initialized by a constant expression}} expected-note {{in call to 'mk(5)'}}

// A reference result.
constexpr int eleven = 11, five = 5;
constexpr const int &ref(const int &x) post(r : r > 10) { return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int ref_ok = ref(eleven);
constexpr int ref_bad = ref(five); // expected-error {{constexpr variable 'ref_bad' must be initialized by a constant expression}} expected-note {{in call to 'ref(five)'}}

// A postcondition capture holds the value at entry.
constexpr int inc(int x) post [old = x] (r : r == old + 1) { x = x + 1; return x; }
static_assert(inc(4) == 5);
constexpr int dec(int x) post [old = x] (r : r == old + 1) { return x - 1; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int dec_bad = dec(4); // expected-error {{constexpr variable 'dec_bad' must be initialized by a constant expression}} expected-note {{in call to 'dec(4)'}}
