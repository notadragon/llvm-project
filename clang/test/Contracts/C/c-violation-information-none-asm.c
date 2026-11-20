// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe \
// RUN:   -fcontract-violation-information=none -S %s -o - | FileCheck %s

// Under -fcontract-violation-information=none the object holds none of the
// text a C contract violation would carry: not the predicate's source text,
// not the function name, not the file name (here a distinctive #line name,
// since the .file directive names the real file), and not what the P3290
// convenience macros of <contracts.h> would pass.
// c-violation-information-full-asm.c finds each of them under =full.
//
// Mirror, in the gnu_gcc fork:
// gcc/testsuite/gcc.dg/contracts/c-violation-information-none-asm.c.

#include <contracts.h>

enum { vi_post_token_value = 1 };

#line 1 "vi_file_name_token.c"
int vi_pre_function_token(int vi_pre_token)
  _Pre(vi_pre_token > 0)
  _Post(r : r != -vi_post_token_value)
{
  _ContractAssert(vi_pre_token != 7);
  return vi_pre_token;
}

void vi_manual_function_token(void) {
  stdc_handle_observed_contract_violation("vi_manual_comment_token");
}

// CHECK-NOT: vi_pre_token
// CHECK-NOT: vi_post_token_value
// CHECK-NOT: vi_manual_comment_token
// CHECK-NOT: vi_file_name_token
// CHECK-NOT: {{[a-z] vi_[a-z_]*_function_token\(}}
// CHECK-NOT: "vi_manual_function_token"
