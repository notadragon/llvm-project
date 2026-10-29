// CLANG-803: -fsanitize=null does not check a downcast of `*p` to a
// reference, `p` null, when the base is at a nonzero offset in the derived
// class.  clang++ -std=c++17 -fsanitize=null, run: each line prints with
// no "downcast of null pointer" (nor "reference binding to null pointer",
// "member access within null pointer", "member call on null pointer")
// report before it; GCC reports each.
extern "C" int printf(const char *, ...);
struct V { virtual void h() {} int v; };
struct X { virtual void x() {} long pad; };
struct D2 : X, V {                        // V at offset 16
  int d;
  __attribute__((noinline)) void dn() {}
};
__attribute__((noinline)) D2 &bind(V *p) { return static_cast<D2 &>(*p); }
__attribute__((noinline)) int *addr(V *p) { return &static_cast<D2 &>(*p).d; }
__attribute__((noinline)) void call(V *p) { static_cast<D2 &>(*p).dn(); }
int main() {
  V *volatile p = nullptr;
  bind(p);
  printf("bind done\n");
  addr(p);
  printf("addr done\n");
  call(p);
  printf("call done\n");
  return 0;
}
