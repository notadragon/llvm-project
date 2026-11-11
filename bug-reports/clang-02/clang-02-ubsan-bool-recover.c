/* clang-02-ubsan-bool-recover.c                                       -*-C-*-
 *
 * CLANG-2: with stock UBSan, `-fsanitize=bool -fsanitize-recover=bool` leaves
 * the invalid representation in place after reporting, where GCC coerces the
 * load to a valid `false` and continues.
 *
 *   clang -std=c17 -fsanitize=bool -fsanitize-recover=bool -O0 x.c
 *     -> runtime error: load of value 2, which is not a valid value for '_Bool'
 *        AFTER r=1          <- raw invalid bits kept; the value is truthy
 *        exit 255
 *
 *   gcc -std=c17 -fsanitize=bool -fsanitize-recover=bool -O0 x.c
 *     -> runtime error: load of value 2, which is not a valid value for '_Bool'
 *        AFTER r=0          <- coerced to false
 *        exit 0
 *
 * Verified on system gcc 13.3.0 and on our Clang, in plain C.  NO CONTRACTS
 * AND NO C++ ARE INVOLVED -- this is the stock, unrouted sanitizer path.
 *
 * Whether this is a Clang defect or merely unspecified recovery behaviour is
 * a fair question to put in the report: `-fsanitize-recover` promises that
 * execution continues after a report, not that the offending operation gets a
 * defined substitute.  What is worth raising either way is that the two
 * implementations differ silently on the value the program then observes.
 * The same class of divergence as the documented `-fsanitize=bounds` one.
 */

#include <stdio.h>
#include <string.h>

int
main (void)
{
  _Bool b;
  unsigned char bits = 2;      /* not a valid _Bool representation */
  memcpy (&b, &bits, 1);

  int r = b ? 1 : 0;           /* -fsanitize=bool reports here */
  printf ("AFTER r=%d\n", r);  /* GCC: 0 (coerced); Clang: 1 (raw bits) */

  return r == 0 ? 0 : -1;
}
