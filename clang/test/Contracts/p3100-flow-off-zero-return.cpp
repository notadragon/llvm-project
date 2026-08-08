// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-ignore.json %libcxx_flags -o %t && %t

// P3100: flowing off the end under "ignore" zeroes the bytes of the returned
// object regardless of its type -- no indeterminate data is leaked.  Covers
// non-int scalars, a by-value aggregate, and a by-reference (sret) return.

#include <contracts>
#include <cstdlib>

struct Trivial { int a; long b; char c; };
struct Big { int a[8]; };
struct WithDtor { int a; long b; ~WithDtor () {} };   // non-trivial dtor -> sret
enum Color { RED = 5, BLUE = 9 };

int *fp (int x) { if (x > 0) return (int *) 1; }
bool fb (int x) { if (x > 0) return true; }
Color fe (int x) { if (x > 0) return BLUE; }
double fd (int x) { if (x > 0) return 3.5; }
Trivial ft (int x) { if (x > 0) return Trivial{1, 2, 3}; }
Big fbig (int x) { if (x > 0) return Big{{1, 2, 3, 4, 5, 6, 7, 8}}; }
WithDtor fw (int x) { if (x > 0) return WithDtor{1, 2}; }

int main () {
  if (fp (-1) != nullptr) std::abort ();
  if (fb (-1) != false) std::abort ();
  if (fe (-1) != (Color) 0) std::abort ();
  if (fd (-1) != 0.0) std::abort ();

  Trivial t = ft (-1);
  if (t.a != 0 || t.b != 0 || t.c != 0) std::abort ();

  Big g = fbig (-1);
  for (int i = 0; i < 8; ++i)
    if (g.a[i] != 0) std::abort ();

  WithDtor w = fw (-1);
  if (w.a != 0 || w.b != 0) std::abort ();

  if (fp (1) == nullptr) std::abort ();
  if (ft (1).a != 1) std::abort ();
  if (fbig (1).a[7] != 8) std::abort ();
}
