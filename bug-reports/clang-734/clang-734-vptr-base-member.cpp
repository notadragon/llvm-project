// CLANG-734.
// -fsanitize=vptr does not check a member of a polymorphic base class reached
// through a glvalue of the wrong derived type.
// Flags: -fsanitize=vptr
struct Base { virtual ~Base() {} int x = 1; };
struct A : Base { int a = 2; };
struct B : Base { int b = 3; };
__attribute__((noinline)) int own(A &r) { return reinterpret_cast<B &>(r).b; }
__attribute__((noinline)) int inherited(A &r) { return reinterpret_cast<B &>(r).x; }
int main() {
  A a;
  inherited(a); // member of Base through a B glvalue: not reported
  own(a);       // member of B through a B glvalue: reported
  return 0;
}
