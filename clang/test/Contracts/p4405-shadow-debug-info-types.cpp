// DEFINE: %{O} = -O0
// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -debug-info-kind=constructor -dwarf-version=5 -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven %{O} -emit-obj
// DEFINE: %{check} = %{cc1} -fcontracts-prove-optimize=0 -o %t.ref.o %s && \
// DEFINE:   %{cc1} -o %t.shadow.o %s && cmp %t.ref.o %t.shadow.o && \
// DEFINE:   %{cc1} -fno-contracts-prove-shadow -o %t.noshadow.o %s && cmp %t.ref.o %t.noshadow.o
// RUN: %{check}
// REDEFINE: %{O} = -Og
// RUN: %{check}
// REDEFINE: %{O} = -O1
// RUN: %{check}
// REDEFINE: %{O} = -O2
// RUN: %{check}
// REDEFINE: %{O} = -O3
// RUN: %{check}
// REQUIRES: x86-registered-target

// D4405 proof shadows and -g: what a predicate names is not described in
// the real module's debug information.  The shadow phase -- the shadows,
// and the functions only they call -- is emitted without the module's
// debug information (CLANG-915), so neither an explicit conversion's type
// (f15's U, f16's W<int>), nor a constructor a predicate alone calls,
// which would complete its class's description (f10's T), nor a lambda's
// call operator, which would describe its closure type in f11's scope, nor
// the __clang_vtable variable of an external vtable a shadow loads (V,
// whose key function is elsewhere) is described.  The object code, the
// debug sections included, is the same with the shadows, without them and
// at proof level 0, at -O0, -Og, -O1 (proofs at level 2), -O2 and -O3.
// Under -fcontracts-prove-assume the assumption regions still leave these
// descriptions: OpenBugs/p4405-shadow-debug-info-assume.cpp (CLANG-916).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-p4405-shadow-debug-info-residue.C
// (f10, f11)

struct T { int n; T(int n) : n(n) {} ~T(); bool ok() const { return n > 0; } };
struct U { int n; U(int n); bool ok() const { return n > 0; } };
template <class X> struct W { X x; W(X x) : x(x) {} bool ok() const { return x > 0; } };

int f10(int n) { contract_assert(T(n).ok()); return 10; }
int f11(int n) { contract_assert([&] { return n > 0; }()); return 11; }
int f15(int n) { contract_assert(U(n).ok()); return 15; }
int f16(int n) { if (n > 1) contract_assert(W<int>(n).ok()); return 16; }

struct V { virtual int f(int); virtual bool ok(int n) const { return n > 0; } };
V *make() { return new V; }
int use(V *v, int n) { if (n > 1) contract_assert(v->V::ok(n)); return v->f(n); }
