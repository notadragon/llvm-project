// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -O2 -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -O0 -emit-llvm-only -verify %s

// D4405's callee premise copies (CLANG-913, fixed): g's premise copy
// assumes nothing of g's postcondition, whose capture (P3098) is emitted
// only where the postcondition is checked -- as a proof shadow defers no
// evaluation with captures -- so the compiler no longer stops with
// "DeclRefExpr for Decl not entered in LocalDeclMap?"; the postcondition is
// unproven.  At -O0 too, which makes premise copies at the default proof
// level.

int cap(int x);
int g(int x) pre(x > 0) post [c = cap(x)] (r: r > c) { return x; }
// expected-error@-1 {{contract assertion cannot be proven}}
// expected-note@-2 {{the predicate of the contract assertion}}

int use(int y) { return g(y); } // expected-error {{contract assertion cannot be proven}}
