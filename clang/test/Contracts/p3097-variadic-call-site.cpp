// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// Under P3097, a virtual call to a C-variadic function is checked at the call
// site (DECISIONS.md K12): the static type's preconditions see the named
// arguments, postconditions see the result (int, class, void), captures are
// made per call, and the variadic arguments still reach the final overrider.
// Two call sites in one function, and a recursive call, each check on their
// own.

#include <contracts>
#include <cstdarg>

static int violations;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

struct R { int v; };

static int sum_list(int base, int n, va_list ap) {
  for (int i = 0; i < n; ++i)
    base += va_arg(ap, int);
  return base;
}

struct B {
  virtual int sum(int n, ...) pre(n >= 0) post(r : r >= 0);
  virtual R make(int n, ...) post(r : r.v != 42);
  virtual void touch(int n, ...) post [c = n] (c < 10);
  virtual int down(int n, ...) pre(n >= 0);
};

int B::sum(int n, ...) {
  va_list ap; va_start(ap, n);
  int s = sum_list(0, n, ap);
  va_end(ap);
  return s;
}
R B::make(int n, ...) {
  va_list ap; va_start(ap, n);
  R r{sum_list(0, n, ap)};
  va_end(ap);
  return r;
}
void B::touch(int, ...) {}
int B::down(int n, ...) { return n == 0 ? 0 : down(n - 1, 7); }

struct D : B {
  int sum(int n, ...) override {
    va_list ap; va_start(ap, n);
    int s = sum_list(100, n, ap);
    va_end(ap);
    return s;
  }
};

[[gnu::noinline]] int call(B &b) { return b.sum(3, 1, 2, 3); }
[[gnu::noinline]] int twice(B &b) { return b.sum(1, 5) + b.sum(2, 5, 6); }

int main() {
  B b;
  D d;
  if (call(b) != 6 || call(d) != 106 || twice(b) != 16 || violations != 0)
    __builtin_abort();

  b.sum(-1);                        // pre fails
  if (violations != 1)
    __builtin_abort();
  b.sum(2, -5, -6);                 // post fails: r == -11
  if (violations != 2)
    __builtin_abort();
  if (b.make(2, 40, 2).v != 42 || violations != 3)
    __builtin_abort();              // post on a class result
  b.touch(3);
  b.touch(12);                      // capture c = 12 fails
  if (violations != 4)
    __builtin_abort();
  if (b.down(3) != 0 || violations != 4)
    __builtin_abort();              // recursive virtual calls
}

// REQUIRES: contracts-libcxx, native
