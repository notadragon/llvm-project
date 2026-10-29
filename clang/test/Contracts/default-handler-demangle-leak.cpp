// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe \
// RUN:   -fsanitize=address %libcxx_flags -o %t && %t

// The default contract-violation handler does not leak when it reports an
// escaping exception under observe: LeakSanitizer finds nothing at exit.
//
// Mirror: gcc/testsuite/g++.dg/asan/default-handler-demangle-leak.C

extern "C" const char *__asan_default_options() { return "detect_leaks=1"; }

struct some_exception {};
bool boom(int) { throw some_exception{}; }
int f(int x) pre(boom(x)) { return x; }

int main() {
  for (int i = 0; i < 3; ++i)
    f(i);
  return 0;
}

// REQUIRES: contracts-libcxx, native
