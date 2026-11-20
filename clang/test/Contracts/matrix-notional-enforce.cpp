// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s
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
// Mirror: g++.dg/contracts/cpp26/matrix-notional-enforce.C

// expected-no-diagnostics
bool rt ();

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body is constant.  Called with an object whose value is not known at compile time, in a const local, under enforce.
// The evaluation completes with no contract problem.
// --------------------------------------------------------------------------
namespace const_local_pp_T_T_u_const_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return 1;
}

int
use (int p)
{
  const int k = f (p);
  static_assert (k == 1);
  return k;
}
}  // namespace const_local_pp_T_T_u_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first constant (true) for the known argument, not constant otherwise, the second constant (true) for the known argument, not constant otherwise; the body is constant.  Called with the constant 1, in a const local, under enforce.
// The evaluation completes with no contract problem.
// --------------------------------------------------------------------------
namespace const_local_pp_CD_CD_k_const_enforce {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  pre (v == 1 || rt ())
{
  return 1;
}

int
use (int p)
{
  const int k = f (1);
  static_assert (k == 1);
  return k;
}
}  // namespace const_local_pp_CD_CD_k_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pp_F_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (false)
  pre (rt ())
{
  return v;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_pp_F_NC_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pp_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (false)
{
  return v;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_pp_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pp_NR_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (rt ())
{
  return v;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_pp_NR_NC_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body reads the argument.  Called with an object whose value is not known at compile time, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pp_T_T_u_reads_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return v;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_pp_T_T_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pp_F_NC_k_rt_enforce {
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
  const int k = f (1);
  return k;
}
}  // namespace const_local_pp_F_NC_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second false; the body calls a non-constexpr function.  Called with the constant 1, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pp_NC_F_k_rt_enforce {
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
  const int k = f (1);
  return k;
}
}  // namespace const_local_pp_NC_F_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second false; the body calls a non-constexpr function.  Called with the constant 1, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pp_F_F_k_rt_enforce {
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
  const int k = f (1);
  return k;
}
}  // namespace const_local_pp_F_F_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under enforce.
// The evaluation completes with no contract problem.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_T_T_u_const_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return 1;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_T_T_u_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first constant (true) for the known argument, not constant otherwise, the second constant (true) for the known argument, not constant otherwise; the body is constant.  Called with the constant 1, in a namespace-scope variable (constant or dynamic initialization), under enforce.
// The evaluation completes with no contract problem.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_CD_CD_k_const_enforce {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  pre (v == 1 || rt ())
{
  return 1;
}

int g = 1;
int k = f (1);
}  // namespace ns_nonconst_pp_CD_CD_k_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_F_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (false)
  pre (rt ())
{
  return v;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_F_NC_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (false)
{
  return v;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_NR_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (rt ())
{
  return v;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_NR_NC_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body reads the argument.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_T_T_u_reads_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return v;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_T_T_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a namespace-scope variable (constant or dynamic initialization), under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_F_NC_k_rt_enforce {
constexpr int
f (const int &v)
  pre (false)
  pre (rt ())
{
  return rt () ? 1 : 1;
}

int g = 1;
int k = f (1);
}  // namespace ns_nonconst_pp_F_NC_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second false; the body calls a non-constexpr function.  Called with the constant 1, in a namespace-scope variable (constant or dynamic initialization), under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_NC_F_k_rt_enforce {
constexpr int
f (const int &v)
  pre (rt ())
  pre (false)
{
  return rt () ? 1 : 1;
}

int g = 1;
int k = f (1);
}  // namespace ns_nonconst_pp_NC_F_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second false; the body calls a non-constexpr function.  Called with the constant 1, in a namespace-scope variable (constant or dynamic initialization), under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_F_F_k_rt_enforce {
constexpr int
f (const int &v)
  pre (false)
  pre (false)
{
  return rt () ? 1 : 1;
}

int g = 1;
int k = f (1);
}  // namespace ns_nonconst_pp_F_F_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under enforce.
// The evaluation completes with no contract problem.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_T_T_u_const_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return 1;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pp_T_T_u_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first constant (true) for the known argument, not constant otherwise, the second constant (true) for the known argument, not constant otherwise; the body is constant.  Called with the constant 1, in a constexpr variable, under enforce.
// The evaluation completes with no contract problem.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_CD_CD_k_const_enforce {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  pre (v == 1 || rt ())
{
  return 1;
}

int g = 1;
constexpr int k = f (1);
}  // namespace constexpr_var_pp_CD_CD_k_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body is constant.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under enforce.
// The evaluation completes with no contract problem.
// --------------------------------------------------------------------------
namespace vla_pp_T_T_u_const_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return 1;
}

int
use (int p)
{
  char a[f (p)];
  static_assert (sizeof a == 1);
  return sizeof a;
}
}  // namespace vla_pp_T_T_u_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first constant (true) for the known argument, not constant otherwise, the second constant (true) for the known argument, not constant otherwise; the body is constant.  Called with the constant 1, in an array bound, where VLAs are available, under enforce.
// The evaluation completes with no contract problem.
// --------------------------------------------------------------------------
namespace vla_pp_CD_CD_k_const_enforce {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  pre (v == 1 || rt ())
{
  return 1;
}

int
use (int p)
{
  char a[f (1)];
  static_assert (sizeof a == 1);
  return sizeof a;
}
}  // namespace vla_pp_CD_CD_k_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body is constant.  Called with an object whose value is not known at compile time, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_T_T_u_const_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return 1;
}

int
use (int p)
{
  int k = f (p);
  return k;
}
}  // namespace auto_nonconst_pp_T_T_u_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second false; the body is constant.  Called with an object whose value is not known at compile time, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_T_F_u_const_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (false)
{
  return 1;
}

int
use (int p)
{
  int k = f (p);
  return k;
}
}  // namespace auto_nonconst_pp_T_F_u_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_T_NC_u_const_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (rt ())
{
  return 1;
}

int
use (int p)
{
  int k = f (p);
  return k;
}
}  // namespace auto_nonconst_pp_T_NC_u_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second true; the body is constant.  Called with an object whose value is not known at compile time, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_F_T_u_const_enforce {
constexpr int
f (const int &v)
  pre (false)
  pre (true)
{
  return 1;
}

int
use (int p)
{
  int k = f (p);
  return k;
}
}  // namespace auto_nonconst_pp_F_T_u_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second false; the body is constant.  Called with an object whose value is not known at compile time, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_F_F_u_const_enforce {
constexpr int
f (const int &v)
  pre (false)
  pre (false)
{
  return 1;
}

int
use (int p)
{
  int k = f (p);
  return k;
}
}  // namespace auto_nonconst_pp_F_F_u_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_F_NC_u_const_enforce {
constexpr int
f (const int &v)
  pre (false)
  pre (rt ())
{
  return 1;
}

int
use (int p)
{
  int k = f (p);
  return k;
}
}  // namespace auto_nonconst_pp_F_NC_u_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second true; the body is constant.  Called with an object whose value is not known at compile time, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_NR_T_u_const_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (true)
{
  return 1;
}

int
use (int p)
{
  int k = f (p);
  return k;
}
}  // namespace auto_nonconst_pp_NR_T_u_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_NR_F_u_const_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (false)
{
  return 1;
}

int
use (int p)
{
  int k = f (p);
  return k;
}
}  // namespace auto_nonconst_pp_NR_F_u_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_NR_NC_u_const_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (rt ())
{
  return 1;
}

int
use (int p)
{
  int k = f (p);
  return k;
}
}  // namespace auto_nonconst_pp_NR_NC_u_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first constant (true) for the known argument, not constant otherwise, the second false; the body is constant.  Called with the constant 1, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_CD_F_k_const_enforce {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  pre (false)
{
  return 1;
}

int
use (int p)
{
  int k = f (1);
  return k;
}
}  // namespace auto_nonconst_pp_CD_F_k_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second constant (true) for the known argument, not constant otherwise; the body is constant.  Called with the constant 1, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_F_CD_k_const_enforce {
constexpr int
f (const int &v)
  pre (false)
  pre (v == 1 || rt ())
{
  return 1;
}

int
use (int p)
{
  int k = f (1);
  return k;
}
}  // namespace auto_nonconst_pp_F_CD_k_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first constant (true) for the known argument, not constant otherwise, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_CD_NC_k_const_enforce {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  pre (rt ())
{
  return 1;
}

int
use (int p)
{
  int k = f (1);
  return k;
}
}  // namespace auto_nonconst_pp_CD_NC_k_const_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_F_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (false)
  pre (rt ())
{
  return v;
}

int
use (int p)
{
  int k = f (p);
  return k;
}
}  // namespace auto_nonconst_pp_F_NC_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (false)
{
  return v;
}

int
use (int p)
{
  int k = f (p);
  return k;
}
}  // namespace auto_nonconst_pp_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_NR_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (rt ())
{
  return v;
}

int
use (int p)
{
  int k = f (p);
  return k;
}
}  // namespace auto_nonconst_pp_NR_NC_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body reads the argument.  Called with an object whose value is not known at compile time, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_T_T_u_reads_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return v;
}

int
use (int p)
{
  int k = f (p);
  return k;
}
}  // namespace auto_nonconst_pp_T_T_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_F_NC_k_rt_enforce {
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
  int k = f (1);
  return k;
}
}  // namespace auto_nonconst_pp_F_NC_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second false; the body calls a non-constexpr function.  Called with the constant 1, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_NC_F_k_rt_enforce {
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
  int k = f (1);
  return k;
}
}  // namespace auto_nonconst_pp_NC_F_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second false; the body calls a non-constexpr function.  Called with the constant 1, in a non-const automatic local (not manifestly constant-evaluated), under enforce.
// Not manifestly constant-evaluated, so nothing is diagnosed at compile time.
// --------------------------------------------------------------------------
namespace auto_nonconst_pp_F_F_k_rt_enforce {
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
  int k = f (1);
  return k;
}
}  // namespace auto_nonconst_pp_F_F_k_rt_enforce

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first false, the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pa_F_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (false)
{
  int x = v;
  contract_assert (rt ());
  return x;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_pa_F_NC_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pa_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
{
  int x = v;
  contract_assert (false);
  return x;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_pa_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pa_NR_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
{
  int x = v;
  contract_assert (rt ());
  return x;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_pa_NR_NC_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first false, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pa_F_NC_k_rt_enforce {
constexpr int
f (const int &v)
  pre (false)
{
  int x = rt () ? 1 : 1;
  contract_assert (rt ());
  return x;
}

int
use (int p)
{
  const int k = f (1);
  return k;
}
}  // namespace const_local_pa_F_NC_k_rt_enforce

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first constant (true) for the known argument, not constant otherwise, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pa_CD_NC_k_rt_enforce {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
{
  int x = rt () ? 1 : 1;
  contract_assert (rt ());
  return x;
}

int
use (int p)
{
  const int k = f (1);
  return k;
}
}  // namespace const_local_pa_CD_NC_k_rt_enforce

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first false, the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pq_F_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (false)
  post (r: rt ())
{
  return v;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_pq_F_NC_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pq_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  post (r: false)
{
  return v;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_pq_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pq_NR_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  post (r: rt ())
{
  return v;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_pq_NR_NC_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first false, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pq_F_NC_k_rt_enforce {
constexpr int
f (const int &v)
  pre (false)
  post (r: rt ())
{
  return rt () ? 1 : 1;
}

int
use (int p)
{
  const int k = f (1);
  return k;
}
}  // namespace const_local_pq_F_NC_k_rt_enforce

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first constant (true) for the known argument, not constant otherwise, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_pq_CD_NC_k_rt_enforce {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  post (r: rt ())
{
  return rt () ? 1 : 1;
}

int
use (int p)
{
  const int k = f (1);
  return k;
}
}  // namespace const_local_pq_CD_NC_k_rt_enforce

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first false, the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_nest_F_NC_u_reads_enforce {
constexpr int
inner (const int &v)
  pre (rt ())
{
  return v;
}

constexpr int
f (const int &v)
  pre (false)
{
  return inner (v);
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_nest_F_NC_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_nest_NR_F_u_reads_enforce {
constexpr int
inner (const int &v)
  pre (false)
{
  return v;
}

constexpr int
f (const int &v)
  pre (v > 0)
{
  return inner (v);
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_nest_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_nest_NR_NC_u_reads_enforce {
constexpr int
inner (const int &v)
  pre (rt ())
{
  return v;
}

constexpr int
f (const int &v)
  pre (v > 0)
{
  return inner (v);
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_nest_NR_NC_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first false, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_nest_F_NC_k_rt_enforce {
constexpr int
inner (const int &v)
  pre (rt ())
{
  return rt () ? 1 : 1;
}

constexpr int
f (const int &v)
  pre (false)
{
  return inner (v);
}

int
use (int p)
{
  const int k = f (1);
  return k;
}
}  // namespace const_local_nest_F_NC_k_rt_enforce

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first constant (true) for the known argument, not constant otherwise, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a const local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace const_local_nest_CD_NC_k_rt_enforce {
constexpr int
inner (const int &v)
  pre (rt ())
{
  return rt () ? 1 : 1;
}

constexpr int
f (const int &v)
  pre (v == 1 || rt ())
{
  return inner (v);
}

int
use (int p)
{
  const int k = f (1);
  return k;
}
}  // namespace const_local_nest_CD_NC_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a static local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace static_local_pp_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (false)
{
  return v;
}

int
use (int p)
{
  static int k = f (p);
  return k;
}
}  // namespace static_local_pp_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body calls a non-constexpr function.  Called with the constant 1, in a static local, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace static_local_pp_T_T_k_rt_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return rt () ? 1 : 1;
}

int
use (int p)
{
  static int k = f (1);
  return k;
}
}  // namespace static_local_pp_T_T_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a namespace-scope const variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace ns_const_pp_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (false)
{
  return v;
}

int g = 1;
const int k = f (g);
}  // namespace ns_const_pp_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body calls a non-constexpr function.  Called with the constant 1, in a namespace-scope const variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace ns_const_pp_T_T_k_rt_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return rt () ? 1 : 1;
}

int g = 1;
const int k = f (1);
}  // namespace ns_const_pp_T_T_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a thread_local variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace thread_local_pp_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (false)
{
  return v;
}

int g = 1;
thread_local int k = f (g);
}  // namespace thread_local_pp_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body calls a non-constexpr function.  Called with the constant 1, in a thread_local variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace thread_local_pp_T_T_k_rt_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return rt () ? 1 : 1;
}

int g = 1;
thread_local int k = f (1);
}  // namespace thread_local_pp_T_T_k_rt_enforce
