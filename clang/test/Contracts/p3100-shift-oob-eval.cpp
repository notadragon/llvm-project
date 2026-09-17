// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -Wno-shift-count-overflow -Wno-shift-count-negative -fcontract-configuration-file=%S/p3100-shift-oob-ignore.json %libcxx_flags -o %t && %t

// P3100: the shift guard evaluates each operand exactly once, and the shifted
// operand's side effects still happen on the violation path.  ignore semantic.

#include <contracts>
#include <cstdlib>

static int a_calls = 0, b_calls = 0;
int A () { ++a_calls; return 1; }
int B (int v) { ++b_calls; return v; }

int main () {
  a_calls = b_calls = 0;
  if ((A () << B (100)) != 0) std::abort ();
  if (a_calls != 1 || b_calls != 1) std::abort ();

  a_calls = b_calls = 0;
  if ((A () << B (4)) != 16) std::abort ();
  if (a_calls != 1 || b_calls != 1) std::abort ();

  a_calls = b_calls = 0;
  if ((A () >> B (-1)) != 0) std::abort ();
  if (a_calls != 1 || b_calls != 1) std::abort ();
}
