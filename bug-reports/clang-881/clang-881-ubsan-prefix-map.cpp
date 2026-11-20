// CLANG-881 (UBSan): build with
//   -fsanitize=integer-divide-by-zero -ffile-prefix-map=/src=/mapped
// and run.  __FILE__, printed first, is "/mapped/repro.cpp"; the report
// names "/src/repro.cpp".  The #line directive gives the file a fixed name
// to map, independent of where this directory is checked out.
#include <stdio.h>
#line 1 "/src/repro.cpp"
int main() {
  volatile int a = 1, b = 0;
  fprintf(stderr, "__FILE__ is %s\n", __FILE__);
  (void)(a / b);
  return 0;
}
