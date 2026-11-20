// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -verify-ignore-unexpected=note -fsyntax-only -verify %s
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
// Mirror: g++.dg/contracts/cpp26/matrix-notional-observe-warnings.C

bool rt ();

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second false; the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_T_F_u_const_observe {
constexpr int
f (const int &v)
  pre (true)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pp_T_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_T_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (true)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pp_T_NC_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second true; the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_F_T_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pp_F_T_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second false; the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_F_F_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pp_F_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_F_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pp_F_NC_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second true; the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_NR_T_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pp_NR_T_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pp_NR_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_NR_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pp_NR_NC_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first constant (true) for the known argument, not constant otherwise, the second false; the body is constant.  Called with the constant 1, in a const local, under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_CD_F_k_const_observe {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pp_CD_F_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second constant (true) for the known argument, not constant otherwise; the body is constant.  Called with the constant 1, in a const local, under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_F_CD_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pp_F_CD_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first constant (true) for the known argument, not constant otherwise, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a const local, under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_CD_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pp_CD_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second constant (true) for the known argument, not constant otherwise; the body is constant.  Called with the constant 1, in a const local, under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_NC_CD_k_const_observe {
constexpr int
f (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pp_NC_CD_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a const local, under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_NR_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pp_NR_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second not constant when the argument is unknown (reads it); the body is constant.  Called with the constant 1, in a const local, under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_NC_NR_k_const_observe {
constexpr int
f (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
  pre (v > 0)
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
}  // namespace const_local_pp_NC_NR_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a const local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_F_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pp_F_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second false; the body is constant.  Called with the constant 1, in a const local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pp_NC_F_k_const_observe {
constexpr int
f (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pp_NC_F_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second false; the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_T_F_u_const_observe {
constexpr int
f (const int &v)
  pre (true)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_T_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_T_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (true)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_T_NC_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second true; the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_F_T_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (true)
{
  return 1;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_F_T_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second false; the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_F_F_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_F_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_F_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_F_NC_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second true; the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_NR_T_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (true)
{
  return 1;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_NR_T_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_NR_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_NR_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_NR_NC_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first constant (true) for the known argument, not constant otherwise, the second false; the body is constant.  Called with the constant 1, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_CD_F_k_const_observe {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int k = f (1);
}  // namespace ns_nonconst_pp_CD_F_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second constant (true) for the known argument, not constant otherwise; the body is constant.  Called with the constant 1, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_F_CD_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (v == 1 || rt ())
{
  return 1;
}

int g = 1;
int k = f (1);
}  // namespace ns_nonconst_pp_F_CD_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first constant (true) for the known argument, not constant otherwise, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_CD_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int k = f (1);
}  // namespace ns_nonconst_pp_CD_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second constant (true) for the known argument, not constant otherwise; the body is constant.  Called with the constant 1, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_NC_CD_k_const_observe {
constexpr int
f (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
  pre (v == 1 || rt ())
{
  return 1;
}

int g = 1;
int k = f (1);
}  // namespace ns_nonconst_pp_NC_CD_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_NR_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int k = f (1);
}  // namespace ns_nonconst_pp_NR_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second not constant when the argument is unknown (reads it); the body is constant.  Called with the constant 1, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_NC_NR_k_const_observe {
constexpr int
f (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
  pre (v > 0)
{
  return 1;
}

int g = 1;
int k = f (1);
}  // namespace ns_nonconst_pp_NC_NR_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_F_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int k = f (1);
}  // namespace ns_nonconst_pp_F_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second false; the body is constant.  Called with the constant 1, in a namespace-scope variable (constant or dynamic initialization), under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_NC_F_k_const_observe {
constexpr int
f (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int k = f (1);
}  // namespace ns_nonconst_pp_NC_F_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second false; the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_T_F_u_const_observe {
constexpr int
f (const int &v)
  pre (true)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pp_T_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_T_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (true)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pp_T_NC_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second true; the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_F_T_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (true)
{
  return 1;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pp_F_T_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second false; the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_F_F_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pp_F_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_F_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pp_F_NC_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second true; the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_NR_T_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (true)
{
  return 1;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pp_NR_T_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pp_NR_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_NR_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pp_NR_NC_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first constant (true) for the known argument, not constant otherwise, the second false; the body is constant.  Called with the constant 1, in a constexpr variable, under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_CD_F_k_const_observe {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (1);
}  // namespace constexpr_var_pp_CD_F_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second constant (true) for the known argument, not constant otherwise; the body is constant.  Called with the constant 1, in a constexpr variable, under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_F_CD_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (v == 1 || rt ())
{
  return 1;
}

int g = 1;
constexpr int k = f (1);
}  // namespace constexpr_var_pp_F_CD_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first constant (true) for the known argument, not constant otherwise, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a constexpr variable, under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_CD_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (1);
}  // namespace constexpr_var_pp_CD_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second constant (true) for the known argument, not constant otherwise; the body is constant.  Called with the constant 1, in a constexpr variable, under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_NC_CD_k_const_observe {
constexpr int
f (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
  pre (v == 1 || rt ())
{
  return 1;
}

int g = 1;
constexpr int k = f (1);
}  // namespace constexpr_var_pp_NC_CD_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a constexpr variable, under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_NR_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (1);
}  // namespace constexpr_var_pp_NR_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second not constant when the argument is unknown (reads it); the body is constant.  Called with the constant 1, in a constexpr variable, under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_NC_NR_k_const_observe {
constexpr int
f (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
  pre (v > 0)
{
  return 1;
}

int g = 1;
constexpr int k = f (1);
}  // namespace constexpr_var_pp_NC_NR_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a constexpr variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_F_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (1);
}  // namespace constexpr_var_pp_F_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second false; the body is constant.  Called with the constant 1, in a constexpr variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_NC_F_k_const_observe {
constexpr int
f (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (1);
}  // namespace constexpr_var_pp_NC_F_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second false; the body is constant.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_T_F_u_const_observe {
constexpr int
f (const int &v)
  pre (true)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace vla_pp_T_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_T_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (true)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace vla_pp_T_NC_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second true; the body is constant.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_F_T_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace vla_pp_F_T_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second false; the body is constant.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_F_F_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace vla_pp_F_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_F_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace vla_pp_F_NC_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second true; the body is constant.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_NR_T_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace vla_pp_NR_T_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace vla_pp_NR_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_NR_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace vla_pp_NR_NC_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first constant (true) for the known argument, not constant otherwise, the second false; the body is constant.  Called with the constant 1, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_CD_F_k_const_observe {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace vla_pp_CD_F_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second constant (true) for the known argument, not constant otherwise; the body is constant.  Called with the constant 1, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_F_CD_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace vla_pp_F_CD_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first constant (true) for the known argument, not constant otherwise, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_CD_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace vla_pp_CD_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second constant (true) for the known argument, not constant otherwise; the body is constant.  Called with the constant 1, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_NC_CD_k_const_observe {
constexpr int
f (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace vla_pp_NC_CD_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the second is reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_NR_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace vla_pp_NR_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second not constant when the argument is unknown (reads it); the body is constant.  Called with the constant 1, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the first is reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_NC_NR_k_const_observe {
constexpr int
f (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
  pre (v > 0)
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
}  // namespace vla_pp_NC_NR_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_F_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace vla_pp_F_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second false; the body is constant.  Called with the constant 1, in an array bound, where VLAs are available, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace vla_pp_NC_F_k_const_observe {
constexpr int
f (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace vla_pp_NC_F_k_const_observe

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pa_F_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  int x = 1;
  contract_assert (rt ());  // expected-warning {{contract failed during execution of constexpr function}}
  return x;
}

int
use (int p)
{
  const int k = f (p);
  static_assert (k == 1);
  return k;
}
}  // namespace const_local_pa_F_NC_u_const_observe

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pa_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
{
  int x = 1;
  contract_assert (false);  // expected-warning {{contract failed during execution of constexpr function}}
  return x;
}

int
use (int p)
{
  const int k = f (p);
  static_assert (k == 1);
  return k;
}
}  // namespace const_local_pa_NR_F_u_const_observe

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pa_NR_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
{
  int x = 1;
  contract_assert (rt ());  // expected-warning {{contract failed during execution of constexpr function}}
  return x;
}

int
use (int p)
{
  const int k = f (p);
  static_assert (k == 1);
  return k;
}
}  // namespace const_local_pa_NR_NC_u_const_observe

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pq_F_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  post (r: rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pq_F_NC_u_const_observe

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pq_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  post (r: false)  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pq_NR_F_u_const_observe

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_pq_NR_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  post (r: rt ())  // expected-warning {{contract failed during execution of constexpr function}}
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
}  // namespace const_local_pq_NR_NC_u_const_observe

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_nest_F_NC_u_const_observe {
constexpr int
inner (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return inner (v);
}

int
use (int p)
{
  const int k = f (p);
  static_assert (k == 1);
  return k;
}
}  // namespace const_local_nest_F_NC_u_const_observe

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_nest_NR_F_u_const_observe {
constexpr int
inner (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return inner (v);
}

int
use (int p)
{
  const int k = f (p);
  static_assert (k == 1);
  return k;
}
}  // namespace const_local_nest_NR_F_u_const_observe

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a const local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace const_local_nest_NR_NC_u_const_observe {
constexpr int
inner (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return inner (v);
}

int
use (int p)
{
  const int k = f (p);
  static_assert (k == 1);
  return k;
}
}  // namespace const_local_nest_NR_NC_u_const_observe

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pa_F_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  int x = 1;
  contract_assert (rt ());  // expected-warning {{contract failed during execution of constexpr function}}
  return x;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pa_F_NC_u_const_observe

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pa_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
{
  int x = 1;
  contract_assert (false);  // expected-warning {{contract failed during execution of constexpr function}}
  return x;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pa_NR_F_u_const_observe

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pa_NR_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
{
  int x = 1;
  contract_assert (rt ());  // expected-warning {{contract failed during execution of constexpr function}}
  return x;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pa_NR_NC_u_const_observe

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pq_F_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  post (r: rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pq_F_NC_u_const_observe

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pq_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  post (r: false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pq_NR_F_u_const_observe

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_pq_NR_NC_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  post (r: rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pq_NR_NC_u_const_observe

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_nest_F_NC_u_const_observe {
constexpr int
inner (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return inner (v);
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_nest_F_NC_u_const_observe

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_nest_NR_F_u_const_observe {
constexpr int
inner (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return inner (v);
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_nest_NR_F_u_const_observe

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constexpr_var_nest_NR_NC_u_const_observe {
constexpr int
inner (const int &v)
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return inner (v);
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_nest_NR_NC_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a static local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace static_local_pp_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int
use (int p)
{
  static int k = f (p);
  return k;
}
}  // namespace static_local_pp_NR_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a static local, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace static_local_pp_F_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int
use (int p)
{
  static int k = f (1);
  return k;
}
}  // namespace static_local_pp_F_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope const variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_const_pp_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
const int k = f (g);
static_assert (k == 1);
}  // namespace ns_const_pp_NR_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a namespace-scope const variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace ns_const_pp_F_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
const int k = f (1);
static_assert (k == 1);
}  // namespace ns_const_pp_F_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a thread_local variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace thread_local_pp_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
thread_local int k = f (g);
}  // namespace thread_local_pp_NR_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a thread_local variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace thread_local_pp_F_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
thread_local int k = f (1);
}  // namespace thread_local_pp_F_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a constinit variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constinit_pp_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constinit int k = f (g);
}  // namespace constinit_pp_NR_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a constinit variable, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace constinit_pp_F_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constinit int k = f (1);
}  // namespace constinit_pp_F_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a static_assert, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace static_assert_pp_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
static_assert (f (g) == 1);
}  // namespace static_assert_pp_NR_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a static_assert, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace static_assert_pp_F_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
static_assert (f (1) == 1);
}  // namespace static_assert_pp_F_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a template argument, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace targ_pp_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
template <int> struct X {};
X<f (g)> x;
}  // namespace targ_pp_NR_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a template argument, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace targ_pp_F_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
template <int> struct X {};
X<f (1)> x;
}  // namespace targ_pp_F_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in an enumerator's value, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace enumerator_pp_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
enum E { e = f (g) };
}  // namespace enumerator_pp_NR_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in an enumerator's value, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace enumerator_pp_F_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
enum E { e = f (1) };
}  // namespace enumerator_pp_F_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a case label, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace case_label_pp_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int
use (int s)
{
  switch (s)
    {
    case f (g):
      return 1;
    }
  return 0;
}
}  // namespace case_label_pp_NR_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in a case label, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace case_label_pp_F_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int
use (int s)
{
  switch (s)
    {
    case f (1):
      return 1;
    }
  return 0;
}
}  // namespace case_label_pp_F_NC_k_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in an if constexpr condition, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace if_constexpr_pp_NR_F_u_const_observe {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int
use ()
{
  if constexpr (f (g) == 1)
    return 1;
  return 0;
}
}  // namespace if_constexpr_pp_NR_F_u_const_observe

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with the constant 1, in an if constexpr condition, under observe.
// The evaluation completes, so the first and the second are each reported, as a warning.
// --------------------------------------------------------------------------
namespace if_constexpr_pp_F_NC_k_const_observe {
constexpr int
f (const int &v)
  pre (false)  // expected-warning {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-warning {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int
use ()
{
  if constexpr (f (1) == 1)
    return 1;
  return 0;
}
}  // namespace if_constexpr_pp_F_NC_k_const_observe
