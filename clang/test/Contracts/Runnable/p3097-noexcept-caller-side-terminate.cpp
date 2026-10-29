// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 %libcxx_flags -o %t
// RUN: %t wrapper
// RUN: %t variadic
// RUN: %t dtor
// RUN: %clangxx -std=c++26 -O2 %s -fcontracts -fcontracts-p3097 %libcxx_flags -o %t.O2
// RUN: %t.O2 wrapper
// RUN: %t.O2 variadic
// RUN: %t.O2 dtor

// P3097: the interface contracts of a noexcept virtual function, evaluated
// outside its body -- in the virtual-call wrapper, or at the call for a
// C-variadic function or a virtual destructor -- terminate the program when
// the violation handler throws, as they would in the function's body; the
// exception never reaches the caller's catch (CLANG-834).  Each call is made
// through a volatile function pointer, so the caller cannot know the callee
// does not throw and keeps its handler.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/
// p3097-noexcept-caller-side-terminate*.C

#include <contracts>
#include <cstdlib>
#include <cstring>
#include <exception>

static int handled;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++handled;
  throw 42;
}

static void on_terminate() {
  if (handled == 1)
    std::_Exit(0);
  std::abort();
}

struct B {
  virtual int f(int x) noexcept pre(x > 0) { return x; }
  virtual int v(int x, ...) noexcept pre(x > 0) { return x; }
  virtual ~B() noexcept pre(false) {}
};

static int call_f(B &b) { return b.f(-1); }        // the wrapper
static int call_v(B &b) { return b.v(-1, 0); }     // checked at the call
static int call_d(B *b) { delete b; return 0; }    // checked at the call

int (*volatile fp)(B &);
int (*volatile dp)(B *);

int main(int argc, char **argv) {
  std::set_terminate(on_terminate);
  if (argc != 2)
    return 1;
  try {
    if (!std::strcmp(argv[1], "wrapper")) {
      fp = call_f;
      B b;
      fp(b);
    } else if (!std::strcmp(argv[1], "variadic")) {
      fp = call_v;
      B b;
      fp(b);
    } else {
      dp = call_d;
      dp(new B);
    }
  } catch (...) {
    std::abort(); // no exception may leave a noexcept function's contracts
  }
  std::abort();
}
