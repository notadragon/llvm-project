// RUN: %clang_cc1 -fcontracts-p4299 -fsyntax-only -verify %s
// expected-no-diagnostics

// <contracts.h> names evaluation semantics 5 (assume), 6 (noexcept_observe)
// and 7 (noexcept_enforce) as well as 1-4: stdc_contract_violation_semantic()
// can return them.  P4299 does not fix these names; they follow the
// STDC_CONTRACT_* pattern.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/contracts-h-semantic-constants.c

#include <contracts.h>

_Static_assert(STDC_CONTRACT_ASSUME == 5, "assume");
_Static_assert(STDC_CONTRACT_NOEXCEPT_OBSERVE == 6, "noexcept_observe");
_Static_assert(STDC_CONTRACT_NOEXCEPT_ENFORCE == 7, "noexcept_enforce");
