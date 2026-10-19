// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: cp %t/ignore.json %t/config.json
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%t/config.json -x c++-header -emit-pch -o %t/h.pch %t/h.h
// RUN: cp %t/enforce.json %t/config.json
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%t/config.json -include-pch %t/h.pch -fsyntax-only -verify=changed %t/m.cpp
// RUN: cp %t/ignore-rewritten.json %t/config.json
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%t/config.json -include-pch %t/h.pch -fsyntax-only -verify=rewritten %t/m.cpp

// A header is precompiled with a contract configuration file that makes
// every contract ignore, and the contents of that same file then change
// before a translation unit using the PCH is compiled with it.  Clang does
// not record the configuration in the PCH: the translation unit's
// configuration applies, as for -fcontract-evaluation-semantic
// (pch-contract-semantic.cpp).  So when the new contents enforce, the
// violated precondition is diagnosed in constant evaluation; when the file
// is instead rewritten with other text but the same configuration, the PCH
// is used and the precondition is not evaluated.
//
// Mirror: gcc/testsuite/g++.dg/pch/contract-config-changed.{C,Hs,json} and
// contract-config-rewritten.{C,Hs,json} in the gnu_gcc fork (GCC records the
// parsed configuration and rejects the PCH when it changed);
// tests/tests/contracts/config/pch_file in the edgcpp_compiler fork.

//--- ignore.json
[{"output": {"semantic": "ignore"}}]

//--- ignore-rewritten.json
[
  {
    "output" : { "semantic" : "ignore" }
  }
]

//--- enforce.json
[{"output": {"semantic": "enforce"}}]

//--- h.h
constexpr int g(int x) pre(x > 0) { return x; }

//--- m.cpp
// rewritten-no-diagnostics
// changed-error@*:* {{contract failed during execution of constexpr function}}
constexpr int v = g(-1); // changed-error {{constexpr variable 'v' must be initialized by a constant expression}} changed-note {{in call to 'g(-1)'}}
