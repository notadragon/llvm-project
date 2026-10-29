// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t | FileCheck %s

// P3400: a combined label runs its components' local violation handlers
// right to left -- control flows back from the predicate towards the global
// handler, which comes last -- and one that returns handled stops the chain
// (they ran left to right).
// (GCC mirror: g++.dg/contracts/cpp26/p3400-local-handler-chain-order.C; the
// Clang side was CLANG-622.)

#include <contracts>
#include <cstdio>

using namespace std::contracts;
using namespace std::contracts::labels;

void handle_contract_violation (const contract_violation &)
{
  std::printf (" global");
}

struct l1_t
{
  using assertion_control_object = l1_t;
  violation_handled handle_contract_violation (const contract_violation &) const
  {
    std::printf (" l1");
    return violation_handled::not_handled;
  }
};
struct l2_t
{
  using assertion_control_object = l2_t;
  void handle_contract_violation (const contract_violation &) const
  {
    std::printf (" l2");
  }
};
struct h_t
{
  using assertion_control_object = h_t;
  static violation_handled handle_contract_violation (const contract_violation &)
  {
    std::printf (" h");
    return violation_handled::handled;
  }
};
constexpr l1_t l1{};
constexpr l2_t l2{};
constexpr h_t h{};

void a (int x) pre<l1 | l2> (x > 0) {}
void b (int x) pre<l1 | h> (x > 0) {}
void c (int x) pre<h | l1> (x > 0) {}
void d (int x) pre<l1 | l2 | h> (x > 0) {}
void e (int x) pre<h | l1 | l2> (x > 0) {}

int
main ()
{
  std::printf ("a:"); a (-1); std::printf ("\n");
  std::printf ("b:"); b (-1); std::printf ("\n");
  std::printf ("c:"); c (-1); std::printf ("\n");
  std::printf ("d:"); d (-1); std::printf ("\n");
  std::printf ("e:"); e (-1); std::printf ("\n");
}

// CHECK: a: l2 l1 global
// CHECK-NEXT: b: h
// CHECK-NEXT: c: l1 h
// CHECK-NEXT: d: h
// CHECK-NEXT: e: l2 l1 h
