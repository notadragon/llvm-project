// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=prove -O0 -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=prove -O2 -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=prove -O2 -fno-contracts-prove-shadow -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=prove -O2 -clear-ast-before-backend -emit-llvm-only -verify %s
// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=prove -O0 -emit-llvm-only %s 2> %t.O0
// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=prove -O2 -emit-llvm-only %s 2> %t.O2
// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=prove -O2 -clear-ast-before-backend -emit-obj -o %t.o %s 2> %t.O2c
// RUN: diff %t.O0 %t.O2
// RUN: diff %t.O0 %t.O2c
// REQUIRES: x86-registered-target

// P4405 stage 3: the diagnostics of the proof shadows are issued after the
// proof module, but each is followed by the template instantiation notes
// Sema gives a diagnostic in its instantiation, exactly as without the
// shadows (-O0, -fno-contracts-prove-shadow), where code generation issues
// it at once: the output is the same, byte for byte, also when the AST is
// cleared before the back end runs (-clear-ast-before-backend, which the
// driver passes): the notes are rendered while code generation can.  The same for an
// evaluation folding decides (always false) while shadows are active,
// whose diagnostic waits for theirs.  The notes are printed once for
// consecutive diagnostics of one instantiation.  An explicit
// instantiation.  A constexpr function template used in a constant
// expression is instantiated there, so its notes name that use.  A
// non-template function's diagnostic has no notes.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-template-context.C
// (each prints its own compiler's backtrace; Clang does not diagnose an
// inline function it does not emit, DECISIONS.md E130, so k<6> is also
// called)

template <int N>
int f (int x)
{
  if (x < 10)
    contract_assert (x + N < 15);  // expected-error 2 {{contract assertion cannot be proven}}
  contract_assert (N < 0);  // expected-error 2 {{contract predicate is always false}}
  return x;
}

template <int N>
int g (int x)
{
  return f<N> (x) + 1;  // expected-note {{in instantiation of function template specialization 'f<6>' requested here}}
}

int h (int x)
{
  return g<6> (x);  // expected-note {{in instantiation of function template specialization 'g<6>' requested here}}
}

template int f<7> (int);  // expected-note {{in instantiation of function template specialization 'f<7>' requested here}}

template <int N>
constexpr int k (int x)
{
  if (x < 10)
    contract_assert (x + N < 15);  // expected-error {{contract assertion cannot be proven}}
  return x;
}

constexpr int kk = k<6> (1);  // expected-note {{in instantiation of function template specialization 'k<6>' requested here}}

int use_k (int x) { return k<6> (x); }

int plain (int x)
{
  contract_assert (x > 0);  // expected-error {{contract assertion cannot be proven}}
  return x;
}
