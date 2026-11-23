// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -verify-ignore-unexpected=note -fsyntax-only -verify %s
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
// Mirror: g++.dg/contracts/cpp26/matrix-notional-quick-enforce-errors.C

bool rt ();

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a const local, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace const_local_pp_F_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (false)  // expected-error {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-error {{contract failed during execution of constexpr function}}
{
  return 1;
}

int
use (int p)
{
  const int k = f (p);  // expected-error {{initialization of constant-initialized variable failed}}
  return k;
}
}  // namespace const_local_pp_F_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a const local, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace const_local_pp_NR_F_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-error {{contract failed during execution of constexpr function}}
  pre (false)  // expected-error {{contract failed during execution of constexpr function}}
{
  return 1;
}

int
use (int p)
{
  const int k = f (p);  // expected-error {{initialization of constant-initialized variable failed}}
  return k;
}
}  // namespace const_local_pp_NR_F_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a const local, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace const_local_pp_NR_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-error {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-error {{contract failed during execution of constexpr function}}
{
  return 1;
}

int
use (int p)
{
  const int k = f (p);  // expected-error {{initialization of constant-initialized variable failed}}
  return k;
}
}  // namespace const_local_pp_NR_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_F_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (false)  // expected-error {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-error {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int k = f (g);  // expected-error {{initialization of constant-initialized variable failed}}
}  // namespace ns_nonconst_pp_F_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_NR_F_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-error {{contract failed during execution of constexpr function}}
  pre (false)  // expected-error {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int k = f (g);  // expected-error {{initialization of constant-initialized variable failed}}
}  // namespace ns_nonconst_pp_NR_F_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_NR_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-error {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-error {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
int k = f (g);  // expected-error {{initialization of constant-initialized variable failed}}
}  // namespace ns_nonconst_pp_NR_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_F_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (false)  // expected-error {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-error {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pp_F_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_NR_F_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-error {{contract failed during execution of constexpr function}}
  pre (false)  // expected-error {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pp_NR_F_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_NR_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-error {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-error {{contract failed during execution of constexpr function}}
{
  return 1;
}

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pp_NR_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace vla_pp_F_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (false)  // expected-error {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-error {{contract failed during execution of constexpr function}}
{
  return 1;
}

int
use (int p)
{
  char a[f (p)];  // expected-warning {{variable length arrays in C}}
  return sizeof a;
}
}  // namespace vla_pp_F_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace vla_pp_NR_F_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-error {{contract failed during execution of constexpr function}}
  pre (false)  // expected-error {{contract failed during execution of constexpr function}}
{
  return 1;
}

int
use (int p)
{
  char a[f (p)];  // expected-warning {{variable length arrays in C}}
  return sizeof a;
}
}  // namespace vla_pp_NR_F_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace vla_pp_NR_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // expected-error {{contract failed during execution of constexpr function}}
  pre (rt ())  // expected-error {{contract failed during execution of constexpr function}}
{
  return 1;
}

int
use (int p)
{
  char a[f (p)];  // expected-warning {{variable length arrays in C}}
  return sizeof a;
}
}  // namespace vla_pp_NR_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under quick_enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace vla_pp_F_NC_u_reads_quick_enforce {
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
  char a[f (p)];  // expected-warning {{variable length arrays in C}}
  return sizeof a;
}
}  // namespace vla_pp_F_NC_u_reads_quick_enforce
