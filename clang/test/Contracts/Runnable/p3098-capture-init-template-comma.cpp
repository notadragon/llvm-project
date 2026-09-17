// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3850 -fcontracts-p3098 \
// RUN:   %libcxx_flags -o %t
// RUN: %t

// A postcondition init-capture whose initializer is a template-id with more
// than one template argument -- e.g. [p = f<1, 2>()] -- must not be mis-split at
// the template-argument comma.  A top-level comparison in an initializer (e.g.
// [lt = a < b, cap = a]) must still treat the following comma as a capture
// separator.  (Clang parses both correctly; no bug reproduced -- coverage
// parity with GCC F32.)
// (GCC mirror: g++.dg/contracts/cpp26/p3098-capture-init-template-comma.C.)

template <int A, int B> constexpr int f() { return A + B; }

// Multi-argument template-id initializer: p == 3, so the postcondition holds
// only if the whole f<1, 2>() was captured (not just f<1).
int g(int a) post [p = f<1, 2>()] (r : r == p) {
  return a - a + 3;
}

// Top-level comparison in an initializer: the comma after 'a < b' separates two
// captures (lt and cap), it is not a template-argument comma.  (Value parameters
// referenced in a postcondition must be const -- P3098.)
bool h(const int a, const int b)
  post [lt = a < b, cap = a] (r : r == (cap == b) && lt == (a < b)) {
  return a == b;
}

int main() {
  if (g(99) != 3)
    __builtin_abort();
  if (h(2, 2) != true)
    __builtin_abort();
  if (h(2, 5) != false)
    __builtin_abort();
  return 0;
}
