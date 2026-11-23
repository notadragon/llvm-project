// RUN: %clang -### -std=c++26 -fcontracts-p4405 -fcontracts-prove-optimize=1 -c %s 2>&1 | FileCheck --check-prefix=FWD %s
// RUN: %clang -### -std=c++26 -fcontracts-p4405 -fcontracts-prove-optimize=0 -fcontracts-prove-optimize=follow -c %s 2>&1 | FileCheck --check-prefix=LAST %s
// RUN: not %clang -### -std=c++26 -fcontracts-p4405 -fcontracts-prove-optimize=4 -fcontracts-prove-optimize=s -c %s 2>&1 | FileCheck --check-prefix=BAD %s
// RUN: not %clang -### -std=c++26 -fcontracts -fcontracts-prove-optimize=g -c %s 2>&1 | FileCheck --check-prefix=BAD-OFF %s
// RUN: not %clang -std=c++26 -fcontracts-p4405 -fno-contracts-prove-optimize=2 -fsyntax-only %s 2>&1 | FileCheck --check-prefix=NEG %s
// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-prove-optimize=z -fsyntax-only %s 2>&1 | FileCheck --check-prefix=CC1 %s
// RUN: %clang -std=c++26 -fcontracts -fcontracts-prove-optimize=0 -fsyntax-only %s 2>&1 | FileCheck --check-prefix=NO-EFFECT %s
// RUN: %clang -std=c++26 -fcontracts -fcontracts-prove-optimize=follow -fsyntax-only %s 2>&1 | FileCheck --check-prefix=NO-EFFECT %s
// RUN: %clang -std=c++26 -fcontracts-p4405 -fcontracts-prove-optimize=2 -fsyntax-only %s 2>&1 | FileCheck --allow-empty --check-prefix=QUIET %s

// D4405's proof optimization level (notadragon_wg21 DECISIONS.md E162-E168,
// E205): -fcontracts-prove-optimize= takes 0, 1, 2, 3 or follow (E168 item
// 2: no s, z or g level), the last one winning, and has no -fno- form; a
// bad value is an error whether or not the option has an effect.  Like the
// strategies, it warns that it has no effect without -fcontracts-p4405,
// whatever its value (a joined option is named without its argument).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove-optimize-bad.C,
// -negative.C, -last.C, contract-options-rule-p4405-sub.C

// FWD: "-cc1"
// FWD-SAME: "-fcontracts-prove-optimize=1"

// LAST: "-cc1"
// LAST-NOT: "-fcontracts-prove-optimize=0"
// LAST-SAME: "-fcontracts-prove-optimize=follow"
// LAST-NOT: "-fcontracts-prove-optimize=0"

// BAD: error: invalid value '4' in '-fcontracts-prove-optimize=', expected one of: 0 1 2 3 follow
// BAD: error: invalid value 's' in '-fcontracts-prove-optimize=', expected one of: 0 1 2 3 follow

// BAD-OFF: warning: '-fcontracts-prove-optimize=' has no effect without '-fcontracts-p4405'
// BAD-OFF: error: invalid value 'g' in '-fcontracts-prove-optimize=', expected one of: 0 1 2 3 follow

// NEG: error: unknown argument{{.*}}'-fno-contracts-prove-optimize=2'

// CC1: error: invalid value 'z' in '-fcontracts-prove-optimize=', expected one of: 0 1 2 3 follow

// NO-EFFECT: warning: '-fcontracts-prove-optimize=' has no effect without '-fcontracts-p4405'
// NO-EFFECT-NOT: {{warning|error}}:

// QUIET-NOT: {{warning|error}}:

int i;
