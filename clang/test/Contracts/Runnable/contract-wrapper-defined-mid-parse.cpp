// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: %t

// A constant evaluation reaching a constexpr virtual function with a P3097
// contract, from a static_assert in the middle of parsing a function, a class,
// a template definition and a lambda.  The enclosing function's locals are
// still usable afterwards.
//
// Mirror of gnu_gcc's contract-wrapper-defined-mid-parse.C (GCC defined the
// wrapper in place and lost the enclosing function's context, GCC-199).

// One class per context, so that each has a wrapper of its own to define.
template <int N> struct B {
  constexpr virtual int f(int x) const pre(x > 0) { return x; }
};
template <int N> struct D : B<N> {
  constexpr int f(int x) const override { return x + N; }
};

int in_function(int a) {
  int local = a;
  static constexpr D<1> d;
  static constexpr const B<1> &b = d;
  static_assert(b.f(1) == 2);
  local += 3;
  return local;
}

struct in_class {
  static constexpr D<2> d{};
  static constexpr const B<2> &b = d;
  static_assert(b.f(2) == 4);
  int m = 4;
  int h() { return m; }
};

template <class T> T in_template(T a) {
  T local = a;
  static constexpr D<3> d;
  static constexpr const B<3> &b = d;
  static_assert(b.f(3) == 6);
  return local + 1;
}

int in_lambda(int a) {
  auto l = [a] {
    int local = a;
    static constexpr D<4> d;
    static constexpr const B<4> &b = d;
    static_assert(b.f(4) == 8);
    return local * 2;
  };
  return l();
}

int main() {
  if (in_function(1) != 4 || in_class().h() != 4 || in_template(3) != 4 ||
      in_lambda(5) != 10)
    __builtin_abort();
}
