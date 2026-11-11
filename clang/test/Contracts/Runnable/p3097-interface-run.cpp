// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097: a virtual call checks the statically chosen function's contract
// assertions around the final overrider's own, passing the arguments on as
// they are: an argument passed by value is copied once (by the caller) and
// a class result is returned once, as for a non-virtual call; a covariant
// result needing a base adjustment is checked after the adjustment; and a
// violation of the statically chosen function's precondition reports that
// function.  (EDG M13c2 shapes: p3097/interface_run,
// p4298cgen/interface_handler.)
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3097-interface-run.C

#include <contracts>
#include <cstring>

static char events[64];
static int nevents;
static bool note (char c) { events[nevents++] = c; return true; }
static bool expect (const char *s)
{
  bool ok = nevents == (int) std::strlen (s)
	    && std::memcmp (events, s, nevents) == 0;
  nevents = 0;
  return ok;
}

static int violations;
static const char *violated_in;
void handle_contract_violation (const std::contracts::contract_violation &v)
{
  ++violations;
  violated_in = v.location ().function_name ();
}

static int copies;
struct Arg
{
  int v;
  Arg (int x) : v (x) {}
  Arg (const Arg &o) : v (o.v) { ++copies; }
};
struct Res
{
  int v;
  Res (int x) : v (x) {}
  Res (const Res &o) : v (o.v) { ++copies; }
};

// Pre/post of B are 'a'/'b', of D 'c'.
struct B
{
  virtual int f (const int x) pre (note ('a')) post (r: note ('b') && r == x)
  { return x; }
  virtual int g (Arg a) pre (a.v > 0) { return a.v; }
  virtual Res h (const int x) post [old = x] (r: r.v == old)
  { return Res (x); }
  virtual B *self () post (r: r == this) { return this; }
  virtual int k (int x) pre (x > 0) { return x; }
  virtual ~B () {}
};
struct Other { int o = 0; virtual ~Other () {} };
struct D : Other, B
{
  int f (const int x) override pre (note ('c')) { return x; }
  int g (Arg a) override { return a.v; }
  Res h (const int x) override { return Res (x); }
  D *self () override { return this; }
  int k (int x) override { return x; }
};

int main ()
{
  B b;
  D d;
  B *pb = &b, *pd = &d;
  // The statically chosen function is also the final overrider here, so
  // its assertions are checked twice, as the interface's and as its own.
  pb->f (1);
  if (!expect ("aabb")) __builtin_abort ();
  pd->f (1);
  if (!expect ("acb")) __builtin_abort ();
  pd->B::f (1);
  if (!expect ("ab")) __builtin_abort ();

  copies = 0;
  if (pd->g (Arg (1)) != 1 || copies != 0)
    __builtin_abort ();
  Arg a (2);
  if (pd->g (a) != 2 || copies != 1)
    __builtin_abort ();
  copies = 0;
  Res r = pd->h (3);
  if (r.v != 3 || copies != 0)
    __builtin_abort ();
  if (pd->self () != static_cast<B *> (&d))
    __builtin_abort ();
  if (violations != 0)
    __builtin_abort ();

  pd->k (0);
  if (violations != 1 || !std::strstr (violated_in, "B::k"))
    __builtin_abort ();
}
