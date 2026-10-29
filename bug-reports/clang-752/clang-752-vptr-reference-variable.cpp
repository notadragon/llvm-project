// CLANG-752.
// -fsanitize=vptr does not check a data member read through a reference
// variable or parameter bound to a glvalue of the wrong polymorphic type.
// Flags: -fsanitize=vptr
struct A { virtual ~A() {} int i = 1, j = 2; };
struct B { virtual ~B() {} int i = 1, j = 2; };
__attribute__((noinline)) int direct(A &a) { return reinterpret_cast<B &>(a).j; }
__attribute__((noinline)) int viaref(A &a) { B &b = reinterpret_cast<B &>(a); return b.j; }
__attribute__((noinline)) int param(B &b) { return b.j; }
int main() {
  A a;
  // The misses first, so the first report names line 8 once one is fixed.
  viaref(a);                       // not reported
  param(reinterpret_cast<B &>(a)); // not reported
  direct(a);                       // reported
  return 0;
}
