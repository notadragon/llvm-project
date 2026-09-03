// KNOWN GAP, pinned so that fixing it shows up as an XPASS.
//
// A mutation through a postcondition's result binding is observable when the
// result is returned directly (contract-result-binding-mutation.cpp), but NOT
// when it is of class type -- neither for one returned indirectly through an
// sret pointer nor for a small one coerced into registers.  GCC observes both
// (measured 2026-09-02 against the current branch build: all shapes correct).
//
// The predicate IS evaluated -- `ran' below reaches the caller as 1 -- so this
// is not the elision latitude in [basic.contract.eval].  The binding simply
// names a copy: EmitPostContracts only stores the value back into the return
// slot and binds the result name there when it has a scalar `RV' in hand, and
// for a class type it does not.
//
// Fixing it means giving the class-typed binding the same one-addressable-home
// treatment: bind it to the return slot and, where the value comes back
// coerced, re-coerce after the checks rather than re-reading the slot raw --
// re-reading it raw is what produced a broken module in a first attempt at
// the scalar fix.
//
// XFAIL: *
// RUN: %clangxx -std=c++26 %s -fcontracts %libcxx_flags -o %t && %t

struct Big {
  int a, b, c, d;
};
struct Small {
  int a;
};

static int ran = 0;

static bool bumpBig(const Big &r) {
  ++ran;
  const_cast<Big &>(r).a += 7;
  return true;
}
static bool bumpSmall(const Small &r) {
  ++ran;
  const_cast<Small &>(r).a += 9;
  return true;
}

// Returned indirectly (sret).
Big big() post(r : bumpBig(r)) { return Big{1, 2, 3, 4}; }

// Small enough to come back coerced into a register.
Small small_() post(r : bumpSmall(r)) { return Small{1}; }

int main() {
  ran = 0;
  int ba = big().a;
  if (ran != 1)
    __builtin_abort(); // the predicate must run at all
  if (ba != 8)
    return 1;

  ran = 0;
  int sa = small_().a;
  if (ran != 1)
    __builtin_abort();
  if (sa != 10)
    return 1;

  return 0;
}
