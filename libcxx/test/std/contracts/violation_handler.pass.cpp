//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// REQUIRES: contracts

// ADDITIONAL_COMPILE_FLAGS: -fcontracts -fcontracts-p3290 -fcontract-evaluation-semantic=observe

// <contracts>
//
// The violation handler is reached for each assertion kind, and
// contract_violation reports the right kind, semantic and detection mode.
// Under the observe semantic the handler returns and execution continues, so
// several violations can be checked in one run.

#include <contracts>
#include <cassert>
#include <cstring>

using namespace std::contracts;

static int handler_calls          = 0;
static assertion_kind seen_kind   = assertion_kind::unspecified;
static evaluation_semantic seen_semantic = evaluation_semantic::unspecified;
static detection_mode seen_mode   = detection_mode::unspecified;
static const char* seen_comment   = nullptr;
static int seen_line              = 0;

// A local definition, rather than relying on weak-symbol interposition of the
// library's default handler, which is unreliable against a shared libc++.
void handle_contract_violation(const contract_violation& v) {
  ++handler_calls;
  seen_kind     = v.kind();
  seen_semantic = v.semantic();
  seen_mode     = v.detection_mode();
  seen_comment  = v.comment();
  seen_line     = static_cast<int>(v.location().line());
}

int pre_fn(int x) pre(x > 0) { return x; }
int post_fn(const int x) post(r: r > 0) { return x; }

int main(int, char**) {
  // Precondition.
  pre_fn(-1);
  assert(handler_calls == 1);
  assert(seen_kind == assertion_kind::pre);
  assert(seen_semantic == evaluation_semantic::observe);
  assert(seen_mode == detection_mode::predicate_false);
  // observe does not terminate, so we are still here.

  // Postcondition.
  post_fn(-1);
  assert(handler_calls == 2);
  assert(seen_kind == assertion_kind::post);

  // contract_assert, and the comment is the predicate's source text.
  int y = -1;
  contract_assert(y > 0);
  assert(handler_calls == 3);
  assert(seen_kind == assertion_kind::assert);
  assert(seen_comment != nullptr);
  assert(std::strstr(seen_comment, "y > 0") != nullptr);
  // The reported location is this file, at the contract_assert above.
  assert(seen_line != 0);

  // A satisfied assertion does not call the handler.
  pre_fn(1);
  assert(handler_calls == 3);

  return 0;
}
