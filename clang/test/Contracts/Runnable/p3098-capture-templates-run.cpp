// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe %s %libcxx_flags -o %t && %t

// P3098 captures in templated functions, at run time: an init-capture pack
// whose initializer is an operator expression (GCC-629 failed it at parse
// time; Clang crashed in CodeGen on its <dependent type> element type), and
// a class-type parameter capture, copied once, as in a non-template.
// (GCC mirror: g++.dg/contracts/cpp26/p3098-capture-templates-run.C)

#include <contracts>

int violations = 0;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

template <class... T> int twice (T... xs)
  post [... ys = xs * 2] (r: r == (ys + ... + 0))
{
  return (xs + ... + 0) * 2;
}

template <class... T> int twice_wrong (T... xs)
  post [... ys = xs * 2] (r: r == (ys + ... + 1))
{
  return (xs + ... + 0) * 2;
}

struct C
{
  int v;
  static inline int copies = 0;
  C (int v) : v (v) {}
  C (const C &o) : v (o.v) { ++copies; }
  ~C () {}
};

template <class T> int f (T x) post [x] (r: r == x.v) { return x.v; }
int g (C x) post [x] (r: r == x.v) { return x.v; }

int
main ()
{
  if (twice (1, 2, 3) != 12 || violations != 0)
    __builtin_abort ();
  // Control: a predicate that the captured values make false is reported.
  if (twice_wrong (1, 2, 3) != 12 || violations != 1)
    __builtin_abort ();

  C c (7);
  C::copies = 0;
  f (c);
  int template_copies = C::copies;
  C::copies = 0;
  g (c);
  if (template_copies != C::copies || violations != 1)
    __builtin_abort ();
}
