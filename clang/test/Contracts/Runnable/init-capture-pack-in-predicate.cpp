// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 %libcxx_flags -o %t && %t

// An init-capture pack `post [...c = t]` is itself a pack, as a lambda's is:
// it can be expanded in the predicate -- in a call, a fold and sizeof... --
// and instantiation gives each element its own capture.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/init-capture-pack-in-predicate.C

template <class... A> int sum(A... a) { return (0 + ... + a); }

template <class... T>
int f(T... t) post [...c = t] (r : sum(c...) == 3) { return 0; }

template <class... T>
int g(T... t) post [...c = t] (r : ((c > 0) && ...)) { return 0; }

int main() {
  f(1, 2);
  g(1, 2);
}
