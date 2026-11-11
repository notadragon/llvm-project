// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4299 -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s

// With -fcontracts-p4299 in C++, _Pre, _Post and _ContractAssert are
// keywords (notadragon_wg21 DECISIONS.md E28), so none of them can name a
// variable or be used as an expression; pre and post stay ordinary
// identifiers.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-p4299-pre-post-keywords.C,
// which GCC fails for _Pre and _Post (GCC-637: it rewrites them to the
// contextual pre and post).

namespace a {
  int _Pre = 1;   // expected-error {{expected unqualified-id}}
  int _Post = 2;  // expected-error {{expected unqualified-id}}
  int _ContractAssert = 3;  // expected-error {{expected unqualified-id}}
  int g() { return _Pre; }  // expected-error {{expected expression}}
}

namespace b {
  int pre = 4;    // ok, an ordinary identifier
  int post = 5;   // ok

  int f(int x) _Pre(x > 0) _Post(r: r > 0) {
    _ContractAssert(x > 0);
    return pre + post;
  }
}
