// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s

// A postcondition result name "_" is name-independent ([basic.scope.scope]:
// a result binding), and [basic.scope.contract] only restricts a
// result-name-introducer that is not name-independent; so it may share the
// name of a parameter "_" or of a lambda's init-capture "_".  (From the EDG
// M16b shapes.)
//
// Mirrors: gcc/testsuite/g++.dg/contracts/cpp26/result-name-shadows-parameter.C
// (the parameter) and result-name-init-capture.C (the init-capture).

// expected-no-diagnostics

int h(int _) post(_ : true);
int s = [_ = 0](int y) post(_ : true) { return y; }(1);
int t = [](int _) post(_ : true) { return 1; }(1);
