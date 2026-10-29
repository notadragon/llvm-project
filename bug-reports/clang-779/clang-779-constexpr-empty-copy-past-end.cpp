// CLANG-779: all three are accepted; each should be rejected.
struct E {};
constexpr bool copy(int i) { E a[2]; E e2 = a[i]; (void)e2; return true; }
static_assert(copy(2));
constexpr bool move(int i) { E a[2]; E e2 = static_cast<E &&>(a[i]); (void)e2; return true; }
static_assert(move(2));
constexpr bool from(int i) { E a[2]; E e; e = a[i]; return true; }
static_assert(from(2));
