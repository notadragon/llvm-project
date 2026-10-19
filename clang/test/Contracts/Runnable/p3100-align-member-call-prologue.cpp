// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.store:noexcept_observe \
// RUN:   %libcxx_flags -o %t
// RUN: %t | FileCheck %s --check-prefix=O0
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.store:noexcept_observe \
// RUN:   %libcxx_flags -o %t2
// RUN: %t2 | FileCheck %s --check-prefix=O2

// P3100: a call to a member function that is not inlined, through a
// misaligned pointer, with the alignment check (.store) configured: the
// check at the call reports it, and the check of `this` in the member
// function's prologue reports it again at -O0; at -O2 the optimizer
// deletes the prologue check (`this` is an `align 4` parameter): once.  CLANG-815, accepted under DECISIONS.md E120 (as
// the null check's CLANG-792, E114 (b)): the prologue check is the only
// check for a caller compiled without the P3100 check or with its leaf
// ignored, so dropping it would need caller-to-callee state.  Forms:
// through a `char *` parameter plus an offset, and through a character
// array plus an offset (CLANG-810's cast).
// (GCC: gnu_gcc makes no prologue check and reports each once.)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++reports;
}

#define NI __attribute__((noinline))
alignas(8) char buf[32];
struct P {
  int a, b;
  NI int f() { return 0; }
};
NI int pcall(char *c, int o) { return ((P *)(c + o))->f(); }
NI int gcall(int o) { return ((P *)(buf + o))->f(); }

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    __VA_ARGS__;                                                               \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  int volatile one = 1, zero = 0;
  COUNT("pcall", pcall(buf, one));
  COUNT("gcall", gcall(one));
  COUNT("aligned", pcall(buf, zero); gcall(zero));
  return 0;
}

// O0: pcall 2
// O0-NEXT: gcall 2
// O0-NEXT: aligned 0

// O2: pcall 1
// O2-NEXT: gcall 1
// O2-NEXT: aligned 0
