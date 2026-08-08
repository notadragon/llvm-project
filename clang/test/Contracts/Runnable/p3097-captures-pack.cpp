// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097+P3098: Pack captures on a virtual member function (only expressible via
// an enclosing class template, since a virtual function cannot itself be a
// function template).  The captured pack in the interface postcondition holds
// call-time values across dispatch.
//
// (Previously BUG-5: a pack capture on a virtual member of a class template ICEd
// during codegen ("should not see dependent types here") because the interface
// contract was never instantiated -- the wrapper tried to emit the dependent
// pattern copy of the pack capture.  Fixed together with BUG-7: Clang now
// instantiates a virtual function's contracts on odr-use.)

#include <contracts>
#include <cstdio>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

template <typename... Ts>
bool allpos(Ts... x) { return (... && (x > 0)); }

template <typename... Ts>
struct Base {
  virtual bool f(Ts... ts)
    post [ts...] (r: r == allpos(ts...))
  { return allpos(ts...); }
  virtual ~Base() = default;
};

struct Derived : Base<int, int, int> {
  bool f(int a, int b, int) override { return a > 0 && b > 0; }
};

int main() {
  Derived d;
  Base<int, int, int>& b = d;

  violation_count = 0;
  if (!b.f(1, 2, 3)) __builtin_abort();
  if (violation_count != 0) __builtin_abort();

  violation_count = 0;
  if (!b.f(1, 2, -1)) __builtin_abort();
  if (violation_count != 1) __builtin_abort();

  std::printf("PASS\n");
}
