// DEFINE: %{run} = %clangxx -std=c++26 %s %libcxx_flags -o %t
//
// RUN: %{run} -fcontracts-p4451 -fcontract-evaluation-semantic=ignore -DSEM_IGNORE && %t
// RUN: %{run} -fcontracts-p4451 -fcontract-evaluation-semantic=observe -DSEM_OBSERVE && %t
// RUN: %{run} -fcontracts-p4451 -fcontract-evaluation-semantic=enforce -DSEM_ENFORCE && %t
// P4451R3: a contract assertion whose specifier names
// std::contracts::controls::defaulted is evaluated with the configured
// semantic, exactly as one without a specifier: with ignore the handler is
// never called, with observe and enforce it is called with that semantic.
// Each kind of assertion: pre, post with and without a result name,
// contract_assert, a member function, a function template and a lambda,
// side by side with the same assertions unlabeled.  The handler checks the
// semantic and throws, so each violation is counted.  Under P3400 the label
// is empty, so `defaulted | review` behaves as `review` alone (enforce
// becomes observe).  Under -fcontracts-p4451 alone, -fcontracts-p3400 alone
// and both.

#include <contracts>  // `contract_violation`, `controls::defaulted`
#include <cstdlib>    // `std::abort`

using std::contracts::evaluation_semantic;
namespace scl = std::contracts::controls;

#if defined(SEM_IGNORE)
constexpr evaluation_semantic configured = evaluation_semantic::ignore;
#elif defined(SEM_OBSERVE)
constexpr evaluation_semantic configured = evaluation_semantic::observe;
#else
constexpr evaluation_semantic configured = evaluation_semantic::enforce;
#endif

static int handled;
static evaluation_semantic expected;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.semantic() != expected)
    std::abort();
  ++handled;
  throw handled;
}

int pre_d(int x) pre<std::contracts::controls::defaulted>(x > 0) { return x; }
int pre_u(int x) pre(x > 0) { return x; }
int post_d(const int x) post<scl::defaulted>(x > 0) { return x; }
int post_r(int x) post<::std::contracts::controls::defaulted>(r : r > 0) { return x; }
void assert_d(int x) {
  using namespace std::contracts::controls;
  contract_assert<defaulted>(x > 0);
}
void assert_u(int x) { contract_assert(x > 0); }

struct S {
  int mem(int x) pre<scl::defaulted>(x > 0) { return x; }
  static int smem(int x) pre<std::contracts::controls::defaulted>(x > 0);
};
int S::smem(int x) pre<scl::defaulted>(x > 0) { return x; }

template <typename T>
T tmpl(const T x) pre<scl::defaulted>(x > 0)
    post<std::contracts::controls::defaulted>(r : r == x) {
  return x;
}

auto lam = [](int x) pre<scl::defaulted>(x > 0) { return x; };

#ifdef __cpp_lib_contracts_labels
int review_d(int x) pre<scl::defaulted | scl::review>(x > 0) { return x; }
int review_r(int x) pre<scl::review | scl::defaulted>(x > 0) { return x; }
int review_u(int x) pre<scl::review>(x > 0) { return x; }
#endif

// Calls F, which violates its assertion when it is checked: the handler is
// called once, with semantic SEM, unless SEM is ignore.
template <typename F> void check(F f, evaluation_semantic sem) {
  expected = sem;
  int before = handled;
  try {
    f();
  } catch (int) {
  }
  if (handled != before + (sem == evaluation_semantic::ignore ? 0 : 1))
    std::abort();
}

int main() {
  S s;
  check([] { pre_d(-1); }, configured);
  check([] { pre_u(-1); }, configured);
  check([] { post_d(-1); }, configured);
  check([] { post_r(-1); }, configured);
  check([] { assert_d(-1); }, configured);
  check([] { assert_u(-1); }, configured);
  check([&] { s.mem(-1); }, configured);
  check([] { S::smem(-1); }, configured);
  check([] { tmpl(-1); }, configured);
  check([] { lam(-1); }, configured);
#ifdef __cpp_lib_contracts_labels
  constexpr evaluation_semantic reviewed =
      configured == evaluation_semantic::enforce ? evaluation_semantic::observe
                                                 : configured;
  check([] { review_d(-1); }, reviewed);
  check([] { review_r(-1); }, reviewed);
  check([] { review_u(-1); }, reviewed);
#endif
  int count = handled;
  // Satisfied assertions do not call the handler.
  pre_d(1); pre_u(1); post_d(1); post_r(1); assert_d(1); assert_u(1);
  s.mem(1); S::smem(1); tmpl(1); lam(1);
  if (handled != count)
    std::abort();
}
