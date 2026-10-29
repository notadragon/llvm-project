// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -verify-ignore-unexpected=note -fsyntax-only -verify %s
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
// Mirror: g++.dg/contracts/cpp26/matrix-notional-enforce-errors-not-constant.C

bool rt ();

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_F_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (false)
  pre (rt ())
{
  return v;
}

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pp_F_NC_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (false)
{
  return v;
}

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pp_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_NR_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (rt ())
{
  return v;
}

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pp_NR_NC_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body reads the argument.  Called with an object whose value is not known at compile time, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_T_T_u_reads_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return v;
}

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pp_T_T_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_F_NC_k_rt_enforce {
constexpr int
f (const int &v)
  pre (false)
  pre (rt ())
{
  return rt () ? 1 : 1;
}

int g = 1;
constexpr int k = f (1);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pp_F_NC_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first never constant (calls a non-constexpr function), the second false; the body calls a non-constexpr function.  Called with the constant 1, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_NC_F_k_rt_enforce {
constexpr int
f (const int &v)
  pre (rt ())
  pre (false)
{
  return rt () ? 1 : 1;
}

int g = 1;
constexpr int k = f (1);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pp_NC_F_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second false; the body calls a non-constexpr function.  Called with the constant 1, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_F_F_k_rt_enforce {
constexpr int
f (const int &v)
  pre (false)
  pre (false)
{
  return rt () ? 1 : 1;
}

int g = 1;
constexpr int k = f (1);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pp_F_F_k_rt_enforce

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first false, the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pa_F_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (false)
{
  int x = v;
  contract_assert (rt ());
  return x;
}

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pa_F_NC_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pa_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
{
  int x = v;
  contract_assert (false);
  return x;
}

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pa_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pa_NR_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
{
  int x = v;
  contract_assert (rt ());
  return x;
}

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pa_NR_NC_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first false, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pa_F_NC_k_rt_enforce {
constexpr int
f (const int &v)
  pre (false)
{
  int x = rt () ? 1 : 1;
  contract_assert (rt ());
  return x;
}

int g = 1;
constexpr int k = f (1);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pa_F_NC_k_rt_enforce

// --------------------------------------------------------------------------
// A precondition, then contract_assert after the body computes its value: the first constant (true) for the known argument, not constant otherwise, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pa_CD_NC_k_rt_enforce {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
{
  int x = rt () ? 1 : 1;
  contract_assert (rt ());
  return x;
}

int g = 1;
constexpr int k = f (1);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pa_CD_NC_k_rt_enforce

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first false, the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pq_F_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (false)
  post (r: rt ())
{
  return v;
}

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pq_F_NC_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pq_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  post (r: false)
{
  return v;
}

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pq_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pq_NR_NC_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  post (r: rt ())
{
  return v;
}

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pq_NR_NC_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first false, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pq_F_NC_k_rt_enforce {
constexpr int
f (const int &v)
  pre (false)
  post (r: rt ())
{
  return rt () ? 1 : 1;
}

int g = 1;
constexpr int k = f (1);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pq_F_NC_k_rt_enforce

// --------------------------------------------------------------------------
// A precondition and a postcondition: the first constant (true) for the known argument, not constant otherwise, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_pq_CD_NC_k_rt_enforce {
constexpr int
f (const int &v)
  pre (v == 1 || rt ())
  post (r: rt ())
{
  return rt () ? 1 : 1;
}

int g = 1;
constexpr int k = f (1);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_pq_CD_NC_k_rt_enforce

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first false, the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_nest_F_NC_u_reads_enforce {
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

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_nest_F_NC_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_nest_NR_F_u_reads_enforce {
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

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_nest_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_nest_NR_NC_u_reads_enforce {
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

int g = 1;
constexpr int k = f (g);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_nest_NR_NC_u_reads_enforce

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first false, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_nest_F_NC_k_rt_enforce {
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

int g = 1;
constexpr int k = f (1);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_nest_F_NC_k_rt_enforce

// --------------------------------------------------------------------------
// A precondition on the called function and one on the function it calls: the first constant (true) for the known argument, not constant otherwise, the second never constant (calls a non-constexpr function); the body calls a non-constexpr function.  Called with the constant 1, in a constexpr variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constexpr_var_nest_CD_NC_k_rt_enforce {
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

int g = 1;
constexpr int k = f (1);  // expected-error {{must be initialized by a constant expression}}
}  // namespace constexpr_var_nest_CD_NC_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a constinit variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constinit_pp_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (false)
{
  return v;
}

int g = 1;
constinit int k = f (g);  // expected-error {{variable does not have a constant initializer}}
}  // namespace constinit_pp_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body calls a non-constexpr function.  Called with the constant 1, in a constinit variable, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace constinit_pp_T_T_k_rt_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return rt () ? 1 : 1;
}

int g = 1;
constinit int k = f (1);  // expected-error {{variable does not have a constant initializer}}
}  // namespace constinit_pp_T_T_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a static_assert, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace static_assert_pp_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (false)
{
  return v;
}

int g = 1;
static_assert (f (g) == 1);  // expected-error {{static assertion expression is not an integral constant expression}}
}  // namespace static_assert_pp_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body calls a non-constexpr function.  Called with the constant 1, in a static_assert, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace static_assert_pp_T_T_k_rt_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return rt () ? 1 : 1;
}

int g = 1;
static_assert (f (1) == 1);  // expected-error {{static assertion expression is not an integral constant expression}}
}  // namespace static_assert_pp_T_T_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a template argument, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace targ_pp_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (false)
{
  return v;
}

int g = 1;
template <int> struct X {};
X<f (g)> x;  // expected-error {{non-type template argument is not a constant expression}}
}  // namespace targ_pp_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body calls a non-constexpr function.  Called with the constant 1, in a template argument, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace targ_pp_T_T_k_rt_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return rt () ? 1 : 1;
}

int g = 1;
template <int> struct X {};
X<f (1)> x;  // expected-error {{non-type template argument is not a constant expression}}
}  // namespace targ_pp_T_T_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in an enumerator's value, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace enumerator_pp_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (false)
{
  return v;
}

int g = 1;
enum E { e = f (g) };  // expected-error {{expression is not an integral constant expression}}
}  // namespace enumerator_pp_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body calls a non-constexpr function.  Called with the constant 1, in an enumerator's value, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace enumerator_pp_T_T_k_rt_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return rt () ? 1 : 1;
}

int g = 1;
enum E { e = f (1) };  // expected-error {{expression is not an integral constant expression}}
}  // namespace enumerator_pp_T_T_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in a case label, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace case_label_pp_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (false)
{
  return v;
}

int g = 1;
int
use (int s)
{
  switch (s)
    {
    case f (g):  // expected-error {{case value is not a constant expression}}
      return 1;
    }
  return 0;
}
}  // namespace case_label_pp_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body calls a non-constexpr function.  Called with the constant 1, in a case label, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace case_label_pp_T_T_k_rt_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return rt () ? 1 : 1;
}

int g = 1;
int
use (int s)
{
  switch (s)
    {
    case f (1):  // expected-error {{case value is not a constant expression}}
      return 1;
    }
  return 0;
}
}  // namespace case_label_pp_T_T_k_rt_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body reads the argument.  Called with an object whose value is not known at compile time, in an if constexpr condition, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace if_constexpr_pp_NR_F_u_reads_enforce {
constexpr int
f (const int &v)
  pre (v > 0)
  pre (false)
{
  return v;
}

int g = 1;
int
use ()
{
  if constexpr (f (g) == 1)  // expected-error {{constexpr if condition is not a constant expression}}
    return 1;
  return 0;
}
}  // namespace if_constexpr_pp_NR_F_u_reads_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first true, the second true; the body calls a non-constexpr function.  Called with the constant 1, in an if constexpr condition, under enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported: the expression is simply not constant.
// --------------------------------------------------------------------------
namespace if_constexpr_pp_T_T_k_rt_enforce {
constexpr int
f (const int &v)
  pre (true)
  pre (true)
{
  return rt () ? 1 : 1;
}

int g = 1;
int
use ()
{
  if constexpr (f (1) == 1)  // expected-error {{constexpr if condition is not a constant expression}}
    return 1;
  return 0;
}
}  // namespace if_constexpr_pp_T_T_k_rt_enforce
