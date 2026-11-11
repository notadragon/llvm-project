constexpr bool f() { int a[1] = {0}; int* p = &*(a + 1); return p != nullptr; }   // undefined per [expr.unary.op]
static_assert(f());
