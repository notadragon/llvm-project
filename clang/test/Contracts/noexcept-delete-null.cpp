// RUN: %clang_cc1 -std=c++11 -fcxx-exceptions -Wno-unevaluated-expression -fsyntax-only -verify %s
// expected-no-diagnostics

// A delete-expression invokes the destructor and the deallocation function,
// so it is potentially-throwing when the destructor is ([except.spec]/6),
// whatever the value of its operand: a null pointer constant operand does
// not make it non-throwing.  (Stock GCC says noexcept for that, GCC-683 /
// PR65174.)  It bears on contracts because a predicate that can throw is
// evaluated under the evaluation_exception handling
// (predicate-throw-delete.cpp).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-noexcept-delete-null.C

struct D { ~D() noexcept(false); };
D *p;
constexpr D *np = nullptr;

static_assert(!noexcept(delete p), "");
static_assert(!noexcept(delete np), "");
static_assert(!noexcept(delete (D *)0), "");
static_assert(!noexcept(delete static_cast<D *>(nullptr)), "");
static_assert(!noexcept(delete[] (D *)0), "");
