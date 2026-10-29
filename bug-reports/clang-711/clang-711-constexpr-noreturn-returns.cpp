[[noreturn]] constexpr void g() {}           // warning only
constexpr bool f() { g(); return true; }    // undefined: g returns
static_assert(f());
