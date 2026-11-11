// RUN: %clangxx -std=c++26 %s -fcontracts-p3850 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// Re-entrant contract instantiation, at run time.
//
// The sema tests (contract-reentrancy-basic.cpp and friends) establish that a
// contract predicate may odr-use a contracted function without the nested
// instantiation tripping over the outer one.  They cannot tell whether the
// contract that was instantiated re-entrantly is any good: a predicate that
// silently never runs, or one substituted against the wrong function's
// parameters, compiles just as quietly as a correct one.
//
// So this checks that the re-entrantly instantiated contracts actually
// evaluate, in the right order, against the right objects -- including when
// the callee is virtual (P3097), has a capturing postcondition (P3098), or
// both.  Everything runs under the observe semantic so that a violation is
// counted rather than terminating.
//
// GCC mirror: g++.dg/contracts/cpp26/contract-reentrancy-run.C.

#include <contracts>

static int viol = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++viol;
}

static int counter = 0;

//===----------------------------------------------------------------------===//
// The callee's precondition is evaluated from inside the caller's, and it is
// evaluated FIRST -- the caller's predicate cannot finish until the call it
// contains returns.
//===----------------------------------------------------------------------===//

template <class T> struct S {
  bool ok() const pre(n >= 0) { return n >= 0; }
  int n;
};

template <class T> int f_plain(S<T> s) pre(s.ok()) { return s.n; }

//===----------------------------------------------------------------------===//
// A capturing postcondition on the re-entrantly instantiated callee.  The
// capture is taken on entry and the predicate runs on exit, so `before` must
// hold the pre-increment value -- if the capture were dropped or re-read at
// exit, `counter == before + 1` would be false.
//===----------------------------------------------------------------------===//

template <class T> bool tick() post [before = counter] (counter == before + 1) {
  ++counter;
  return true;
}

template <class T> int f_captures(T v) pre(tick<T>()) { return v; }

// The same shape with a predicate that must FAIL, so that a capture which was
// quietly never evaluated cannot pass for a correct one.
template <class T> bool bad_tick()
    post [before = counter] (counter == before + 2) {
  ++counter;
  return true;
}

template <class T> int f_bad_captures(T v) pre(bad_tick<T>()) { return v; }

//===----------------------------------------------------------------------===//
// A virtual callee (P3097).  Reached from a predicate through a base
// reference, so the interface contract evaluated by the wrapper around the
// vtable dispatch is the one instantiated re-entrantly.
//===----------------------------------------------------------------------===//

template <class T> struct V {
  virtual bool ok() const pre(n >= 0) { return true; }
  int n;
  V(int v) : n(v) {}
  virtual ~V() = default;
};

template <class T> struct VD : V<T> {
  VD(int v) : V<T>(v) {}
  bool ok() const override pre(this->n >= -10) { return true; }
};

template <class T> int f_virtual(V<T> &v) pre(v.ok()) { return v.n; }

//===----------------------------------------------------------------------===//
// Virtual AND capturing (P3097 + P3098) on the callee: both control-flow
// rewrites, reached re-entrantly.
//===----------------------------------------------------------------------===//

// `bump` is const because a predicate constifies the object it names, so a
// non-const member could not be called from one; the mutation it performs is
// on the file-scope counter, not on the object.
template <class T> struct VC {
  virtual bool bump() const post [before = counter] (counter == before + 1) {
    ++counter;
    return true;
  }
  virtual ~VC() = default;
};

template <class T> int f_virtual_captures(VC<T> &v) pre(v.bump()) { return 0; }

int main() {
  // Neither contract fails: the callee's precondition holds and it returns
  // true, so the caller's holds too.
  if (f_plain(S<int>{1}) != 1)
    return 1;
  if (viol != 0)
    return 2;

  // Both fail, callee first: S::ok's `n >= 0` is violated, ok() returns false,
  // and that makes f_plain's own `s.ok()` false as well.
  if (f_plain(S<int>{-1}) != -1)
    return 3;
  if (viol != 2)
    return 4;

  // The callee's capturing postcondition runs and holds.
  viol = 0;
  counter = 0;
  if (f_captures(7) != 7)
    return 5;
  if (counter != 1)
    return 6; // the body never ran
  if (viol != 0)
    return 7; // the capture did not hold the entry value

  // ... and when it must fail, it does -- so the check above is not passing
  // by virtue of the predicate never being evaluated at all.
  viol = 0;
  counter = 0;
  if (f_bad_captures(7) != 7)
    return 8;
  if (counter != 1)
    return 9;
  if (viol != 1)
    return 10;

  // The virtual callee's interface precondition is evaluated through the
  // wrapper.  n is -5: V::ok's `n >= 0` fails, VD::ok's `n >= -10` holds.
  viol = 0;
  VD<int> d(-5);
  V<int> &b = d;
  if (f_virtual(b) != -5)
    return 11;
  if (viol == 0)
    return 12; // the re-entrantly instantiated interface contract never ran

  // A passing value must leave the count alone.
  viol = 0;
  VD<int> ok_d(5);
  V<int> &ok_b = ok_d;
  if (f_virtual(ok_b) != 5)
    return 13;
  if (viol != 0)
    return 14;

  // Virtual and capturing at once.
  viol = 0;
  counter = 0;
  VC<int> vc;
  if (f_virtual_captures(vc) != 0)
    return 15;
  if (counter != 1)
    return 16;
  if (viol != 0)
    return 17;

  return 0;
}
