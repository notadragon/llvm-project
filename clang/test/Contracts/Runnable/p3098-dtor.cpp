// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: %t

// P3098: postcondition capture destruction ordering -- captures are destroyed
// in reverse lexical order, after all predicates have run.  Predicates here are
// trivially true, so enforce runs to completion.
// (GCC mirror: g++.dg/contracts/cpp26/p3098-dtor.C)

static int log_idx = 0;
static int order_log[20];

struct Logger {
  int id;
  Logger(int i) : id(i) { order_log[log_idx++] = i; }
  Logger(const Logger& o) : id(o.id) { order_log[log_idx++] = id + 100; }
  ~Logger() { order_log[log_idx++] = id + 200; }
};

int f(int i)
  post [a = Logger(1), b = Logger(2)] (true)
  post [c = Logger(3)] (true)
{
  return i;
}

int main() {
  log_idx = 0;
  f(1);
  // 1,2,3 construct (a,b,c); then reverse destroy 203,202,201.
  if (order_log[0] != 1) __builtin_abort();    // construct a
  if (order_log[1] != 2) __builtin_abort();    // construct b
  if (order_log[2] != 3) __builtin_abort();    // construct c
  if (order_log[3] != 203) __builtin_abort();  // destroy c (last built, first gone)
  if (order_log[4] != 202) __builtin_abort();  // destroy b
  if (order_log[5] != 201) __builtin_abort();  // destroy a
  if (log_idx != 6) __builtin_abort();
}
