// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: %clangxx -std=c++26 %libcxx_flags -Wno-reserved-module-identifier \
// RUN:   --precompile %libcxx_moduledir/std.cppm -o %t/std.pcm
// RUN: %clangxx -std=c++26 %libcxx_flags -Wno-reserved-module-identifier \
// RUN:   -fmodule-file=std=%t/std.pcm \
// RUN:   --precompile %libcxx_moduledir/std.compat.cppm -o %t/std.compat.pcm
// RUN: %clangxx -std=c++26 %libcxx_flags -fmodule-file=std=%t/std.pcm \
// RUN:   -fsyntax-only %s
// RUN: %clangxx -std=c++26 %libcxx_flags -fmodule-file=std=%t/std.pcm \
// RUN:   -fmodule-file=std.compat=%t/std.compat.pcm -DCOMPAT -fsyntax-only %s
// REQUIRES: contracts-libcxx, libcxx-std-module

// CLANG-882 (bug-reports/; fixed here, open upstream).
//
// D3400R6 [std.modules]: `std` exports the using-directives the importable
// library headers provide, and `std.compat` the same.  <chrono> declares
// `using namespace literals::chrono_literals;` in std::chrono, so after
// `using namespace std::chrono;` the literal `1h` is found.  Stock libc++'s
// modules/std/chrono.inc does not export that directive, so both imports
// fail there with "no matching literal operator"; this branch exports it.

#ifdef COMPAT
import std.compat;
#else
import std;
#endif

using namespace std::chrono;
auto x = 1h;
