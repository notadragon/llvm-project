// CLANG-664: the noexcept operator ignores the elements an initializer list
// leaves to its array filler.  Every static_assert below should hold; stock
// Clang fails the three that construct S through a filler.
//
// clang++ -std=c++20 -fsyntax-only clang-664-noexcept-array-filler.cpp

struct S { S() { throw 2; } };   // potentially-throwing default constructor
struct A { S s[2]; };

static_assert(!noexcept(S()));       // OK
static_assert(!noexcept(A{{S()}}));  // OK: S() throws itself (s[1] is the filler's)
static_assert(!noexcept(A{}));       // fails: s[0] and s[1] from the filler
static_assert(!noexcept(A{{}}));     // fails: the same, one level down
using S2 = S[2];
static_assert(!noexcept(S2{}));      // fails: a braced array of S
