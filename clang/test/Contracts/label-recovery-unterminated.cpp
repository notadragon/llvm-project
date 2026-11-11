// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify=expected,noflag %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s

// Without -fcontracts-p3400 an assertion-control label is reported as
// needing the flag before anything wrong inside it: a label with no '>' was
// reported only as the missing '>' (CLANG-648), at namespace scope and in a
// class, where the contract is cached to be parsed later.  With the flag or
// without it, recovery stops at the end of the declaration, so later
// declarations are still diagnosed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/label-recovery-unterminated.C
// (without the flag; GCC-665) and contract-label-malformed-recovery.C (with
// it; GCC-662) in the gnu_gcc fork, whose CLANG-647 row asked for them.

void j(int x) pre<1 x; // noflag-error {{assertion-control labels require '-fcontracts-p3400'}} expected-error {{expected '>'}}
struct A { int a; };
int after = undeclared; // expected-error {{use of undeclared identifier 'undeclared'}}

struct B {
  void k(int x) pre<1 x; // noflag-error {{assertion-control labels require '-fcontracts-p3400'}} expected-error {{expected '>'}}
  int b;
};
int after2 = undeclared2; // expected-error {{use of undeclared identifier 'undeclared2'}}

// A well-formed label is reported once without the flag, in a class too.
struct L { using assertion_control_object = L; };
constexpr L l{};
struct C {
  void m(int x) pre<l>(x > 0); // noflag-error {{assertion-control labels require '-fcontracts-p3400'}}
  void n(int x) pre<l>(x > 0) {} // noflag-error {{assertion-control labels require '-fcontracts-p3400'}}
};
