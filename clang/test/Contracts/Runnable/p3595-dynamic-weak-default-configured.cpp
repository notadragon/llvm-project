// RUN: %clangxx -std=c++26 %s -fcontracts \
// RUN:   -fcontract-configuration-file=%S/p3595-dynamic-weak-default-configured.json \
// RUN:   %libcxx_flags -o %t 2> %t.err
// RUN: FileCheck %s < %t.err
// RUN: %t

// P3595: the weak default of a dynamic selector returns the semantic the
// configuration specifies (D3595R1), not that semantic adjusted to the
// semantics the flags allow: the check adjusts the value it gets.  So the
// weak definition is the same whatever the flags of the translation unit
// that emits it, the one the linker keeps.  Without -fcontracts-p4298 the
// configured noexcept_observe is evaluated as observe (with a warning):
// the handler is called and the program continues, while the weak default
// returns noexcept_observe (CLANG-837 returned observe).  The C form is
// C/c-dynamic-weak-default-configured.c, which shares this configuration.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-weak-default-configured.C

// CHECK: warning: 'noexcept_observe' in contract configuration requires '-fcontracts-p4298'; 'observe' is used instead
#include <contracts>

using std::contracts::evaluation_semantic;

extern "C" evaluation_semantic configured_weak_sel();

static int handled;
static evaluation_semantic seen;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++handled;
  seen = v.semantic();
}

void f(int x) pre(x > 0) {}

int main() {
  if (configured_weak_sel() != evaluation_semantic::noexcept_observe)
    __builtin_abort();
  f(-1);
  if (handled != 1 || seen != evaluation_semantic::observe)
    __builtin_abort();
}
