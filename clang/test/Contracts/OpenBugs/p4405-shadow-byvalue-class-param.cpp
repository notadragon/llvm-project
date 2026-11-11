// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Xclang -verify %s
// REQUIRES: contracts-libcxx
// XFAIL: *

// WATCH (CLANG-917 in this fork's open-issues/).  P4405: a precondition that
// reads a member of a by-value class-type parameter is never proven at a
// call, even for a constant argument: the caller-side obligation is not
// deferred to the proof shadow, which gives only a scalar or reference
// parameter the call's argument.  The same precondition on a const T &
// parameter, or on an int parameter, is proven.  When f1 and f2 are proven,
// this XFAIL becomes an XPASS.

#include <contracts>
using namespace std::contracts::labels;

struct T { int c; };
int byval(T t) pre<always_prove>(t.c == 1);

int f1() { T a = {1}; return byval(a); } // OK
int f2() { return byval(T{1}); }         // OK

// expected-no-diagnostics
