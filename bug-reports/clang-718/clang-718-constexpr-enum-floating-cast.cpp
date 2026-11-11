enum A { a0, a1, a2, a3 };                 // values [0, 3]
constexpr A x = static_cast<A>(4.0);       // undefined: 4.0 -> 4 is outside [0, 3]
