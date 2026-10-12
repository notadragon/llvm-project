// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s
// expected-no-diagnostics

// What a contract predicate modifies during constant evaluation is undone
// once its value is known (see contract-constexpr-side-effect.cpp).  These are
// the shapes of modification the undo has to cope with: a member, a whole
// aggregate, array elements including one the initializer omitted, a union's
// active member, the object a constructor is building, a loop variable, a
// named result, and heap storage the predicate deallocates or allocates.  Each
// predicate also checks, under enforce, that it sees its own modifications
// while it runs.
//
// Mirror of gnu_gcc's contract-constexpr-side-effect-rollback.C.

template <class T>
constexpr T &mut(const T &r) { return const_cast<T &>(r); }

struct pt { int x, y; };

// A member, then the whole aggregate.
constexpr int member() {
  pt p{1, 2};
  contract_assert((mut(p).x = 10, mut(p) = pt{7, 8}, p.x == 7));
  return p.x * 10 + p.y;
}
static_assert(member() == 12);

// Array elements in a nested aggregate, one of them omitted from the
// initializer, so the predicate adds an element that was not there.
struct box { pt pts[3]; int n; };

constexpr int array() {
  box b{{{1, 1}, {2, 2}}, 2};
  contract_assert((mut(b).pts[2] = pt{5, 5}, mut(b).pts[0].x = 9,
                   ++mut(b).n, b.pts[2].y + b.n == 8));
  return b.pts[0].x * 100 + b.pts[2].x * 10 + b.n;
}
static_assert(array() == 102);

// A union's active member.
union num { int i; float f; };

constexpr int union_member() {
  num v{.i = 3};
  contract_assert((mut(v).f = 1.0f, v.f == 1.0f));
  return v.i;
}
static_assert(union_member() == 3);

// The object a constructor is building, through a mutable member, both in
// the constructor body and afterwards; and as the initializer of a constexpr
// variable.
struct counted {
  mutable int hits = 0;
  int v;
  constexpr counted(int a) : v(a) { contract_assert((++hits, hits == 1)); }
  constexpr bool check() const {
    ++hits;
    return hits == 1;
  }
};

constexpr int under_construction() {
  counted c(4);
  contract_assert(c.check());
  return c.hits * 10 + c.v;
}
static_assert(under_construction() == 4);

constexpr int via_variable() {
  counted c(5);
  return c.hits * 10 + c.v;
}
static_assert(via_variable() == 5);

constexpr counted cc(6);
static_assert(cc.v == 6);

// A member initializer that reaches a contract which modifies a member the
// constructor has already initialized, while the constructor is still
// initializing the object in place.
struct building {
  int a;
  int b;
  constexpr building()
      : a(1), b(([this] { contract_assert((++mut(a), a == 2)); }(), a)) {}
};
static_assert(building().a == 1 && building().b == 1);

// A loop variable: the loop still runs five times.
constexpr int loop() {
  int iters = 0;
  for (int i = 0; i < 5; ++i) {
    contract_assert((mut(i) += 2, i % 2 == 0 || i > 2));
    ++iters;
  }
  return iters;
}
static_assert(loop() == 5);

// A named result, of scalar and of class type.
constexpr int post_scalar(const int a) post(r : (++mut(r), r == a + 1)) {
  return a;
}
static_assert(post_scalar(3) == 3);

struct big {
  int a;
  constexpr big(int x) : a(x) {}
  constexpr big(const big &o) : a(o.a) {}
};

constexpr big post_class(const int a) post(r : (++mut(r).a, r.a == a + 1)) {
  return big(a);
}
static_assert(post_class(3).a == 3);

// Heap storage: the predicate deallocates an allocation of the enclosing
// evaluation, which is then still alive and still the caller's to free; and
// a predicate that allocates and frees its own.
constexpr bool free_it(int *p) {
  delete p;
  return true;
}

constexpr int heap() {
  int *p = new int(6);
  contract_assert(free_it(p));
  int v = *p;
  delete p;
  return v;
}
static_assert(heap() == 6);

// An array allocation, modified and then deallocated by the predicate.
constexpr bool scribble_and_free(int *p) {
  p[0] = 9;
  p[1] = 9;
  delete[] p;
  return true;
}

constexpr int heap_array() {
  int *p = new int[2]{3, 4};
  contract_assert(scribble_and_free(p));
  int v = p[0] * 10 + p[1];
  delete[] p;
  return v;
}
static_assert(heap_array() == 34);

constexpr bool own_heap() {
  int *q = new int(1);
  bool ok = *q == 1;
  delete q;
  return ok;
}

constexpr int heap_own() {
  int *p = new int(2);
  contract_assert(own_heap() && *p == 2);
  int v = *p;
  delete p;
  return v;
}
static_assert(heap_own() == 2);
