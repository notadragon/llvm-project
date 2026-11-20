// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 \
// RUN:   -fcontract-evaluation-semantic=observe \
// RUN:   -fcontract-violation-information=full %libcxx_flags -o %t
// RUN: %t

// -fcontract-violation-information=full, the default: the P3290 manual API's
// default location argument is the location of the call, for the plain and
// the nothrow_t overloads; a location passed explicitly, and the comment,
// are kept.  p3290-api-violation-information-none.cpp is the same under
// =none.
//
// Mirror, in the gnu_gcc fork:
// gcc/testsuite/g++.dg/contracts/cpp26/p3290-api-violation-information-full.C.

#include <contracts>
#include <cstring>
#include <new>
#include <source_location>

// What the handler saw: the location's strings outlive the handler, the
// source_location object returned by location() need not.
struct seen {
  const char *file_name_, *function_name_;
  unsigned line_, column_;
  const char *file_name() const { return file_name_; }
  const char *function_name() const { return function_name_; }
  unsigned line() const { return line_; }
  unsigned column() const { return column_; }
};
static seen last;
static const char *last_comment;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.kind() != std::contracts::assertion_kind::manual)
    __builtin_abort();
  auto l = v.location();
  last = {l.file_name(), l.function_name(), l.line(), l.column()};
  last_comment = v.comment();
}

static bool empty(const seen &l) {
  return l.file_name()[0] == '\0' && l.function_name()[0] == '\0' &&
         l.line() == 0 && l.column() == 0;
}

static bool same(const seen &a, const std::source_location &b) {
  return std::strcmp(a.file_name(), b.file_name()) == 0 &&
         std::strcmp(a.function_name(), b.function_name()) == 0 &&
         a.line() == b.line() && a.column() == b.column();
}

int main() {
  unsigned line = __LINE__ + 1;
  std::contracts::handle_observed_contract_violation("default");
  if (std::strcmp(last.file_name(), __FILE__) != 0 ||
      !std::strstr(last.function_name(), "main") || last.line() != line ||
      std::strcmp(last_comment, "default") != 0)
    __builtin_abort();

  line = __LINE__ + 1;
  std::contracts::handle_observed_contract_violation(std::nothrow,
                                                     "nothrow default");
  if (std::strcmp(last.file_name(), __FILE__) != 0 ||
      !std::strstr(last.function_name(), "main") || last.line() != line ||
      std::strcmp(last_comment, "nothrow default") != 0)
    __builtin_abort();

  auto loc = std::source_location::current();
  std::contracts::handle_observed_contract_violation("explicit", loc);
  if (!same(last, loc) || empty(last))
    __builtin_abort();

  std::contracts::handle_observed_contract_violation(std::nothrow,
                                                     "nothrow explicit", loc);
  if (!same(last, loc) || empty(last))
    __builtin_abort();
}
