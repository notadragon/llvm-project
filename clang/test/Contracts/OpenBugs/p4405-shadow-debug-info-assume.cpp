// DEFINE: %{O} = -O0
// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -debug-info-kind=constructor -dwarf-version=5 -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven %{O} -emit-obj
// DEFINE: %{check} = %{cc1} -fcontracts-prove-optimize=0 -o %t.ref.o %s && \
// DEFINE:   %{cc1} -o %t.assume.o %s && cmp %t.ref.o %t.assume.o
// RUN: %{check}
// REDEFINE: %{O} = -O2
// RUN: %{check}
// REQUIRES: x86-registered-target
// XFAIL: *

// OPEN BUG (CLANG-916 in this fork's open-issues/; deferred, DECISIONS.md
// E211 (c): a documented limitation).
//
// D4405 under -fcontracts-prove-assume (on by default): the debug
// information describes what only an unproven obligation's predicate
// names.  The real function holds an assumption region for each
// obligation, emitted with the real code -- with its debug information --
// and deleted by the back end when the obligation is not proven soundly.
// So an explicit conversion's type is retained (f10's T, f12's W<int>), a
// constructor the predicate alone calls is emitted and completes its
// class's description (T), and a lambda's call operator describes its
// closure type in f11's scope, though no code of theirs is left.  None of
// these obligations is proven, so no assumption is kept; once fixed, the
// object code, the debug sections included, is the same as at proof level
// 0, where no region is placed, at -O0 as at -O2 (as the code is).

struct T { int n; T(int n) : n(n) {} ~T(); bool ok() const { return n > 0; } };
template <class X> struct W { X x; W(X x) : x(x) {} bool ok() const { return x > 0; } };

int f10(int n) { contract_assert(T(n).ok()); return 10; }
int f11(int n) { contract_assert([&] { return n > 0; }()); return 11; }
int f12(int n) { if (n > 1) contract_assert(W<int>(n).ok()); return 12; }
