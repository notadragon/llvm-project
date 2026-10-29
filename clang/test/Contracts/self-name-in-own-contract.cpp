// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// The locus of a function's name is immediately after its complete declarator
// ([basic.scope.pdecl]), and the function-contract-specifier-seq follows the
// declarator, so a function can be named in its own contracts.  The parser
// caches a function's contracts and parses them once it is declared; Sema
// then checks them against its earlier declarations as it would have.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/self-name-in-own-contract.C

int rec(int x) pre(x == 0 || rec(x - 1) >= 0);
int rec2(int x) pre(x == 0 || rec2(x - 1) >= 0) { return x; }
constexpr int rec3(const int x) post(r : x == 0 || rec3(x - 1) == r - 1) { return x; }
static_assert(rec3(3) == 3);

template <class T> T trec(T x) pre(x == 0 || trec(x - 1) >= 0) { return x; }
int use_trec = trec(2);

int a, grp(int x) pre(x == 0 || grp(x - 1) >= 0);

struct S {
  int m(int x) const pre(x == 0 || m(x - 1) >= 0);
  static int sm(int x) pre(x == 0 || sm(x - 1) >= 0);
};
int S::m(int x) const pre(x == 0 || m(x - 1) >= 0) { return x; }
int S::sm(int x) pre(x == 0 || sm(x - 1) >= 0) { return x; }

void blk() {
  int local(int x) pre(x == 0 || local(x - 1) >= 0);
}

// Late-parsed contracts are still checked against earlier declarations.
int same(int x) pre(x > 0);
int same(int x) pre(x > 0);
int same(int x) pre(x > 0) { return x; }
int same(int x);

int differ(int x) pre(x > 0); // expected-note {{contract previously specified with a non-equivalent condition}}
int differ(int x) pre(x > 1); // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}

int later(int x); // expected-note {{previously declared without contracts here}}
int later(int x) pre(x > 0); // expected-error {{function redeclaration differs in contract specifier sequence}}

void del(int x) pre(x > 0) = delete; // expected-error {{deleted function 'del' cannot have a function-contract-specifier}}

// A contract on a declarator that declares no function is still diagnosed.
typedef int F(int) pre(true); // expected-error {{'pre' can only appear on a function declaration}}
