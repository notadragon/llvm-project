struct B0 { int z; };
struct B { int b; };
struct D : B0, B { int x; };      // B at offset 4
struct VB : virtual B { int x; };
[[gnu::noinline]] int nonvirtual(D *p) { return p->b; } // not reported
[[gnu::noinline]] int virt(VB *p) { return p->b; }      // not reported
int main(int argc, char **) {
  // Either faults (SIGSEGV) with no "member access within null pointer".
  return argc > 1 ? virt(nullptr) : nonvirtual(nullptr);
}
