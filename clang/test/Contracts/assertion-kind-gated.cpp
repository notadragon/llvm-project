// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags -fsyntax-only %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3290 -fcontracts-p3098 \
// RUN:   -DENABLED %libcxx_flags -fsyntax-only %s

// The assertion_kind enumerators of unadopted papers are declared only where
// the paper is enabled: manual and cassert with P3290's library API,
// post_capture with P3098's postcondition captures, each with its value.
//
// Mirror of gnu_gcc's assertion-kind-gated.C and assertion-kind-gated-enabled.C.

#include <contracts>

using std::contracts::assertion_kind;

template <class E> concept has_manual = requires { E::manual; };
template <class E> concept has_cassert = requires { E::cassert; };
template <class E> concept has_post_capture = requires { E::post_capture; };
template <class E> concept has_implicit = requires { E::implicit; };

#ifdef ENABLED
static_assert(static_cast<int>(assertion_kind::manual) == 4);
static_assert(static_cast<int>(assertion_kind::cassert) == 5);
static_assert(static_cast<int>(assertion_kind::post_capture) == 6);
static_assert(static_cast<int>(assertion_kind::implicit) == 7);
#else
static_assert(!has_manual<assertion_kind>);
static_assert(!has_cassert<assertion_kind>);
static_assert(!has_post_capture<assertion_kind>);
static_assert(has_implicit<assertion_kind>);
static_assert(static_cast<int>(assertion_kind::assert) == 3);
#endif

// REQUIRES: contracts-libcxx
