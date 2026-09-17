// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-invalid-value.json %libcxx_flags -o %t && %t

// P3100: invalid bool/enum value load (ub:conv.lval.valid.representation).
// ignore -> a defined valid value (false / an in-range enum value, 0 here), no
// handler; observe -> handler runs (assertion_kind::implicit) then continues
// with that value.  Unlike GCC (whose middle end cannot host throwing observe),
// Clang supports throwing observe directly here.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-invalid-value.C, which uses
// -fcontracts-p4298 to reach noexcept_observe.)

#include <contracts>
#include <cstring>
#include <cstdlib>

namespace cs = std::contracts;

enum A { B = -3, C = 2 };   // non-fixed; valid range [-4, 3]

static int calls = 0;
static bool all_implicit = true;
void handle_contract_violation(const cs::contract_violation& v) {
  ++calls;
  if (v.kind() != cs::assertion_kind::implicit)
    all_implicit = false;
}

namespace ign_ns {                    // ignore
  bool loadb(const bool* p) { return *p; }
  A    loade(const A* p)    { return *p; }
}
namespace obs_ns {                    // observe
  bool loadb(const bool* p) { return *p; }
}

int main() {
  if (sizeof(int) != sizeof(A) || sizeof(bool) != 1)
    return 0;
  bool badb; unsigned char cb = 4; std::memcpy(&badb, &cb, 1);
  A bade; int ce = 9; std::memcpy(&bade, &ce, sizeof(int));

  // ignore: defined valid value 0, no handler.
  if (ign_ns::loadb(&badb) != false) std::abort();
  if (ign_ns::loade(&bade) != (A)0) std::abort();
  if (calls != 0) std::abort();

  // observe: handler runs, then continues with the defined value.
  if (obs_ns::loadb(&badb) != false) std::abort();
  if (calls != 1) std::abort();
  if (!all_implicit) std::abort();

  // valid values pass through unchanged.
  bool okb = true; A oke = C;
  if (ign_ns::loadb(&okb) != true) std::abort();
  if (ign_ns::loade(&oke) != C) std::abort();
  if (obs_ns::loadb(&okb) != true) std::abort();
  if (calls != 1) std::abort();
}
