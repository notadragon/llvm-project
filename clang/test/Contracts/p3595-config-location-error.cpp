// RUN: %clangxx -std=c++26 %s -fcontracts \
// RUN:   -fcontract-configuration-file=%S/p3595-config-location-error.json \
// RUN:   %libcxx_flags -fsyntax-only

// P3595 x config: a malformed line-range list in a "location" match criterion
// (":10x" trailing junk, ":1-2z" trailing junk after a range, ":5-" a range
// with no upper bound) must not send the parser into an infinite loop.
//
// KNOWN DIVERGENCE from GCC: GCC
// diagnoses each malformed range against the JSON source and skips the entry;
// Clang's location parser (llvm::StringRef::split + getAsInteger) cannot hang
// and silently coerces malformed input (leaving the parsed value at 0) rather
// than diagnosing it.  This test therefore only pins the essential guarantee --
// no hang, clean compile -- and documents the no-diagnostic difference.
// (GCC mirror: g++.dg/contracts/cpp26/p3595-config-location-error.C.)

int f(int x) { return x; }
