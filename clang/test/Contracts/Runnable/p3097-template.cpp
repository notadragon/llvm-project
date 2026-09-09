// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097 x templates: contracts on a virtual member function of a class template,
// checked across dispatch for each instantiation.  (GCC mirror: p3097-template.C)
//
// (Previously: a contract on a VIRTUAL member of a class template ICEd
// during codegen because the interface contract was never instantiated -- an
// inline virtual member that is only ever called polymorphically never has its
// definition instantiated, so its contract stayed the dependent pattern copy
// that the P3097 contract wrapper then tried to emit.  Clang now instantiates a
// virtual function's contracts on odr-use, independent of its definition.)

#include <contracts>
#include <cstdio>

static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

template <typename T>
struct Base {
  virtual T f(T x) pre(x > T{}) post(r: r > T{}) { return x; }
  virtual ~Base() = default;
};

template <typename T>
struct Derived : Base<T> {
  T f(T x) override { return x - 1; }
};

int main() {
  Derived<int> d;
  Base<int>& b = d;

  violation_count = 0;
  if (b.f(5) != 4) __builtin_abort();
  if (violation_count != 0) __builtin_abort();

  violation_count = 0;
  if (b.f(1) != 0) __builtin_abort();
  if (violation_count != 1) __builtin_abort();

  violation_count = 0;
  if (b.f(0) != -1) __builtin_abort();
  if (violation_count != 2) __builtin_abort();

  Derived<double> dd;
  Base<double>& bd = dd;
  violation_count = 0;
  if (bd.f(5.0) != 4.0) __builtin_abort();
  if (violation_count != 0) __builtin_abort();

  std::printf("PASS\n");
}
