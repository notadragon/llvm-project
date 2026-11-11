// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3850 \
// RUN:   -fcontract-evaluation-semantic=observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero:observe \
// RUN:   -fcontract-violation-information=full %libcxx_flags \
// RUN:   -Wno-unused-command-line-argument -S %s -o - | FileCheck %s
// REQUIRES: contracts-libcxx

// Under -fcontract-violation-information=full, the default, the object holds
// the text of a contract-violation record: each string
// contract-violation-information-none-asm.cpp requires to be absent under
// =none is present here, so that test cannot pass for want of the text.
//
// Mirror, in the gnu_gcc fork:
// gcc/testsuite/g++.dg/contracts/cpp26/contract-violation-information-full-asm.C.

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

// CHECK-DAG: vi_pre_token
// CHECK-DAG: vi_assert_token
// CHECK-DAG: vi_message_token
// CHECK-DAG: vi_computed_comment_token
// CHECK-DAG: vi_computed_message_token
// CHECK-DAG: integer division by zero
// CHECK-DAG: vi_file_name_token
// CHECK-DAG: {{[a-z] vi_[a-z_]*_function_token\(}}
