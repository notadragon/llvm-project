// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s
// expected-no-diagnostics

// Re-entrant contract instantiation crossing P3098 postcondition captures.
//
// A capture list is substituted alongside the predicate, and each capture's
// initializer is an expression in its own right -- so a capture initializer
// can itself odr-use a contracted function, giving a second way into the
// nested pass that has nothing to do with the predicate.  Captures also alter
// the function's control flow (the capture is built on entry, the predicate
// runs on exit), which is why they are worth pairing with re-entrancy rather
// than assuming the plain case covers them.
//
// See contract-reentrancy-basic.cpp for the shape of the underlying defect.
//
// GCC mirror: g++.dg/contracts/cpp26/contract-reentrancy-p3098.C.

//===----------------------------------------------------------------------===//
// The function the predicate odr-uses has a capturing postcondition, so its
// capture list must substitute during the nested pass.
//===----------------------------------------------------------------------===//

template <class T> struct S {
  int get() const post [old = n] (old == n) { return n; }
  int n = 0;
};

template <class T> int f_callee_captures(S<T> s) pre(s.get() >= 0) {
  return s.n;
}
int use_callee_captures() { return f_callee_captures(S<int>{}); }

//===----------------------------------------------------------------------===//
// The OUTER contract is a capturing postcondition whose predicate odr-uses a
// contracted function.
//===----------------------------------------------------------------------===//

template <class T> struct P {
  bool ok() const pre(n >= 0) { return n >= 0; }
  int n = 0;
};

template <class T>
int f_outer_captures(const P<T> s) post [old = s.n] (old >= 0 && s.ok()) {
  return s.n;
}
int use_outer_captures() { return f_outer_captures(P<int>{}); }

//===----------------------------------------------------------------------===//
// The odr-use is in the CAPTURE INITIALIZER rather than the predicate, so the
// nested pass starts while the capture list -- not the predicate -- is being
// transformed.
//===----------------------------------------------------------------------===//

template <class T> struct G {
  int get() const pre(n >= 0) { return n; }
  int n = 0;
};

template <class T>
int f_capture_init(G<T> s) post [old = s.get()] (old >= 0) {
  return s.n;
}
int use_capture_init() { return f_capture_init(G<int>{}); }

//===----------------------------------------------------------------------===//
// Capturing postconditions at BOTH ends: the outer capture initializer calls a
// function whose own postcondition captures.
//===----------------------------------------------------------------------===//

template <class T> struct BothEnds {
  int get() const post [was = n] (was == n) { return n; }
  int n = 0;
};

template <class T>
int f_both_ends(BothEnds<T> s) post [old = s.get()] (old >= 0) {
  return s.n;
}
int use_both_ends() { return f_both_ends(BothEnds<int>{}); }

//===----------------------------------------------------------------------===//
// The re-entrantly instantiated callee's postcondition names its own result
// binding as well as a capture, so both name kinds must resolve against the
// callee rather than leaking the outer function's names.
//===----------------------------------------------------------------------===//

template <class T> struct WithResult {
  int get() const post [was = n] (r: r == was) { return n; }
  int n = 0;
};

template <class T> int f_result(WithResult<T> s) pre(s.get() >= 0) {
  return s.n;
}
int use_result() { return f_result(WithResult<int>{}); }

//===----------------------------------------------------------------------===//
// A pack-expanded capture on the re-entrantly instantiated callee.
//===----------------------------------------------------------------------===//

template <class... Ts> struct Pack {
  int get(Ts... vs) const post [vs...] (((vs == vs) && ...)) { return n; }
  int n = 0;
};

template <class T> int f_pack(Pack<T> p) pre(p.get(T{}) >= 0) { return 0; }
int use_pack() { return f_pack(Pack<int>{}); }
