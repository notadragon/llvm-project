// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3098: a postcondition capture of a class type is copy-initialized, so the
// capture's copy constructor runs.  A bare bit-copy would leave the capture's
// destructor running on an object no constructor ever made -- for an owning
// type, a double free.
// (GCC mirror: g++.dg/contracts/cpp26/p3098-capture-copy-run.C)
//
// Clang has always been correct here; this is a regression pin, mirrored
// because GCC was not.  Nothing on either side covered it: every capture test
// used a scalar or a prvalue init-capture, and a prvalue needs no copy, so the
// copy constructor was never reached.  p3098-dtor.cpp logs copy-constructor
// calls but captures `Logger(1)` -- a prvalue -- so it never logs one.
//
// A compile-only test cannot see a bit-copy, so this counts calls at run time.

#include <cstdio>

static int copies = 0;
static int dtors = 0;

struct S {
  int v;
  S(int x) : v(x) {}
  S(const S &o) : v(o.v) { ++copies; }
  ~S() { ++dtors; }
};

bool ok(int x) { return x == 7; }

void by_param(S p) post [p] (ok(p.v)) {}
void by_init(S p) post [q = p] (ok(q.v)) {}

// Each call copies twice -- once for the by-value parameter, once for the
// capture -- and destroys both.
int main() {
  {
    S s(7);
    int c0 = copies, d0 = dtors;
    by_param(s);
    if (copies - c0 != 2) __builtin_abort();
    if (dtors - d0 != 2) __builtin_abort();
  }

  {
    S s(7);
    int c0 = copies, d0 = dtors;
    by_init(s);
    if (copies - c0 != 2) __builtin_abort();
    if (dtors - d0 != 2) __builtin_abort();
  }

  std::printf("PASS\n");
}
