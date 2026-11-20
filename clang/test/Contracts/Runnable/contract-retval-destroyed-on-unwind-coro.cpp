// The coroutine half of contract-retval-destroyed-on-unwind.cpp: when a
// violation handler throws out of a postcondition on a coroutine, the
// coroutine's return object must still be destroyed, exactly once.  A
// coroutine's ramp reaches the postcondition by a different route than an
// ordinary function's return statement, which is why it is tested
// separately: a coroutine that completes in the ramp, one that suspends (the
// frame outlives the ramp), and the normal-completion control.
//
// See the header of contract-retval-destroyed-on-unwind.cpp for why the
// returned object is destroyed here at all: no wording currently requires it
// and a core issue is open, so this deliberately runs ahead of the standard.
// Do not "fix" it back to expecting a leak.
//
// The return type counts constructions against destructions, so a leak and a
// double destroy both fail.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/contract-retval-destroyed-on-unwind-coro.C
//
// RUN: %clangxx -std=c++26 %s -fcontracts \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

#include <contracts>
#include <coroutine>

int live = 0;
int destroyed = 0;

struct E { };

bool handler_should_throw = false;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  if (handler_should_throw)
    throw E { };
}

/* A coroutine return object that counts.  Non-trivially destructible, so it
   is exactly the kind of object an unwind must not leak.  */

struct CountedTask {
  int v;

  struct promise_type {
    int v = 0;
    CountedTask get_return_object () { return CountedTask (v); }
    std::suspend_never initial_suspend () { return { }; }
    std::suspend_never final_suspend () noexcept { return { }; }
    void return_void () { }
    void unhandled_exception () { }
  };

  CountedTask (int x) : v (x) { ++live; }
  CountedTask (const CountedTask &o) : v (o.v) { ++live; }
  ~CountedTask () { --live; ++destroyed; }
};

/* The postcondition is on the RAMP, so it is checked against the return
   object once the coroutine has been started -- after that object exists.  */

CountedTask
coro_post_fails () post (r : r.v > 100)
{
  co_return;
}

CountedTask
coro_post_passes () post (r : r.v == 0)
{
  co_return;
}

/* A coroutine that suspends, so the frame outlives the ramp and the return
   object is destroyed on a path where the coroutine is still alive.  */

CountedTask
coro_suspends () post (r : r.v > 100)
{
  co_await std::suspend_always { };
}

static void
check (int got, int want)
{
  if (got != want)
    __builtin_abort ();
}

template <class F>
static void
expect_throw_balanced (F f)
{
  live = 0;
  destroyed = 0;
  bool threw = false;
  try { f (); } catch (E &) { threw = true; }
  if (!threw)
    __builtin_abort ();
  check (live, 0);			/* 0 means: no leak, no double destroy */
  if (destroyed == 0)
    __builtin_abort ();			/* something must have been destroyed */
}

int
main ()
{
  /* The handler throws out of the ramp's postcondition: the return object
     exists and must be destroyed on the way out.  */
  handler_should_throw = true;
  expect_throw_balanced ([] { coro_post_fails (); });
  expect_throw_balanced ([] { coro_suspends (); });

  /* Control: normal completion destroys it exactly once, when the caller is
     done with it -- not early, and not twice.  */
  handler_should_throw = false;
  live = 0;
  destroyed = 0;
  {
    CountedTask got = coro_post_passes ();
    check (got.v, 0);
    check (live, 1);			/* still alive: the caller holds it */
    check (destroyed, 0);
  }
  check (live, 0);
  check (destroyed, 1);

  return 0;
}
