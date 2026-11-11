struct V { virtual void f() {} };
struct Bx : virtual V {
  bool good;
  constexpr Bx() { V* v = this; good = (dynamic_cast<Bx*>(v) == this); }
};
struct D : Bx { constexpr D() {} };
constexpr bool t1() { D d; return d.good; }
static_assert(t1());
