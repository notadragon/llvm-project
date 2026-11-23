// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4298 -fcontracts-allow-assume -O2 %libcxx_flags -fcontract-configuration-file=%t/k.json -c %t/k.cpp -o %t/k.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4298 -fcontracts-allow-assume -O2 %libcxx_flags -fcontract-configuration-file=%t/u.json -c %t/u.cpp -o %t/u.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4298 -fcontracts-allow-assume -O2 %libcxx_flags -c %t/main.cpp -o %t/main.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4298 -fcontracts-allow-assume -O2 %libcxx_flags %t/k.o %t/u.o %t/main.o -o %t/ku && %t/ku
//
// The refinable-body rule for every evaluation semantic: a function another
// translation unit may also define (here an inline function) whose body
// holds a contract assertion must give the callers that do not inline it
// no fact derived from this translation unit's definition -- another
// definition may evaluate the assertion with another semantic, and the
// linker may keep that one.
//
// u.cpp holds the callers and the definitions they are compiled with;
// k.cpp, linked first, holds the definitions the linker keeps.  Each f is
// in a namespace of its own, which u.json and k.json give its semantic.
//
// a_*: the callers' definition evaluates the assertion with a semantic that
// does not continue past a violation (enforce, noexcept_enforce,
// quick_enforce and assume), but the kept definition ignores or observes
// it: use_a_* (-5) must be 0.  LLVM takes no fact from a linkonce_odr
// definition, so these passed before the refinable-body work too.
//
// b_*: the callers' definition cannot throw (ignore, quick_enforce,
// noexcept_enforce, noexcept_observe, assume, and a dynamic selection
// (P3595) that selects ignore), but the kept definition enforces the
// assertion and the violation handler throws: use_b_* (-5) must catch it
// (CLANG-824: TryMarkNoThrow marked the callers' definition nounwind).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/refinable-fork-sem.C

//--- k.json
[
  {"match": {"namespace": "a_enforce"}, "output": {"semantic": "ignore"}},
  {"match": {"namespace": "a_nx_enforce"}, "output": {"semantic": "ignore"}},
  {"match": {"namespace": "a_quick"}, "output": {"semantic": "ignore"}},
  {"match": {"namespace": "a_assume"}, "output": {"semantic": "ignore"}},
  {"match": {"namespace": "a_vs_observe"}, "output": {"semantic": "observe"}},
  {"match": {"namespace": "b_ignore"}, "output": {"semantic": "enforce"}},
  {"match": {"namespace": "b_quick"}, "output": {"semantic": "enforce"}},
  {"match": {"namespace": "b_nx_enforce"}, "output": {"semantic": "enforce"}},
  {"match": {"namespace": "b_nx_observe"}, "output": {"semantic": "enforce"}},
  {"match": {"namespace": "b_assume"}, "output": {"semantic": "enforce"}},
  {"match": {"namespace": "b_dyn"}, "output": {"semantic": "enforce"}}
]

//--- u.json
[
  {"match": {"namespace": "a_enforce"}, "output": {"semantic": "enforce"}},
  {"match": {"namespace": "a_nx_enforce"}, "output": {"semantic": "noexcept_enforce"}},
  {"match": {"namespace": "a_quick"}, "output": {"semantic": "quick_enforce"}},
  {"match": {"namespace": "a_assume"}, "output": {"semantic": "assume"}},
  {"match": {"namespace": "a_vs_observe"}, "output": {"semantic": "enforce"}},
  {"match": {"namespace": "b_ignore"}, "output": {"semantic": "ignore"}},
  {"match": {"namespace": "b_quick"}, "output": {"semantic": "quick_enforce"}},
  {"match": {"namespace": "b_nx_enforce"}, "output": {"semantic": "noexcept_enforce"}},
  {"match": {"namespace": "b_nx_observe"}, "output": {"semantic": "noexcept_observe"}},
  {"match": {"namespace": "b_assume"}, "output": {"semantic": "assume"}},
  {"match": {"namespace": "b_dyn"}, "output": {"semantic": "enforce", "dynamic": {"linkage": "C++", "name": "refinable_fork_sem_sel"}}}
]

//--- k.cpp
namespace a_enforce {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // ignore
  return x;
}
}

namespace a_nx_enforce {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // ignore
  return x;
}
}

namespace a_quick {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // ignore
  return x;
}
}

namespace a_assume {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // ignore
  return x;
}
}

namespace a_vs_observe {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // observe
  return x;
}
}

namespace b_ignore {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // enforce
  return x;
}
}

namespace b_quick {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // enforce
  return x;
}
}

namespace b_nx_enforce {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // enforce
  return x;
}
}

namespace b_nx_observe {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // enforce
  return x;
}
}

namespace b_assume {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // enforce
  return x;
}
}

namespace b_dyn {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // enforce
  return x;
}
}

int (*volatile keep[]) (int) = {
  a_enforce::f,
  a_nx_enforce::f,
  a_quick::f,
  a_assume::f,
  a_vs_observe::f,
  b_ignore::f,
  b_quick::f,
  b_nx_enforce::f,
  b_nx_observe::f,
  b_assume::f,
  b_dyn::f,
};

//--- u.cpp
#include <contracts>

// Selected dynamically for b_dyn; visible here, so the selection folds.
std::contracts::evaluation_semantic refinable_fork_sem_sel ()
{
  return std::contracts::evaluation_semantic::ignore;
}

namespace a_enforce {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // enforce
  return x;
}
}

namespace a_nx_enforce {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // noexcept_enforce
  return x;
}
}

namespace a_quick {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // quick_enforce
  return x;
}
}

namespace a_assume {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // assume
  return x;
}
}

namespace a_vs_observe {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // enforce
  return x;
}
}

namespace b_ignore {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // ignore
  return x;
}
}

namespace b_quick {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // quick_enforce
  return x;
}
}

namespace b_nx_enforce {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // noexcept_enforce
  return x;
}
}

namespace b_nx_observe {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // noexcept_observe
  return x;
}
}

namespace b_assume {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // assume
  return x;
}
}

namespace b_dyn {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // dynamic, ignore
  return x;
}
}

int use_a_enforce (int y) { return a_enforce::f (y) > 0; }
int use_a_nx_enforce (int y) { return a_nx_enforce::f (y) > 0; }
int use_a_quick (int y) { return a_quick::f (y) > 0; }
int use_a_assume (int y) { return a_assume::f (y) > 0; }
int use_a_vs_observe (int y) { return a_vs_observe::f (y) > 0; }

int use_b_ignore (int y) { try { return b_ignore::f (y); } catch (int) { return -1; } }
int use_b_quick (int y) { try { return b_quick::f (y); } catch (int) { return -1; } }
int use_b_nx_enforce (int y) { try { return b_nx_enforce::f (y); } catch (int) { return -1; } }
int use_b_nx_observe (int y) { try { return b_nx_observe::f (y); } catch (int) { return -1; } }
int use_b_assume (int y) { try { return b_assume::f (y); } catch (int) { return -1; } }
int use_b_dyn (int y) { try { return b_dyn::f (y); } catch (int) { return -1; } }

//--- main.cpp
#include <contracts>
#include <cstdlib>

int handled;
bool throwing;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++handled;
  if (throwing)
    throw 42;
}

int use_a_enforce (int);
int use_a_nx_enforce (int);
int use_a_quick (int);
int use_a_assume (int);
int use_a_vs_observe (int);
int use_b_ignore (int);
int use_b_quick (int);
int use_b_nx_enforce (int);
int use_b_nx_observe (int);
int use_b_assume (int);
int use_b_dyn (int);

int main ()
{
  if (use_a_enforce (-5) != 0
      || use_a_nx_enforce (-5) != 0
      || use_a_quick (-5) != 0
      || use_a_assume (-5) != 0
      || handled != 0)
    std::abort ();
  if (use_a_vs_observe (-5) != 0 || handled != 1)
    std::abort ();

  throwing = true;
  int (*const uses[]) (int) = { use_b_ignore, use_b_quick, use_b_nx_enforce,
                                use_b_nx_observe, use_b_assume, use_b_dyn };
  for (auto use : uses)
    {
      int h = handled;
      if (use (-5) != -1 || handled != h + 1)
        std::abort ();
    }
}
