// RUN: %clang_cc1 -std=c++26 -fcontracts -Wno-vla-cxx-extension -fsyntax-only -verify %s

// A contract assertion in the body of a lambda: an entity that lambda
// captured is declared outside the contract assertion, so it is const in the
// predicate ([expr.prim.id.unqual]) -- also when a lambda in the predicate
// captures it by reference in turn, whether the enclosing lambda captured it
// by reference or by copy (a mutable one may modify its own copy outside the
// contract assertion), implicitly, explicitly or with an init-capture, and
// whatever the entity is (a scalar, an array, a VLA).  A by-copy capture made
// by a lambda that is itself in the predicate still cuts the chain.
//
// The copy of a mutable lambda used to be diagnosed as "captured by copy in
// a non-mutable lambda": the contract scope opened in that lambda's body was
// dropped along with the scopes outside its copy.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-enclosing-lambda-capture-constify.C

int take(int *); // expected-note 1+ {{candidate function not viable}}

void by_ref(int x) {
  auto l1 = [&] { (void)x; contract_assert([&] { return ++x; }()); }; // expected-error {{cannot assign to a variable captured by reference which was captured as const because it is inside a contract}}
  auto l2 = [&] { contract_assert([&] { return ++x; }()); (void)x; }; // expected-error {{cannot assign to a variable captured by reference which was captured as const because it is inside a contract}}
  auto l3 = [&x] { contract_assert([&] { return ++x; }()); };         // expected-error {{cannot assign to a variable captured by reference which was captured as const because it is inside a contract}}
  auto l4 = [&r = x] { contract_assert([&r] { return ++r; }()); };    // expected-error {{cannot assign to variable 'r' because it is considered 'const' inside of a contract}}
  auto l5 = [&] { (void)x; contract_assert([&] { return [&] { return ++x; }(); }()); }; // expected-error {{cannot assign to a variable captured by reference which was captured as const because it is inside a contract}}
  l1(); l2(); l3(); l4(); l5();
}

void by_copy(int x) {
  auto l1 = [=]() mutable { ++x; contract_assert([&] { return ++x; }()); };       // expected-error {{cannot assign to a variable captured by reference which was captured as const because it is inside a contract}}
  auto l2 = [y = x]() mutable { ++y; contract_assert([&] { return ++y; }()); };   // expected-error {{cannot assign to a variable captured by copy which was captured as const because it is inside a contract}}
  auto l3 = [x]() mutable { ++x; contract_assert([&x] { return ++x; }()); };      // expected-error {{cannot assign to a variable captured by reference which was captured as const because it is inside a contract}}
  // The predicate's own copy is its own.
  auto l4 = [=]() mutable { ++x; contract_assert([=]() mutable { return [&] { return ++x; }(); }()); };
  l1(); l2(); l3(); l4();
}

// Not in GCC's test: a copy reached through two lambdas in the predicate, and
// a precondition of the copying lambda itself.
void by_copy_more(int x) {
  auto l1 = [x]() mutable { contract_assert([&] { return [&] { return ++x; }(); }()); }; // expected-error {{cannot assign to a variable captured by reference which was captured as const because it is inside a contract}}
  auto l2 = [x]() mutable pre([&] { return ++x; }()) {};                                 // expected-error {{cannot assign to a variable captured by reference which was captured as const because it is inside a contract}}
  l1(); l2();
}

void arrays(int n) {
  int a[3] = {};
  int v[n];
  auto l1 = [&] { (void)a; contract_assert([&] { return ++a[0]; }()); };         // expected-error {{cannot assign to variable 'a'}}
  auto l2 = [&] { (void)v; contract_assert([&] { return ++v[0]; }()); };         // expected-error {{cannot assign to variable 'v'}}
  auto l3 = [&] { (void)v; contract_assert([&] { return take(v); }()); };        // expected-error {{no matching function for call to 'take'}}
  auto l4 = [&v] { contract_assert([&v] { return take(v); }()); };               // expected-error {{no matching function for call to 'take'}}
  auto l5 = [=]() mutable { ++a[0]; contract_assert([&] { return take(a); }()); }; // expected-error {{no matching function for call to 'take'}}
  l1(); l2(); l3(); l4(); l5();
}

template <class T> void tby_ref(T x) {
  auto l1 = [&] { (void)x; contract_assert([&] { return ++x; }()); }; // expected-error {{cannot assign to a variable captured by reference which was captured as const because it is inside a contract}} \
    // expected-note 2 {{while substituting into a lambda expression here}}
  auto l2 = [=]() mutable { ++x; contract_assert([&] { return ++x; }()); }; // expected-error {{cannot assign to a variable captured by reference which was captured as const because it is inside a contract}} \
    // expected-note 2 {{while substituting into a lambda expression here}}
  l1(); l2();
}
template void tby_ref<int>(int); // expected-note 2 {{in instantiation of}}
