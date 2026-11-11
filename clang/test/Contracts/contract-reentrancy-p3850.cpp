// RUN: %clang_cc1 -std=c++26 -fcontracts-p3850 -fsyntax-only -verify %s
// expected-no-diagnostics

// Re-entrant contract instantiation against the rest of the P3850 extension
// set, with every extension enabled at once.
//
// Each of these attaches something extra to the contract that has to survive
// the nested substitution: a P3400 label (an expression evaluated to pick the
// semantic), a P4283 requires-clause (evaluated *before* the predicate, and
// able to discard the contract outright), and a P3099 message.
//
// See contract-reentrancy-basic.cpp for the shape of the underlying defect.
//
// GCC mirror: g++.dg/contracts/cpp26/contract-reentrancy-p3850.C.

struct lbl_t {
  using assertion_control_object = lbl_t;
};
constexpr lbl_t lbl{};

template <class T> struct is_int {
  static constexpr bool value = false;
};
template <> struct is_int<int> {
  static constexpr bool value = true;
};
template <class T> concept Int = is_int<T>::value;

//===----------------------------------------------------------------------===//
// P3400: the re-entrantly instantiated callee's contract carries a label.
//===----------------------------------------------------------------------===//

template <class T> struct Labelled {
  bool ok() const pre<lbl>(n >= 0) { return n >= 0; }
  int n = 0;
};

template <class T> int f_label_callee(Labelled<T> s) pre(s.ok()) {
  return s.n;
}
int use_label_callee() { return f_label_callee(Labelled<int>{}); }

// ... and labels at both ends.
template <class T> int f_label_both(Labelled<T> s) pre<lbl>(s.ok()) {
  return s.n;
}
int use_label_both() { return f_label_both(Labelled<int>{}); }

//===----------------------------------------------------------------------===//
// P4283: the callee's contract has a satisfied requires-clause, which the
// nested pass must evaluate before substituting the predicate.
//===----------------------------------------------------------------------===//

template <class T> struct Constrained {
  bool ok() const pre requires(Int<T>) (n >= 0) { return n >= 0; }
  int n = 0;
};

template <class T> int f_requires(Constrained<T> s) pre(s.ok()) {
  return s.n;
}
int use_requires() { return f_requires(Constrained<int>{}); }

//===----------------------------------------------------------------------===//
// P4283: the callee's only contract is DISCARDED by an unsatisfied
// requires-clause.  Control -- the nested pass finds nothing left to
// substitute, so it must cope with an empty result rather than a null one.
//===----------------------------------------------------------------------===//

template <class T> struct is_flt {
  static constexpr bool value = false;
};
template <class T> concept Flt = is_flt<T>::value;

template <class T> struct Discarded {
  bool ok() const pre requires(Flt<T>) (n >= 0) { return n >= 0; }
  int n = 0;
};

template <class T> int f_discarded(Discarded<T> s) pre(s.ok()) { return s.n; }
int use_discarded() { return f_discarded(Discarded<int>{}); }

//===----------------------------------------------------------------------===//
// P3099: the callee's contract carries a message.
//===----------------------------------------------------------------------===//

template <class T> struct Messaged {
  bool ok() const pre(n >= 0, "n must be non-negative") { return n >= 0; }
  int n = 0;
};

template <class T> int f_message(Messaged<T> s) pre(s.ok()) { return s.n; }
int use_message() { return f_message(Messaged<int>{}); }

//===----------------------------------------------------------------------===//
// A contract_assert in a function BODY odr-uses a contracted template.  There
// is no enclosing contract scope here, so this is the control that says the
// on-use pass is fine when it is not re-entered.
//===----------------------------------------------------------------------===//

template <class T> bool body_ok(T v) pre(v == v) { return v >= 0; }
template <class T> void f_body(T v) { contract_assert(body_ok(v)); }
void use_body() { f_body(1); }

//===----------------------------------------------------------------------===//
// Everything at once: label, requires-clause, message, virtual dispatch and a
// capturing postcondition, on both ends of the re-entrancy.
//===----------------------------------------------------------------------===//

template <class T> struct H {
  virtual int get() const
      post<lbl> requires(Int<T>) [was = n] (was == n, "get must not mutate") {
    return n;
  }
  int n = 0;
};

template <class T> struct Everything {
  virtual int f(H<T> &h)
      post<lbl> requires(Int<T>) [old = h.get()] (old >= 0, "must stay sane") {
    return h.n;
  }
};

int use_everything() {
  Everything<int> s;
  H<int> h;
  return s.f(h);
}
