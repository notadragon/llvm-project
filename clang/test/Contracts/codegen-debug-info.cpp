// RUN: %clangxx -fcontracts -fcontract-evaluation-semantic=enforce -g -std=c++26 -c  %s -o /dev/null
// RUN: %clangxx -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe -g -std=c++26 -c -DTHROWS %s -o /dev/null

// Code generation with debug information for contract checks: a free
// function's postcondition, and a constructor's and a destructor's
// preconditions and postconditions.  It must compile.
//
// With -DTHROWS, the checks whose predicate or P3098 capture may throw: their
// handlers are emitted as IR with no statement of their own, so no lexical
// scope is opened for them (CLANG-129: the debug-information guards that
// tolerated a missing scope are gone, and an assertions build would assert
// on one).

int foo(const int x) post(x != 0) { return x; }


struct S {
  S(const int x) pre(x != 0) post(x != 1) : x(x) {}
  ~S() pre(x != 0) post(true) {}
  int x;
};

#ifdef THROWS
bool may_throw(int);
struct C { C(); C(const C &); };

void pred(const int x) pre(may_throw(x)) post(may_throw(x)) {
  contract_assert(may_throw(x));
}
void capture(C c) post [d = c] (true) {}
auto lambda = [](int x) pre(may_throw(x)) { return [x] { return x; }(); };
template <class T> void tmpl(const T x) pre(may_throw(x)) post [y = x] (y == x) {}
template void tmpl<int>(int);
#endif
