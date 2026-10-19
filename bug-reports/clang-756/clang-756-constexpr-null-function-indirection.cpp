// CLANG-756.
// An indirection through a null pointer to function is accepted in constant
// evaluation when its result is bound to a reference, has its address taken
// or is discarded.  Each static_assert should be rejected.
using F = int();
constexpr int bind(F *fp) { F &r = *fp; (void)r; return 1; }
static_assert(bind(nullptr) == 1);
constexpr bool addr(F *fp) { return &*fp == nullptr; }
static_assert(addr(nullptr));
constexpr int disc(F *fp) { *fp; return 1; }
static_assert(disc(nullptr) == 1);
