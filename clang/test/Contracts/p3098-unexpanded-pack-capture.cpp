// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce -emit-llvm-only -verify %s
// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -fdiagnostics-parseable-fixits %s 2>&1 | FileCheck %s

// A P3098 postcondition capture naming a function parameter pack unexpanded
// (`post [xs]`), or an init-capture that is not a pack whose initializer
// names one (`post [y = xs]`), is diagnosed in the template definition, as a
// lambda's `[xs]` and `[y = xs]` are.  The instantiation then has no capture
// and compiles.  The expanded forms are in p3098-unexpanded-pack-predicate.cpp.
//
// The ellipsis goes after the name of a simple capture (`[xs...]`) and before
// the name of an init-capture (`[...y = xs]`), as in a lambda (D3098R3's
// grammar); on the other side it is diagnosed with a fix-it moving it
// (CLANG-663, the mirror of GCC-680), and the capture recovers as a pack.
// Written on both sides (`[...y... = xs]`), it is diagnosed once, and an
// error in one capture does not stop the rest of the list being parsed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3098-unexpanded-pack-capture.C
// and p3098-unexpanded-pack-capture-inst.C.

template <class... T>
int p3(T... xs) post [xs] (true) { return 0; } // expected-error {{initializer contains unexpanded parameter pack 'xs'}}

template <class... T>
int p4(T... xs) post [y = xs] (true) { return 0; } // expected-error {{initializer contains unexpanded parameter pack 'xs'}}

template <class... T>
int ok(T... xs) post [...y = xs] (((y > 0) && ...)) { return 0; } // OK
template <class... T>
int ok2(T... xs) post [xs...] (((xs > 0) && ...)) { return 0; } // OK
template <class... T>
int bad(T... xs) post [...xs] (((xs > 0) && ...)) { return 0; } // expected-error {{ellipsis in pack capture must appear after the name of the capture}}
template <class... T>
int bad2(T... xs) post [...xs...] (((xs > 0) && ...)) { return 0; } // expected-error {{ellipsis in pack capture must appear after the name of the capture}}
template <class... T>
int bad3(T... xs) post [y... = xs] (((y > 0) && ...)) { return 0; } // expected-error {{ellipsis in pack init-capture must appear before the name of the capture}}
template <class... T>
int bad4(T... xs) post [...y... = xs] (((y > 0) && ...)) { return 0; } // expected-error {{ellipsis in pack init-capture must appear before the name of the capture}}

// CHECK: fix-it:"{{.*}}":{[[@LINE-8]]:24-[[@LINE-8]]:27}:""
// CHECK: fix-it:"{{.*}}":{[[@LINE-9]]:29-[[@LINE-9]]:29}:"..."
// CHECK: fix-it:"{{.*}}":{[[@LINE-8]]:25-[[@LINE-8]]:28}:""
// CHECK: fix-it:"{{.*}}":{[[@LINE-7]]:25-[[@LINE-7]]:25}:"..."
// CHECK: fix-it:"{{.*}}":{[[@LINE-8]]:26-[[@LINE-8]]:29}:""
// CHECK: fix-it:"{{.*}}":{[[@LINE-7]]:29-[[@LINE-7]]:32}:""

// The same on a declaration, whose contracts are parsed with it.
template <class... T>
int d3(T... xs) post [xs] (true); // expected-error {{initializer contains unexpanded parameter pack 'xs'}}
template <class... T>
int d5(T... xs) post [...y = xs] (y > 0); // expected-error {{expression contains unexpanded parameter pack 'y'}}
template <class... T>
int d6(T... xs) post [...xs] (((xs > 0) && ...)); // expected-error {{ellipsis in pack capture must appear after the name of the capture}}
// The rest of the capture list is still parsed: `z` is declared.
template <class... T>
int d7(T... xs) post [y... = xs, z = 0] (((y > z) && ...)); // expected-error {{ellipsis in pack init-capture must appear before the name of the capture}}
// CHECK: fix-it:"{{.*}}":{[[@LINE-1]]:23-[[@LINE-1]]:23}:"..."
// CHECK: fix-it:"{{.*}}":{[[@LINE-2]]:24-[[@LINE-2]]:27}:""

int main() { return p3(1, 2) + p4(1, 2) + ok(1, 2) + ok2(1, 2) + bad(1, 2); }
