// RUN: FileCheck %s --input-file=%libcxx_libdir/libc++.so

// libc++.so is a linker script that makes libc++'s own dependencies direct
// dependencies of the program.  libcontracts is one of them: libc++.so.1's
// DT_NEEDED on it is not found through the program's DT_RUNPATH, which covers
// only direct dependencies, so a program linked with -rpath and no contracts
// flag would not load.

// CHECK: INPUT({{.*}}-lcontracts{{.*}})
