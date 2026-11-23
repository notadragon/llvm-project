// RUN: %clangxx %s -fsanitize=address -ffile-prefix-map=%S=/mapped %libcxx_flags -o %t
// RUN: env ASAN_OPTIONS=check_initialization_order=true %t

// CLANG-881: -ffile-prefix-map remaps the module name of a translation
// unit's globals and the one it passes to __asan_before_dynamic_init alike,
// so its own dynamic initialization is not reported as an
// initialization-order fiasco.
//
// Mirror: gcc/testsuite/g++.dg/asan/init-order-file-prefix-map.C

struct S {
  S() { asm volatile("" : : : "memory"); }
};
static S c;
static int a = 0;
static int b = a;

int main() { return b; }
