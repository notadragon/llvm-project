// <contracts.h> defines the post_capture (0x06) and implicit (0x07) assertion
// kind constants for mixed-language handlers (D4299 interop).
// RUN: %clang_cc1 -fcontracts-p4299 -fsyntax-only -verify %s
// expected-no-diagnostics
// (GCC mirror: the interop kind constants added in.)

#include <contracts.h>

_Static_assert(STDC_CONTRACT_POST_CAPTURE == 0x06,
               "STDC_CONTRACT_POST_CAPTURE must be 0x06");
_Static_assert(STDC_CONTRACT_IMPLICIT == 0x07,
               "STDC_CONTRACT_IMPLICIT must be 0x07");
