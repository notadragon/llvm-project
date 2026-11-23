// clang-39-typedef-requires-clause.cpp                              -*-C++-*-
//
// CLANG-39: a trailing requires-clause on a typedef of a function type is
// accepted.  No contracts involved, plain C++20.
//
// [dcl.decl.general]: "The optional requires-clause in an init-declarator or
// member-declarator shall be present only if the declarator declares a
// templated function."  A typedef declares a type, not a function.
//
// Expected: an error on line A (GCC: "requires-clause on typedef").
// Actual:   accepted.  Line B, the same clause on a variable, is diagnosed
//           ("trailing requires clause can only be used when declaring a
//           function").
//
// clang++ -std=c++20 -fsyntax-only clang-39-typedef-requires-clause.cpp

typedef int F(int) requires true;  // A: accepted (bug)

#ifdef CONTROL
int x requires true;               // B: rejected (correct)
#endif
