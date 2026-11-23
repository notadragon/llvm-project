constexpr bool t() {
  int* p;
  { int y = 1; p = &y; }
  int& r = *p;   // UB: indirection through a dangling pointer
  (void)r;
  return true;
}
static_assert(t());
