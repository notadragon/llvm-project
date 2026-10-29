// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t
// Generated from a contract-feature cross-product specification.
//
// Contract assertions met during a notional or manifestly constant evaluation.
//
// A contract assertion met while evaluating an initializer, an array
// bound or a required constant is evaluated, but what is reported depends on the
// rest of the evaluation.  A false predicate, or one that is not a core constant
// expression, is recorded and evaluation continues.  If the evaluation outside
// the predicates then completes as a constant, each recorded problem is a
// contract violation ([basic.contract.eval]): an error under a terminating
// semantic, a warning under observe, reported once at its own assertion.  If it
// does not complete, everything recorded is discarded -- the variable is
// initialized at run time (an array bound becomes a VLA), or a required constant
// is simply not constant -- and the predicates are evaluated at run time.
//
// Each cell sequences two assertions, each true, false, never constant, constant
// only for a known argument, or reading the argument, against a body that is
// constant, reads the argument, or is never constant, in contexts that decide
// by notional evaluation, that require a constant, or that are not manifestly
// constant-evaluated at all.  The run cells check the run-time half: where the
// notional evaluation fails, the violations are still reported, once each.
//
// Mirror: g++.dg/contracts/cpp26/matrix-notional-run.C

#include <contracts>

int violations = 0;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

bool
rt ()
{
  return true;
}

// --------------------------------------------------------------------------
// Two preconditions, the first false, the second never constant (calls a non-constexpr function); the body is not constant, so a const local is initialized at run time and the predicates are evaluated there: 1 violation reported.
// --------------------------------------------------------------------------
namespace run_const_local_F_NC {
constexpr int
f (const int &v)
  pre (false)
  pre (rt ())
{
  return rt () ? 1 : 1;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}

int
check ()
{
  int before = violations;
  if (use (1) != 1)
    return 1;
  return violations - before == 1 ? 0 : 2;
}
}  // namespace run_const_local_F_NC

// --------------------------------------------------------------------------
// Two preconditions, the first never constant (calls a non-constexpr function), the second false; the body is not constant, so a const local is initialized at run time and the predicates are evaluated there: 1 violation reported.
// --------------------------------------------------------------------------
namespace run_const_local_NC_F {
constexpr int
f (const int &v)
  pre (rt ())
  pre (false)
{
  return rt () ? 1 : 1;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}

int
check ()
{
  int before = violations;
  if (use (1) != 1)
    return 1;
  return violations - before == 1 ? 0 : 2;
}
}  // namespace run_const_local_NC_F

// --------------------------------------------------------------------------
// Two preconditions, the first false, the second false; the body is not constant, so a const local is initialized at run time and the predicates are evaluated there: 2 violations reported.
// --------------------------------------------------------------------------
namespace run_const_local_F_F {
constexpr int
f (const int &v)
  pre (false)
  pre (false)
{
  return rt () ? 1 : 1;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}

int
check ()
{
  int before = violations;
  if (use (1) != 1)
    return 1;
  return violations - before == 2 ? 0 : 2;
}
}  // namespace run_const_local_F_F

// --------------------------------------------------------------------------
// Two preconditions, the first true, the second never constant (calls a non-constexpr function); the body is not constant, so a const local is initialized at run time and the predicates are evaluated there: 0 violations reported.
// --------------------------------------------------------------------------
namespace run_const_local_T_NC {
constexpr int
f (const int &v)
  pre (true)
  pre (rt ())
{
  return rt () ? 1 : 1;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}

int
check ()
{
  int before = violations;
  if (use (1) != 1)
    return 1;
  return violations - before == 0 ? 0 : 2;
}
}  // namespace run_const_local_T_NC

// --------------------------------------------------------------------------
// Two preconditions, the first false, the second never constant (calls a non-constexpr function); the body is not constant, so a static local is initialized at run time and the predicates are evaluated there: 1 violation reported.
// --------------------------------------------------------------------------
namespace run_static_local_F_NC {
constexpr int
f (const int &v)
  pre (false)
  pre (rt ())
{
  return rt () ? 1 : 1;
}

int
use (int p)
{
  static int k = f (p);
  return k;
}

int
check ()
{
  int before = violations;
  if (use (1) != 1)
    return 1;
  return violations - before == 1 ? 0 : 2;
}
}  // namespace run_static_local_F_NC

// --------------------------------------------------------------------------
// Two preconditions, the first never constant (calls a non-constexpr function), the second false; the body is not constant, so a static local is initialized at run time and the predicates are evaluated there: 1 violation reported.
// --------------------------------------------------------------------------
namespace run_static_local_NC_F {
constexpr int
f (const int &v)
  pre (rt ())
  pre (false)
{
  return rt () ? 1 : 1;
}

int
use (int p)
{
  static int k = f (p);
  return k;
}

int
check ()
{
  int before = violations;
  if (use (1) != 1)
    return 1;
  return violations - before == 1 ? 0 : 2;
}
}  // namespace run_static_local_NC_F

// --------------------------------------------------------------------------
// Two preconditions, the first false, the second false; the body is not constant, so a static local is initialized at run time and the predicates are evaluated there: 2 violations reported.
// --------------------------------------------------------------------------
namespace run_static_local_F_F {
constexpr int
f (const int &v)
  pre (false)
  pre (false)
{
  return rt () ? 1 : 1;
}

int
use (int p)
{
  static int k = f (p);
  return k;
}

int
check ()
{
  int before = violations;
  if (use (1) != 1)
    return 1;
  return violations - before == 2 ? 0 : 2;
}
}  // namespace run_static_local_F_F

// --------------------------------------------------------------------------
// Two preconditions, the first true, the second never constant (calls a non-constexpr function); the body is not constant, so a static local is initialized at run time and the predicates are evaluated there: 0 violations reported.
// --------------------------------------------------------------------------
namespace run_static_local_T_NC {
constexpr int
f (const int &v)
  pre (true)
  pre (rt ())
{
  return rt () ? 1 : 1;
}

int
use (int p)
{
  static int k = f (p);
  return k;
}

int
check ()
{
  int before = violations;
  if (use (1) != 1)
    return 1;
  return violations - before == 0 ? 0 : 2;
}
}  // namespace run_static_local_T_NC

// --------------------------------------------------------------------------
// Two preconditions, the first false, the second never constant (calls a non-constexpr function); the body is not constant, so a non-const automatic local (not manifestly constant-evaluated) is initialized at run time and the predicates are evaluated there: 1 violation reported.
// --------------------------------------------------------------------------
namespace run_auto_nonconst_F_NC {
constexpr int
f (const int &v)
  pre (false)
  pre (rt ())
{
  return rt () ? 1 : 1;
}

int
use (int p)
{
  int k = f (p);
  return k;
}

int
check ()
{
  int before = violations;
  if (use (1) != 1)
    return 1;
  return violations - before == 1 ? 0 : 2;
}
}  // namespace run_auto_nonconst_F_NC

// --------------------------------------------------------------------------
// Two preconditions, the first never constant (calls a non-constexpr function), the second false; the body is not constant, so a non-const automatic local (not manifestly constant-evaluated) is initialized at run time and the predicates are evaluated there: 1 violation reported.
// --------------------------------------------------------------------------
namespace run_auto_nonconst_NC_F {
constexpr int
f (const int &v)
  pre (rt ())
  pre (false)
{
  return rt () ? 1 : 1;
}

int
use (int p)
{
  int k = f (p);
  return k;
}

int
check ()
{
  int before = violations;
  if (use (1) != 1)
    return 1;
  return violations - before == 1 ? 0 : 2;
}
}  // namespace run_auto_nonconst_NC_F

// --------------------------------------------------------------------------
// Two preconditions, the first false, the second false; the body is not constant, so a non-const automatic local (not manifestly constant-evaluated) is initialized at run time and the predicates are evaluated there: 2 violations reported.
// --------------------------------------------------------------------------
namespace run_auto_nonconst_F_F {
constexpr int
f (const int &v)
  pre (false)
  pre (false)
{
  return rt () ? 1 : 1;
}

int
use (int p)
{
  int k = f (p);
  return k;
}

int
check ()
{
  int before = violations;
  if (use (1) != 1)
    return 1;
  return violations - before == 2 ? 0 : 2;
}
}  // namespace run_auto_nonconst_F_F

// --------------------------------------------------------------------------
// Two preconditions, the first true, the second never constant (calls a non-constexpr function); the body is not constant, so a non-const automatic local (not manifestly constant-evaluated) is initialized at run time and the predicates are evaluated there: 0 violations reported.
// --------------------------------------------------------------------------
namespace run_auto_nonconst_T_NC {
constexpr int
f (const int &v)
  pre (true)
  pre (rt ())
{
  return rt () ? 1 : 1;
}

int
use (int p)
{
  int k = f (p);
  return k;
}

int
check ()
{
  int before = violations;
  if (use (1) != 1)
    return 1;
  return violations - before == 0 ? 0 : 2;
}
}  // namespace run_auto_nonconst_T_NC

// --------------------------------------------------------------------------
// Two preconditions, the first false, the second never constant (calls a non-constexpr function); the body is not constant, so a namespace-scope variable (constant or dynamic initialization) is initialized at run time and the predicates are evaluated there: 1 violation reported.
// --------------------------------------------------------------------------
namespace run_ns_nonconst_F_NC {
constexpr int
f (const int &v)
  pre (false)
  pre (rt ())
{
  return rt () ? 1 : 1;
}

int g = 1;
int before_k = violations;
int k = f (g);
int after_k = violations;

int
check ()
{
  int before = violations - (after_k - before_k);
  if (k != 1)
    return 1;
  return violations - before == 1 ? 0 : 2;
}
}  // namespace run_ns_nonconst_F_NC

// --------------------------------------------------------------------------
// Two preconditions, the first never constant (calls a non-constexpr function), the second false; the body is not constant, so a namespace-scope variable (constant or dynamic initialization) is initialized at run time and the predicates are evaluated there: 1 violation reported.
// --------------------------------------------------------------------------
namespace run_ns_nonconst_NC_F {
constexpr int
f (const int &v)
  pre (rt ())
  pre (false)
{
  return rt () ? 1 : 1;
}

int g = 1;
int before_k = violations;
int k = f (g);
int after_k = violations;

int
check ()
{
  int before = violations - (after_k - before_k);
  if (k != 1)
    return 1;
  return violations - before == 1 ? 0 : 2;
}
}  // namespace run_ns_nonconst_NC_F

// --------------------------------------------------------------------------
// Two preconditions, the first false, the second false; the body is not constant, so a namespace-scope variable (constant or dynamic initialization) is initialized at run time and the predicates are evaluated there: 2 violations reported.
// --------------------------------------------------------------------------
namespace run_ns_nonconst_F_F {
constexpr int
f (const int &v)
  pre (false)
  pre (false)
{
  return rt () ? 1 : 1;
}

int g = 1;
int before_k = violations;
int k = f (g);
int after_k = violations;

int
check ()
{
  int before = violations - (after_k - before_k);
  if (k != 1)
    return 1;
  return violations - before == 2 ? 0 : 2;
}
}  // namespace run_ns_nonconst_F_F

// --------------------------------------------------------------------------
// Two preconditions, the first true, the second never constant (calls a non-constexpr function); the body is not constant, so a namespace-scope variable (constant or dynamic initialization) is initialized at run time and the predicates are evaluated there: 0 violations reported.
// --------------------------------------------------------------------------
namespace run_ns_nonconst_T_NC {
constexpr int
f (const int &v)
  pre (true)
  pre (rt ())
{
  return rt () ? 1 : 1;
}

int g = 1;
int before_k = violations;
int k = f (g);
int after_k = violations;

int
check ()
{
  int before = violations - (after_k - before_k);
  if (k != 1)
    return 1;
  return violations - before == 0 ? 0 : 2;
}
}  // namespace run_ns_nonconst_T_NC

int
main()
{
  if (int rc = run_const_local_F_NC::check ())
    return 1 * 100 + rc;
  if (int rc = run_const_local_NC_F::check ())
    return 2 * 100 + rc;
  if (int rc = run_const_local_F_F::check ())
    return 3 * 100 + rc;
  if (int rc = run_const_local_T_NC::check ())
    return 4 * 100 + rc;
  if (int rc = run_static_local_F_NC::check ())
    return 5 * 100 + rc;
  if (int rc = run_static_local_NC_F::check ())
    return 6 * 100 + rc;
  if (int rc = run_static_local_F_F::check ())
    return 7 * 100 + rc;
  if (int rc = run_static_local_T_NC::check ())
    return 8 * 100 + rc;
  if (int rc = run_auto_nonconst_F_NC::check ())
    return 9 * 100 + rc;
  if (int rc = run_auto_nonconst_NC_F::check ())
    return 10 * 100 + rc;
  if (int rc = run_auto_nonconst_F_F::check ())
    return 11 * 100 + rc;
  if (int rc = run_auto_nonconst_T_NC::check ())
    return 12 * 100 + rc;
  if (int rc = run_ns_nonconst_F_NC::check ())
    return 13 * 100 + rc;
  if (int rc = run_ns_nonconst_NC_F::check ())
    return 14 * 100 + rc;
  if (int rc = run_ns_nonconst_F_F::check ())
    return 15 * 100 + rc;
  if (int rc = run_ns_nonconst_T_NC::check ())
    return 16 * 100 + rc;
  return 0;
}

// REQUIRES: contracts-libcxx, native
