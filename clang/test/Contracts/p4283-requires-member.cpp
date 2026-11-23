// P4283: a requires-clause on a *member* function's contract is honored.  Unlike
// a free function (parsed immediately), a member's contract specifier is token-
// cached and re-parsed after the class is complete, so the requires-clause has
// to be cached too.  This covers class-template members and member-function
// templates, non-virtual and virtual, with bare/grouped/conjoined constraints
// and captures.
//
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3850 -fcontracts-p4283 \
// RUN:   -fcontracts-p3097 -fcontracts-p3098 -fcontracts-p3400 \
// RUN:   -fsyntax-only -verify %s
// expected-no-diagnostics

template <class T> concept Small = sizeof(T) <= 8;
template <class T> concept Any = true;

template <class T>
struct S {
  // Bare concept-id constraint (the paper's primary form).
  void a(T x) pre requires Small<T> (x > 0) {}

  // Grouped constraint (a parenthesized primary-expression).
  void b(T x) pre requires (Small<T>) (x > 0) {}

  // Conjunction / disjunction of concept-ids.
  void c(T x) pre requires Small<T> && Any<T> (x > 0) {}
  void d(T x) pre requires Small<T> || Any<T> (x > 0) {}

  // A requires-expression as the constraint's primary.
  void e(T x) pre requires requires(T t) { t > t; } (x > 0) {}

  // Virtual member with a requires-clause on its contract.
  virtual void f(T x) pre requires (Small<T>) (x > 0) {}

  // Postcondition with captures after the requires-clause.
  void g(T x) post requires Small<T> [x] (x == x) {}

  // Member function template with a constraint on its parameter.
  template <class U>
  void h(U y) pre requires Small<U> (y == y) {}

  // Virtual member function is required for the vtable; keep a dtor.
  virtual ~S() = default;
};

template struct S<int>;

void use() {
  S<int> s;
  s.a(1);
  s.b(1);
  s.c(1);
  s.d(1);
  s.e(1);
  s.f(1);
  s.g(1);
  s.h(1);
  s.h(1.0);
}
