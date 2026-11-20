// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-analysis-config-errors.json -fsyntax-only -verify %s

// D4405 ("P3595R1 Analysis Evaluation Configuration"): the configuration
// errors of the "analysis" match key.  It must be a boolean; it cannot
// appear in a match with "caller" (whatever "caller" says); and an entry
// with "analysis": true must have an output semantic of ignore, diagnose
// or prove, and no dynamic selector.  "analysis": false is the same as
// leaving the key out, so it may appear with "caller", and the key itself
// is known (no unknown-key warning).  Each erroneous entry is skipped.  The
// diagnostics describe the configuration, so they have no source location.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-config-errors.C

// expected-error@*:* {{'analysis' in entry 0 of contract configuration from}}
// expected-error@*:* {{'analysis' and 'caller' cannot both appear in 'match' (entry 1 of}}
// expected-error@*:* {{'analysis' and 'caller' cannot both appear in 'match' (entry 2 of}}
// expected-error@*:* {{an entry matching 'analysis' cannot have a 'dynamic' output (entry 3 of}}
// expected-error@*:* {{the 'semantic' of an entry matching 'analysis' must be 'ignore', 'diagnose' or 'prove' (entry 4 of}}
// expected-error@*:* {{the 'semantic' of an entry matching 'analysis' must be 'ignore', 'diagnose' or 'prove' (entry 5 of}}

int f(int x) { return x; }
