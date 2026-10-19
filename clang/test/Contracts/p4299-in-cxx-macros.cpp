// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4299 -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s

// -fcontracts-p4299 in C++: _Pre, _Post and _ContractAssert are keywords, so
// none can name a variable; a macro named _Pre is expanded as a macro, a _Pre
// produced by a macro's expansion is read as pre, and pre and post stay
// ordinary identifiers.  (From the EDG fork's p4299/keyword and
// spellings_option.)  Constant evaluation shows the assertions are attached.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4299-in-cxx-macros.C (GCC
// still reads _Pre and _Post as identifiers there, a GCC defect:
// open-bug-p4299-pre-post-keywords.C).

int _ContractAssert = 1; // expected-error {{expected unqualified-id}}
int _Pre = 1;            // expected-error {{expected unqualified-id}}
int _Post = 1;           // expected-error {{expected unqualified-id}}

#define _Pre post
constexpr int f(int x) _Pre(r : r > 0) { return x; }
#undef _Pre
#define PRECONDITION(c) _Pre(c)
constexpr int h(int x) PRECONDITION(x > 0) { return x; }

constexpr int vf = f(-1); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'f(-1)'}}
// expected-error@18 {{contract failed during execution of constexpr function}}
constexpr int vh = h(-1); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'h(-1)'}}
// expected-error@21 {{contract failed during execution of constexpr function}}

int pre = 1, post = 2; // OK, ordinary identifiers
int g() { return pre + post; }
