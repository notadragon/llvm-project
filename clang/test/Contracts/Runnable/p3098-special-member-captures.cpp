// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3098: postcondition captures on special member functions.  Each capture
// is initialized on entry, before the body runs, and the predicate sees the
// entry value: a copy and a move constructor capturing the source's member,
// copy and move assignment capturing the target's old value, and a
// destructor capturing a member's value while the object is still alive
// (the predicate must use the capture, not the member, which is dead by
// then; [P3098] sec. 4.3).  One check of each is violated, and reported;
// the rest hold.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3098-special-member-captures.C

#include <contracts>

static int violations;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

static int last_destroyed;
static bool note_destroyed(int v) {
  last_destroyed = v;
  return v >= 0;
}

struct S {
  int m;
  explicit S(int v) : m(v) {}
  S(const S &o) post [src = o.m] (m == src) : m(o.m + o.skew) {}
  S(S &&o) post [src = o.m] (m == src && o.m == 0) : m(o.m) { o.m = 0; }
  S &operator=(const S &o) post [old = m] (r : &r == this && m != old) {
    m = o.m;
    return *this;
  }
  S &operator=(S &&o) post [old = m] (r : m == old + o.m) {
    m += o.m;
    return *this;
  }
  ~S() post [v = m] (note_destroyed(v)) { m = -100; }
  int skew = 0;
};

int main() {
  S a(1);
  S b(a); // copy: 1 == 1
  a.skew = 1;
  S c(a); // copy: 2 != 1, violated
  if (violations != 1 || b.m != 1 || c.m != 2)
    __builtin_abort();

  S d(static_cast<S &&>(b)); // move: holds
  if (violations != 1 || d.m != 1 || b.m != 0)
    __builtin_abort();

  c = d; // copy-assign 2 -> 1: holds
  c = d; // copy-assign 1 -> 1: m == old, violated
  if (violations != 2 || c.m != 1)
    __builtin_abort();

  S e(5);
  c = static_cast<S &&>(e); // move-assign: 1 + 5 == 6
  if (violations != 2 || c.m != 6)
    __builtin_abort();

  {
    S f(7);
  } // destructor captured 7 on entry
  if (violations != 2 || last_destroyed != 7)
    __builtin_abort();
  {
    S g(-3);
  } // captured -3: violated
  if (violations != 3 || last_destroyed != -3)
    __builtin_abort();
}
