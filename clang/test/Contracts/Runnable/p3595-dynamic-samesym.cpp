// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-samesym.json %libcxx_flags -o %t && %t

// P3595 dynamic selection: two DISTINCT config entries name the SAME emitted
// selector symbol but spell it differently --
//   entry A: linkage "C++", name "mylib::sel"        -> mangles to _ZN5mylib3selEv
//   entry B: linkage "C",   name "_ZN5mylib3selEv"   -> used verbatim
// Both request provideweak (default true).  Each entry matches a different
// contract in this one TU (keyed on groups "a" and "b").
//
// getOrCreateDynamicSelector dedups on the FINAL emitted symbol
// (CGM.getModule().getFunction(Symbol)), not on the config name string, so it
// must emit at most ONE weak definition of _ZN5mylib3selEv.  If it instead
// keyed on the name string it would emit two definitions of the same symbol and
// the program would fail to link (duplicate definition).  This test links and
// runs => single weak def.  Nothing else in the TU defines the symbol, so both
// contracts use the weak default ("observe") and each failing precondition is
// handled and continues.

#include <contracts>
#include <cstdlib>

static int violations = 0;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

// Matched by entry A (group "a", C++ name mylib::sel).
void fa(const int x) pre [[clang::contract_group("a")]] (x > 0) {}

// Matched by entry B (group "b", C name _ZN5mylib3selEv == same symbol).
void fb(const int x) pre [[clang::contract_group("b")]] (x > 0) {}

int main() {
  fa(-1);
  fb(-1);
  if (violations != 2)
    std::abort();
  fa(1);
  fb(1);
  if (violations != 2)
    std::abort();
}
