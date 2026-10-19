struct S { int m; };
[[gnu::noinline]] void f(S *sp, S &s) { sp->operator=(s); } // not reported
int main() {
  S *volatile vs = nullptr;
  S s{};
  // Faults (SIGSEGV) with no "store to null pointer"; (*vs).operator=(s)
  // and *vs = s are reported.
  f(vs, s);
}
