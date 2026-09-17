// P3100: the "assume" evaluation_semantic enumerator is available
// unconditionally in <contracts> (no -fcontracts-p3100 required), so a
// violation handler can name it without preprocessor guards.
// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags -fsyntax-only %s

#include <contracts>

static_assert(static_cast<int>(std::contracts::evaluation_semantic::assume) == 5,
              "assume must have value 5");
