// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// P3097 x templates: the final overrider of a contract-carrying virtual
// function comes from a class template, and from explicit and partial
// specializations of it that carry different contract assertions of their
// own (or none).  A virtual call through the interface checks the
// interface's precondition and then whichever overrider's own the dynamic
// type selects: the primary template's, the explicit specialization's or
// the partial specialization's.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3097-specialization-override.C

#include <contracts>
#include <cstdlib>

constexpr unsigned iface_line = __LINE__ + 2;
struct Iface {
  virtual int f(int x) pre(x >= 0) { return x; }
  virtual ~Iface() {}
};

static int iface, own;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.location().line() == iface_line)
    ++iface;
  else
    ++own;
}

template <class T> struct Impl : Iface {
  int f(int x) override pre(x != 1) { return x; } // primary: rejects 1
};

template <> struct Impl<char> : Iface {
  int f(int x) override pre(x > 10) { return x; } // explicit: rejects <= 10
};

template <class T> struct Impl<T *> : Iface {
  int f(int x) override { return x; } // partial: no own pre
};

static bool calls(Iface &i, int x, int want_iface, int want_own) {
  iface = own = 0;
  i.f(x);
  return iface == want_iface && own == want_own;
}

int main() {
  Impl<int> p;
  Impl<char> e;
  Impl<int *> q;
  if (!calls(p, 1, 0, 1) || !calls(p, 5, 0, 0) || !calls(p, -1, 1, 0))
    std::abort();
  if (!calls(e, 1, 0, 1) || !calls(e, 11, 0, 0) || !calls(e, -1, 1, 1))
    std::abort();
  if (!calls(q, 1, 0, 0) || !calls(q, -1, 1, 0))
    std::abort();
}
