// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c++26 %s 2>&1 | FileCheck %s --check-prefix=GNU
// RUN: %clang -### --target=arm64-apple-macosx14 -std=c++26 %s 2>&1 | FileCheck %s --check-prefix=GNU
// RUN: %clang -### --target=x86_64-w64-mingw32 -std=c++26 %s 2>&1 | FileCheck %s --check-prefix=GNU
// RUN: %clang -### --target=armv7m-none-eabi -std=c++26 %s 2>&1 | FileCheck %s --check-prefix=GNU
// RUN: %clang -### --target=x86_64-pc-windows-msvc -std=c++26 %s 2>&1 | FileCheck %s --check-prefix=MSVC
// RUN: %clang -### --target=arm64-apple-macosx14 -std=c++26 -nostdlib %s 2>&1 | FileCheck %s --check-prefix=NONE
// RUN: %clang -### --target=x86_64-w64-mingw32 -std=c++26 -fno-contracts %s 2>&1 | FileCheck %s --check-prefix=NONE
// RUN: %clang -### --target=armv7m-none-eabi -std=c++26 -nodefaultlibs %s 2>&1 | FileCheck %s --check-prefix=NONE
// RUN: %clang -### --target=x86_64-pc-windows-msvc -std=c++26 -fno-contracts %s 2>&1 | FileCheck %s --check-prefix=NONE

// Every toolchain that links a C++ standard library links the contracts
// runtime for a translation unit with contracts, as the GNU one does: the
// objects reference it directly.  Not under -nostdlib or -nodefaultlibs, and
// not without contracts.
//
// No GCC mirror: the link line is the only observable, which a DejaGnu test
// cannot inspect.

// GNU: "-lcontracts"
// MSVC: "-defaultlib:contracts.lib"
// NONE-NOT: "-lcontracts"
// NONE-NOT: "-defaultlib:contracts.lib"

int main() { return 0; }
