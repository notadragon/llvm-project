// CLANG-881 (ASan): build with
//   -fsanitize=address -ffile-prefix-map=<this directory>=/mapped
// and run.  __FILE__, printed first, is under "/mapped"; ASan's description
// of the global XXX says it is "defined in" this file's unmapped path, the
// global's module name, which a global-buffer-overflow report on XXX prints
// too.
#include <stdio.h>
#include <sanitizer/asan_interface.h>

char XXX[10];

int main() {
  fprintf(stderr, "__FILE__ is %s\n", __FILE__);
  __asan_describe_address(XXX + 10);
  return 0;
}
