// P3100: with -fcontracts-allow-assume the "assume" semantic survives
// resolution, but its code generation is identical to "ignore" for now --
// no check is emitted, the predicate is not evaluated, and a failing
// precondition does not report a violation.
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-evaluation-semantic=assume -fcontracts-allow-assume %libcxx_flags -o %t
// RUN: %t

int side_effect = 0;

bool check(int i) { ++side_effect; return i > 0; }

int f(int i) pre(check(i)) { return i; }

int main() {
  f(-1);              // assume emits no check (same as ignore for now)
  return side_effect; // predicate not evaluated -> 0 -> exit success
}
