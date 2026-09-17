// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
//
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4283 \
// RUN:   %t/direct.cpp -fsyntax-only -verify
//
// Serialization round-trip: exercises ContractStmt::CreateEmpty, which sizes
// the same trailing-object array on the deserialization path.
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4283 \
// RUN:   %t/mod.cppm -emit-module-interface -o %t/mod.pcm
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4283 \
// RUN:   -fprebuilt-module-path=%t %t/use.cpp -fsyntax-only -verify

// P4283 x attributes: a contract may carry BOTH an attribute and a requires
// clause.  ContractStmt stores its sub-statements and its attributes in one
// trailing-object allocation, and the offset of the attribute array depends on
// whether a requires-clause slot is present.  If HasRequiresClause is not set
// before the attributes are copied in, the attributes are written one slot
// short of where every later read looks for them -- so the requires clause
// overwrites the first attribute and getAttrs() returns uninitialized memory
// (an assertion failure in getAttrAs<>, a wild pointer in a release build).
//
// Note a requires clause is only permitted on a templated function, so every
// case below is a template.

//--- direct.cpp
// expected-no-diagnostics
template<typename T> concept Integral = __is_integral(T);
template<typename T> concept Signed = Integral<T> && __is_signed(T);

// pre: attribute after the requires clause.
template<typename T>
T pre_attr_after(T x)
  pre requires(Integral<T>) [[clang::contract_group("g1")]] (x > 0)
{ return x; }

// post: same, with a result name.  (A parameter named in a postcondition must
// be const.)
template<typename T>
T post_attr_after(const T x)
  post requires(Integral<T>) [[clang::contract_group("g2")]] (r: r == x)
{ return x; }

// contract_assert inside a template.
template<typename T>
void assert_attr(T x) {
  contract_assert requires(Signed<T>) [[clang::contract_group("g3")]] (x != T{});
}

// More than one attribute, to catch an off-by-one that only shows past index 0.
// (contract_group and contract_semantic are mutually exclusive, so pair the
// group with a message instead.)
template<typename T>
T multi_attr(T x)
  pre requires(Integral<T>)
      [[clang::contract_group("g4")]] [[clang::contract_message("m4")]]
      (x >= 0)
{ return x; }

// (The attribute must follow the requires clause: the grammar is
// `pre requires(C) [[attr]] (predicate)`, so `pre [[attr]] requires(C) (...)`
// is a syntax error and is not exercised here.)

template int pre_attr_after<int>(int);
template int post_attr_after<int>(const int);
template void assert_attr<int>(int);
template int multi_attr<int>(int);

//--- mod.cppm
export module mod;
export template<typename T> concept Integral = __is_integral(T);

export template<typename T>
T serialized(T x)
  pre requires(Integral<T>) [[clang::contract_group("mod")]] (x > 0)
{ return x; }

//--- use.cpp
// expected-no-diagnostics
import mod;
int use() { return serialized(1); }
