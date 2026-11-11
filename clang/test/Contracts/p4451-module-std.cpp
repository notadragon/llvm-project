// DEFINE: %{v} = -fcontracts-p4451
// DEFINE: %{cxx} = %clangxx -std=c++26 %{v} %libcxx_flags
// DEFINE: %{build} = %{cxx} -Wno-reserved-module-identifier --precompile %libcxx_moduledir/std.cppm -o %t/std.pcm && \
// DEFINE:   %{cxx} -Wno-reserved-module-identifier -fmodule-file=std=%t/std.pcm \
// DEFINE:   --precompile %libcxx_moduledir/std.compat.cppm -o %t/std.compat.pcm
// DEFINE: %{check} = %{cxx} -fmodule-file=std=%t/std.pcm -fsyntax-only -Xclang -verify %s && \
// DEFINE:   %{cxx} -fmodule-file=std=%t/std.pcm -fmodule-file=std.compat=%t/std.compat.pcm \
// DEFINE:   -DCOMPAT -fsyntax-only -Xclang -verify %s
//
// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: %{build}
// RUN: %{check}
// REDEFINE: %{v} = -fcontracts-p3400
// RUN: %{build}
// RUN: %{check}
// REDEFINE: %{v} = -fcontracts-p3400 -fcontracts-p4451
// RUN: %{build}
// RUN: %{check}
// REQUIRES: contracts-libcxx, libcxx-std-module

// expected-no-diagnostics

// P4451: `import std;` and `import std.compat;` declare
// std::contracts::labels::terminating, so a `<terminating>`
// assertion-control-specifier after either is accepted.  Under
// -fcontracts-p4451 alone, -fcontracts-p3400 alone (the label, found through
// the exported contract_control directive) and both, the std modules built
// with the same options.
//
// Mirror: gcc/testsuite/g++.dg/modules/contract-p4451-std.C and
// contract-p4451-std-compat.C (and their -p3400 and -both variants)

#ifdef COMPAT
import std.compat;
#else
import std;
#endif

int f(int x) pre<terminating>(x > 0) post<terminating>(r : r > 0) {
  contract_assert<terminating>(x > 1);
  return x;
}

struct S {
  int m(int x) pre<terminating>(x > 0) { return x; }
};

constexpr auto &t = std::contracts::labels::terminating;
