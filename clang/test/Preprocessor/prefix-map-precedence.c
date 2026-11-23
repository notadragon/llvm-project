// UNSUPPORTED: system-windows, system-cygwin
// RUN: %clang -E -P -fmacro-prefix-map==MACRO/ -ffile-prefix-map==FILE/ %s \
// RUN:   | FileCheck %s --check-prefix=MACRO-FIRST
// RUN: %clang -E -P -ffile-prefix-map==FILE/ -fmacro-prefix-map==MACRO/ %s \
// RUN:   | FileCheck %s --check-prefix=FILE-FIRST
// RUN: %clang -E -P -fmacro-prefix-map==FIRST/ -fmacro-prefix-map==SECOND/ %s \
// RUN:   | FileCheck %s --check-prefix=TWO-MACRO
// RUN: %clang -E -P -fmacro-prefix-map==SHORT/ -fmacro-prefix-map=././=LONG/ %s \
// RUN:   | FileCheck %s --check-prefix=GREATEST
// RUN: %clang -E -P -fmacro-prefix-map=././=LONG/ -ffile-prefix-map==SHORT/ %s \
// RUN:   | FileCheck %s --check-prefix=GREATEST
// RUN: %clang -E -P -fmacro-prefix-map=./=/mapped %s \
// RUN:   | FileCheck %s --check-prefix=AS-WRITTEN
// RUN: not %clang -E -P -fcanon-prefix-map -fmacro-prefix-map=./=/mapped %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CANON

// Clang's __FILE__ prefix maps, pinned where they differ from GCC's (a
// divergence: open-issues/README.md CLANG-896, accepted by the user under
// notadragon_wg21 DECISIONS.md E188 (a)).  This is upstream Clang's
// behaviour, not the fork's.
//
// -ffile-prefix-map= and -fmacro-prefix-map= go into one map keyed by the
// old prefix and tried greatest key first, so for an equal old prefix the
// first option given wins, whichever it is (g++ tries every
// -ffile-prefix-map= before any -fmacro-prefix-map=, and of two of one kind
// the last given wins).  The old prefix is compared with the name as written:
// "./" matches the first two characters of "././x.c", and only the result
// has its "./" components removed.  There is no -fcanon-prefix-map.
//
// Mirror: gcc/testsuite/c-c++-common/cpp/fmacro-prefix-map-file-1.c,
// -file-2.c, fmacro-prefix-map-last.c and fcanon-prefix-map-1.c, which pin
// g++'s rules.

FILE: __FILE__
#line 100 "././x.c"
LINE: __FILE__

// MACRO-FIRST: FILE: "MACRO/{{.*}}prefix-map-precedence.c"
// MACRO-FIRST: LINE: "MACRO/x.c"

// FILE-FIRST: FILE: "FILE/{{.*}}prefix-map-precedence.c"
// FILE-FIRST: LINE: "FILE/x.c"

// TWO-MACRO: FILE: "FIRST/{{.*}}prefix-map-precedence.c"
// TWO-MACRO: LINE: "FIRST/x.c"

// GREATEST: LINE: "LONG/x.c"

// AS-WRITTEN: LINE: "/mapped./x.c"

// CANON: error: unknown argument: '-fcanon-prefix-map'
