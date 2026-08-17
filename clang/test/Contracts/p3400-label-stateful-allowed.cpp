// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t && %t

// An allowed_semantics facet held as a non-static data member must restrict
// the semantic just as a static one does.
//
// Mirror of gnu_gcc's p3400-label-stateful-allowed.C, companion to
// p3400-label-stateful-facet.cpp and the same root cause there: the probe of
// label.allowed_semantics.contains() could not fold against a materialized
// label, so the mask stayed permissive and the restriction silently did not
// apply -- enforcing, and terminating, an assertion the label said to
// observe.  Clang does not reproduce it; carried over as a guard.
//
// The build default is enforce and every label below allows only observe and
// ignore, so a correct implementation clamps to observe: the handler runs and
// the program continues.  Getting it wrong terminates.

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

static int ran = 0;

static constexpr int key = 0;

static constexpr evaluation_semantic_set observe_or_ignore
  = { evaluation_semantic::observe, evaluation_semantic::ignore };

/* Non-static member, with a query facet so the label is materialized.  */
struct MemAllowed
{
  using assertion_control_object = MemAllowed;
  evaluation_semantic_set allowed_semantics;
  constexpr MemAllowed (evaluation_semantic_set a) : allowed_semantics (a) {}
  void *query (const void *k, __SIZE_TYPE__ i) const
  { return (i != 0 || k != &key) ? nullptr : const_cast<char *> ("M"); }
};

/* Static member: the form that always worked.  */
struct StatAllowed
{
  using assertion_control_object = StatAllowed;
  static constexpr evaluation_semantic_set allowed_semantics = observe_or_ignore;
  void *query (const void *k, __SIZE_TYPE__ i) const
  { return (i != 0 || k != &key) ? nullptr : const_cast<char *> ("S"); }
};

/* Non-static member, and no facet needing an address: never materialized,
   so this always worked too.  */
struct MemAllowedNoFacet
{
  using assertion_control_object = MemAllowedNoFacet;
  evaluation_semantic_set allowed_semantics;
  constexpr MemAllowedNoFacet (evaluation_semantic_set a)
    : allowed_semantics (a) {}
};

constexpr MemAllowed named { observe_or_ignore };

struct Factory
{
  static constexpr MemAllowed make (evaluation_semantic_set a
				      = observe_or_ignore)
  { return MemAllowed (a); }
};

void
handle_contract_violation (const contract_violation &v)
{
  ++ran;
  if (v.is_terminating ())
    __builtin_abort ();		/* the label restricted us to observe */
}

static void
expect_clamped ()
{
  if (ran != 1)
    __builtin_abort ();
  ran = 0;
}

int
main ()
{
  contract_assert<MemAllowed{observe_or_ignore}> (false);
  expect_clamped ();

  contract_assert<named> (false);
  expect_clamped ();

  contract_assert<Factory::make (observe_or_ignore)> (false);
  expect_clamped ();

  contract_assert<Factory::make ()> (false);
  expect_clamped ();

  contract_assert<StatAllowed{}> (false);
  expect_clamped ();

  contract_assert<MemAllowedNoFacet{observe_or_ignore}> (false);
  expect_clamped ();

  return 0;
}
