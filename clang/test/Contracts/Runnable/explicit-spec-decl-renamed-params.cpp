// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe %s %libcxx_flags -o %t && %t

// An explicit specialization's first declaration may name its parameters
// differently from the template's, and its contracts refer to them by the
// names it wrote -- also when that declaration is not a definition, where
// the specialization keeps the parameters of the implicit instantiation it
// replaces (GCC-626: "'a' was not declared in this scope").  Each call
// below violates the specialization's own contract and satisfies the
// template's, so the count shows which one was checked.
// (GCC mirror: g++.dg/contracts/cpp26/explicit-spec-decl-renamed-params.C.
// Clang had GCC-626 right; the member template case is CLANG-621: Clang
// compared its contracts with the primary member template's.)

#include <contracts>

int violations = 0;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

// A function template.
template<typename T> int g (T t) pre (t > 0) { return 0; }
template<> int g<double> (double a) pre (a > 1);
template<> int g<double> (double a) pre (a > 1) { return 1; }

// A member of a class template.
template<typename T> struct B { int m (T t) pre (t > 0) { return 0; } };
template<> int B<double>::m (double a) pre (a > 1);
template<> int B<double>::m (double a) pre (a > 1) { return 1; }

// A member template of a class template.
template<typename T>
struct A
{
  template<typename P> int f (T t, P) pre (t > 0) { return 0; }
};
template<>
template<typename Q>
int A<double>::f (double a, Q) pre (a > 1);
template<>
template<typename Q>
int A<double>::f (double a, Q) pre (a > 1) { return 1; }

// Declared without the definition in sight: the call is checked against
// the declaration's contract alone.
template<> int g<long> (long a) pre (a > 1);

int
main ()
{
  if (g (0.5) != 1 || violations != 1)
    __builtin_abort ();
  if (B<double> ().m (0.5) != 1 || violations != 2)
    __builtin_abort ();
  if (A<double> ().f (0.5, 0) != 1 || violations != 3)
    __builtin_abort ();
  if (g (1L) != 2 || violations != 4)
    __builtin_abort ();
  return 0;
}

template<> int g<long> (long a) pre (a > 1) { return 2; }
