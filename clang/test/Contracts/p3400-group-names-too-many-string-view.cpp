// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-group-evaluation-semantic=safety:ignore -fsyntax-only \
// RUN:   -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P3400 [support.contract.control.ident] and notadragon_wg21 DECISIONS.md
// E187 (a), E190: a label's group_names may hold at most 4096 names; a
// range that has not ended by then, or an array of more names, is an error
// rather than a label silently in no group.  4096 names are read in full:
// the last of them is "safety", whose group is ignored, so a violated
// precondition is not evaluated during constant evaluation.  The range is
// walked once, so reading 4096 names through a class-type iterator is
// quick, and a 4097-name one stops at the cap before reading any name.  As
// p3400-group-names-too-many.cpp, but with <string_view> included, so the
// names are read as string_view rather than as const char*; and a range
// whose iterator returns its names by value, as objects holding their
// characters, is read too.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/
// p3400-group-names-too-many-string-view.C (gnu_gcc).

#include <string_view>

struct names_t {
  const char *a[4097];
};
constexpr names_t make() {
  names_t r{};
  for (int i = 0; i != 4097; ++i)
    r.a[i] = "a";
  r.a[4095] = "safety";
  return r;
}
constexpr names_t names = make();

// A range of N names over pointers.
template <int N> struct range {
  constexpr const char *const *begin() const { return names.a; }
  constexpr const char *const *end() const { return names.a + N; }
};

template <int N> struct range_label {
  using assertion_control_object = range_label;
  static constexpr range<N> group_names{};
};
constexpr range_label<4097> range_too_many{};
constexpr range_label<4096> range_most{};

// An array of N names, whose length is its type's.  Past the first, the
// names of array_too_many are null: the cap is diagnosed before any name is
// read.
template <int N> struct array_label {
  using assertion_control_object = array_label;
  static constexpr const char *group_names[N] = {"a"};
};
constexpr array_label<4097> array_too_many{};

// An array of 4096 names, the last "safety".
template <class T, T... I> struct array_most_t {
  using assertion_control_object = array_most_t;
  static constexpr const char *group_names[sizeof...(I)] = {
      (I == 4095 ? "safety" : "a")...};
};
constexpr __make_integer_seq<array_most_t, int, 4096> array_most{};

// A range of N names through a class-type iterator, the last "safety".
struct name_iterator {
  int i, last;
  constexpr const char *operator*() const {
    return i == last ? "safety" : "xname";
  }
  constexpr name_iterator &operator++() {
    ++i;
    return *this;
  }
  constexpr bool operator==(const name_iterator &) const = default;
};

template <int N> struct class_range {
  constexpr name_iterator begin() const { return {0, N - 1}; }
  constexpr name_iterator end() const { return {N, N - 1}; }
};

template <int N> struct class_label {
  using assertion_control_object = class_label;
  static constexpr class_range<N> group_names{};
};
constexpr class_label<4097> class_too_many{};
constexpr class_label<4096> class_most{};

void f1(int x) pre<range_too_many>(x > 0) {}  // expected-error {{group_names facet of assertion-control object 'range_label<4097>' holds more than 4096 names}}
void f2(int x) pre<array_too_many>(x > 0) {}  // expected-error {{group_names facet of assertion-control object 'array_label<4097>' holds more than 4096 names}}
void f3(int x) pre<class_too_many>(x > 0) {}  // expected-error {{group_names facet of assertion-control object 'class_label<4097>' holds more than 4096 names}}

constexpr int g1(int x) pre<range_most>(x > 0) { return x; }
static_assert(g1(0) == 0);

constexpr int g3(int x) pre<array_most>(x > 0) { return x; }
static_assert(g3(0) == 0);

constexpr int g2(int x) pre<class_most>(x > 0) { return x; }
static_assert(g2(0) == 0);

// Names returned by value, as objects holding their characters: each name
// is read within the full-expression that dereferences the iterator, before
// its temporary is destroyed.
struct owned_name {
  char s[8];
  constexpr operator std::string_view() const { return s; }
};

struct owned_iterator {
  int i;
  constexpr owned_name operator*() const {
    return i == 1 ? owned_name{"safety"} : owned_name{"xname"};
  }
  constexpr owned_iterator &operator++() {
    ++i;
    return *this;
  }
  constexpr bool operator==(const owned_iterator &) const = default;
};

struct owned_range {
  constexpr owned_iterator begin() const { return {0}; }
  constexpr owned_iterator end() const { return {2}; }
};

struct owned_label_t {
  using assertion_control_object = owned_label_t;
  static constexpr owned_range group_names{};
};
constexpr owned_label_t owned_label{};

constexpr int g4(int x) pre<owned_label>(x > 0) { return x; }
static_assert(g4(0) == 0);
