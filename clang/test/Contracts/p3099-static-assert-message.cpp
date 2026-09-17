// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3099 -fsyntax-only -verify %s

// P3099: the contract diagnostic-message grammar (a string literal, or a
// constant of class type with .size()/.data()) is the same grammar static_assert
// uses; verify both forms deliver the message in a static_assert diagnostic.
// Note: the custom-type form works here in static_assert even though the same
// custom-type message ICEs as a *contract* message on Clang -- that
// is specific to the contract message-parse path.
// (GCC mirror: g++.dg/contracts/cpp26/p3099-static-assert-message.C)

struct Msg {
  constexpr int size() const { return 13; }
  constexpr const char* data() const { return "custom sa msg"; }
};

static_assert(sizeof(int) > 100, Msg{}); // expected-error {{custom sa msg}} expected-note {{expression evaluates to '4 > 100'}}
static_assert(sizeof(int) > 100, "plain literal sa msg"); // expected-error {{plain literal sa msg}} expected-note {{expression evaluates to '4 > 100'}}
