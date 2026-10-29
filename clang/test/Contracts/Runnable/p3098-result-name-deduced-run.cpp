// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// A postcondition's result name in a function with a deduced return type is
// checked at run time: postconditions in declaration order, with a capture,
// on a member naming the object, on a decltype(auto) reference result, in a
// function template's instance and in a lambda.  Each is first satisfied,
// then made to fail on its own through `bad'.  (EDG M13c2 shape:
// codegen/result_name_deduced.)
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3098-result-name-deduced-run.C

#include <contracts>
#include <cstring>

static char events[16];
static int nevents;
static bool note (char c) { events[nevents++] = c; return true; }

static int violations;
void handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (v.kind () != std::contracts::assertion_kind::post)
    __builtin_abort ();
  ++violations;
}

int bad;

auto f (const int x) post (note ('a')) post (r: note ('b') && r == x + (bad == 1))
  post (note ('c'))
{
  if (x > 10)
    return x;
  return x;
}
auto cap (int &x) post [old = x] (r: r == old + 1 + (bad == 2))
{
  ++x;
  return x;
}
struct S
{
  int m = 4;
  auto get () const post (r: r == m + (bad == 3)) { return m; }
};
int g = 7;
decltype(auto) ref () post (r: &r == &g && bad != 4) { return (g); }
template <class T> auto twice (const T t) post (r: r == t * 2 + (bad == 5))
{
  return t * 2;
}
auto lam = [] (const int y) post (r: r == y + (bad == 6)) { return y; };

static void run ()
{
  f (3);
  int i = 1;
  cap (i);
  S s;
  s.get ();
  int &gr = ref ();
  if (&gr != &g)
    __builtin_abort ();
  twice (4);
  lam (2);
}

int main ()
{
  for (bad = 0; bad <= 6; ++bad)
    {
      nevents = 0;
      violations = 0;
      run ();
      if (nevents != 3 || std::memcmp (events, "abc", 3) != 0)
	__builtin_abort ();
      if (violations != (bad != 0))
	__builtin_abort ();
    }
}
