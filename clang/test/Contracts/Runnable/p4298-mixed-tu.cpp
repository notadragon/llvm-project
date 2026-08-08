// D4298: the P3290 nothrow_t overloads report the noexcept_* semantics based
// on the CALLING translation unit's -fcontracts-p4298 flag, and this works
// when TUs built with and without the flag are linked into one program
// against one libc++.  The flag TU binds the std::contracts::__p4298 symbol
// (reporting noexcept_observe); the non-flag TU binds the plain
// std::contracts symbol (reporting observe); each mangled name has exactly
// one definition, so the mixed-flag link is well-formed and each TU observes
// its own semantic.
//
// (GCC has no in-tree equivalent: DejaGnu dg-additional-sources compiles all
// sources with the same options, so a per-TU-flag test is not expressible
// there; the GCC mechanism is verified by its two single-flag nothrow tests
// plus manual mixed-link verification.)
//
// RUN: rm -rf %t && mkdir -p %t
// RUN: split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3290 -fcontracts-p4298 %libcxx_flags -c %t/p4298_tu.cpp -o %t/p4298_tu.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3290 %libcxx_flags -c %t/classic_tu.cpp -o %t/classic_tu.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3290 %libcxx_flags -c %t/main.cpp -o %t/main.o
// RUN: %clangxx -fcontracts %libcxx_flags %t/main.o %t/p4298_tu.o %t/classic_tu.o -o %t/a.out
// RUN: %t/a.out

//--- p4298_tu.cpp
// Compiled WITH -fcontracts-p4298 -> binds the __p4298 nothrow symbol.
#include <contracts>
void do_p4298() {
  std::contracts::handle_observed_contract_violation(std::nothrow, "p4298 TU");
}

//--- classic_tu.cpp
// Compiled WITHOUT -fcontracts-p4298 -> binds the plain nothrow symbol.
#include <contracts>
void do_classic() {
  std::contracts::handle_observed_contract_violation(std::nothrow, "classic TU");
}

//--- main.cpp
#include <contracts>
using namespace std::contracts;

static evaluation_semantic g_last = evaluation_semantic::unspecified;
void handle_contract_violation(const contract_violation& v) { g_last = v.semantic(); }

void do_p4298();
void do_classic();

int main() {
  do_p4298();
  if (g_last != evaluation_semantic::noexcept_observe)
    __builtin_trap();   // the p4298 TU must report noexcept_observe
  do_classic();
  if (g_last != evaluation_semantic::observe)
    __builtin_trap();   // the non-p4298 TU must report classic observe
}
