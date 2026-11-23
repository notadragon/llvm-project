// RUN: %clangxx_default_config -std=c++26 -fcontracts-p3400 -stdlib=libstdc++ \
// RUN:   -fcontract-evaluation-semantic=observe \
// RUN:   -fcontract-group-evaluation-semantic=safety:observe,audit:ignore %s \
// RUN:   -Wl,-rpath,%libcxx_libdir -Wl,-rpath,%libstdcxx_libdir -o %t
// RUN: %t | FileCheck %s --check-prefix=P3400
// RUN: not --crash %t enforced
// RUN: %clangxx_default_config -std=c++26 -fcontracts-p3400 -fcontracts-p4405 \
// RUN:   -stdlib=libstdc++ -fcontract-evaluation-semantic=observe \
// RUN:   -fcontract-group-evaluation-semantic=safety:observe,audit:ignore %s \
// RUN:   -Wl,-rpath,%libcxx_libdir -Wl,-rpath,%libstdcxx_libdir -o %t.prove
// RUN: %t.prove | FileCheck %s --check-prefix=PROVE
// REQUIRES: contracts-libstdcxx

// P3400 with libstdc++, run: group labels select the configured group
// semantics, and the library's control objects compute or restrict the
// semantic, as with libc++.  The handler reads the violation's semantic,
// which libstdc++ defines in libstdc++exp: the driver links it with no
// -lstdc++exp on the command line.  controls::enforced does not allow the
// configured observe, so its violation terminates.
//
// Mirror: libstdc++-v3/include/std/contracts

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ct = std::contracts;
namespace scl = std::contracts::controls;
using ES = ct::evaluation_semantic;

static int calls = 0;
static ES last = ES::ignore;
void handle_contract_violation(const ct::contract_violation &v) {
  ++calls;
  last = v.semantic();
}

struct ptr_groups_t {
  using assertion_control_object = ptr_groups_t;
  static constexpr const char *group_names[2] = {"zeta", "beta"};
};
constexpr ptr_groups_t ptr_groups{};

void f_safety(int x) pre<"safety"group>(x > 0) {}  // safety:observe
void f_audit(int x) pre<"audit"group>(x > 0) {}    // audit:ignore
void f_both(int x) pre<("audit"group | "safety"group)>(x > 0) {}
void f_ptr(int x) pre<(ptr_groups | "audit"group)>(x > 0) {}
void f_always_observe(int x) pre<scl::always_observe>(x > 0) {}
void f_never_enforce(int x) pre<scl::never_enforce>(x > 0) {}
void f_review(int x) pre<scl::review>(x > 0) {}
void f_always_ignore(int x) pre<scl::always_ignore>(x > 0) {}
void f_enforced(int x) pre<scl::enforced>(x > 0) {}
#ifdef __cpp_contracts_prove
void f_prove(int x) pre<scl::prove>(x > 0) {}
void f_always_prove(int x) pre<scl::always_prove>(x > 0) {}
#endif

#define CHECK(c)                                                               \
  do {                                                                         \
    if (!(c)) {                                                                \
      std::printf("FAIL line %d\n", __LINE__);                                 \
      std::exit(1);                                                            \
    }                                                                          \
  } while (0)

int main(int argc, char **argv) {
  int bad = -argc; // Not a constant.
  if (argc > 1 && !std::strcmp(argv[1], "enforced")) {
    f_enforced(bad);
    return 0;
  }
  f_safety(bad);
  CHECK(calls == 1 && last == ES::observe);
  f_audit(bad);
  CHECK(calls == 1);
  f_both(bad);
  CHECK(calls == 2);
  f_ptr(bad); // Of audit, beta and zeta only audit is configured: ignore.
  CHECK(calls == 2);
  f_always_observe(bad);
  CHECK(calls == 3 && last == ES::observe);
  f_never_enforce(bad);
  CHECK(calls == 4);
  f_review(bad);
  CHECK(calls == 5 && last == ES::observe);
  f_always_ignore(bad);
  CHECK(calls == 5);
#ifdef __cpp_contracts_prove
  f_prove(bad);
  CHECK(calls == 6 && last == ES::observe);
  f_always_prove(bad);
  CHECK(calls == 6);
#endif
  std::printf("PASS calls=%d\n", calls);
}

// P3400: PASS calls=5
// PROVE: PASS calls=6
