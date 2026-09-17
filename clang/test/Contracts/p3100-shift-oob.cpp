// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -Wno-shift-count-overflow -Wno-shift-count-negative -fcontract-configuration-file=%S/p3100-shift-oob.json %libcxx_flags -o %t && %t

// P3100: shift by a negative amount or an amount >= the promoted left operand's
// width is core-language UB ({expr.shift.neg.and.width}).  ignore -> defined 0;
// observe -> handler (assertion_kind::implicit) then 0.  Covers << and >>.

#include <contracts>
#include <cstdlib>

namespace cs = std::contracts;
static int calls = 0;
static bool all_implicit = true;
void handle_contract_violation (const cs::contract_violation& v) {
  ++calls;
  if (v.kind () != cs::assertion_kind::implicit)
    all_implicit = false;
}

namespace ign_ns {                    // ignore
  int shl (int a, int b) { return a << b; }
  int shr (int a, int b) { return a >> b; }
}
namespace obs_ns {                    // observe
  int shl (int a, int b) { return a << b; }
  int shr (int a, int b) { return a >> b; }
}

int main () {
  if (ign_ns::shl (1, 100) != 0) std::abort ();
  if (ign_ns::shl (1, -1) != 0) std::abort ();
  if (ign_ns::shr (256, 100) != 0) std::abort ();
  if (calls != 0) std::abort ();

  if (obs_ns::shl (1, 100) != 0) std::abort ();
  if (obs_ns::shr (256, -3) != 0) std::abort ();
  if (calls != 2) std::abort ();
  if (!all_implicit) std::abort ();

  if (ign_ns::shl (1, 4) != 16) std::abort ();
  if (obs_ns::shr (256, 2) != 64) std::abort ();
  if (calls != 2) std::abort ();
}
