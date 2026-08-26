// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags \
// RUN:   -Wno-exceptions -S -emit-llvm -o %t.ll
// RUN: FileCheck --check-prefix=EX %s < %t.ll
// RUN: FileCheck --check-prefix=PF %s < %t.ll

// P3400: the bypass is only sound if the rethrow actually escapes to the
// caller.  A rethrow that unwinds out of a nothrow function calls
// std::terminate instead, which is not what skipping the EH region would do,
// so any nothrow frame between the rethrow and the check disqualifies it.
//
// The walk applies that test at every frame as it unwinds, not just at the
// handler.  -Wno-exceptions: several handlers here deliberately rethrow from
// a nothrow frame, which is the very thing being tested.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-10.C)

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::detection_mode;

bool boom ();

// 1. The handler itself is noexcept and rethrows: terminates, does not escape.
struct nothrow_handler_t {
  using assertion_control_object = nothrow_handler_t;
  void handle_contract_violation (const contract_violation& v) const noexcept {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      throw;
  }
};
constexpr nothrow_handler_t nothrow_handler{};
int f1 (int i) pre<nothrow_handler> (boom ()) { return i; }

// 2. The handler may throw, but the helper it delegates to may not.
inline void nothrow_helper () noexcept { throw; }

struct via_nothrow_helper_t {
  using assertion_control_object = via_nothrow_helper_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      nothrow_helper ();
  }
};
constexpr via_nothrow_helper_t via_nothrow_helper{};
int f2 (int i) pre<via_nothrow_helper> (boom ()) { return i; }

// 3. The nothrow frame is two deep, reached through a throwing one.
inline void throwing_outer () { nothrow_helper (); }

struct via_two_levels_t {
  using assertion_control_object = via_two_levels_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      throwing_outer ();
  }
};
constexpr via_two_levels_t via_two_levels{};
int f3 (int i) pre<via_two_levels> (boom ()) { return i; }

// 4. A dependent noexcept-specification that resolves to true, on a helper ...
template <class T> void dependent_helper () noexcept (sizeof (T) > 0)
{ throw; }

struct via_dependent_t {
  using assertion_control_object = via_dependent_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      dependent_helper<int> ();
  }
};
constexpr via_dependent_t via_dependent{};
int f4 (int i) pre<via_dependent> (boom ()) { return i; }

// 5. ... and on the handler itself.
template <class T>
struct dependent_handler_t {
  using assertion_control_object = dependent_handler_t;
  void handle_contract_violation (const contract_violation& v) const
    noexcept (sizeof (T) > 0)
  {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      throw;
  }
};
constexpr dependent_handler_t<int> dependent_handler{};
int f5 (int i) pre<dependent_handler> (boom ()) { return i; }

// 6. Control: the same shape as 3 with nothing nothrow anywhere, which must
//    still optimize -- otherwise the five above would prove nothing.
inline void throwing_helper () { throw; }
inline void throwing_outer2 () { throwing_helper (); }

struct all_throwing_t {
  using assertion_control_object = all_throwing_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      throwing_outer2 ();
  }
};
constexpr all_throwing_t all_throwing{};
int f6 (int i) pre<all_throwing> (boom ()) { return i; }

// 7. A nothrow call that merely *returns* disqualifies nothing.  The rethrow
//    that follows it is not inside that frame, so it still escapes -- and a
//    handler doing some trivial nothrow bookkeeping before rethrowing is a
//    perfectly ordinary shape to want optimized.
inline void nothrow_but_returns () noexcept { }

struct nothrow_call_then_throw_t {
  using assertion_control_object = nothrow_call_then_throw_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      {
    nothrow_but_returns ();
    throw;
      }
  }
};
constexpr nothrow_call_then_throw_t nothrow_call_then_throw{};
int f7 (int i) pre<nothrow_call_then_throw> (boom ()) { return i; }

// Seven checks emitted; the five disqualified ones keep their catch, the two
// controls do not.

// Seven checks emitted; the five disqualified ones keep their catch, the two
// controls do not.  The counts include the declaration as well as the call
// sites, and each trailing NOT pins the total exactly.
//
// EX-COUNT-6: @__cxa_contract_violation_pre_enforce_ex
// EX-NOT: @__cxa_contract_violation_pre_enforce_ex
// PF-COUNT-8: @__cxa_contract_violation_pre_enforce_pf
// PF-NOT: @__cxa_contract_violation_pre_enforce_pf
