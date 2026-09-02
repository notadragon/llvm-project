// KNOWN GAP, pinned so that fixing it shows up as an XPASS.
//
// A mutation performed through a postcondition's result binding is not
// observable by the caller under Clang; GCC makes it observable as of
// gnu_gcc 20eed05e8c4 / 0377d4408ab.
//
// The position this branch takes (2026-09-02) is that a predicate we evaluate
// is evaluated faithfully, so its effects are observable.  [basic.contract.eval]
// permits *not evaluating* a predicate -- "an alternative evaluation that
// produces the same value ... but has no side effects" may be substituted --
// but that is all-or-nothing.  It does not permit evaluating a predicate and
// then discarding what it did, which is what happens here: Clang evaluates
// `const_cast<int&>(r)++' and the increment lands somewhere the caller cannot
// see.
//
// GCC reached the same state by handing the check a copy of the result, and
// the fix there was to give the binding one addressable home and copy it back
// once the checks are done.  Clang needs the equivalent.
//
// XFAIL: *
// RUN: %clangxx -std=c++26 %s -fcontracts %libcxx_flags -o %t && %t

int mutation_observed() post(r : (const_cast<int &>(r)++, true)) { return 1; }

int main() {
  // Required to be 2: the predicate is evaluated, so its effect stands.
  if (mutation_observed() != 2)
    return 1;
  return 0;
}
