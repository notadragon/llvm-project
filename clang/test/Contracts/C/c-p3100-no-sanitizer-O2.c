// RUN: %clang -std=c23 -fcontracts-p4299 -fcontracts-p3100 -O2 -c %s -o %t.o

// GCC-965 (the gnu_gcc fork): -fcontracts-p3100 without a -fsanitize= option
// compiles C at -O2.  GCC's ubsan pass, which runs for P3100 alone,
// declared the sanitizer builtins after the C front end's file scope had
// gone and crashed there.  Clang does not have the bug (measured
// 2026-11-04); these tests guard it under the bug-fix mirror rule.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p3100-no-sanitizer-O2.c

void f(void) {}
int g(int a, int b) { return a / b + a * b; }
int h(int *p) { return *p; }
