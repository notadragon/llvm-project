// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -S -emit-llvm -o - | \
// RUN:   FileCheck %s \
// RUN:     --implicit-check-not=__cxa_contract_violation_pre_enforce_ex
//
// And again under -g.  GCC lost this optimization to debug info and nothing
// caught it, because every test there compiled without -g -- while Compiler
// Explorer, and most real builds, always pass it (gnu_gcc: the walk treated
// DEBUG_BEGIN_STMT as an unanalysable statement).  Clang walks the AST and so
// never had that hole, but nothing pinned it either.  GCC mirror:
// g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-9.C.
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -g \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -S -emit-llvm -o - | \
// RUN:   FileCheck %s \
// RUN:     --implicit-check-not=__cxa_contract_violation_pre_enforce_ex

// P3400: codegen for the bypass.  Each of these handlers provably
// rethrows an evaluation_exception, so no check wraps its predicate in an EH
// region -- the _ex entry point, only ever called from that region, is absent.
// The predicate-false path is untouched, which the positive CHECK pins.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-3.C)

#include <contracts>
#include <exception>

using std::contracts::contract_violation;
using std::contracts::detection_mode;
using std::contracts::violation_handled;
using std::contracts::labels::operator|;

bool boom(); // Not noexcept: the predicate might throw.

// 1. The canonical shape.
struct plain_t {
  using assertion_control_object = plain_t;
  void handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      throw;
  }
};
constexpr plain_t plain{};
int f1(int i) pre<plain>(boom()) { return i; }

// 2. Through a local variable, with the test inverted and an early return.
struct via_local_t {
  using assertion_control_object = via_local_t;
  void handle_contract_violation(const contract_violation& v) const {
    const auto dm = v.detection_mode();
    if (dm != detection_mode::evaluation_exception)
      return;
    throw;
  }
};
constexpr via_local_t via_local{};
int f2(int i) pre<via_local>(boom()) { return i; }

// 3. rethrow_exception(current_exception()) reaches the same place.
struct via_ptr_t {
  using assertion_control_object = via_ptr_t;
  void handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      std::rethrow_exception(std::current_exception());
  }
};
constexpr via_ptr_t via_ptr{};
int f3(int i) pre<via_ptr>(boom()) { return i; }

// 4. The semantic is known where the check is emitted too, so a handler that
//    also tests is_terminating() still resolves (enforce is terminating).
struct checks_terminating_t {
  using assertion_control_object = checks_terminating_t;
  void handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception &&
        v.is_terminating())
      throw;
  }
};
constexpr checks_terminating_t checks_terminating{};
int f4(int i) pre<checks_terminating>(boom()) { return i; }

// 5. A handler returning violation_handled: the rethrow comes first, so the
//    return is unreachable on the exception path.
struct returns_handled_t {
  using assertion_control_object = returns_handled_t;
  violation_handled handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      throw;
    return violation_handled::not_handled;
  }
};
constexpr returns_handled_t returns_handled{};
int f5(int i) pre<returns_handled>(boom()) { return i; }

// 6. Delegation: the walk follows the call, so a helper whose body is just a
//    rethrow counts the same as writing the rethrow here.
inline void rethrow_helper() { throw; }

struct delegates_t {
  using assertion_control_object = delegates_t;
  void handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      rethrow_helper();
  }
};
constexpr delegates_t delegates{};
int f6(int i) pre<delegates>(boom()) { return i; }

// 7. The helper can be the one that inspects the violation: the accessors keep
//    folding across the call because the parameter carries through.
inline void rethrow_if_exception(const contract_violation& v) {
  if (v.detection_mode() == detection_mode::evaluation_exception)
    throw;
}

struct delegates_with_arg_t {
  using assertion_control_object = delegates_with_arg_t;
  void handle_contract_violation(const contract_violation& v) const {
    rethrow_if_exception(v);
  }
};
constexpr delegates_with_arg_t delegates_with_arg{};
int f7(int i) pre<delegates_with_arg>(boom()) { return i; }

// 8. A combined label reaches its component handler through
//    __combined_label::handle_contract_violation, which is just more
//    delegation -- no special case in the analysis.
struct comment_t {
  using assertion_control_object = comment_t;
  constexpr const char* compute_comment(const char* c) const { return c; }
};
constexpr comment_t comment{};
int f8(int i) pre<plain | comment>(boom()) { return i; }

// 9. Same, with the rethrowing component on the right: it only works because
//    the component before it provably does nothing under this detection mode
//    and returns a value the walk can fold.
struct only_predicate_false_t {
  using assertion_control_object = only_predicate_false_t;
  violation_handled handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::predicate_false)
      throw;
    return violation_handled::not_handled;
  }
};
constexpr only_predicate_false_t only_predicate_false{};
int f9(int i) pre<only_predicate_false | returns_handled>(boom()) { return i; }

// Every check is still emitted; the implicit-check-not above proves not one of
// them catches the predicate's exception.
// CHECK-COUNT-9: @__cxa_contract_violation_pre_enforce_pf
