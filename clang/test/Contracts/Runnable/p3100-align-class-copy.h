// A class copied from, or assigned from, a misaligned `*(P *)(c + o)`, for the
// p3100-align-class-copy*.cpp tests: each run prints the number of
// implicit-assertion reports of one form.  The trivial copy constructor's (or
// assignment operator's) reference parameter is bound to the object and the
// object is then read, at one place: one check reports it.  A reference
// variable bound to it and read through later is two operations (the
// binding, then the read through the reference), both checked; a member read
// and a store to the object are one access.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-align-class-copy.h)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++reports;
}

#define NI __attribute__((noinline))
struct P {
  int a, b;
};
alignas(8) char buf[32];
P d;
// A copy returned, from a global array and through a parameter.
NI P gcopy(int o) { return *(P *)(buf + o); }
NI P pcopy(char *c, int o) { return *(P *)(c + o); }
// An assignment from it.
NI void passign(char *c, int o) { d = *(P *)(c + o); }
// Direct-, list- and copy-initialization from it, and through a pointer
// variable.
NI int pinit(char *c, int o) {
  P x(*(P *)(c + o));
  return x.a;
}
NI int pbrace(char *c, int o) {
  P x{*(P *)(c + o)};
  return x.a;
}
NI int pvar(char *c, int o) {
  P *p = (P *)(c + o);
  P x = *p;
  return x.a;
}
// A copy passed by value.
NI int take(P p) { return p.a; }
NI int parg(char *c, int o) { return take(*(P *)(c + o)); }
// The controls: an assignment to it, a reference variable and a read through
// it, a member read.
NI void passignto(char *c, int o) { *(P *)(c + o) = d; }
NI int pref(char *c, int o) {
  const P &r = *(P *)(c + o);
  return r.a;
}
NI int pmem(char *c, int o) { return (*(P *)(c + o)).a; }

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    __VA_ARGS__;                                                               \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  int volatile one = 1, zero = 0;
  COUNT("gcopy", gcopy(one));
  COUNT("pcopy", pcopy(buf, one));
  COUNT("passign", passign(buf, one));
  COUNT("pinit", pinit(buf, one));
  COUNT("pbrace", pbrace(buf, one));
  COUNT("pvar", pvar(buf, one));
  COUNT("parg", parg(buf, one));
  COUNT("passignto", passignto(buf, one));
  COUNT("pref", pref(buf, one));
  COUNT("pmem", pmem(buf, one));
  COUNT("aligned", gcopy(zero); pcopy(buf, zero); passign(buf, zero);
        pinit(buf, zero); pbrace(buf, zero); pvar(buf, zero);
        parg(buf, zero); passignto(buf, zero); pref(buf, zero);
        pmem(buf, zero));
  return 0;
}
