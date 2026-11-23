// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4298 \
// RUN:   -fcontracts-p4405 %libcxx_flags -fsyntax-only %s
// REQUIRES: contracts-libcxx

// P3400 [support.contract.control.semfun]: std::contracts::is_continuing is
// true for ignore, observe, and an implementation-defined semantic after
// whose contract violation execution continues with well-defined behaviour;
// such a semantic may be neither terminating nor continuing (CLANG-825).  So
// noexcept_observe, prove and diagnose are continuing, enforce,
// quick_enforce and noexcept_enforce are not, and neither is assume, since
// continuing after a violation under it is undefined behaviour: assume is
// neither terminating nor continuing.  unspecified is continuing and not
// terminating (notadragon_wg21 DECISIONS-resolved.md E142): it might do
// anything, so it could continue, and it does not promise to terminate.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-is-continuing.C

#include <contracts>

using std::contracts::is_continuing;
using std::contracts::is_terminating;
using enum std::contracts::evaluation_semantic;

static_assert(is_continuing(ignore));
static_assert(is_continuing(observe));
static_assert(!is_continuing(enforce));
static_assert(!is_continuing(quick_enforce));
static_assert(is_continuing(noexcept_observe));
static_assert(!is_continuing(noexcept_enforce));
static_assert(is_continuing(prove));
static_assert(is_continuing(diagnose));

static_assert(!is_terminating(assume));
static_assert(!is_continuing(assume));

static_assert(is_continuing(unspecified));
static_assert(!is_terminating(unspecified));
