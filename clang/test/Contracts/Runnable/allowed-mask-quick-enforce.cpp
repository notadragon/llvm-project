// Regression test for the contract allowed-semantics mask.
//
// A contract with no assertion-control label has an "all semantics allowed"
// mask.  Previously that mask was 0xF, which (since bit N corresponds to
// evaluation-semantic value N) covered Ignore(1)/Observe(2)/Enforce(3) but
// NOT QuickEnforce(4).  As a result a contract resolved to quick_enforce was
// silently downgraded through the fallback chain to observe, so a violation
// did not terminate.  With the mask fixed, quick_enforce is honored and a
// failing precondition traps.
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -o %t
// RUN: not --crash %t

int f(int i) pre(i > 0) { return i; }

int main() {
  return f(-1); // quick_enforce: must trap, not fall back to observe
}
