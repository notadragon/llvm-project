// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-dynamic-bad-name-chars.json -fsyntax-only %s 2>&1 | FileCheck %s --implicit-check-not=error:

// A P3595 dynamic selector name is emitted as a symbol (C linkage) or mangled
// into one (C++ linkage), so it must be an identifier, or identifiers
// separated by "::" for C++; anything else is rejected when the configuration
// is parsed.  A template-id, an operator-function-id, a parameter list, a
// component starting with a digit or containing a space, and an empty or
// qualified C name used to reach the object file as an invalid symbol
// (GCC-671 on GCC).  The last two entries are valid names, with digits, '$'
// and '_' where an identifier allows them, and are not diagnosed.
// p3595-dynamic-bad-name.cpp covers empty components.
// (GCC mirror: g++.dg/contracts/cpp26/p3595-dynamic-bad-name-chars.C)

// CHECK: error: 'name' 'ns::sel<int>' in 'dynamic' in entry 0 {{.*}} is not a valid qualified name
// CHECK: error: 'name' 'ns::operator()' in 'dynamic' in entry 1 {{.*}} is not a valid qualified name
// CHECK: error: 'name' 'sel(int)' in 'dynamic' in entry 2 {{.*}} is not a valid qualified name
// CHECK: error: 'name' 'ns::1sel' in 'dynamic' in entry 3 {{.*}} is not a valid qualified name
// CHECK: error: 'name' 'a b' in 'dynamic' in entry 4 {{.*}} is not a valid qualified name
// CHECK: error: 'name' 'sel<int>' in 'dynamic' in entry 5 {{.*}} is not a valid identifier
// CHECK: error: 'name' 'ns::sel' in 'dynamic' in entry 6 {{.*}} is not a valid identifier
// CHECK: error: 'name' '1sel' in 'dynamic' in entry 7 {{.*}} is not a valid identifier
// CHECK: error: 'name' '' in 'dynamic' in entry 8 {{.*}} is not a valid identifier

void f(int x) pre(x > 0) {}
