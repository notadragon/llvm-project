// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// Under P3097 a virtually dispatched destructor call gets the interface check
// of the static type's destructor, as any virtual call does, so the static
// type's contracts are evaluated twice: once for the dispatch and once when
// the base subobject is destroyed (DECISIONS.md K8).  A non-virtual
// destructor call evaluates them only once.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3097-virtual-dtor.C

static int b_pre, b_post, d_pre, d_post;

// A predicate sees every name as const, so the counting is done by calls.
static bool hit_b_pre () { ++b_pre; return true; }
static bool hit_b_post () { ++b_post; return true; }
static bool hit_d_pre () { ++d_pre; return true; }
static bool hit_d_post () { ++d_post; return true; }

struct B {
  virtual ~B () pre (hit_b_pre ()) post (hit_b_post ()) {}
};

struct D : B {
  ~D () override pre (hit_d_pre ()) post (hit_d_post ()) {}
};

int main ()
{
  B *p = new D;
  delete p;
  if (b_pre != 2 || b_post != 2 || d_pre != 1 || d_post != 1)
    __builtin_abort ();

  b_pre = b_post = d_pre = d_post = 0;
  {
    D d;
  }
  if (b_pre != 1 || b_post != 1 || d_pre != 1 || d_post != 1)
    __builtin_abort ();
}

// REQUIRES: contracts-libcxx, native
