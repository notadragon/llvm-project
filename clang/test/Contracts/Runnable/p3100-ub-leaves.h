// The operations whose implicit assertions report the leaves of
// DECISIONS.md E125 (a), for the p3100-ub-leaves*.cpp tests: integer
// division and remainder by zero (ub:expr.mul.div.by.zero.int.div /
// .int.mod), INT_MIN / -1 and INT_MIN % -1
// (ub:expr.mul.representable.type.result.int.div / .int.mod), a shift out of
// range (ub:expr.shift.neg.and.width.shl / .shr), a signed overflow
// (ub:expr.expr.eval.signed.integer.add / .sub / .mul / .negate, an
// increment being an addition and a decrement a subtraction) and the load of
// an invalid value (ub:conv.lval.valid.representation.bool / .enum).
// Each operation runs in a child process, so that one which terminates or
// traps does not end the test; a line per operation shows the semantic of
// each report (" R<n>", n the evaluation_semantic), then " =<value>" if the
// operation returned, or the signal it died by: " abort" (a terminating
// semantic), " trap" (quick_enforce), " fpe" (a division the "assume"
// semantic left unchecked).
// (GCC mirror: g++.dg/contracts/cpp26/p3100-ub-leaves.h)

#include <climits>
#include <contracts>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

namespace cs = std::contracts;

void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    std::printf(" R%d", (int)v.semantic());
  std::fflush(stdout);
}

static volatile int vz = 0, vone = 1, vtwo = 2, vm1 = -1, vbig = 40,
                    vneg = -1, vmax = INT_MAX, vimin = INT_MIN;
enum E3 { A0, A1, A2 }; // values 0..3

#define NI __attribute__((noinline))
NI bool loadb(const bool *p) { return *p; }
NI E3 loade(const E3 *p) { return *p; }

#define LEAF_OPS                                                               \
  OP(div_zero, return 7 / vz;)                                                 \
  OP(div_zero_assign, int x = 7; x /= vz; return x;)                           \
  OP(mod_zero, return 7 % vz;)                                                 \
  OP(div_repr, return vimin / vm1;)                                            \
  OP(mod_repr, return vimin % vm1;)                                            \
  OP(shl, return 1 << vbig;)                                                   \
  OP(shl_neg, return 1 << vneg;)                                               \
  OP(shr, return 1 >> vbig;)                                                   \
  OP(add, return vmax + vone;)                                                 \
  OP(inc, int x = vmax; ++x; return x;)                                        \
  OP(sub, return vimin - vone;)                                                \
  OP(sub_const, return vimin - 1;)                                             \
  OP(dec, int x = vimin; x--; return x;)                                       \
  OP(mul, return vmax * vtwo;)                                                 \
  OP(mul_assign, int x = vmax; x *= vtwo; return x;)                           \
  OP(negate, return -vimin;)                                                   \
  OP(load_bool, bool b; unsigned char c = 2; std::memcpy(&b, &c, 1);           \
     return loadb(&b);)                                                        \
  OP(load_enum, E3 e; int i = 7; std::memcpy(&e, &i, sizeof e);                \
     return loade(&e);)

#define OP(name, body)                                                         \
  NI long name() { body }
LEAF_OPS
#undef OP

static void run(const char *name, long (*fn)()) {
  std::printf("%s:", name);
  std::fflush(stdout);
  pid_t pid = fork();
  if (pid == 0) {
    long v = fn();
    std::printf(" =%ld\n", v);
    std::fflush(stdout);
    std::_Exit(0);
  }
  int st = 0;
  waitpid(pid, &st, 0);
  if (WIFSIGNALED(st))
    switch (WTERMSIG(st)) {
    case SIGABRT:
      std::printf(" abort\n");
      break;
    case SIGILL:
    case SIGTRAP:
      std::printf(" trap\n");
      break;
    case SIGFPE:
      std::printf(" fpe\n");
      break;
    default:
      std::printf(" signal %d\n", WTERMSIG(st));
      break;
    }
  else if (!WIFEXITED(st) || WEXITSTATUS(st) != 0)
    std::printf(" exit %d\n", WEXITSTATUS(st));
  std::fflush(stdout);
}

int main() {
#define OP(name, body) run(#name, name);
  LEAF_OPS
#undef OP
  return 0;
}
