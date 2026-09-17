// RUN: %clangxx -std=c++26 %s -fno-exceptions -fcontracts -fcontracts-p3850 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// The contracts surface under -fno-exceptions.
//
// The whole violation model is exception-shaped -- a handler may throw, a
// predicate may throw, P3098 gives captures a five-phase exception model,
// P4298 classifies checks as terminating -- so every part of it has to
// degrade sensibly when there are no exceptions at all.  Clang gets all of
// it right, and produces the same observable behaviour as with exceptions
// enabled.
//
// The POSTCONDITION CAPTURES below are the reason this file matters rather
// than merely duplicating the GCC mirror.  GCC rejects every capture shape
// outright with -fno-exceptions ("exception handling disabled, use
// '-fexceptions' to enable"), because its capture lowering builds the
// try/catch for a throwing capture initializer unconditionally.  Clang does
// not, so this file pins Clang's correct behaviour and stops it regressing
// to match -- the GCC mirror,
// g++.dg/contracts/cpp26/noexceptions-postcondition-capture.C, is an xfail
// for the same property.
//
// GCC mirror: g++.dg/contracts/cpp26/noexceptions-contracts.C

#include <contracts>
#include <coroutine>

static int violations = 0;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

static int clamp(const int x) pre(x >= 0) post(r : r >= 0) { return x; }

// A predicate that calls a function with no exception specification is what
// turns on the catch around predicate evaluation when exceptions are enabled;
// with -fno-exceptions it must simply not be built.
bool maybe_throws(int x) { return x > 0; }

static int predicate_may_throw(const int x) pre(maybe_throws(x)) { return x; }

// Postcondition captures, in every shape GCC rejects here.
struct Tracked {
  static int live;
  int v;
  Tracked(int v) : v(v) { ++live; }
  Tracked(const Tracked &o) : v(o.v) { ++live; }
  ~Tracked() { --live; }
};
int Tracked::live = 0;

struct TrivialDtor {
  int v;
};

static int scalar_capture(const int x) post[old = x](old == x) { return x; }

static int trivial_dtor_capture(const int x)
    post[old = TrivialDtor{x}](old.v == x) {
  return x;
}

static int non_trivial_capture(const int x) post[old = Tracked(x)](old.v == x) {
  return x;
}

static int several_captures(const int x) post[a = x, b = x + 1](a + 1 == b) {
  return x;
}

struct Base {
  virtual int f(const int x) pre(x > 0) post(r : r > 0) { return x; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int f(const int x) override { return x; }
};

struct VirtualCapture {
  virtual int f(const int x) post[old = x](old == x) { return x; }
  virtual ~VirtualCapture() = default;
};

template <class T> static T templated(const T x) pre(x != T{}) post(r : r == x) {
  return x;
}

static int non_throwing(const int x) noexcept pre(x > 0) { return x; }

// A coroutine: its promise still has unhandled_exception, and the ramp still
// evaluates the contracts, with no exceptions available anywhere.
struct Task {
  struct promise_type {
    int value = 0;
    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_value(int v) { value = v; }
    void unhandled_exception() {}
  };

  std::coroutine_handle<promise_type> h;
  ~Task() {
    if (h)
      h.destroy();
  }
  int get() const { return h.promise().value; }
};

static Task doubler(const int x) pre(x > 0) post(r : true) { co_return x * 2; }

int main() {
  int before;

  before = violations;
  clamp(1);
  if (violations != before)
    __builtin_abort();

  before = violations;
  clamp(-1); // both the precondition and the postcondition fail
  if (violations - before != 2)
    __builtin_abort();

  before = violations;
  predicate_may_throw(-1);
  if (violations - before != 1)
    __builtin_abort();

  before = violations;
  if (scalar_capture(4) != 4 || trivial_dtor_capture(4) != 4)
    __builtin_abort();
  if (non_trivial_capture(4) != 4 || several_captures(4) != 4)
    __builtin_abort();
  if (violations != before)
    __builtin_abort();
  if (Tracked::live != 0) // the capture was destroyed
    __builtin_abort();

  {
    VirtualCapture vc;
    VirtualCapture *pv = &vc;
    before = violations;
    if (pv->f(4) != 4 || violations != before)
      __builtin_abort();
  }

  Derived d;
  Base *p = &d;
  before = violations;
  p->f(1);
  if (violations != before)
    __builtin_abort();
  before = violations;
  p->f(-1);
  if (violations - before != 2)
    __builtin_abort();

  before = violations;
  templated(7);
  templated(0);
  if (violations - before != 1)
    __builtin_abort();

  before = violations;
  non_throwing(-1);
  if (violations - before != 1)
    __builtin_abort();

  before = violations;
  contract_assert(1 == 2);
  if (violations - before != 1)
    __builtin_abort();

  before = violations;
  {
    Task t = doubler(3);
    if (t.get() != 6 || violations != before)
      __builtin_abort();
  }
  before = violations;
  {
    Task t = doubler(-1);
    (void)t;
    if (violations - before != 1)
      __builtin_abort();
  }

  // A non-throwing function stays non-throwing; a contract does not make an
  // ordinary function noexcept.
  if (!noexcept(non_throwing(1)))
    __builtin_abort();
  if (noexcept(clamp(1)))
    __builtin_abort();

  return 0;
}
