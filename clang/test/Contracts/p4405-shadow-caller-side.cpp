// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-shadow-caller-side.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3: a precondition evaluated caller-side with prove is proven
// in the caller's proof shadow, with the call's arguments for the callee's
// parameters, so a check in the caller proves it.  P4021R2 Example 1:
// log_message's precondition is proven where output_string tests the
// pointer and diagnosed where it does not.  P4363R0's examples: func(x)
// called from g(int x) is diagnosed, set_data_idx(c, -1, 500) is always
// false (folding).  A member function's precondition naming the object, a
// reference parameter, and a call in a constexpr function, which constant
// evaluation still evaluates.  The definitions are not diagnosed: their
// own evaluations are ignore.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-caller-side.C

extern "C" int puts(const char *);

// P4021R2 Example 1.
static void log_message(const char *p)
  pre(p)                        // expected-note {{the predicate of the contract assertion}}
{
  puts(p);
}

void output_string(const char *ptr) {
  if (ptr != nullptr)
    log_message(ptr);
}

void output_string_unchecked(const char *ptr) {
  log_message(ptr);             // expected-error {{contract assertion cannot be proven}}
}

// P4363R0.
void func(int x)
  pre(x > 0);                   // expected-note {{the predicate of the contract assertion}}

void g(int x) {
  func(x);                      // expected-error {{contract assertion cannot be proven}}
}

void g_checked(int x) {
  if (x > 0)
    func(x);
}

struct container { long idx_buf[16]; };
constexpr int max_index = 16;

int set_data_idx(container *a, int idx, long data)
  pre(idx < max_index)
  pre(idx >= 0)                 // expected-note {{the predicate of the contract assertion}}
{
  a->idx_buf[idx] = data;
  return 0;
}

void store(container *c, int i) {
  set_data_idx(c, -1, 500);     // expected-error {{contract predicate is always false}}
  if (i >= 0 && i < max_index)
    set_data_idx(c, i, 500);
}

struct S {
  int n;
  int at(int i) const pre(i < n);  // expected-note {{the predicate of the contract assertion}}
};

int use_member(const S &s, int i) {
  if (i < s.n)
    return s.at(i);
  return s.at(i);               // expected-error {{contract assertion cannot be proven}}
}

void by_ref(const int &v) pre(v != 0);  // expected-note {{the predicate of the contract assertion}}

void use_ref(int v) {
  if (v != 0)
    by_ref(v);
  by_ref(v);                    // expected-error {{contract assertion cannot be proven}}
}

constexpr int half(int v) pre(v % 2 == 0) { return v / 2; }

constexpr int quarter(int v) {
  if (v % 2 == 0)
    return half(v) / 2;
  return 0;
}

static_assert(quarter(8) == 2);
static_assert(quarter(5) == 0);
int run_quarter(int v) { return quarter(v); }
