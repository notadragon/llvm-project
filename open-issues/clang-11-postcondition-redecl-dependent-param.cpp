// CLANG-11: a redeclaration whose corresponding parameter has a DEPENDENT
// type escapes the postcondition const rule.
//
//   clang++ -std=c++26 -fcontracts -fsyntax-only \
//       clang-11-postcondition-redecl-dependent-param.cpp
//
// [dcl.contract.func] requires the parameter a postcondition predicate
// odr-uses "and the corresponding parameter on all declarations" to have
// const type.  `a' is declared `T' on the declaration carrying the contract,
// which for T = int is not const, so the instantiation is ill-formed.
// Accepted.

// THE BUG: accepted, should be rejected.
template <typename T> void f (T a) post (a);
template <typename T> void f (T const a) { }
template void f<int> (int);

// CONTROL: the same shape with concrete types IS rejected, so the check runs
// and it is the dependence that defeats it.  Uncomment to confirm.
//
//   void h (int a) post (a);
//   void h (const int a) { }

// CONTROL: both declarations const -- well-formed, and must stay accepted.
template <typename T> void ok (T const a) post (a);
template <typename T> void ok (T const a) { }
template void ok<int> (int);

// CONTROL: the declarations disagree about writing `const', but T is deduced
// as a const type, so BOTH parameters are const after substitution and the
// program is well-formed.  A fix that compares the written cv-qualifiers
// instead of substituting would wrongly reject this.
template <typename T> void deduced (T a) post (a);
template <typename T> void deduced (T const a) { }
template void deduced<const int> (const int);

int
main ()
{
  f (1);
  ok (1);
}
