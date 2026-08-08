// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -fsyntax-only -fcontract-evaluation-semantic=enforce -fcontracts-group-evaluation-semantic=safety:observe
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -fsyntax-only -fcontract-evaluation-semantic=enforce -fcontracts-group-evaluation-semantic=safety:observe -fcontracts-group-evaluation-semantic=safety.memory:ignore

// Label groups feed into P3595 config resolution.
// Group-specific -fcontracts-group-evaluation-semantic flags match labels.

#include <contracts>
using namespace std::contracts;
using namespace std::contracts::labels;

// With -fcontracts-group-evaluation-semantic=safety:observe,
// contracts in group "safety" get observe semantic.
void f(int x) pre<"safety"group>(x > 0) {}

// Hierarchical: "safety.memory" also matches the "safety"group config.
void g(int x) pre<"safety.memory"group>(x > 0) {}

// Unrelated group: "performance" falls through to default (enforce).
void h(int x) pre<"performance"group>(x > 0) {}

// No group: falls through to default (enforce).
void i(int x) pre(x > 0) {}

// Combined labels: group concatenation
void j(int x) pre<"safety"group | "performance"group>(x > 0) {}
