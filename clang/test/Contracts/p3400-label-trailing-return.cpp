// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3850 -fcontracts-p3400 -fsyntax-only -verify %s
// expected-no-diagnostics

// A P3400 assertion-control label on a contract that follows a trailing return
// type was dropped: the trailing-return type-id's abstract-declarator parse
// swallowed `pre<lbl>` (a labelled contract looks like a template-id), so the
// enclosing declarator never saw the contract. Mirrors.

struct L { using assertion_control_object = L; };
constexpr L lbl{};

// Labelled contract after a trailing return type (the fixed case).
auto f(int a) -> int pre<lbl>(a > 0) post<lbl>(r : r > 0) { return a; }

// Control: labelled contract on a leading return type (already worked).
int g(int a) pre<lbl>(a > 0);

// Control: bare (unlabelled) contract after a trailing return type (already
// worked).
auto h(int a) -> int pre(a > 0) { return a; }
