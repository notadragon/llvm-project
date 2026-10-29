// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-shadow-caller-side.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3: a caller-side evaluation of a precondition of a
// specialization of a template with a definition is proven in the caller's
// proof shadow, with the call's arguments for the specialization's
// parameters, as for any other function.  A function template, a member
// template, a member function of a class template and a pack; each proven
// where the caller tests the argument and diagnosed where it does not.  The
// definitions are not diagnosed: their own evaluations are ignore.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-template-caller.C
// (gnu_gcc, P5b-GCC).

extern "C" int puts(const char *);

template <class P>
void log_message(P p)
  pre(p)                        // expected-note {{the predicate of the contract assertion}}
{
  puts(p);
}

void output_string(const char *ptr) {
  if (ptr != nullptr)
    log_message(ptr);           // proven
}

void output_string_unchecked(const char *ptr) {
  log_message(ptr);             // expected-error {{contract assertion cannot be proven}}
}

struct M {
  int n;
  template <class U>
  int at(U i) const pre(i < n) { return i; } // expected-note {{the predicate of the contract assertion}}
};

int use_member(const M &m, long i) {
  if (i < m.n)
    return m.at(i);             // proven
  return m.at(i);               // expected-error {{contract assertion cannot be proven}}
}

template <class T>
struct C {
  T get(T i) pre(i > 0) { return i; } // expected-note {{the predicate of the contract assertion}}
};

int use_class(C<int> *c, int i) {
  if (i > 0)
    return c->get(i);           // proven
  return c->get(i);             // expected-error {{contract assertion cannot be proven}}
}

template <class... T>
int all(T... t) pre(((t > 0) && ...)) { return 0; } // expected-note {{the predicate of the contract assertion}}

int use_pack(int a, int b) {
  if (a > 0 && b > 0)
    return all(a, b);           // proven
  return all(a, b);             // expected-error {{contract assertion cannot be proven}}
}
