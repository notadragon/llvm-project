// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4298 -fcontracts-allow-assume -O2 %libcxx_flags -fcontract-configuration-file=%t/k.json -c %t/k.cpp -o %t/k.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4298 -fcontracts-allow-assume -O2 %libcxx_flags -fcontract-configuration-file=%t/u.json -c %t/u.cpp -o %t/u.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4298 -fcontracts-allow-assume -O2 %libcxx_flags -c %t/main.cpp -o %t/main.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4298 -fcontracts-allow-assume -O2 %libcxx_flags %t/k.o %t/u.o %t/main.o -o %t/ku && %t/ku
//
// The refinable-body rule with P3400 labels: an assertion whose label's
// allowed_semantics facet (which every translation unit sees in the
// declaration) settles its evaluation does not keep its function from being
// marked nounwind from its body; any other does
// (isContractEvaluationSettled).
//
// u.cpp holds the callers and the definitions they are compiled with;
// k.cpp, linked first, holds the definitions the linker keeps.  Each f is in
// a namespace of its own, which u.json and k.json give its semantic.
//
// u_open: {enforce, observe}: the callers' definition enforces, the kept one
// observes and continues; use_u_open (-5) must be 0 (LLVM takes no fact
// from a linkonce_odr definition).
// u_mixed: {enforce, quick_enforce}: the callers' definition traps and
// cannot throw, the kept one enforces with a handler that throws;
// use_u_mixed (-5) must catch it.
// s_*: settled -- {quick_enforce, noexcept_enforce, assume}, {enforce,
// quick_enforce} on a noexcept function, {enforce} alone: whichever
// definition runs, a call that returns returns the same; valid arguments
// only, since a violation ends the program (refinable-body-label.cpp shows
// that their callers drop their landing pads).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/refinable-fork-label.C

//--- k.json
[
  {"match": {"namespace": "u_open"}, "output": {"semantic": "observe"}},
  {"match": {"namespace": "u_mixed"}, "output": {"semantic": "enforce"}},
  {"match": {"namespace": "s_settled"}, "output": {"semantic": "noexcept_enforce"}},
  {"match": {"namespace": "s_mixed_nx"}, "output": {"semantic": "enforce"}},
  {"match": {"namespace": "s_enforce_only"}, "output": {"semantic": "enforce"}}
]

//--- u.json
[
  {"match": {"namespace": "u_open"}, "output": {"semantic": "enforce"}},
  {"match": {"namespace": "u_mixed"}, "output": {"semantic": "quick_enforce"}},
  {"match": {"namespace": "s_settled"}, "output": {"semantic": "quick_enforce"}},
  {"match": {"namespace": "s_mixed_nx"}, "output": {"semantic": "quick_enforce"}},
  {"match": {"namespace": "s_enforce_only"}, "output": {"semantic": "enforce"}}
]

//--- k.cpp
#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template <evaluation_semantic... S>
struct allow {
  using assertion_control_object = allow;
  static constexpr evaluation_semantic_set allowed_semantics{S...};
};

inline constexpr allow<enforce, observe> l_open{};
inline constexpr allow<enforce, quick_enforce> l_mixed{};
inline constexpr allow<quick_enforce, noexcept_enforce, assume> l_settled{};
inline constexpr allow<enforce> l_enforce{};

namespace u_open {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert<l_open> (x > 0);  // observe
  return x;
}
}

namespace u_mixed {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert<l_mixed> (x > 0);  // enforce
  return x;
}
}

namespace s_settled {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert<l_settled> (x > 0);  // noexcept_enforce
  return x;
}
}

namespace s_mixed_nx {
[[gnu::noinline]] inline int f (int x) noexcept
{
  contract_assert<l_mixed> (x > 0);  // enforce
  return x;
}
}

namespace s_enforce_only {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert<l_enforce> (x > 0);  // enforce
  return x;
}
}

int (*volatile keep[]) (int) = {
  u_open::f,
  u_mixed::f,
  s_settled::f,
  s_mixed_nx::f,
  s_enforce_only::f,
};

//--- u.cpp
#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template <evaluation_semantic... S>
struct allow {
  using assertion_control_object = allow;
  static constexpr evaluation_semantic_set allowed_semantics{S...};
};

inline constexpr allow<enforce, observe> l_open{};
inline constexpr allow<enforce, quick_enforce> l_mixed{};
inline constexpr allow<quick_enforce, noexcept_enforce, assume> l_settled{};
inline constexpr allow<enforce> l_enforce{};

namespace u_open {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert<l_open> (x > 0);  // enforce
  return x;
}
}

namespace u_mixed {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert<l_mixed> (x > 0);  // quick_enforce
  return x;
}
}

namespace s_settled {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert<l_settled> (x > 0);  // quick_enforce
  return x;
}
}

namespace s_mixed_nx {
[[gnu::noinline]] inline int f (int x) noexcept
{
  contract_assert<l_mixed> (x > 0);  // quick_enforce
  return x;
}
}

namespace s_enforce_only {
[[gnu::noinline]] inline int f (int x)
{
  contract_assert<l_enforce> (x > 0);  // enforce
  return x;
}
}

int use_u_open (int y) { return u_open::f (y) > 0; }
int use_u_mixed (int y) { try { return u_mixed::f (y); } catch (int) { return -1; } }
int use_s_settled (int y) { return s_settled::f (y) > 0; }
int use_s_mixed_nx (int y) { return s_mixed_nx::f (y) > 0; }
int use_s_enforce_only (int y) { return s_enforce_only::f (y) > 0; }

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

int use_u_open (int);
int use_u_mixed (int);
int use_s_settled (int);
int use_s_mixed_nx (int);
int use_s_enforce_only (int);

int main ()
{
  if (use_u_open (-5) != 0 || handled != 1)
    std::abort ();
  throwing = true;
  if (use_u_mixed (-5) != -1 || handled != 2)
    std::abort ();
  if (use_s_settled (5) != 1
      || use_s_mixed_nx (5) != 1
      || use_s_enforce_only (5) != 1
      || handled != 2)
    std::abort ();
}
