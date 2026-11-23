// RUN: %clangxx -std=c++26 %s -O0 -fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: %t
// RUN: %clangxx -std=c++26 %s -O2 -fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t.O2
// RUN: %t.O2

// D4405 ("The Analysis Evaluation"): a temporary's destructor call, analyzed
// only when the temporary is materialized, still destroys the right object,
// once, and only when the temporary is materialized -- in a function and at
// namespace scope, without and with optimization.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-elided-temporary-run.C
// (which also builds caller-side wrappers, -fcontracts-client-check=all;
// Clang does not implement caller-side checks, so the destructor's
// preconditions are callee-side here)

int g = 1;
static int ctors, dtors;
static void *last_destroyed;

struct T {
  T *self;
  T() : self(this) { ++ctors; }
  T(const T &) = delete;
  ~T() pre(g > 0) pre(self == this) {
    ++dtors;
    last_destroyed = this;
  }
};

T make() { return T(); }
void take(T t) {
  if (t.self != &t)
    __builtin_abort();
}

T nx = T();
T ny = make();
T *np = new T(T());
int nz = (T(), 1);
static int ns_ctors = ctors, ns_dtors = dtors;

T ret() { return g ? T() : make(); }

int main() {
  if (ns_ctors != 4 || ns_dtors != 1 || nx.self != &nx || ny.self != &ny ||
      np->self != np)
    __builtin_abort();
  ctors = dtors = 0;
  {
    T a = T();
    T b = make();
    T c = ret();
    if (a.self != &a || b.self != &b || c.self != &c || dtors != 0)
      __builtin_abort();
  }
  if (ctors != 3 || dtors != 3)
    __builtin_abort();
  ctors = dtors = 0;
  int i = (T(), 1);
  if (i != 1 || ctors != 1 || dtors != 1)
    __builtin_abort();
  take(T());
  take(make());
  if (ctors != 3 || dtors != 3)
    __builtin_abort();
  T *p = new T(make());
  if (p->self != p || dtors != 3)
    __builtin_abort();
  delete p;
  if (dtors != 4 || last_destroyed != p)
    __builtin_abort();
}
