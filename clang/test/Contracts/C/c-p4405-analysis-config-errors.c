// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-configuration-file=%S/c-p4405-analysis-config-errors.json -fsyntax-only -verify %s

// D4405 in C: the configuration errors of the "analysis" match key, which
// the C and C++ front ends share -- it cannot appear with "caller", and an
// entry with it must have a semantic of ignore, diagnose or prove and no
// dynamic selector.  Nor can an entry for constant evaluation alone
// ("constexpr": true) select prove or diagnose (notadragon_wg21
// DECISIONS.md E142).
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-analysis-config-errors.c

// expected-error@*:* {{'analysis' and 'caller' cannot both appear in 'match' (entry 0 of}}
// expected-error@*:* {{the 'semantic' of an entry matching 'analysis' must be 'ignore', 'diagnose' or 'prove' (entry 1 of}}
// expected-error@*:* {{an entry matching 'analysis' cannot have a 'dynamic' output (entry 2 of}}
// expected-error@*:* {{the 'semantic' of an entry matching 'constexpr' cannot be 'prove' or 'diagnose' (entry 3 of}}

int f(int x) { return x; }
