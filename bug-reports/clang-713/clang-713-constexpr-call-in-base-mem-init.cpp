struct A { int a; constexpr A(int x) : a(x) {} };
struct B : A {
  constexpr int g() const { return 1; }
  constexpr B() : A(g()) {}     // undefined [class.base.init]
};
static_assert((B{}, true));
