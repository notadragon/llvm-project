// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-div-by-zero-ignore.json %libcxx_flags -o %t && %t

// P3100: the divide-by-zero guard evaluates each operand exactly once, and the
// dividend's side effects still happen on the violation path.  Covers / and %.
// ignore semantic (no handler; continues with 0).

#include <contracts>
#include <cstdlib>

static int a_calls = 0, b_calls = 0;
int A () { ++a_calls; return 10; }
int B (int v) { ++b_calls; return v; }

int main () {
  a_calls = b_calls = 0;
  if (A () / B (0) != 0) std::abort ();
  if (a_calls != 1 || b_calls != 1) std::abort ();

  a_calls = b_calls = 0;
  if (A () / B (2) != 5) std::abort ();
  if (a_calls != 1 || b_calls != 1) std::abort ();

  a_calls = b_calls = 0;
  if (A () % B (0) != 0) std::abort ();
  if (a_calls != 1 || b_calls != 1) std::abort ();

  a_calls = b_calls = 0;
  if (A () % B (3) != 1) std::abort ();
  if (a_calls != 1 || b_calls != 1) std::abort ();
}
