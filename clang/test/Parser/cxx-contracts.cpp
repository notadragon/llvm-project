// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 %s -verify

// pre and post, with and without a result name, parse on a free function, a
// constructor, a member function and a lambda; several of each in any order;
// after a trailing return type, a virt-specifier (P3097 allows contracts on
// virtual functions) and a requires-clause; and contract_assert as a
// statement.  Then the positions where a contract specifier or assertion
// cannot appear.  The semantics are tested under test/Contracts.

int f(const int x) pre(x != 0)
             post(r : x > 0) {
  return x;
}

struct Foo {
    int x;
    Foo(const int x) pre(x != 0 && this->x != 0) : x(x) {}
    int get() post(this->x > 0) {
      return x;
    }
};

auto lam = [](const int x) pre(x != 0) post(r : x > 0) { return x; };

// Several of each, in any order.
int g(int x) pre(x > 0) pre(x < 100) post(r : r > 0) post(r : r < 100);
int h(int x) post(r : r > 0) pre(x > 0) post(r : r < 100) pre(x < 100);

// After a trailing return type, a virt-specifier and a requires-clause.
auto t(int x) -> int pre(x > 0);
struct Base {
  virtual int v(int x) const;
};
struct Derived : Base {
  int v(int x) const override pre(x > 0);
};
template <class T> int c(T x) requires(sizeof(T) > 1) pre(x > 0);

// contract_assert as a statement.
void s(int x) {
  contract_assert(x > 0);
  if (x)
    contract_assert(x != 0);
  for (;;) {
    contract_assert(true);
    break;
  }
}

// Invalid positions.
void e1(int x) pre(x > 0) const; // expected-error {{expected ';' after top level declarator}} \
                                 // expected-warning {{declaration does not declare anything}}
void e2(int x) pre(x > 0) noexcept; // expected-error {{expected function body after function declarator}}
void e3(int x) pre x > 0; // expected-error {{expected '(' after 'pre'}} \
                          // expected-error {{expected function body after function declarator}}
int e4 pre(true); // expected-error {{'pre' can only appear on a function declaration}}
void e5(int x) { pre(x > 0); } // expected-error {{use of undeclared identifier 'pre'}}
void e6(int x) { int y = contract_assert(x); } // expected-error {{expected expression}}
