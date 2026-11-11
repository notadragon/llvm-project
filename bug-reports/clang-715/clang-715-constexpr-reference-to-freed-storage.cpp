constexpr int t() {
  int* ip = new int(5);
  int& i = *ip;
  delete ip;
  int& j = i;   // UB: names a reference whose referent's storage was released
  (void)j;
  return 1;
}
static_assert(t());
