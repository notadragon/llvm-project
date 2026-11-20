// RUN: not %clang --target=x86_64-linux-gnu -c -std=c++26 -fcontracts -fcontracts-p3100 -fsanitize=undefined -fsanitize-semantic=undefined:enforce,all:observe,undefined:quick_enforce %s -### 2>&1 | FileCheck %s
// RUN: %clang --target=x86_64-linux-gnu -c -std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 -fsanitize=address,undefined -fsanitize-semantic=address:noexcept_observe,undefined:noexcept_enforce %s -### 2>&1 | FileCheck %s --check-prefix=OK

// P3100: a meta-group skips the members that do not take the semantic, but
// when none of them takes it the request would change nothing, so it is an
// error (notadragon_wg21 DECISIONS-resolved.md E60).  Every member of
// "undefined" is routed, so none takes plain enforce or observe, and the
// routed members of "all" do not either (its non-routed members, such as
// leak or cfi, would take them, but route nothing, so they do not count);
// quick_enforce is taken, so that pair is accepted.  The second RUN line is
// the accepted form: every member of "undefined" takes noexcept_enforce.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-sanitize-semantic-err-group-empty.C
// and p3100-sanitize-semantic-parse-group.C in the gnu_gcc fork.

// CHECK: error: '-fsanitize-semantic=undefined:enforce' is not supported: no members of 'undefined' support 'enforce'
// CHECK-NEXT: note: a throwing handler cannot propagate from a routed sanitizer check; use 'noexcept_enforce' (with '-fcontracts-p4298') or 'quick_enforce'
// CHECK-NEXT: error: '-fsanitize-semantic=all:observe' is not supported: no members of 'all' support 'observe'
// CHECK-NEXT: note: a throwing handler cannot propagate from a routed sanitizer check; use 'noexcept_observe' with '-fcontracts-p4298'
// CHECK-NOT: error:

// OK-NOT: error:
// OK-DAG: "-fsanitize-semantic=address:noexcept_observe"
// OK-DAG: "-fsanitize-semantic=signed-integer-overflow:noexcept_enforce"
