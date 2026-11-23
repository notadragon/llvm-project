// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify -verify-ignore-unexpected=note %s

// An implicit capture made only because contract assertions of the lambda
// name the entity is ill-formed (P2900 [expr.prim.lambda.closure]): the edge
// cases of the EDG fork's sema/contract_only_capture and
// sema/generic_lambda_capture tests beyond lambda-contract-only-capture.cpp --
// "this" through a member call in a generic lambda and in default member
// initializers, a structured binding, an init-capture, the capture made by
// an outer lambda for a lambda in a contract assertion (or in one's own
// precondition), a lambda in a template, a generic lambda's own
// precondition and postcondition, and the controls that are not captures
// for contract assertions only.  One error per line.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/lambda-contract-only-capture-edges.C
// in the gnu_gcc fork.

struct S {
  int m;
  int get () const { return m; }
  void h1 () { [&] (auto a) { contract_assert (get () > a); } (1); }	// expected-error {{must be captured explicitly}}
  void h2 () { auto l = [&] { contract_assert (get () > 0); get (); }; l (); }
};

struct T {
  int m = 0;
  int f = [this] { [&] { contract_assert (m > 0); } (); return 1; } ();	// expected-error {{must be captured explicitly}}
  int g = [&] { contract_assert (this->m > 0); return 1; } ();	// expected-error {{must be captured explicitly}}
};

void f (int x)
{
  auto l1 = [=] { return [=] { contract_assert (x > 0); }; };	// expected-error {{must be captured explicitly}}
  auto l2 = [=] { contract_assert ([=] { return x > 0; } ()); };	// expected-error {{must be captured explicitly}}
  auto l3 = [=] { return [x] { contract_assert (x > 0); }; };
  auto l4 = [=] { contract_assert ([x] { return x > 0; } ()); };	// expected-error {{must be captured explicitly}}
  auto l5 = [y = x] { [=] { contract_assert (y > 0); } (); };	// expected-error {{must be captured explicitly}}
  auto l6 = [&] { contract_assert (x > 0); contract_assert (x < 9); };	// expected-error {{must be captured explicitly}}
  auto l7 = [=] { contract_assert ([=] { contract_assert (x > 0); return x > 1; } ()); };	// expected-error {{must be captured explicitly}}
  auto l8 = [&] { [&] { contract_assert (x > 0); } (); (void) x; };	// expected-error {{must be captured explicitly}}
  auto l9 = [&] { [&] { (void) x; } (); contract_assert (x > 0); };
  auto l10 = [&] { contract_assert (sizeof (x) > 0); };
  auto l11 = [=] () mutable { contract_assert (x > 0); x = 1; };
  l1 (); l2 (); l3 (); l4 (); l5 (); l6 (); l7 (); l8 (); l9 (); l10 (); l11 ();
}

void sb ()
{
  struct B { int a, b; };
  auto [a, b] = B {1, 2};
  auto l = [&] { contract_assert (a > 0); };	// expected-error {{must be captured explicitly}}
  l ();
}

template <class T>
void g (T x)
{
  auto l = [=] { contract_assert (x > 0); };	// expected-error {{must be captured explicitly}}
  auto m = [=] { contract_assert (x > 0); return x; };
  l (); m ();
}
template void g (int);

void own (int i)
{
  bool y = true;
  auto p1 = [=] (const int a) post (i > a) {};	// expected-error {{must be captured explicitly}}
  auto p2 = [=] pre ([=] { return y; } ()) {};	// expected-error {{must be captured explicitly}}
  auto p3 = [=] { return [=] pre (i > 0) {}; };	// expected-error {{must be captured explicitly}}
  auto p4 = [=] { (void) i; return [=] pre (i > 0) {}; };	// expected-error {{must be captured explicitly}}
  auto p5 = [=] (auto a) pre (i > a) {};	// expected-error {{must be captured explicitly}}
  auto p6 = [=] (const auto a) post (i > a) {};	// expected-error {{must be captured explicitly}}
  p1 (1); p2 (); p3 (); p4 (); p5 (0); p6 (0);
}

template <class T>
void tg (T i)
{
  auto q1 = [=] pre (i > 0) {};	// expected-error {{must be captured explicitly}}
  auto q2 = [=] (auto a) pre (i > a) {};	// expected-error {{must be captured explicitly}}
  q1 (); q2 (0);
}
template void tg (int);

void nested (int j)
{
  auto r = [=] (auto a) { return [=] (auto b) pre (b > j) { return a + b; }; };	// expected-error {{must be captured explicitly}}
  r (0) (1);
}
