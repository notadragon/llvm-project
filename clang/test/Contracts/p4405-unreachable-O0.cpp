// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-prove-optimize=0 %libcxx_flags -O0 -S -emit-llvm -o %t.ll -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P4405, notadragon_wg21 DECISIONS.md E195 (CLANG-909): an analysis
// evaluation that is never reached is proven, not "always false" or
// unproven -- reaching no point is a proof.  At proof level 0 there is no proof
// shadow, so only code generation decides: it never emits code a constant
// condition folds away or that follows a statement that never completes
// without a label in between (ConstantFoldsToSimpleInteger,
// ContainsLabel), and an evaluation in code never emitted is never made
// (-fcontract-analysis-skip-dead-calls, implied by -fcontracts-p4405): S1..S4,
// S7..S9, S11, S12.  A label makes what follows it reachable, so a goto or
// a switch into such code is still analyzed (N4..N7; Clang emits a skipped
// substatement that contains a label).  What only the optimizers know (S5,
// S6, S10, S13) is "always false" here; the proof shadow finds it never
// reached from -O1 (p4405-unreachable-O1.cpp, p4405-unreachable-O2.cpp).
// The shapes, and what each level catches, are tabulated in
// notadragon_wg21's p4405-clang-implementation.md.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-unreachable-O0.C

#include <contracts>
#include <array>
#include <cstddef>
using namespace std::contracts::labels;

void g (int x) pre<always_prove> (x != 0);  // expected-note 5 {{predicate}}
int sink (int);

// S1: after `if' with a constant condition whose selected branch returns
// (red_dead.cpp f1).
void s1 ()
{
  const unsigned i = 9;
  if (i >= 6)
    return;
  contract_assert<always_prove> (i < 6);  // S1: OK
}

// S2: `if (0)', a predicate that folds to false (red_dead2.cpp).
void s2 ()
{
  if (0) { contract_assert<always_prove> (false); }  // S2: OK
}

// S3: `if (0)', a predicate that does not fold.
void s3 (int a)
{
  if (0) { contract_assert<always_prove> (a != 1); }  // S3: OK
}

// S4: a precondition at a call under `if (0)' (E169 (14) already).
void s4 (int a)
{
  if (0) { g (a); }  // S4: OK
}

// S5: a precondition at a call in a branch only the optimizers know is
// dead (red_dead.cpp f2).
void s5 (int *p)
{
  if (!p)
    return;
  if (p != nullptr)
    g (1);
  else
    g (0);  // S5: expected-error {{always false}}
}

// S6: an assertion behind a condition earlier code rules out.
void s6 (int x)
{
  if (x > 5)
    return;
  if (x > 10)
    contract_assert<always_prove> (false);  // S6: expected-error {{always false}}
}

// S7: P4021's main6_b: checks that become early returns.
const unsigned char tga[] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            2, 0, 2, 0, 32, 0, 0, 255, 255, 0, 0, 255, 255,
            0, 0, 255, 255, 0, 0, 255, 255, 0, 17 };
int s7 ()
{
  if (tga[2] == 1) { sink (1); return -1; }
  if (sizeof (tga) != 16 + 18) { sink (2); return -2; }
  contract_assert<always_prove> (tga[2] == 1);  // S7a: OK
  contract_assert<always_prove> (sizeof (tga) == 16 + 18);  // S7b: OK
  return 0;
}

// S8: P4021's main26 with std::array, fixed.
int s8 ()
{
  std::array<std::size_t, 6> c = { 1, 2, 3, 4, 5, 6 };
  const std::size_t index = 9;
  if (index >= c.size ())
    return -1;
  contract_assert<always_prove> (index < c.size ());  // S8: OK
  return 0;
}

// S9: after an unconditional return.
void s9 ()
{
  return;
  contract_assert<always_prove> (false);  // S9: OK
}

// S10: a loop whose body never runs.
void s10 ()
{
  for (int i = 0; i < 0; ++i)
    contract_assert<always_prove> (false);  // S10: expected-error {{always false}}
}

// S11: in a template, after a constant early return, and under a
// dependent constant condition.
template <int N> void s11 ()
{
  if (N > 0)
    return;
  contract_assert<always_prove> (N > 0);  // S11a: OK
  if (N)
    contract_assert<always_prove> (false);  // S11b: OK
}
template void s11<1> ();

// S13: a call behind a condition that is not a constant expression,
// though the optimizers fold it.
int s13 (int y)
{
  return y > 0 && y < 0 ? (g (0), 1) : y;  // S13: expected-error {{always false}}
}

// Negatives: reached, so diagnosed at every level.
void n1 ()
{
  const unsigned i = 9;
  contract_assert<always_prove> (i < 6);  // N1: expected-error {{always false}}
}
void n2 (int c)
{
  const unsigned i = 9;
  if (c)
    return;
  contract_assert<always_prove> (i < 6);  // N2: expected-error {{always false}}
}
void n3 (int c)
{
  if (c)
    g (0);  // N3: expected-error {{always false}}
}
// A label makes the code after it reachable (GCC-992).
void n4 (int c)
{
  if (c)
    goto L;
  if (0)
    {
    L:
      g (0);  // N4: expected-error {{always false}}
    }
}
void n5 (int c)
{
  switch (c)
    {
    case 0:
      return;
      contract_assert<always_prove> (false);  // S12: OK
    case 1:
      contract_assert<always_prove> (false);  // N5: expected-error {{always false}}
    }
}
void n6 (int c)
{
  if (c)
    goto M;
  return;
 M:
  contract_assert<always_prove> (false);  // N6: expected-error {{always false}}
}
template <int N> void n7 (int c)
{
  if (c)
    goto L;
  if (N)
    {
    L:
      g (0);  // N7: expected-error {{always false}}
    }
}
template void n7<0> (int);  // expected-note {{in instantiation}}
