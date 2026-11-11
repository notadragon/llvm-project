// DEFINE: %{v} = -fcontracts-p4451
// DEFINE: %{cxx} = %clangxx -std=c++26 %{v} -fcontract-evaluation-semantic=ignore %libcxx_flags
// DEFINE: %{check} = %{cxx} --precompile %t/m.cppm -o %t/m.pcm && \
// DEFINE:   %{cxx} -c %t/m.pcm -o %t/m.o && \
// DEFINE:   %{cxx} -fmodule-file=p4451s=%t/m.pcm %t/use.cpp %t/m.o -o %t/use && %t/use
//
// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %{check}
// REDEFINE: %{v} = -fcontracts-p3400
// RUN: %{check}
// REDEFINE: %{v} = -fcontracts-p3400 -fcontracts-p4451
// RUN: %{check}

// P4451: a terminating contract assertion on an exported inline function, an
// exported function and an exported function template keeps its terminating
// semantics through the BMI: the importer evaluates it with enforce although
// ignore is configured.  Under -fcontracts-p4451 alone, -fcontracts-p3400
// alone and both.
//
// Mirror: gcc/testsuite/g++.dg/modules/contract-p4451-stream_{a,b}.C (and
// their -p3400 and -both variants)

//--- m.cppm
module;
#include <contracts>
export module p4451s;

export inline int fi(int x) pre<terminating>(x > 0) { return x; }
export int fn(int x) pre<terminating>(x > 0) { return x; }
export template <typename T>
T ft(T x) pre<terminating>(x > 0) { return x; }

//--- use.cpp
import p4451s;
#include <contracts>
#include <cstdlib>

static int handled;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.semantic() != std::contracts::evaluation_semantic::enforce)
    std::abort();
  ++handled;
  throw handled;
}

int main() {
  try { fi(-1); std::abort(); } catch (int) { }
  try { fn(-1); std::abort(); } catch (int) { }
  try { ft(-1); std::abort(); } catch (int) { }
  if (handled != 3)
    std::abort();
}
