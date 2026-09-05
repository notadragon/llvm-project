// clang-13-contract-instantiation-reentrancy.cpp                    -*-C++-*-
//
// CLANG-13: instantiating one function's contracts from inside another
// contract's predicate trips an assertion in PushContractScope.
//
//   clang++ -std=c++26 -fcontracts -fsyntax-only \
//       clang-13-contract-instantiation-reentrancy.cpp
//
// This branch's clang:
//   SemaContract.cpp:3477: Sema::PushContractScope(...):
//   Assertion `LastScope && !LastScope->isInContract()' failed.
//
// This branch's g++ compiles it clean, with -fcontracts and with
// -fcontracts-p3850 -- the defect is Clang's alone.
//
// PLAIN -fcontracts.  Nothing P3850-specific is involved.
//
// THE SHAPE THAT MATTERS: `f`'s precondition calls a member that ITSELF has a
// contract, and both are templates, so transforming the outer predicate
// odr-uses the inner function and re-enters contract instantiation while the
// outer contract scope is still open.

template <class T>
struct S {
  bool ok () const pre (n >= 0) { return n >= 0; }
  int n = 0;
};

// THE BUG.
template <class T>
int f (S<T> s) pre (s.ok ()) { return s.n; }

int
main ()
{
  return f (S<int>{});
}

// CONTROL: the same call with a predicate that does NOT reach a contracted
// function is fine, which is what places the defect in the re-entrant
// instantiation rather than in contracts on templates generally.
template <class T>
int g (S<T> s) pre (s.n >= 0) { return s.n; }

int
control ()
{
  return g (S<int>{});
}
