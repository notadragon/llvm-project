// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3099 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3099: a diagnostic message that depends on a template parameter pack --
// built by a pack expansion, and selected by C++26 pack indexing -- is
// computed for each instantiation and reaches the handler with that
// instantiation's text.  Two instantiations of each template differ only in
// their packs, so a message computed once for the template, or from the
// wrong instantiation, would show.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3099-message-pack.C

#include <contracts>
#include <cstring>

static const char *last;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  last = v.message();
}

struct A { static constexpr char tag = 'a'; };
struct B { static constexpr char tag = 'b'; };
struct C { static constexpr char tag = 'c'; };

// The tags of the pack, in order, by a pack expansion.
template <class... Ts> struct Joined {
  char buf[sizeof...(Ts) + 1] = {Ts::tag..., '\0'};
  constexpr int size() const { return sizeof...(Ts); }
  constexpr const char *data() const { return buf; }
};

template <class... Ts> void joined(int x) pre(x > 0, Joined<Ts...>{}) {}

// The first element of the pack, by pack indexing.
struct Lit {
  const char *s;
  int n;
  constexpr int size() const { return n; }
  constexpr const char *data() const { return s; }
};
constexpr Lit one{"one", 3}, two{"two", 3};

template <const Lit &...Ms> void first(int x) pre(x > 0, Ms...[0]) {}

int main() {
  joined<A, B, C>(0);
  if (!last || std::strcmp(last, "abc") != 0)
    __builtin_abort();
  joined<C, A>(0);
  if (!last || std::strcmp(last, "ca") != 0)
    __builtin_abort();
  first<one, two>(0);
  if (!last || std::strcmp(last, "one") != 0)
    __builtin_abort();
  first<two, one>(0);
  if (!last || std::strcmp(last, "two") != 0)
    __builtin_abort();
}
