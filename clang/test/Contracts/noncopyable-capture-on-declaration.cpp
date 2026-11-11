// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s

// A postcondition capture must be copy-initializable from its initializer,
// and a capture that is not is diagnosed on any declaration, at the capture,
// and only once for a function that is also defined.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/noncopyable-capture-on-declaration.C

struct NC { NC(int) {} NC(const NC &) = delete; }; // expected-note 4 {{explicitly marked deleted here}}

int f(NC p) post [p] (true);     // expected-error {{call to deleted constructor of 'NC'}}
int g(NC p) post [q = p] (true); // expected-error {{call to deleted constructor of 'NC'}}

int h(NC p) post [p] (true)      // expected-error {{call to deleted constructor of 'NC'}}
{
  return 0;
}

auto l = [](NC p) post [p] (true) { return 0; }; // expected-error {{call to deleted constructor of 'NC'}}

// Copyable captures, and a capture in a template whose type is dependent,
// are unaffected.
struct C { C(int) {} };
int ok(C p) post [p] (true);
template <class T> int t(T p) post [p] (true);
