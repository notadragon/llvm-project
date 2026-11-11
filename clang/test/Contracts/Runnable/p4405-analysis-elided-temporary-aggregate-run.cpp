// RUN: %clangxx -std=c++26 %s -O0 -fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: %t
// RUN: %clangxx -std=c++26 %s -O2 -fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t.O2
// RUN: %t.O2

// D4405 ("The Analysis Evaluation"): the cleanup of a member of an
// aggregate whose initialization throws part-way, which has an analysis
// evaluation, still destroys that member, once -- in a function and for a
// static local, whose initialization is retried by the next call, without
// and with optimization.  (At namespace scope the exception reaches
// std::terminate, with no unwinding required.)  The member's own
// initializer is elided.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-elided-temporary-aggregate-run.C
// (which also builds caller-side wrappers, -fcontracts-client-check=all;
// Clang does not implement caller-side checks, so the destructor's
// preconditions are callee-side here)

int g = 1;
static int ctors, dtors;
static void *last_built, *last_destroyed;
static bool throwing;

struct T {
  T *self;
  T() : self(this) {
    ++ctors;
    last_built = this;
  }
  T(const T &) = delete;
  ~T() pre(g > 0) pre(self == this) {
    ++dtors;
    last_destroyed = this;
  }
};

T make() { return T(); }

int thr() {
  if (throwing)
    throw 1;
  return 2;
}

struct A { T p; int i; };

void in_function(bool use_make) {
  ctors = dtors = 0;
  last_built = last_destroyed = nullptr;
  try {
    A b = use_make ? A{make(), thr()} : A{T(), thr()};
    __builtin_abort();
  } catch (int) {
  }
  if (ctors != 1 || dtors != 1 || last_destroyed != last_built)
    __builtin_abort();
}

A *static_local() {
  static A c{T(), thr()};
  return &c;
}

int main() {
  throwing = true;
  in_function(false);
  in_function(true);

  ctors = dtors = 0;
  last_built = last_destroyed = nullptr;
  try {
    static_local();
    __builtin_abort();
  } catch (int) {
  }
  if (ctors != 1 || dtors != 1 || last_destroyed != last_built)
    __builtin_abort();

  throwing = false;
  A *c = static_local();
  if (ctors != 2 || dtors != 1 || c->p.self != &c->p || c->i != 2 ||
      static_local() != c || ctors != 2)
    __builtin_abort();
}
