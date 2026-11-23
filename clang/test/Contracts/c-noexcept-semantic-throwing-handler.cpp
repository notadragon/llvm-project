// RUN: rm -rf %t && split-file %s %t
// RUN: %clang -std=c23 -fexceptions -fcontracts-p4299 -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_observe %libcxx_flags -c %t/f.c -o %t/f.o
// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags %t/main.cpp %t/f.o -o %t/a
// RUN: not --crash %t/a

// A noexcept semantic in C terminates when the violation handler throws:
// __dispatch_with_override_core, which C code generation reports it through,
// dispatches through the C++ runtime's terminate-on-throw core.
//
// No GCC mirror: a mixed C/C++ link does not fit a DejaGnu test.

//--- f.c
int f(int x) _Pre(x > 0) { return x; }

//--- main.cpp
#include <contracts>
extern "C" int f(int);
void handle_contract_violation(const std::contracts::contract_violation &) {
  throw 1;
}
int main() {
  try {
    f(0);
  } catch (int) {
    return 0; // the exception escaped a noexcept semantic
  }
  return 0;
}

// REQUIRES: contracts-libcxx, native
