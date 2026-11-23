union U { int i; short s; };
constexpr int f() {
  U u;
  u.s = 1;
  u.i = u.s;    // UB: stored value read from an inexactly overlapping object
  return u.i;
}
static_assert(f() == 1);
