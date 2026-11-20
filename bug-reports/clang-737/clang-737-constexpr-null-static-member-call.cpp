struct S { static constexpr int sf() { return 1; } static constexpr int k = 1; };
constexpr bool f() {
  S* sp = nullptr;
  sp->sf();       // undefined: evaluates *sp, an indirection through a null pointer
  return true;
}
static_assert(f());
constexpr bool g() {
  S* sp = nullptr;
  return sp->k == 1;   // the same, for a static data member
}
static_assert(g());
