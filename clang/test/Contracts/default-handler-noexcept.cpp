// The library-provided default contract-violation handler is noexcept: it
// cannot throw. (GCC mirror: g++.dg/contracts/cpp26/contract-violation-noexcept.C,
// which verifies the same property by forcing the default handler's diagnostics
// to throw and confirming std::terminate runs.)
// RUN: %clangxx -std=c++26 -fsyntax-only -fcontracts %s %libcxx_flags

#include <contracts>

// Binding to a noexcept function pointer only succeeds if the callee is noexcept.
void (*kDefaultHandler)(const std::contracts::contract_violation&) noexcept =
    &std::contracts::invoke_default_contract_violation_handler;
