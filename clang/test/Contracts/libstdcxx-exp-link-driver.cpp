// RUN: rm -rf %t && mkdir -p %t/exp/lib/gcc/x86_64-unknown-linux-gnu/14
// RUN: mkdir -p %t/noexp/lib/gcc/x86_64-unknown-linux-gnu/14
// RUN: touch %t/exp/lib/gcc/x86_64-unknown-linux-gnu/14/crtbegin.o
// RUN: touch %t/exp/lib/gcc/x86_64-unknown-linux-gnu/14/libstdc++exp.a
// RUN: touch %t/noexp/lib/gcc/x86_64-unknown-linux-gnu/14/crtbegin.o
//
// RUN: %clangxx -### --target=x86_64-unknown-linux-gnu -std=c++26 \
// RUN:   -stdlib=libstdc++ --sysroot=%t/exp \
// RUN:   --gcc-install-dir=%t/exp/lib/gcc/x86_64-unknown-linux-gnu/14 %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=EXP
// RUN: %clangxx -### --target=x86_64-unknown-linux-gnu -std=c++26 \
// RUN:   -stdlib=libstdc++ -static-libstdc++ --sysroot=%t/exp \
// RUN:   --gcc-install-dir=%t/exp/lib/gcc/x86_64-unknown-linux-gnu/14 %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=STATIC
// RUN: %clangxx -### --target=x86_64-unknown-linux-gnu -std=c++23 \
// RUN:   -fcontracts-p3400 -stdlib=libstdc++ --sysroot=%t/exp \
// RUN:   --gcc-install-dir=%t/exp/lib/gcc/x86_64-unknown-linux-gnu/14 %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=EXP
//
// No libstdc++exp: no contracts, the -nostdlib family, libc++, a GCC
// installation without the archive, and a link the C driver makes.
// RUN: %clangxx -### --target=x86_64-unknown-linux-gnu -std=c++26 \
// RUN:   -fno-contracts -stdlib=libstdc++ --sysroot=%t/exp \
// RUN:   --gcc-install-dir=%t/exp/lib/gcc/x86_64-unknown-linux-gnu/14 %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NONE
// RUN: %clangxx -### --target=x86_64-unknown-linux-gnu -std=c++23 \
// RUN:   -stdlib=libstdc++ --sysroot=%t/exp \
// RUN:   --gcc-install-dir=%t/exp/lib/gcc/x86_64-unknown-linux-gnu/14 %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NONE
// RUN: %clangxx -### --target=x86_64-unknown-linux-gnu -std=c++26 \
// RUN:   -stdlib=libstdc++ -nostdlib --sysroot=%t/exp \
// RUN:   --gcc-install-dir=%t/exp/lib/gcc/x86_64-unknown-linux-gnu/14 %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NONE
// RUN: %clangxx -### --target=x86_64-unknown-linux-gnu -std=c++26 \
// RUN:   -stdlib=libstdc++ -nodefaultlibs --sysroot=%t/exp \
// RUN:   --gcc-install-dir=%t/exp/lib/gcc/x86_64-unknown-linux-gnu/14 %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NONE
// RUN: %clangxx -### --target=x86_64-unknown-linux-gnu -std=c++26 \
// RUN:   -stdlib=libstdc++ -nostdlib++ --sysroot=%t/exp \
// RUN:   --gcc-install-dir=%t/exp/lib/gcc/x86_64-unknown-linux-gnu/14 %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NONE
// RUN: %clangxx -### --target=x86_64-unknown-linux-gnu -std=c++26 \
// RUN:   -stdlib=libc++ --sysroot=%t/exp \
// RUN:   --gcc-install-dir=%t/exp/lib/gcc/x86_64-unknown-linux-gnu/14 %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NONE
// RUN: %clangxx -### --target=x86_64-unknown-linux-gnu -std=c++26 \
// RUN:   -stdlib=libstdc++ --sysroot=%t/noexp \
// RUN:   --gcc-install-dir=%t/noexp/lib/gcc/x86_64-unknown-linux-gnu/14 %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NOEXP
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c++26 \
// RUN:   -stdlib=libstdc++ --sysroot=%t/exp \
// RUN:   --gcc-install-dir=%t/exp/lib/gcc/x86_64-unknown-linux-gnu/14 %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NONE

// With libstdc++, a link with contracts on also links libstdc++exp, which
// holds libstdc++'s C++ contracts runtime (the contract_violation accessors
// and the default violation handler), as g++ does: the default handler and
// the noexcept dispatch barrier forced in with -u, since libcontracts refers
// to them only weakly, and the archive before libstdc++ and libcontracts.
// Only when the GCC installation has the archive.
//
// Mirror: gcc/cp/g++spec.cc (lang_specific_driver, need_experimental); the
// link line is not observable from a DejaGnu test.

// EXP: "-u" "__contract_invoke_default_handler"
// EXP-SAME: "-u" "__contract_dispatch_core_noexcept" "-lstdc++exp" "-lstdc++"
// EXP-SAME: "-lcontracts"

// STATIC: "-u" "__contract_dispatch_core_noexcept" "-lstdc++exp"
// STATIC-SAME: "-Bstatic" "-lstdc++" "-Bdynamic"

// NONE-NOT: "-lstdc++exp"
// NONE-NOT: "__contract_invoke_default_handler"

// NOEXP-NOT: "-lstdc++exp"
// NOEXP: "-lstdc++"
// NOEXP-SAME: "-lcontracts"

int main() { return 0; }
