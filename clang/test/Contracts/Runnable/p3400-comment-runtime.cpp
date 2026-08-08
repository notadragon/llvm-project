// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=observe && %t

// Runtime test: verify compute_comment facet transforms the comment
// string visible to the violation handler.

#include <contracts>
#include <cstring>

using std::contracts::labels::operator|;

struct redact_comment_t {
  using assertion_control_object = redact_comment_t;
  constexpr const char* compute_comment(const char*) const {
    return "[redacted]";
  }
};
constexpr redact_comment_t redact_comment{};

struct passthrough_comment_t {
  using assertion_control_object = passthrough_comment_t;
  constexpr const char* compute_comment(const char* c) const {
    return c;
  }
};
constexpr passthrough_comment_t passthrough_comment{};

static const char* last_comment = nullptr;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  last_comment = v.comment();
}

void f_plain(int x) pre(x > 0) {}
void f_redact(int x) pre<redact_comment>(x > 0) {}
void f_passthrough(int x) pre<passthrough_comment>(x > 0) {}
void f_redact_pass(int x)
  pre<(redact_comment | passthrough_comment)>(x > 0) {}
void f_pass_redact(int x)
  pre<(passthrough_comment | redact_comment)>(x > 0) {}
void f_pass_pass(int x)
  pre<(passthrough_comment | passthrough_comment)>(x > 0) {}

int main() {
  // Plain: comment is predicate source text
  f_plain(-1);
  if (!last_comment || std::strcmp(last_comment, "x > 0") != 0)
    __builtin_abort();

  // Redact: comment replaced
  f_redact(-1);
  if (!last_comment || std::strcmp(last_comment, "[redacted]") != 0)
    __builtin_abort();

  // Passthrough: comment unchanged
  f_passthrough(-1);
  if (!last_comment || std::strcmp(last_comment, "x > 0") != 0)
    __builtin_abort();

  // redact | passthrough: redact first -> "[redacted]", passthrough keeps it
  f_redact_pass(-1);
  if (!last_comment || std::strcmp(last_comment, "[redacted]") != 0)
    __builtin_abort();

  // passthrough | redact: passthrough keeps original, redact replaces
  f_pass_redact(-1);
  if (!last_comment || std::strcmp(last_comment, "[redacted]") != 0)
    __builtin_abort();

  // passthrough | passthrough: both identity, original kept
  f_pass_pass(-1);
  if (!last_comment || std::strcmp(last_comment, "x > 0") != 0)
    __builtin_abort();

  return 0;
}
