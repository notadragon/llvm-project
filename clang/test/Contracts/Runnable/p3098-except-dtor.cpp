// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3098: an exception thrown by a capture's destructor propagates after the
// predicate has been evaluated.  (GCC mirror: p3098-except-dtor.C)
//
// Clang, fixed: the destructor exception used to occur BEFORE the
// predicate was evaluated -- the predicate never ran (predicate_count == 0)
// though the throw (e == 1) was caught.  GCC evaluates the predicate first
// (predicate_count == 1), per the P3098 "all predicates, then destroy in
// reverse" model; Clang now matches via a prologue cleanup that runs
// postcondition evaluation ahead of capture destructor cleanups.  See wg21
// testing-gap-catalogue.md section 10.

#include <contracts>
#include <cstdio>

static int predicate_count = 0;
bool count_predicate() { ++predicate_count; return true; }

struct ThrowOnDestroy {
  int id;
  ThrowOnDestroy(int i) : id(i) {}
  ThrowOnDestroy(const ThrowOnDestroy& o) : id(o.id) {}
  ~ThrowOnDestroy() noexcept(false) { throw id; }
};

int f(int i) post [t = ThrowOnDestroy(1)] (r: count_predicate()) { return i; }

int main() {
  predicate_count = 0;
  try {
    f(10);
  } catch (int e) {
    if (predicate_count != 1) __builtin_abort();  // predicate ran before dtor
    if (e != 1) __builtin_abort();
    std::printf("PASS\n");
    return 0;
  }
  __builtin_abort();  // destructor must have thrown
}
