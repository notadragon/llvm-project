// D4298: during constant evaluation there is no throw/terminate distinction,
// so noexcept_enforce behaves exactly like enforce (a violation is a hard
// error, making the expression non-constant) and noexcept_observe behaves
// exactly like observe (a violation is a warning; the expression stays
// constant).
// (GCC mirror: g++.dg/contracts/cpp26/p4298-constexpr-{terminating,observe}.C)
//
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_enforce -fsyntax-only -verify=enforce %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_observe -fsyntax-only -verify=observe %s

constexpr int f(int x) pre(x > 0) { return x; }
// enforce-error@-1 {{contract failed during execution of constexpr function}}
// observe-warning@-2 {{contract failed during execution of constexpr function}}

// noexcept_enforce: the violation makes this a non-constant expression.
// noexcept_observe: warning only (above); the initialization still succeeds.
constexpr int bad = f(-1);
// enforce-error@-1 {{constexpr variable 'bad' must be initialized by a constant expression}}
// enforce-note@-2 {{in call to 'f(-1)'}}

int main() { return 0; }
