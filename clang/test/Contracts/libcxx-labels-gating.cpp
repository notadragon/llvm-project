// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags -fsyntax-only -Xclang -verify=none %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -fsyntax-only -Xclang -verify=p3400 -DP3400 %s
//
// The same through the std module, built under each option set.
// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags -Wno-reserved-module-identifier \
// RUN:   --precompile %libcxx_moduledir/std.cppm -o %t/std-none.pcm
// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags -fmodule-file=std=%t/std-none.pcm \
// RUN:   -DMODULE -fsyntax-only -Xclang -verify=none %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -Wno-reserved-module-identifier \
// RUN:   --precompile %libcxx_moduledir/std.cppm -o %t/std-p3400.pcm
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -fmodule-file=std=%t/std-p3400.pcm \
// RUN:   -DMODULE -DP3400 -fsyntax-only -Xclang -verify=p3400 %s
// REQUIRES: contracts-libcxx, libcxx-std-module

// libc++'s <contracts> and the std module declare P3400's labels namespace,
// and the contract_control directive, only under -fcontracts-p3400
// (CLANG-883).

#ifdef MODULE
import std;
#else
#include <contracts>
#endif

#ifdef P3400
// p3400-no-diagnostics
constexpr auto a = std::contracts::labels::empty_label;
constexpr auto b = std::contracts::labels::terminating;
void f(int x) pre<terminating>(x > 0); // the directive's short name
#else
auto x = std::contracts::labels::empty_label; // none-error {{no member named 'labels'}}
#endif
