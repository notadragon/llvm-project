// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// A static_cast from void* to a pointer to a type the pointee is not similar
// to is not a constant expression (P2738), here with an operand formed by
// pointer arithmetic, `a + 1`.  Clang rejects it cleanly, in a constexpr
// variable and in a static_assert; GCC ICEs (stock, gnu_gcc GCC-711,
// PR125199, deferred), and its watch test is the first form.  Not
// contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-void-cast-pointer-plus-ice.C

constexpr bool f() {
  int a[2]{};
  void *v = a + 1;
  unsigned *p = static_cast<unsigned *>(v); // expected-note 2 {{cast from 'void *' is not allowed in a constant expression because the pointed object type 'int' is not similar to the target type 'unsigned int'}}
  return p;
}
constexpr bool x = f(); // expected-error {{constexpr variable 'x' must be initialized by a constant expression}} \
                        // expected-note {{in call to 'f()'}}
static_assert(f()); // expected-error {{not an integral constant expression}} \
                    // expected-note {{in call to 'f()'}}
