// General contracts/noexcept correctness (surfaced during D4298 review):
// same guarantee as p4298-noexcept-fn-inline.cpp, exercised through the
// P3098 postcondition-capture codegen path (a captured postcondition,
// EmitPostconditionCaptureInit + the IsPostCapture violation-call path)
// instead of a plain inline check.  Plain "enforce" semantic, not one of
// D4298's new semantics.  Unlike GCC, Clang has no outlined postcondition
// function -- captures are ordinary automatic locals -- so this exercises
// the capture path rather than an outlined-function noexcept boundary.
// (GCC mirror in spirit: g++.dg/contracts/cpp26/p4298-noexcept-fn-p3098.C)
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: %t

#include <contracts>
#include <exception>
#include <cstdlib>

struct MyException {};

void my_term()
{
  try { throw; }
  catch (MyException) { std::exit(0); }
}

void handle_contract_violation(const std::contracts::contract_violation&)
{
  throw MyException{};
}

// Captured postcondition: c is captured at entry (= x), checked after.
// For x = -1 the check c > 0 fails -> violation in a noexcept function.
int f(int x) noexcept post [c = x] (c > 0) { return x; }

int main()
{
  std::set_terminate(my_term);
  try {
    f(-1);
  } catch (...) {
  }
  __builtin_trap();  // We should not get here.
}
