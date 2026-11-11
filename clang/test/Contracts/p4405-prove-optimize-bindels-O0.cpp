// DEFINE: %{cxx} = %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -S -emit-llvm -o %t.ll
// RUN: %{cxx} -O0 -Xclang -verify=on %s
// RUN: %{cxx} -O0 -fcontract-evaluation-semantic=observe -Xclang -verify=on %s
// RUN: %{cxx} -O0 -fcontract-evaluation-semantic=observe -fno-contracts-prove-callee-premises -Xclang -verify=nopremise %s
// RUN: %{cxx} -O0 -fcontracts-prove-optimize=follow -Xclang -verify=noshadow,nopremise %s
// RUN: %{cxx} -O0 -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-callee-premises -Xclang -verify=noshadow,nopremise %s
// RUN: %{cxx} -O2 -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-callee-premises -Xclang -verify=noshadow,nopremise %s
// RUN: %{cxx} -O0 -fcontracts-prove-optimize=follow -fcontracts-prove-shadow -fcontracts-prove-callee-premises -Xclang -verify=noshadow,nopremise %s
// REQUIRES: contracts-libcxx

// D4405R0's Peter Bindels `expected` example (p4405-prove-optimize-bindels.cpp)
// in -O0 builds (notadragon_wg21 DECISIONS.md E162-E168, E166): at the
// default proof level, 2, the proof shadows, optimized at that level, prove
// both calls as an -O2 build does, under observe too, where the call of
// value() needs index()'s premise copy.  follow at -O0 is level 0: folding
// only.  A shadow given explicitly at level 0 runs at level 0, as at -O0
// (DECISIONS.md E208 (a)), and proves neither call, in an -O0 or an -O2
// build.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove-optimize-bindels-O0.C,
// -bindels-enforce-O0.C, -bindels-0-explicit.C

#include <contracts>
using namespace std::contracts::labels;

struct Expected {
  int d_value;
  int d_error;
  int d_index;

  int index() const post(r : r == 0 || r == 1) { return d_index; }
  bool is_error() const { return index() == 1; }
  int error() const pre<prove>(index() == 1) { return d_error; } // noshadow-note {{predicate}}
  int value() const pre<prove>(index() == 0) { return d_value; } // nopremise-note {{predicate}}
};

Expected get12thPrime();
void use(int);

void g()
{
  Expected p = get12thPrime();
  if (p.is_error()) {
    use(p.error());  // noshadow-error {{cannot be proven}}
  }
  else {
    use(p.value());  // nopremise-error {{cannot be proven}}
  }
}

// on-no-diagnostics
