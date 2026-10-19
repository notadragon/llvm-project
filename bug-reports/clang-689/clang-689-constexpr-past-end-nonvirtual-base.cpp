struct B { int b = 1; };
struct NV : B { int x = 2; };
constexpr bool t() { NV ds[1]; NV* p = ds + 1; B* b = p; return b != nullptr; }
static_assert(t());
