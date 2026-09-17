// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-assume-attr-quick.json %libcxx_flags -o %t && not --crash %t

// P3100: [[assume]] with quick_enforce -- a false side-effect-free predicate
// fails fast (llvm.trap).
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assume-attr-quick.C)

int f(int x) { [[assume(x > 0)]]; return x; }

int main() { return f(-1); }
