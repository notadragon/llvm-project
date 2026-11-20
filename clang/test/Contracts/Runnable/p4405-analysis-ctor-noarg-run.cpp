// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -O2 %libcxx_flags -o %t
// RUN: %t
// REQUIRES: contracts-libcxx

// D4405 ("The Analysis Evaluation"): the analysis evaluations at a call
// with no argument but the object (a default constructor, a destructor, a
// function returning a class with no parameters; gnu_gcc's GCC-919) change
// nothing the program does: the object is evaluated once, the temporary of
// a prvalue is still elided into the object it initializes, and every
// object is constructed and destroyed once.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-ctor-noarg-run.C

int g = 1;
static int ctors, dtors;

struct T {
  T *self;
  int v;
  T() pre(g > 0) : self(this), v(1) { ++ctors; }
  ~T() pre(g > 0) { ++dtors; }
};

struct R { int v; };

[[gnu::noinline]] R mk() pre(g > 0) { return R{7}; }

struct H {
  T t;
  H() : t() {}
};

T ret() { return T(); }

[[gnu::noinline]] T *
next(T *p, int &n)
{
  ++n;
  return p;
}

int
main()
{
  {
    T a = T();
    if (a.self != &a)
      __builtin_abort();
  }
  {
    T b;
    if (b.self != &b)
      __builtin_abort();
  }
  {
    H h;
    if (h.t.self != &h.t)
      __builtin_abort();
  }
  {
    T r = ret();
    if (r.self != &r)
      __builtin_abort();
  }
  {
    T arr[3];
    for (int i = 0; i < 3; ++i)
      if (arr[i].self != &arr[i])
        __builtin_abort();
  }
  {
    T *p = new T;
    if (p->self != p)
      __builtin_abort();
    int n = 0;
    next(p, n)->~T();
    if (n != 1)
      __builtin_abort();
    ::operator delete (p);
  }
  if (T().v + T {}.v != 2)
    __builtin_abort();
  if (mk().v != 7)
    __builtin_abort();
  if (ctors != 10 || dtors != 10)
    __builtin_abort();
}
