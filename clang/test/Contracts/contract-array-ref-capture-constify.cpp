// RUN: %clang_cc1 -std=c++26 -fcontracts -Wno-vla-cxx-extension -fsyntax-only -verify %s

// An array declared outside a contract predicate is const in it
// ([expr.prim.id.unqual]), and so is the array a lambda in the predicate
// captures by reference: of constant size or of runtime bound (a GNU
// extension), captured implicitly ([&]) or explicitly ([&a]), it can be
// neither modified nor passed to a pointer to non-const.  The same holds in a
// template, and for an array the predicate's lambda reaches through a capture
// of an enclosing lambda (contract-enclosing-lambda-capture-constify.cpp
// covers that for every kind of entity).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-array-ref-capture-constify.C

int take(int *); // expected-note 1+ {{candidate function not viable}}
int look(const int *);

void fixed() {
  int a[3] = {};
  contract_assert([&] { return ++a[0] == 0; }());  // expected-error {{cannot assign to variable 'a'}}
  contract_assert([&a] { return ++a[0] == 0; }()); // expected-error {{cannot assign to variable 'a'}}
  contract_assert([&] { return take(a); }());      // expected-error {{no matching function for call to 'take'}}
  contract_assert([&a] { return take(a); }());     // expected-error {{no matching function for call to 'take'}}
  contract_assert([&] { return look(a); }());
  contract_assert([&a] { return look(a) + a[1]; }());
}

void vla(int n) {
  int a[n];
  a[0] = 0;
  contract_assert([&] { return ++a[0] == 0; }());  // expected-error {{cannot assign to variable 'a'}}
  contract_assert([&a] { return ++a[0] == 0; }()); // expected-error {{cannot assign to variable 'a'}}
  contract_assert([&] { return take(a); }());      // expected-error {{no matching function for call to 'take'}}
  contract_assert([&a] { return take(a); }());     // expected-error {{no matching function for call to 'take'}}
  contract_assert([&] { return take(&a[0]); }());  // expected-error {{no matching function for call to 'take'}}
  contract_assert([&] { int *p = a; return p != 0; }()); // expected-error {{cannot initialize a variable of type 'int *' with an lvalue of type 'const int[n]'}}
  contract_assert([&] { a[1] = 1; return true; }()); // expected-error {{cannot assign to variable 'a'}}
  contract_assert([&] { return look(a) + a[0]; }());
  contract_assert([&a] { return look(a) + a[0]; }());
  contract_assert([&] { return [&] { return ++a[0]; }(); }()); // expected-error {{cannot assign to variable 'a'}}
}

// A lambda in the predicate whose by-copy capture is the array's copy, or
// that declares an array itself, may modify it.
void inside() {
  int a[3] = {};
  contract_assert([=]() mutable { return ++a[0] == 1; }());
  contract_assert([] { int b[3] = {}; return [&] { return ++b[0]; }(); }());
}

void inside_vla(int n) {
  contract_assert([n] {
    int b[n];
    return [&] { b[0] = 1; return take(b); }();
  }());
}

template <class T> void tfixed() {
  T a[3] = {};
  contract_assert([&] { return ++a[0] == 0; }()); // expected-error {{cannot assign to variable 'a'}} \
                                                  // expected-note {{while substituting into a lambda expression here}}
  contract_assert([&] { return take(a); }());     // expected-error {{no matching function for call to 'take'}} \
                                                  // expected-note {{while substituting into a lambda expression here}}
}
template void tfixed<int>(); // expected-note 2 {{in instantiation of}}

template <class T> void tvla(int n) {
  T a[n];
  contract_assert([&] { return ++a[0] == 0; }()); // expected-error {{cannot assign to variable 'a'}} \
                                                  // expected-note {{while substituting into a lambda expression here}}
  contract_assert([&] { return take(a); }());     // expected-error {{no matching function for call to 'take'}} \
                                                  // expected-note {{while substituting into a lambda expression here}}
}
template void tvla<int>(int); // expected-note 2 {{in instantiation of}}

template <class T> void tvla_nondep(int n) {
  int a[n];
  contract_assert([&] { return ++a[0] == 0; }());  // expected-error {{cannot assign to variable 'a'}}
  contract_assert([&a] { return take(a); }());     // expected-error {{no matching function for call to 'take'}}
}
template void tvla_nondep<int>(int);
