// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags -fsyntax-only -Xclang -verify -DHDR_VERSION %s
// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

// libc++'s violation handler is replaceable, so <version> and <contracts>
// define __cpp_lib_replaceable_contract_violation_handler as 202603L.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/ftm-replaceable-handler.C

#ifdef HDR_VERSION
#include <version>
#else
#include <contracts>
#endif

#if !defined(__cpp_lib_replaceable_contract_violation_handler) ||             \
    __cpp_lib_replaceable_contract_violation_handler != 202603L
#error "__cpp_lib_replaceable_contract_violation_handler"
#endif

// REQUIRES: contracts-libcxx
