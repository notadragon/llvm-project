// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s

// P3098 postcondition captures and result names, evaluated during constant
// evaluation, in every kind of function: an in-class member (GCC-133: the
// capture is bound in the callee's frame), and templated functions, where
// substitution drops the capture's copy-initialization and used to leave it
// bound to the parameter itself -- not constant, or a crash on arithmetic
// (CLANG-623) -- and a class template member's result name, which the
// substituted predicate kept referring to by its pattern declaration
// (CLANG-624).  GCC failed the operator init-captures below at parse
// time (GCC-629).
// (GCC mirror: g++.dg/contracts/cpp26/p3098-capture-constexpr-templates.C)

struct S {
  int v = 0;
  constexpr int bump(int x) post [x] (r: r == x + 1) { return x + 1; }
  constexpr void add(const int x) post [old = v] (v == old + x) { v += x; }
};
static_assert(S{}.bump(5) == 6);
static_assert([] { S s; s.add(3); s.add(4); return s.v; }() == 7);

template <class T> struct TS {
  constexpr T bump(T x) post [x] (r: r == x + 1) { return x + 1; }
  constexpr int nondep(int x) post [x] (r: r == x + 1) { return x + 1; }
  constexpr int result_only(const int x) post (r: r == x + 1) { return x + 1; }
  constexpr int out_of_line(const int x) post (r: r == x + 1);
};
template <class T>
constexpr int TS<T>::out_of_line(const int x) post (r: r == x + 1) {
  return x + 1;
}
static_assert(TS<int>{}.bump(5) == 6);
static_assert(TS<int>{}.nondep(5) == 6);
static_assert(TS<int>{}.result_only(5) == 6);
static_assert(TS<int>{}.out_of_line(5) == 6);

template <class T> constexpr T f(T x) post [x] (x == 5) { return x + 1; }
static_assert(f(5) == 6);

template <class T> constexpr T g(T x) post [y = x * 2] (r: r == y + 1) {
  return x * 2 + 1;
}
static_assert(g(5) == 11);

template <class... T> constexpr int sum(T... xs)
  post [xs...] (r: r == (xs + ... + 0)) {
  return (xs + ... + 0);
}
static_assert(sum(1, 2, 3) == 6);

struct M {
  template <class T> constexpr T bump(T x) post [x] (r: r == x + 1) {
    return x + 1;
  }
};
static_assert(M{}.bump(5) == 6);

// An init-capture whose initializer is an operator expression on a
// dependent parameter, alone and as a pack, and in a function that is only
// called at run time (GCC-629: "sorry, unimplemented: use of
// dependent_operator_type in template").
template <class T> constexpr T neg(T x) post [y = -x] (r: r == -y * 2 + 1) {
  return x * 2 + 1;
}
static_assert(neg(5) == 11);
template <class... T> constexpr int twice(T... xs)
  post [... ys = xs * 2] (r: r == (ys + ... + 0)) {
  return (xs + ... + 0) * 2;
}
static_assert(twice(1, 2, 3) == 12);
template <class T> T runtime_only(T x) post [y = x * 2] (r: r == y + 1) {
  return x * 2 + 1;
}
int use_runtime_only() { return runtime_only(5); }

// A violated postcondition is still diagnosed, through the capture.
template <class T> constexpr T h(T x) post [x] (r: r == x) { return x + 1; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int bad = h(5); // expected-error {{constexpr variable 'bad' must be initialized by a constant expression}} expected-note {{in call to 'h<int>(5)'}}
