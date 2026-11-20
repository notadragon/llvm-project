// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontracts-prove-optimize=2
// DEFINE: %{shadow} = -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven
// RUN: %{cc1} %{shadow} -O1 -emit-llvm -o %t.sc-O1.shadow.ll %S/p4405-shadow-same-code.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O1 -emit-llvm -o %t.sc-O1.ignore.ll %S/p4405-shadow-same-code.cpp
// RUN: diff %t.sc-O1.shadow.ll %t.sc-O1.ignore.ll
// RUN: %{cc1} %{shadow} -O3 -emit-llvm -o %t.sc-O3.shadow.ll %S/p4405-shadow-same-code.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O3 -emit-llvm -o %t.sc-O3.ignore.ll %S/p4405-shadow-same-code.cpp
// RUN: diff %t.sc-O3.shadow.ll %t.sc-O3.ignore.ll
// RUN: %{cc1} %{shadow} -Oz -emit-llvm -o %t.sc-Oz.shadow.ll %S/p4405-shadow-same-code.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -Oz -emit-llvm -o %t.sc-Oz.ignore.ll %S/p4405-shadow-same-code.cpp
// RUN: diff %t.sc-Oz.shadow.ll %t.sc-Oz.ignore.ll
// RUN: %{cc1} %{shadow} -O1 -emit-llvm -o %t.sc-called-once-O1.shadow.ll %S/p4405-shadow-same-code-called-once.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O1 -emit-llvm -o %t.sc-called-once-O1.ignore.ll %S/p4405-shadow-same-code-called-once.cpp
// RUN: diff %t.sc-called-once-O1.shadow.ll %t.sc-called-once-O1.ignore.ll
// RUN: %{cc1} %{shadow} -O3 -emit-llvm -o %t.sc-called-once-O3.shadow.ll %S/p4405-shadow-same-code-called-once.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O3 -emit-llvm -o %t.sc-called-once-O3.ignore.ll %S/p4405-shadow-same-code-called-once.cpp
// RUN: diff %t.sc-called-once-O3.shadow.ll %t.sc-called-once-O3.ignore.ll
// RUN: %{cc1} %{shadow} -Oz -emit-llvm -o %t.sc-called-once-Oz.shadow.ll %S/p4405-shadow-same-code-called-once.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -Oz -emit-llvm -o %t.sc-called-once-Oz.ignore.ll %S/p4405-shadow-same-code-called-once.cpp
// RUN: diff %t.sc-called-once-Oz.shadow.ll %t.sc-called-once-Oz.ignore.ll
// RUN: %{cc1} %{shadow} -O1 -emit-llvm -o %t.sc-comdat-O1.shadow.ll %S/p4405-shadow-same-code-comdat.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O1 -emit-llvm -o %t.sc-comdat-O1.ignore.ll %S/p4405-shadow-same-code-comdat.cpp
// RUN: diff %t.sc-comdat-O1.shadow.ll %t.sc-comdat-O1.ignore.ll
// RUN: %{cc1} %{shadow} -O3 -emit-llvm -o %t.sc-comdat-O3.shadow.ll %S/p4405-shadow-same-code-comdat.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O3 -emit-llvm -o %t.sc-comdat-O3.ignore.ll %S/p4405-shadow-same-code-comdat.cpp
// RUN: diff %t.sc-comdat-O3.shadow.ll %t.sc-comdat-O3.ignore.ll
// RUN: %{cc1} %{shadow} -Oz -emit-llvm -o %t.sc-comdat-Oz.shadow.ll %S/p4405-shadow-same-code-comdat.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -Oz -emit-llvm -o %t.sc-comdat-Oz.ignore.ll %S/p4405-shadow-same-code-comdat.cpp
// RUN: diff %t.sc-comdat-Oz.shadow.ll %t.sc-comdat-Oz.ignore.ll
// RUN: %{cc1} %{shadow} -O1 -emit-llvm -o %t.sc-frequency-O1.shadow.ll %S/p4405-shadow-same-code-frequency.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O1 -emit-llvm -o %t.sc-frequency-O1.ignore.ll %S/p4405-shadow-same-code-frequency.cpp
// RUN: diff %t.sc-frequency-O1.shadow.ll %t.sc-frequency-O1.ignore.ll
// RUN: %{cc1} %{shadow} -O3 -emit-llvm -o %t.sc-frequency-O3.shadow.ll %S/p4405-shadow-same-code-frequency.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O3 -emit-llvm -o %t.sc-frequency-O3.ignore.ll %S/p4405-shadow-same-code-frequency.cpp
// RUN: diff %t.sc-frequency-O3.shadow.ll %t.sc-frequency-O3.ignore.ll
// RUN: %{cc1} %{shadow} -Oz -emit-llvm -o %t.sc-frequency-Oz.shadow.ll %S/p4405-shadow-same-code-frequency.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -Oz -emit-llvm -o %t.sc-frequency-Oz.ignore.ll %S/p4405-shadow-same-code-frequency.cpp
// RUN: diff %t.sc-frequency-Oz.shadow.ll %t.sc-frequency-Oz.ignore.ll
// RUN: %{cc1} %{shadow} -O1 -emit-llvm -o %t.sc-icf-O1.shadow.ll %S/p4405-shadow-same-code-icf.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O1 -emit-llvm -o %t.sc-icf-O1.ignore.ll %S/p4405-shadow-same-code-icf.cpp
// RUN: diff %t.sc-icf-O1.shadow.ll %t.sc-icf-O1.ignore.ll
// RUN: %{cc1} %{shadow} -O3 -emit-llvm -o %t.sc-icf-O3.shadow.ll %S/p4405-shadow-same-code-icf.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O3 -emit-llvm -o %t.sc-icf-O3.ignore.ll %S/p4405-shadow-same-code-icf.cpp
// RUN: diff %t.sc-icf-O3.shadow.ll %t.sc-icf-O3.ignore.ll
// RUN: %{cc1} %{shadow} -Oz -emit-llvm -o %t.sc-icf-Oz.shadow.ll %S/p4405-shadow-same-code-icf.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -Oz -emit-llvm -o %t.sc-icf-Oz.ignore.ll %S/p4405-shadow-same-code-icf.cpp
// RUN: diff %t.sc-icf-Oz.shadow.ll %t.sc-icf-Oz.ignore.ll
// RUN: %{cc1} %{shadow} -O1 -emit-llvm -o %t.sc-ipa-cp-O1.shadow.ll %S/p4405-shadow-same-code-ipa-cp.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O1 -emit-llvm -o %t.sc-ipa-cp-O1.ignore.ll %S/p4405-shadow-same-code-ipa-cp.cpp
// RUN: diff %t.sc-ipa-cp-O1.shadow.ll %t.sc-ipa-cp-O1.ignore.ll
// RUN: %{cc1} %{shadow} -O3 -emit-llvm -o %t.sc-ipa-cp-O3.shadow.ll %S/p4405-shadow-same-code-ipa-cp.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O3 -emit-llvm -o %t.sc-ipa-cp-O3.ignore.ll %S/p4405-shadow-same-code-ipa-cp.cpp
// RUN: diff %t.sc-ipa-cp-O3.shadow.ll %t.sc-ipa-cp-O3.ignore.ll
// RUN: %{cc1} %{shadow} -Oz -emit-llvm -o %t.sc-ipa-cp-Oz.shadow.ll %S/p4405-shadow-same-code-ipa-cp.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -Oz -emit-llvm -o %t.sc-ipa-cp-Oz.ignore.ll %S/p4405-shadow-same-code-ipa-cp.cpp
// RUN: diff %t.sc-ipa-cp-Oz.shadow.ll %t.sc-ipa-cp-Oz.ignore.ll
// RUN: %{cc1} %{shadow} -O1 -emit-llvm -o %t.sc-ipa-sra-O1.shadow.ll %S/p4405-shadow-same-code-ipa-sra.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O1 -emit-llvm -o %t.sc-ipa-sra-O1.ignore.ll %S/p4405-shadow-same-code-ipa-sra.cpp
// RUN: diff %t.sc-ipa-sra-O1.shadow.ll %t.sc-ipa-sra-O1.ignore.ll
// RUN: %{cc1} %{shadow} -O3 -emit-llvm -o %t.sc-ipa-sra-O3.shadow.ll %S/p4405-shadow-same-code-ipa-sra.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O3 -emit-llvm -o %t.sc-ipa-sra-O3.ignore.ll %S/p4405-shadow-same-code-ipa-sra.cpp
// RUN: diff %t.sc-ipa-sra-O3.shadow.ll %t.sc-ipa-sra-O3.ignore.ll
// RUN: %{cc1} %{shadow} -Oz -emit-llvm -o %t.sc-ipa-sra-Oz.shadow.ll %S/p4405-shadow-same-code-ipa-sra.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -Oz -emit-llvm -o %t.sc-ipa-sra-Oz.ignore.ll %S/p4405-shadow-same-code-ipa-sra.cpp
// RUN: diff %t.sc-ipa-sra-Oz.shadow.ll %t.sc-ipa-sra-Oz.ignore.ll
// RUN: %{cc1} %{shadow} -O1 -emit-llvm -o %t.sc-numbering-O1.shadow.ll %S/p4405-shadow-same-code-numbering.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O1 -emit-llvm -o %t.sc-numbering-O1.ignore.ll %S/p4405-shadow-same-code-numbering.cpp
// RUN: diff %t.sc-numbering-O1.shadow.ll %t.sc-numbering-O1.ignore.ll
// RUN: %{cc1} %{shadow} -O3 -emit-llvm -o %t.sc-numbering-O3.shadow.ll %S/p4405-shadow-same-code-numbering.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O3 -emit-llvm -o %t.sc-numbering-O3.ignore.ll %S/p4405-shadow-same-code-numbering.cpp
// RUN: diff %t.sc-numbering-O3.shadow.ll %t.sc-numbering-O3.ignore.ll
// RUN: %{cc1} %{shadow} -Oz -emit-llvm -o %t.sc-numbering-Oz.shadow.ll %S/p4405-shadow-same-code-numbering.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -Oz -emit-llvm -o %t.sc-numbering-Oz.ignore.ll %S/p4405-shadow-same-code-numbering.cpp
// RUN: diff %t.sc-numbering-Oz.shadow.ll %t.sc-numbering-Oz.ignore.ll
// RUN: %{cc1} %{shadow} -O1 -emit-llvm -o %t.sc-references-O1.shadow.ll %S/p4405-shadow-same-code-references.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O1 -emit-llvm -o %t.sc-references-O1.ignore.ll %S/p4405-shadow-same-code-references.cpp
// RUN: diff %t.sc-references-O1.shadow.ll %t.sc-references-O1.ignore.ll
// RUN: %{cc1} %{shadow} -O3 -emit-llvm -o %t.sc-references-O3.shadow.ll %S/p4405-shadow-same-code-references.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O3 -emit-llvm -o %t.sc-references-O3.ignore.ll %S/p4405-shadow-same-code-references.cpp
// RUN: diff %t.sc-references-O3.shadow.ll %t.sc-references-O3.ignore.ll
// RUN: %{cc1} %{shadow} -Oz -emit-llvm -o %t.sc-references-Oz.shadow.ll %S/p4405-shadow-same-code-references.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -Oz -emit-llvm -o %t.sc-references-Oz.ignore.ll %S/p4405-shadow-same-code-references.cpp
// RUN: diff %t.sc-references-Oz.shadow.ll %t.sc-references-Oz.ignore.ll
// RUN: %{cc1} %{shadow} -O1 -emit-llvm -o %t.sc-unit-growth-O1.shadow.ll %S/p4405-shadow-same-code-unit-growth.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O1 -emit-llvm -o %t.sc-unit-growth-O1.ignore.ll %S/p4405-shadow-same-code-unit-growth.cpp
// RUN: diff %t.sc-unit-growth-O1.shadow.ll %t.sc-unit-growth-O1.ignore.ll
// RUN: %{cc1} %{shadow} -O3 -emit-llvm -o %t.sc-unit-growth-O3.shadow.ll %S/p4405-shadow-same-code-unit-growth.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O3 -emit-llvm -o %t.sc-unit-growth-O3.ignore.ll %S/p4405-shadow-same-code-unit-growth.cpp
// RUN: diff %t.sc-unit-growth-O3.shadow.ll %t.sc-unit-growth-O3.ignore.ll
// RUN: %{cc1} %{shadow} -Oz -emit-llvm -o %t.sc-unit-growth-Oz.shadow.ll %S/p4405-shadow-same-code-unit-growth.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -Oz -emit-llvm -o %t.sc-unit-growth-Oz.ignore.ll %S/p4405-shadow-same-code-unit-growth.cpp
// RUN: diff %t.sc-unit-growth-Oz.shadow.ll %t.sc-unit-growth-Oz.ignore.ll
// RUN: %{cc1} %{shadow} -O1 -emit-obj -o %t.O1.shadow.o %S/p4405-shadow-same-code.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -O1 -emit-obj -o %t.O1.ignore.o %S/p4405-shadow-same-code.cpp
// RUN: cmp %t.O1.shadow.o %t.O1.ignore.o
// RUN: %{cc1} %{shadow} -Oz -emit-obj -o %t.Oz.shadow.o %S/p4405-shadow-same-code.cpp
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -Oz -emit-obj -o %t.Oz.ignore.o %S/p4405-shadow-same-code.cpp
// RUN: cmp %t.Oz.shadow.o %t.Oz.ignore.o
// REQUIRES: x86-registered-target

// D4405's proof optimization level (notadragon_wg21 DECISIONS.md E162-E168):
// at a fixed proof level, 2, the real code is the same with the proof
// shadows (and the proof module optimized at that level, with what the -O
// level alone implies replaced there: -O1's llvm.loop.unroll.disable, -Oz's
// optsize and minsize) as with every evaluation ignore, whatever the
// code-generation level: -O1 and -Oz (below and beside it) and -O3 (above
// it), for the shapes of p4405-shadow-same-code.cpp and X12's
// p4405-shadow-same-code-*.cpp (GCC-907's and GCC-995's), their IR and, for
// the first, its object code.  Clang's shadows live in a proof module of
// their own; the lists code generation records for it are erased from the
// real module.
//
// Mirror (results PO2-GCC should match; GCC-995's references shapes stay
// out of GCC's, E204): none yet.

int unused_p4405_prove_optimize_same_code;
