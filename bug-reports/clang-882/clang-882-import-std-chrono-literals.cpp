// CLANG-882: libc++'s `std` and `std.compat` modules do not export the
// using-directive <chrono> declares in namespace std::chrono.
//
// Build the modules from libc++'s installed sources (<prefix>/share/libc++/v1),
// then compile this file against each:
//
//   clang++ -std=c++26 -stdlib=libc++ -Wno-reserved-module-identifier \
//     --precompile <v1>/std.cppm -o std.pcm
//   clang++ -std=c++26 -stdlib=libc++ -Wno-reserved-module-identifier \
//     -fmodule-file=std=std.pcm --precompile <v1>/std.compat.cppm \
//     -o std.compat.pcm
//   clang++ -std=c++26 -stdlib=libc++ -fmodule-file=std=std.pcm \
//     -fsyntax-only clang-882-import-std-chrono-literals.cpp
//   clang++ -std=c++26 -stdlib=libc++ -fmodule-file=std=std.pcm \
//     -fmodule-file=std.compat=std.compat.pcm -DCOMPAT \
//     -fsyntax-only clang-882-import-std-chrono-literals.cpp
//
// Expected: both compile.  Actual: both report "no matching literal operator
// for call to 'operator""h'".  The same program with `#include <chrono>` or
// `import <chrono>;` compiles.

#ifdef COMPAT
import std.compat;
#else
import std;
#endif

using namespace std::chrono;
auto x = 1h;
