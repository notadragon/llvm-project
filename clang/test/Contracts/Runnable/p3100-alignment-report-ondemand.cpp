// P3100 (UBSan runtime routing): on the routed alignment path the sanitizer
// emits NOTHING itself; the report leg captures the rendered UBSan text live and
// registers a lazy populator, which the handler renders ON DEMAND via
// contract_violation::report().  Proves (a) nothing before the handler and
// (b) the full diagnostic appears only between the markers, then execution
// continues (noexcept_observe).

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontracts-p4301 -fsanitize=alignment -fsanitize-recover=alignment \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("REPORT-BEGIN\n");
  std::fflush(stdout);
  const char *r = v.report();
  std::printf("%s\n", r ? r : "(null)");
  std::fflush(stdout);
  std::printf("REPORT-END\n");
  std::fflush(stdout);
}

int __attribute__((noinline)) load(int *p) { return *p; }

int main() {
  alignas(int) char buf[8];
  int *p = reinterpret_cast<int *>(buf + 1);
  volatile int sink = load(p);
  (void)sink;
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// CHECK: REPORT-BEGIN
// CHECK: misaligned address
// CHECK: REPORT-END
// CHECK: survived
