// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify -verify-ignore-unexpected=note %s

// A friend declaration whose declarator-id names a member function of another
// class redeclares that member ([dcl.contract.func]), so its contract
// specifiers, if any, must be the same as the member's (CLANG-639: Clang
// accepted the marked mismatches; the friend's contracts are parsed at the end
// of the class, after the declarations were merged, and were never compared).
// So must a friend's redeclaring a function declared at namespace scope.
// CLANG-120 is the task that asked for this pin.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/qualified-friend-member-contract-mismatch.C
// in the gnu_gcc fork.  Found by the EDG fork's
// sema/friend_member_redeclaration test.

struct Inner {
  void fn(int n) pre(n > 0);
  void fn3(int n) pre(n > 0);
  void fn4(int n) pre(n > 0);
  void fn5(int n);
};
struct Outer {
  friend void Inner::fn3(int m) pre(m > 0);  // OK, parameter renamed
  friend void Inner::fn4(int n);  // OK, omitted
  friend void Inner::fn(int n) pre(n < 0);  // expected-error {{function redeclaration differs in contract specifier sequence}}
  friend void Inner::fn5(int n) pre(n > 0);  // expected-error {{function redeclaration differs in contract specifier sequence}}
};

// The befriending class is complete in the contract assertions.
struct O2 {
  struct In {
    void fn(int n) pre(n > 0 && bob > 1);
    void fn2(int n) pre(n > 0);
  };
  friend void In::fn(int n) pre(n > 0 && bob > 1);  // OK
  friend void In::fn2(int n) pre(n > 1);  // expected-error {{function redeclaration differs in contract specifier sequence}}
  static int bob;
};

// A friend redeclaring a function declared at namespace scope.
void g(int n) pre(n > 0);
void h(int n);
void k(int n) pre(n > 0);
struct A {
  friend void g(int n) pre(n < 0);  // expected-error {{function redeclaration differs in contract specifier sequence}}
  friend void h(int n) pre(n < 0);  // expected-error {{function redeclaration differs in contract specifier sequence}}
  friend void k(int m) pre(m > 0);  // OK
};

// A qualified friend redeclaration's contract assertions are parsed and
// checked like any other: an undeclared name is diagnosed.
struct O3 {
  struct In {
    void fn(int n) pre(n > 0 && bob > 1);
  };
  friend void In::fn(int n) pre(n > 0 && nope > 1);  // expected-error {{use of undeclared identifier 'nope'}}
  static int bob;
};
