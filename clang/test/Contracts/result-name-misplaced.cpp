// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A result-name introducer is only meaningful on a postcondition.  It is
// nevertheless parsed on every contract kind so that the real error can be
// reported, instead of the identifier falling through to the predicate and
// producing an unrelated "use of undeclared identifier".

int f(int x) pre(r: r > 0) { return x; }
// expected-error@-1 {{result name 'r' not allowed outside of post condition specifier}}

void g(int x) {
  contract_assert(r: r > 0);
  // expected-error@-1 {{result name 'r' not allowed outside of post condition specifier}}
}

// A result name on a void-returning function is separately diagnosed.
void h() post(r: r > 0) {}
// expected-error@-1 {{result name 'r' cannot be used with a void return type}}

// The valid case still works.
int ok(int x) post(r: r > 0) { return x; }
