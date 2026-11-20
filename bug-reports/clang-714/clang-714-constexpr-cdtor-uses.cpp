struct V { virtual void f() {} };
struct A : virtual V {};
struct Bx : virtual V {
  bool ok;
  constexpr Bx(A* a) : ok(dynamic_cast<Bx*>(a) != nullptr) {} // UB [class.cdtor]/6
};
struct D : A, Bx { constexpr D() : Bx(this) {} };
static_assert((D{}, true));
