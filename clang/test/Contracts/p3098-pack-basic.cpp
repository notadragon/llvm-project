// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s
// expected-no-diagnostics

template<typename... Args>
int sum(Args... args) post [args...] (true) { return (args + ...); }

template int sum<int, int>(int, int);
template int sum<int, int, int>(int, int, int);
