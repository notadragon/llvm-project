// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3850 \
// RUN:   -fcontract-evaluation-semantic=observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero:observe \
// RUN:   -fcontract-violation-information=none %libcxx_flags \
// RUN:   -Wno-unused-command-line-argument -S %s -o - | FileCheck %s
// REQUIRES: contracts-libcxx

// Under -fcontract-violation-information=none the object holds none of the
// text a contract-violation record would carry: not the predicate's source
// text, not the user message, not a label's computed comment or message, not
// an implicit assertion's comment, not the function name and not the file
// name (here a distinctive #line name, since the .file directive names the
// real file).  contract-violation-information-full-asm.cpp finds each of
// them under =full.
//
// Mirror, in the gnu_gcc fork:
// gcc/testsuite/g++.dg/contracts/cpp26/contract-violation-information-none-asm.C.

#define __STDC_WANT_ASSERT_USES_CONTRACTS__ 1
#include <cassert>
#include <contracts>

struct vi_label_t {
  using assertion_control_object = vi_label_t;
  constexpr const char *compute_comment(const char *) const {
    return "vi_computed_comment_token";
  }
  constexpr const char *compute_message(const char *) const {
    return "vi_computed_message_token";
  }
};
constexpr vi_label_t vi_label{};

#line 1 "vi_file_name_token.cpp"
int vi_pre_function_token(int vi_pre_token)
  pre(vi_pre_token > 0, "vi_message_token")
  post(r : r != -1)
{
  contract_assert(vi_pre_token != 7);
  return vi_pre_token;
}
void vi_label_function_token(int x) pre<vi_label>(x > 0) {}
int vi_div_function_token(int a, int b) { return a / b; }
void vi_assert_function_token(int vi_assert_token) {
  assert(vi_assert_token > 0);
}

// CHECK-NOT: vi_pre_token
// CHECK-NOT: vi_assert_token
// CHECK-NOT: vi_message_token
// CHECK-NOT: vi_computed_comment_token
// CHECK-NOT: vi_computed_message_token
// CHECK-NOT: integer division by zero
// CHECK-NOT: vi_file_name_token
// CHECK-NOT: {{[a-z] vi_[a-z_]*_function_token\(}}
