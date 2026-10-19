// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -fsigned-char %s %libcxx_flags -o %t && %t | FileCheck %s

// Contracts on template specializations: each specialization -- full,
// partial, a member of either, a member template, a constructor -- has its
// own contracts, or none.  Mirror of GCC's
// g++.dg/contracts/cpp26/contracts-tmpl-spec2.C, run with a handler that
// prints the line and predicate of each violation.  One difference from
// the GCC copy: a contract repeated on an out-of-line definition
// (G3<int, S>::f) is reported at the in-class declaration, where GCC
// reports the definition.  The location is implementation-defined.

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("violation %u: %s\n", (unsigned)v.location().line(), v.comment());
}

// template specializations can have differing contracts
template<typename T>
int body(int a)
  pre ( a > 0 )
{
  T t = a * 2.5;
  return t;
}

template<>
int body<double>(int a)
  pre ( a > 1 )
{
  double t = a * 3.3;
  return t;
}

template<typename T>
int none(int a)
  pre ( a > 0 )
{
  return -a;
}

template<>
int none<double>(int a)
  pre ( a > 1 )
{
  return a - 100;
}

template<typename T>
int arg0(T t)
  pre ( t > 0 )
{
  return -t - 10;
}

template<>
int arg0<double>(double t)
  pre ( t > 1 )
{
  return -t + 10;
}

template<typename T>
int arg1(int a, T t)
  pre ( a > 0 )
  pre ( t > 0 )
{
  return -t * a;
}

template<>
int arg1<double>(int a, double t)
  pre ( a > 1 )
  pre ( t > 1 )
{
  return -t * a + 17;
}

template<typename T>
T ret(int a)
  pre ( a > 0 )
{
  return -a;
}

template<>
double ret<double>(int a)
  pre ( a > 1 )
{
  return -a * 3.3;
}

// template specializations can have no contracts
template<typename T>
int g1(T t) pre ( t > 0 )
{
  return (int)t;
}

template<>
int g1<double>(double t)
{
  return (int)t;
}

// a template specialization's contracts are those of its own first
// declaration, which a later redeclaration repeats (one that adds them is
// ill-formed: contracts-tmpl-spec-redecl-adds.C)
template<typename T>
int g2(T t) pre ( t > 0 )
{
  return (int)t;
}

template<>
int g2<double>(double t)
  pre ( t < 0 );

template<>
int g2<double>(double t)
  pre ( t < 0 )
{
  return (int)t;
}

template<>
int g2<char>(char t)
  pre ( t < 'c' )
{
  return (int)t;
}

// contracts can be different on the general template, partial and full specs
template<typename T, typename S>
struct G3
{
  void f(T t, S s)
    pre ( t > 0 )
    pre ( s > 0 )
  {
    printf ("G3 general T S\n");
  }
};

template<typename S>
struct G3<int, S>
{
  void f(int t, S s)  pre ( t > 1 )  pre ( s > 1 ) ;
};

template<typename S>
void G3<int, S>::f(int t, S s)
  pre ( t > 1 )
  pre ( s > 1 )
{
  printf ("G3 partial int S\n");
}

template<>
void G3<int, double>::f(int t, double s)
  pre ( t > 2 )
  pre ( s > 2 )
{
  printf ("G3 full int double\n");
}

struct C
{
  bool operator>(int rhs) { return false; }
};

// deletes contracts
template<>
void G3<int, C>::f(int t, C s);

template<>
void G3<int, C>::f(int t, C s)
{
  printf ("G3 full int C\n");
};

// specialized ctors
template<typename T, typename S>
struct G4
{
  G4(T t, S s)
    pre ( t > 0 )
    pre ( s > 0 )
    post ( x > 0 )
  {
    printf ("G4 general T S\n");
    return;
  }
  int x{-1};
};

template<typename S>
struct G4<char, S>
{
  G4(char t, S s)
    pre ( t > 'c' )
    pre ( s > 3 )
    post ( x2 > 3 )
  {
    printf ("G4 partial char S\n");
    return;
  }
  int x2{-1};
};

template<>
G4<double, double>::G4(double, double)
{
  printf ("G4 full double double\n");
  return;
}

template<>
G4<double, char>::G4(double a, char b)
  pre ( a > 0 )
  pre ( b > 'b' )
  post ( x > 1 )
{
  printf ("G4 full double char\n");
  return;
}

// crossover of template classes and template members ok
template<typename T, typename S>
struct G5
{
  template<typename P>
  void f(T t, S s, P r)
    pre ( t > 0 )
    pre ( s > 0 )
    pre ( r > 0 )
  {
    printf ("G5 gen T S, f gen R\n");
  }
};

template<typename S>
struct G5<char, S>
{
  template<typename R>
  void f(char x, S y, R z)
    pre ( x > 'z' )
    pre ( y > 1 )
    pre ( z > 1 )
  {
    printf ("G5 partial char S, f gen R\n");
  }
};

template<>
template<typename Q>
void G5<double, double>::f(double a, double b, Q c)
  pre ( a > 2 )
  pre ( b > 2 )
  pre ( c > 2 )
{
  printf ("G5 full double double, f gen R\n");
}

int main(int, char**)
{
  printf("%d\n", body<int>(-1));
  printf("%d\n", body<double>(-1));
  printf("%d\n", none<int>(-1));
  printf("%d\n", none<double>(-1));
  printf("%d\n", arg0(-1));
  printf("%d\n", arg0(-1.0));
  printf("%d\n", arg1(-3, -1));
  printf("%d\n", arg1(-3, -1.0));
  printf("%d\n", (int)ret<int>(-1));
  printf("%d\n", (int)ret<double>(-1));
  printf("%f\n", ret<double>(-1));

  printf("%d\n", g1(-1));
  printf("%d\n", g1(-1.0));

  printf("%d\n", g2(-1));
  printf("%d\n", g2(1.0));
  printf("%d\n", g2('d'));

  G3<double, double> g3_gen;
  G3<int, int> g3_partial;
  G3<int, double> g3_full;
  g3_gen.f(-1.0, -1.0); // general
  g3_partial.f(-2, -2); // partial spec
  g3_full.f(-3, -3.0); // full spec

  G3<char, char> g3_gen2;
  G3<int, char> g3_partial2;
  g3_gen2.f((char)-1, (char)-1);
  g3_partial2.f(-1, (char)-1);

  G3<int, C> g3_full2;
  g3_full2.f(5, C{});
  g3_full2.f(-5, C{});

  G4 g4_gen{-1, -1};
  G4 g4_full1{-1.0, -1.0};
  G4 g4_full2{-1.0, (char)'b'};
  G4 g4_partial{(char)'c', -5};

  G5<int, int> g5_gen;
  g5_gen.f(-1, -1, -2);
  g5_gen.f(-1, -1, -2.0);

  G5<char, int> g5_part;
  g5_part.f('a', -1, -2);
  g5_part.f('a', -1, -2.1);

  G5<double, double> g5_full;
  g5_full.f(-1.0, -1.0, -2);
  g5_full.f(-1.0, -1.0, -2.1);
  return 0;
}


// CHECK: violation 22: a > 0
// CHECK-NEXT: -2
// CHECK-NEXT: violation 30: a > 1
// CHECK-NEXT: -3
// CHECK-NEXT: violation 38: a > 0
// CHECK-NEXT: 1
// CHECK-NEXT: violation 45: a > 1
// CHECK-NEXT: -101
// CHECK-NEXT: violation 52: t > 0
// CHECK-NEXT: -9
// CHECK-NEXT: violation 59: t > 1
// CHECK-NEXT: 11
// CHECK-NEXT: violation 66: a > 0
// CHECK-NEXT: violation 67: t > 0
// CHECK-NEXT: -3
// CHECK-NEXT: violation 74: a > 1
// CHECK-NEXT: violation 75: t > 1
// CHECK-NEXT: 14
// CHECK-NEXT: violation 82: a > 0
// CHECK-NEXT: 1
// CHECK-NEXT: violation 89: a > 1
// CHECK-NEXT: 3
// CHECK-NEXT: violation 89: a > 1
// CHECK-NEXT: 3.300000
// CHECK-NEXT: violation 96: t > 0
// CHECK-NEXT: -1
// CHECK-NEXT: -1
// CHECK-NEXT: violation 111: t > 0
// CHECK-NEXT: -1
// CHECK-NEXT: violation 122: t < 0
// CHECK-NEXT: 1
// CHECK-NEXT: violation 129: t < 'c'
// CHECK-NEXT: 100
// CHECK-NEXT: violation 139: t > 0
// CHECK-NEXT: violation 140: s > 0
// CHECK-NEXT: G3 general T S
// CHECK-NEXT: violation 149: t > 1
// CHECK-NEXT: violation 149: s > 1
// CHECK-NEXT: G3 partial int S
// CHECK-NEXT: violation 162: t > 2
// CHECK-NEXT: violation 163: s > 2
// CHECK-NEXT: G3 full int double
// CHECK-NEXT: violation 139: t > 0
// CHECK-NEXT: violation 140: s > 0
// CHECK-NEXT: G3 general T S
// CHECK-NEXT: violation 149: t > 1
// CHECK-NEXT: violation 149: s > 1
// CHECK-NEXT: G3 partial int S
// CHECK-NEXT: G3 full int C
// CHECK-NEXT: G3 full int C
// CHECK-NEXT: violation 188: t > 0
// CHECK-NEXT: violation 189: s > 0
// CHECK-NEXT: G4 general T S
// CHECK-NEXT: violation 190: x > 0
// CHECK-NEXT: G4 full double double
// CHECK-NEXT: violation 221: a > 0
// CHECK-NEXT: violation 222: b > 'b'
// CHECK-NEXT: G4 full double char
// CHECK-NEXT: violation 223: x > 1
// CHECK-NEXT: violation 202: t > 'c'
// CHECK-NEXT: violation 203: s > 3
// CHECK-NEXT: G4 partial char S
// CHECK-NEXT: violation 204: x2 > 3
// CHECK-NEXT: violation 235: t > 0
// CHECK-NEXT: violation 236: s > 0
// CHECK-NEXT: violation 237: r > 0
// CHECK-NEXT: G5 gen T S, f gen R
// CHECK-NEXT: violation 235: t > 0
// CHECK-NEXT: violation 236: s > 0
// CHECK-NEXT: violation 237: r > 0
// CHECK-NEXT: G5 gen T S, f gen R
// CHECK-NEXT: violation 248: x > 'z'
// CHECK-NEXT: violation 249: y > 1
// CHECK-NEXT: violation 250: z > 1
// CHECK-NEXT: G5 partial char S, f gen R
// CHECK-NEXT: violation 248: x > 'z'
// CHECK-NEXT: violation 249: y > 1
// CHECK-NEXT: violation 250: z > 1
// CHECK-NEXT: G5 partial char S, f gen R
// CHECK-NEXT: violation 259: a > 2
// CHECK-NEXT: violation 260: b > 2
// CHECK-NEXT: violation 261: c > 2
// CHECK-NEXT: G5 full double double, f gen R
// CHECK-NEXT: violation 259: a > 2
// CHECK-NEXT: violation 260: b > 2
// CHECK-NEXT: violation 261: c > 2
// CHECK-NEXT: G5 full double double, f gen R
