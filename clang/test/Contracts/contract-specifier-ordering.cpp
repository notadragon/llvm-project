// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fsyntax-only -verify %s

// [class.mem.general]p11.4.1:
//
//   member-declarator:
//     declarator virt-specifier-seq[opt]
//         function-contract-specifier-seq[opt] pure-specifier[opt]
//     declarator requires-clause function-contract-specifier-seq[opt]
//
// so a function-contract-specifier-seq is the last thing before an optional
// pure-specifier: neither a virt-specifier nor a requires-clause may follow
// it.  Both reversals used to be accepted silently.
//
// The check cannot live where it looks like it should.  For a member function
// declarator the contract is consumed inside ParseFunctionDeclarator, before
// ParseCXXMemberDeclaratorBeforeInitializer ever looks at `override` -- so by
// the time the virt-specifier is parsed the contract is already gone, and the
// discriminator has to be "did this declarator already take a contract",
// not "is the current token a contract keyword".

template <typename> concept C = true;

struct Base {
  virtual void f();
  virtual void g();
  virtual void h();
};

struct Derived : Base {
  void f() pre(true) override; // expected-error {{'override' must appear before the function contract specifiers}}
  // expected-note@-1 {{function contract specifier written here}}

  void g() pre(true) final; // expected-error {{'final' must appear before the function contract specifiers}}
  // expected-note@-1 {{function contract specifier written here}}

  // The GNU-attribute route reaches a second, separate virt-specifier parse,
  // so a fix applied to only the first one misses this.
  void h() pre(true) __attribute__((unused)) override; // expected-error {{'override' must appear before the function contract specifiers}}
  // expected-note@-1 {{function contract specifier written here}}
  // expected-warning@-2 {{GCC does not allow an attribute in this position on a function declaration}}
};

struct MemberRequires {
  template <class T>
  void p() pre(true) requires C<T>; // expected-error {{'requires' must appear before the function contract specifiers}}
  // expected-note@-1 {{function contract specifier written here}}
};

template <typename T>
int free_wrong_order(T v) pre(v > T{}) requires C<T>; // expected-error {{'requires' must appear before the function contract specifiers}}
// expected-note@-1 {{function contract specifier written here}}

// ---------------------------------------------------------------------------
// Controls.  The well-formed order must keep working, and a virt-specifier or
// requires-clause with NO contract in front of it must not be touched -- the
// discriminator is the declarator having taken a contract, and getting that
// wrong would reject every `void f() override;` in the language.
// ---------------------------------------------------------------------------

struct Ok : Base {
  void f() override pre(true);
  void g() final pre(true);
  void h() override;
};

struct OkRequires {
  template <class T>
  void p() requires C<T> pre(true);

  template <class T>
  void q() requires C<T>;
};

template <typename T>
int free_right_order(T v) requires C<T> pre(v > T{});

struct OkPure : Base {
  void f() override pre(true) = 0;
};
