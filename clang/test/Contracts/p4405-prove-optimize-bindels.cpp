// DEFINE: %{cxx} = %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -S -emit-llvm -o %t.ll
// RUN: %{cxx} -Oz -Xclang -verify=on %s
// RUN: %{cxx} -Oz -fcontracts-prove-optimize=follow -Xclang -verify=on %s
// RUN: %{cxx} -Oz -fcontract-evaluation-semantic=observe -Xclang -verify=on %s
// RUN: %{cxx} -Os -Xclang -verify=on %s
// RUN: %{cxx} -Oz "-DATTR=[[clang::minsize]]" -Xclang -verify=minsize %s
// RUN: %{cxx} -O2 -fcontract-evaluation-semantic=observe -fcontracts-prove-optimize=1 -Xclang -verify=nopremise,l1 %s
// RUN: %{cxx} -O2 -fcontract-evaluation-semantic=observe -fcontracts-prove-optimize=1 -fcontracts-prove-callee-premises -Xclang -verify=on %s
// RUN: %{cxx} -O2 -fcontract-evaluation-semantic=observe -fcontracts-prove-optimize=0 -fcontracts-prove-callee-premises -Xclang -verify=noshadow,nopremise %s
// RUN: %{cxx} -O1 -fcontract-evaluation-semantic=observe -fcontracts-prove-optimize=2 -Xclang -verify=on %s
// RUN: %{cxx} -O1 -fcontract-evaluation-semantic=observe -fcontracts-prove-optimize=follow -Xclang -verify=nopremise,l1 %s
// RUN: %{cxx} -O1 -fcontract-evaluation-semantic=observe -Xclang -verify=on %s
// RUN: %{cxx} -Og -fcontract-evaluation-semantic=observe -Xclang -verify=on %s
// RUN: %{cxx} -Os -fcontract-evaluation-semantic=observe -Xclang -verify=on %s
// RUN: %{cxx} -O1 -fcontract-evaluation-semantic=observe -fno-contracts-prove-callee-premises -Xclang -verify=nopremise %s
// RUN: %{cxx} -Og -fcontract-evaluation-semantic=observe -fno-contracts-prove-callee-premises -Xclang -verify=nopremise %s
// RUN: %{cxx} -Os -fcontract-evaluation-semantic=enforce -Xclang -verify=on %s
// RUN: %{cxx} -Oz -fcontract-evaluation-semantic=enforce -Xclang -verify=on %s
// RUN: %{cxx} -O1 -fcontract-evaluation-semantic=observe -fcontracts-prove-optimize=1 -Xclang -verify=nopremise,l1 %s
// RUN: %{cxx} -Og -fcontract-evaluation-semantic=observe -fcontracts-prove-optimize=1 -Xclang -verify=nopremise,l1 %s
// RUN: %{cxx} -Os -fcontract-evaluation-semantic=observe -fcontracts-prove-optimize=1 -Xclang -verify=nopremise,l1 %s
// RUN: %{cxx} -O1 -fcontracts-prove-optimize=1 -Xclang -verify=on %s
// REQUIRES: contracts-libcxx

// D4405R0's Peter Bindels `expected` example (p4405-shadow-callee-premises-
// bindels.cpp) at proof optimization levels (notadragon_wg21 DECISIONS.md
// E162-E168, E205).  -Oz's minsize, which the -O level alone implies, is not
// the proof level's, so at -Oz the example is proven as at -O2 (follow at
// -Oz is level 2 too); g's own minsize is kept, and its calls are not
// proven.  Under observe the call of value() is proven only through
// index()'s premise copy (-fcontracts-prove-callee-premises), on from level
// 2, and needing the proof shadow, which level 0 turns off; one given
// explicitly wins.  At level 1 the call of error() is not proven either
// without the copies: the -O1 pipeline does not merge the loads of d_index
// across the observed postcondition's check of the inlined index() (the
// handler may write memory), which a premise copy assumes instead.  So
// at -O1, -Og and -Os too the default level, 2, proves both calls under
// observe (value()'s only with the copies) and under enforce at -Os and
// -Oz, and level 1 loses both under observe at every -O level.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove-optimize-premises-0-explicit.C,
// -premises-1.C, -premises-1-explicit.C, -premises-2.C, -premises-follow-O1.C,
// -bindels-O1.C, -bindels-O1-no-premises.C, -bindels-Og.C, -bindels-Os.C,
// -bindels-Oz.C, -bindels-enforce-Os.C, -bindels-enforce-Oz.C

#include <contracts>
using namespace std::contracts::labels;

#ifndef ATTR
#define ATTR
#endif

struct Expected {
  int d_value;
  int d_error;
  int d_index;

  int index() const post(r : r == 0 || r == 1) { return d_index; }
  bool is_error() const { return index() == 1; }
  int error() const pre<prove>(index() == 1) { return d_error; } // noshadow-note {{predicate}} minsize-note {{predicate}} l1-note {{predicate}}
  int value() const pre<prove>(index() == 0) { return d_value; } // nopremise-note {{predicate}} minsize-note {{predicate}}
};

Expected get12thPrime();
void use(int);

ATTR void g()
{
  Expected p = get12thPrime();
  if (p.is_error()) {
    use(p.error());  // noshadow-error {{cannot be proven}} minsize-error {{cannot be proven}} l1-error {{cannot be proven}}
  }
  else {
    use(p.value());  // nopremise-error {{cannot be proven}} minsize-error {{cannot be proven}}
  }
}

// on-no-diagnostics
