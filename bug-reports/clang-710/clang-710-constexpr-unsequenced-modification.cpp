constexpr int f() { int i = 0; i = i++ + i; return i; }   // undefined
static_assert(f() >= 0);
