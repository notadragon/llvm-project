// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags \
// RUN:   -fsyntax-only %s
// REQUIRES: contracts-libcxx

// P3400 [support.contract.control.ident] makes a type an identification
// label when its const group_names member is a range whose elements convert
// to string_view, so a label whose group_names is a std::array or a C array
// of std::string_view satisfies identification_label (CLANG-826; libc++'s
// concept used to require group_names[0] to convert to const char*).  The
// run-time half, that such a label's groups reach the configuration, is
// Runnable/p3400-identification-label-string-view-groups.cpp.

#include <array>
#include <contracts>
#include <string_view>

struct sv_std_array_t {
  using assertion_control_object = sv_std_array_t;
  static constexpr std::array<std::string_view, 2> group_names = {"safety",
                                                                  "audit"};
};
constexpr sv_std_array_t sv_std_array{};

struct sv_c_array_t {
  using assertion_control_object = sv_c_array_t;
  static constexpr std::string_view group_names[] = {"safety"};
};

static_assert(std::contracts::labels::identification_label<sv_std_array_t>);
static_assert(std::contracts::labels::identification_label<sv_c_array_t>);
static_assert(std::contracts::labels::is_identification_label<sv_std_array>);
