// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 %libcxx_flags -o %t
// RUN: %t

// Uses libc++ <contracts> for the API, but provides a local handler
// to work around weak-symbol interposition limitations with shared libc++.

#include <contracts>
#include <cstdio>
#include <cstring>
#include <cstdlib>

using namespace std::contracts;

int test_num = 0;
assertion_kind expected_kind;
evaluation_semantic expected_semantic;
detection_mode expected_mode;

void handle_contract_violation(const contract_violation& v) {
  ++test_num;
  bool ok = true;
  if (v.kind() != expected_kind) {
    std::fprintf(stderr, "FAIL test %d: kind expected %d, got %d\n",
                 test_num, (int)expected_kind, (int)v.kind());
    ok = false;
  }
  if (v.semantic() != expected_semantic) {
    std::fprintf(stderr, "FAIL test %d: semantic expected %d, got %d\n",
                 test_num, (int)expected_semantic, (int)v.semantic());
    ok = false;
  }
  if (v.detection_mode() != expected_mode) {
    std::fprintf(stderr, "FAIL test %d: detection_mode expected %d, got %d\n",
                 test_num, (int)expected_mode, (int)v.detection_mode());
    ok = false;
  }
  if (!ok) std::abort();
  std::fprintf(stderr, "PASS test %d\n", test_num);
}

int main() {
  // Test 1: observe — handler called, returns normally
  expected_kind = assertion_kind::manual;
  expected_semantic = evaluation_semantic::observe;
  expected_mode = detection_mode::unspecified;
  handle_observed_contract_violation("test observe");

  // Test 2: nothrow observe.  This TU does NOT enable -fcontracts-p4298, so
  // the nothrow_t overload binds to the plain std::contracts variant and
  // reports classic observe.  (D4298's noexcept_observe is reported only when
  // the caller sets -fcontracts-p4298; see p4298-p3290-nothrow.cpp.)
  expected_kind = assertion_kind::manual;
  expected_semantic = evaluation_semantic::observe;
  expected_mode = detection_mode::unspecified;
  handle_observed_contract_violation(std::nothrow, "test nothrow observe");

  // Test 3: FTM defined
  #ifndef __cpp_lib_contracts_api
  std::fprintf(stderr, "FAIL: __cpp_lib_contracts_api not defined\n");
  return 1;
  #else
  std::fprintf(stderr, "PASS FTM: __cpp_lib_contracts_api = %ldL\n",
               (long)__cpp_lib_contracts_api);
  #endif

  std::fprintf(stderr, "All %d tests passed\n", test_num);
  return 0;
}
