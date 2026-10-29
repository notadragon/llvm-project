// RUN: %clangxx -std=c++26 %s -O2 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -Wno-contract-unproven -fcontract-analysis-semantic=prove -fcontract-evaluation-semantic=ignore %libcxx_flags -o %t
// RUN: %t
// REQUIRES: contracts-libcxx

// D4405 ("The Analysis Evaluation", "Adjusting to the Allowed Semantics"):
// the runtime half of the label x configuration matrix, configured by flags.
// Configured: analysis prove, runtime ignore.  The runtime evaluations keep
// their own semantic, which is never prove or diagnose and which the
// analysis configuration never changes: the configured one for no label and
// for an analysis_semantic facet, enforce for terminating and
// always_enforce (and a label combined with them), and ignore for
// always_diagnose, symbolic and a compute_semantic result of prove or
// diagnose (moved to the analysis evaluation).  The violation handler
// records the semantic and throws, so an enforced violation is seen and
// survived.  Each precondition is called through a pointer, which has no
// analysis evaluation, so the callee's own check is what runs; a
// postcondition and a contract_assert are run only where their analysis
// evaluation is not prove (which could not prove them).  always_prove,
// whose assertions are never checked and whose address cannot be taken, is
// in the compile half (p4405-analysis-matrix-*.cpp).
//
// GCC's gnu::noipa attribute is gnu::noinline here (Clang has no noipa).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-matrix-run-prove-ignore.C

#include <contracts>

namespace L = std::contracts::labels;
using L::operator|;
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set allowed_semantics{diagnose};
};
constexpr always_diagnose_t always_diagnose{};

template <evaluation_semantic S>
struct analyzed {
  using assertion_control_object = analyzed;
  constexpr evaluation_semantic analysis_semantic() const { return S; }
};
constexpr analyzed<diagnose> diagnosed{};
constexpr analyzed<prove> proven{};

template <evaluation_semantic S>
struct computes {
  using assertion_control_object = computes;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const
  { return S; }
};
constexpr computes<prove> computes_prove{};
constexpr computes<diagnose> computes_diagnose{};

struct violated {};
static int evaluated, violations;
static evaluation_semantic last;
[[gnu::noinline]] bool chk(int x) { ++evaluated; return x > 0; }

void handle_contract_violation(const std::contracts::contract_violation &v)
{
  ++violations;
  last = v.semantic();
  throw violated {};
}

// Run F, then check that the evaluation was checked with enforce (ENFORCED)
// or not evaluated at all.
template <typename F>
[[gnu::noinline]] void expect(F f, bool enforced)
{
  int e = evaluated, v = violations;
  bool caught = false;
  try { f(); } catch (violated) { caught = true; }
  if (enforced)
    {
      if (!caught || evaluated != e + 1 || violations != v + 1
          || last != enforce)
        __builtin_abort();
    }
  else if (caught || evaluated != e || violations != v)
    __builtin_abort();
}


// no label: analysis prove, runtime ignore.
[[gnu::noinline]] int pre_none(int x) pre(chk(x)) { return x; }
int(*volatile p_none) (int) = pre_none;

// always_diagnose: analysis diagnose, runtime ignore.
[[gnu::noinline]] int pre_ad(int x) pre<always_diagnose>(chk(x)) { return x; }
int(*volatile p_ad) (int) = pre_ad;
[[gnu::noinline]] int post_ad(int x) post<always_diagnose>(r: chk(r)) { return x; }
[[gnu::noinline]] int assert_ad(int x) { contract_assert<always_diagnose>(chk(x)); return x; }

// L::terminating: analysis prove, runtime enforce.
[[gnu::noinline]] int pre_term(int x) pre<L::terminating>(chk(x)) { return x; }
int(*volatile p_term) (int) = pre_term;

// L::symbolic: analysis prove, runtime ignore.
[[gnu::noinline]] int pre_sym(int x) pre<L::symbolic>(chk(x)) { return x; }
int(*volatile p_sym) (int) = pre_sym;

// L::always_enforce: analysis ignore, runtime enforce.
[[gnu::noinline]] int pre_ae(int x) pre<L::always_enforce>(chk(x)) { return x; }
int(*volatile p_ae) (int) = pre_ae;
[[gnu::noinline]] int post_ae(int x) post<L::always_enforce>(r: chk(r)) { return x; }
[[gnu::noinline]] int assert_ae(int x) { contract_assert<L::always_enforce>(chk(x)); return x; }

// diagnosed: analysis prove, runtime ignore.
[[gnu::noinline]] int pre_fd(int x) pre<diagnosed>(chk(x)) { return x; }
int(*volatile p_fd) (int) = pre_fd;

// proven: analysis prove, runtime ignore.
[[gnu::noinline]] int pre_fp(int x) pre<proven>(chk(x)) { return x; }
int(*volatile p_fp) (int) = pre_fp;

// diagnosed | proven: analysis prove, runtime ignore.
[[gnu::noinline]] int pre_fdfp(int x) pre<diagnosed | proven>(chk(x)) { return x; }
int(*volatile p_fdfp) (int) = pre_fdfp;

// diagnosed | L::symbolic: analysis prove, runtime ignore.
[[gnu::noinline]] int pre_fdsym(int x) pre<diagnosed | L::symbolic>(chk(x)) { return x; }
int(*volatile p_fdsym) (int) = pre_fdsym;

// diagnosed | L::terminating: analysis prove, runtime enforce.
[[gnu::noinline]] int pre_fdterm(int x) pre<diagnosed | L::terminating>(chk(x)) { return x; }
int(*volatile p_fdterm) (int) = pre_fdterm;

// proven | L::always_enforce: analysis ignore, runtime enforce.
[[gnu::noinline]] int pre_fpae(int x) pre<proven | L::always_enforce>(chk(x)) { return x; }
int(*volatile p_fpae) (int) = pre_fpae;
[[gnu::noinline]] int post_fpae(int x) post<proven | L::always_enforce>(r: chk(r)) { return x; }
[[gnu::noinline]] int assert_fpae(int x) { contract_assert<proven | L::always_enforce>(chk(x)); return x; }

// computes_prove: analysis prove, runtime ignore.
[[gnu::noinline]] int pre_cp(int x) pre<computes_prove>(chk(x)) { return x; }
int(*volatile p_cp) (int) = pre_cp;

// computes_diagnose: analysis prove, runtime ignore.
[[gnu::noinline]] int pre_cd(int x) pre<computes_diagnose>(chk(x)) { return x; }
int(*volatile p_cd) (int) = pre_cd;

int main()
{
  expect([] { p_none(-1); }, false);
  expect([] { p_ad(-1); }, false);
  expect([] { post_ad(-1); }, false);
  expect([] { assert_ad(-1); }, false);
  expect([] { p_term(-1); }, true);
  expect([] { p_sym(-1); }, false);
  expect([] { p_ae(-1); }, true);
  expect([] { post_ae(-1); }, true);
  expect([] { assert_ae(-1); }, true);
  expect([] { p_fd(-1); }, false);
  expect([] { p_fp(-1); }, false);
  expect([] { p_fdfp(-1); }, false);
  expect([] { p_fdsym(-1); }, false);
  expect([] { p_fdterm(-1); }, true);
  expect([] { p_fpae(-1); }, true);
  expect([] { post_fpae(-1); }, true);
  expect([] { assert_fpae(-1); }, true);
  expect([] { p_cp(-1); }, false);
  expect([] { p_cd(-1); }, false);
}
