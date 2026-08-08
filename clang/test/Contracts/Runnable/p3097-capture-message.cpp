// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fcontracts-p3099 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097+P3098+P3099: a virtual function whose postcondition has BOTH a capture
// and a user-defined message; the violation carries the message and sees the
// captured value.  Clang handles this correctly; GCC currently ICEs (BUG-3, see
// wg21 testing-gap-catalogue.md sec 10) -- this Clang mirror is a passing
// regression test documenting the asymmetry.

#include <contracts>
#include <cstdio>
#include <cstring>

static int violation_count = 0;
static const char* last_message = nullptr;

void handle_contract_violation(const std::contracts::contract_violation& v) {
  ++violation_count;
  last_message = v.message();
}

static int state = 0;

struct Base {
  virtual int f()
    post [old = state] (r: r == old + 1, "bad increment")
  { return ++state; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int f() override { return state += 2; }  // disagrees with interface post (+1)
};

int main() {
  Derived d;
  Base& b = d;
  state = 0;
  violation_count = 0;
  last_message = nullptr;

  // captures old = 0, Derived sets state = 2; post 2 == 0+1 -> false.
  int r = b.f();
  if (r != 2) __builtin_abort();
  if (violation_count != 1) __builtin_abort();
  if (!last_message || std::strcmp(last_message, "bad increment") != 0)
    __builtin_abort();

  std::printf("PASS\n");
}
