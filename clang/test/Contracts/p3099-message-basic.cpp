// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3099 -fsyntax-only -verify %s
// expected-no-diagnostics

// Basic P3099 syntactic message parsing: string literal on pre/post/contract_assert.

void f(int x) pre(x > 0, "x must be positive") {}

int g(int x) post(r: r >= 0, "non-negative result") { return x; }

void h(int *p) {
  contract_assert(p != nullptr, "null pointer");
}

// Message with result name
int square(int x)
  pre(x >= 0, "non-negative input")
  post(r: r >= 0, "non-negative output")
{
  return x * x;
}

// Multiple contracts, some with messages, some without
void mixed(int x)
  pre(x > 0, "positive")
  pre(x < 100)
{
}

// Empty string message is valid
void empty_msg(int x) pre(x > 0, "") {}

// Template with message
template <class T>
T clamp(T x, const T lo, const T hi)
  pre(lo <= hi, "invalid range")
  post(r: r >= lo, "result >= lo")
  post(r: r <= hi, "result <= hi")
{
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

void use_template() {
  clamp(5, 0, 10);
}
