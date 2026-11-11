// DEFINE: %{O} = -O0
// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu %{O} -emit-obj
// DEFINE: %{shadow} = -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven
// DEFINE: %{sc} = %{cc1} -fcontract-evaluation-semantic=ignore -o %t.sc.ref.o %S/p4405-shadow-same-code.cpp && \
// DEFINE:   %{cc1} %{shadow} -o %t.sc.shadow.o %S/p4405-shadow-same-code.cpp && cmp %t.sc.ref.o %t.sc.shadow.o && \
// DEFINE:   %{cc1} %{shadow} -fno-contracts-prove-shadow -o %t.sc.noshadow.o %S/p4405-shadow-same-code.cpp && cmp %t.sc.ref.o %t.sc.noshadow.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -o %t.sc.level0.o %S/p4405-shadow-same-code.cpp && cmp %t.sc.ref.o %t.sc.level0.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -o %t.sc.explicit0.o %S/p4405-shadow-same-code.cpp && cmp %t.sc.ref.o %t.sc.explicit0.o
// DEFINE: %{called-once} = %{cc1} -fcontract-evaluation-semantic=ignore -o %t.called-once.ref.o %S/p4405-shadow-same-code-called-once.cpp && \
// DEFINE:   %{cc1} %{shadow} -o %t.called-once.shadow.o %S/p4405-shadow-same-code-called-once.cpp && cmp %t.called-once.ref.o %t.called-once.shadow.o && \
// DEFINE:   %{cc1} %{shadow} -fno-contracts-prove-shadow -o %t.called-once.noshadow.o %S/p4405-shadow-same-code-called-once.cpp && cmp %t.called-once.ref.o %t.called-once.noshadow.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -o %t.called-once.level0.o %S/p4405-shadow-same-code-called-once.cpp && cmp %t.called-once.ref.o %t.called-once.level0.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -o %t.called-once.explicit0.o %S/p4405-shadow-same-code-called-once.cpp && cmp %t.called-once.ref.o %t.called-once.explicit0.o
// DEFINE: %{comdat} = %{cc1} -fcontract-evaluation-semantic=ignore -o %t.comdat.ref.o %S/p4405-shadow-same-code-comdat.cpp && \
// DEFINE:   %{cc1} %{shadow} -o %t.comdat.shadow.o %S/p4405-shadow-same-code-comdat.cpp && cmp %t.comdat.ref.o %t.comdat.shadow.o && \
// DEFINE:   %{cc1} %{shadow} -fno-contracts-prove-shadow -o %t.comdat.noshadow.o %S/p4405-shadow-same-code-comdat.cpp && cmp %t.comdat.ref.o %t.comdat.noshadow.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -o %t.comdat.level0.o %S/p4405-shadow-same-code-comdat.cpp && cmp %t.comdat.ref.o %t.comdat.level0.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -o %t.comdat.explicit0.o %S/p4405-shadow-same-code-comdat.cpp && cmp %t.comdat.ref.o %t.comdat.explicit0.o
// DEFINE: %{frequency} = %{cc1} -fcontract-evaluation-semantic=ignore -o %t.frequency.ref.o %S/p4405-shadow-same-code-frequency.cpp && \
// DEFINE:   %{cc1} %{shadow} -o %t.frequency.shadow.o %S/p4405-shadow-same-code-frequency.cpp && cmp %t.frequency.ref.o %t.frequency.shadow.o && \
// DEFINE:   %{cc1} %{shadow} -fno-contracts-prove-shadow -o %t.frequency.noshadow.o %S/p4405-shadow-same-code-frequency.cpp && cmp %t.frequency.ref.o %t.frequency.noshadow.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -o %t.frequency.level0.o %S/p4405-shadow-same-code-frequency.cpp && cmp %t.frequency.ref.o %t.frequency.level0.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -o %t.frequency.explicit0.o %S/p4405-shadow-same-code-frequency.cpp && cmp %t.frequency.ref.o %t.frequency.explicit0.o
// DEFINE: %{icf} = %{cc1} -fcontract-evaluation-semantic=ignore -o %t.icf.ref.o %S/p4405-shadow-same-code-icf.cpp && \
// DEFINE:   %{cc1} %{shadow} -o %t.icf.shadow.o %S/p4405-shadow-same-code-icf.cpp && cmp %t.icf.ref.o %t.icf.shadow.o && \
// DEFINE:   %{cc1} %{shadow} -fno-contracts-prove-shadow -o %t.icf.noshadow.o %S/p4405-shadow-same-code-icf.cpp && cmp %t.icf.ref.o %t.icf.noshadow.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -o %t.icf.level0.o %S/p4405-shadow-same-code-icf.cpp && cmp %t.icf.ref.o %t.icf.level0.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -o %t.icf.explicit0.o %S/p4405-shadow-same-code-icf.cpp && cmp %t.icf.ref.o %t.icf.explicit0.o
// DEFINE: %{ipa-cp} = %{cc1} -fcontract-evaluation-semantic=ignore -o %t.ipa-cp.ref.o %S/p4405-shadow-same-code-ipa-cp.cpp && \
// DEFINE:   %{cc1} %{shadow} -o %t.ipa-cp.shadow.o %S/p4405-shadow-same-code-ipa-cp.cpp && cmp %t.ipa-cp.ref.o %t.ipa-cp.shadow.o && \
// DEFINE:   %{cc1} %{shadow} -fno-contracts-prove-shadow -o %t.ipa-cp.noshadow.o %S/p4405-shadow-same-code-ipa-cp.cpp && cmp %t.ipa-cp.ref.o %t.ipa-cp.noshadow.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -o %t.ipa-cp.level0.o %S/p4405-shadow-same-code-ipa-cp.cpp && cmp %t.ipa-cp.ref.o %t.ipa-cp.level0.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -o %t.ipa-cp.explicit0.o %S/p4405-shadow-same-code-ipa-cp.cpp && cmp %t.ipa-cp.ref.o %t.ipa-cp.explicit0.o
// DEFINE: %{ipa-sra} = %{cc1} -fcontract-evaluation-semantic=ignore -o %t.ipa-sra.ref.o %S/p4405-shadow-same-code-ipa-sra.cpp && \
// DEFINE:   %{cc1} %{shadow} -o %t.ipa-sra.shadow.o %S/p4405-shadow-same-code-ipa-sra.cpp && cmp %t.ipa-sra.ref.o %t.ipa-sra.shadow.o && \
// DEFINE:   %{cc1} %{shadow} -fno-contracts-prove-shadow -o %t.ipa-sra.noshadow.o %S/p4405-shadow-same-code-ipa-sra.cpp && cmp %t.ipa-sra.ref.o %t.ipa-sra.noshadow.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -o %t.ipa-sra.level0.o %S/p4405-shadow-same-code-ipa-sra.cpp && cmp %t.ipa-sra.ref.o %t.ipa-sra.level0.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -o %t.ipa-sra.explicit0.o %S/p4405-shadow-same-code-ipa-sra.cpp && cmp %t.ipa-sra.ref.o %t.ipa-sra.explicit0.o
// DEFINE: %{numbering} = %{cc1} -fcontract-evaluation-semantic=ignore -o %t.numbering.ref.o %S/p4405-shadow-same-code-numbering.cpp && \
// DEFINE:   %{cc1} %{shadow} -o %t.numbering.shadow.o %S/p4405-shadow-same-code-numbering.cpp && cmp %t.numbering.ref.o %t.numbering.shadow.o && \
// DEFINE:   %{cc1} %{shadow} -fno-contracts-prove-shadow -o %t.numbering.noshadow.o %S/p4405-shadow-same-code-numbering.cpp && cmp %t.numbering.ref.o %t.numbering.noshadow.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -o %t.numbering.level0.o %S/p4405-shadow-same-code-numbering.cpp && cmp %t.numbering.ref.o %t.numbering.level0.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -o %t.numbering.explicit0.o %S/p4405-shadow-same-code-numbering.cpp && cmp %t.numbering.ref.o %t.numbering.explicit0.o
// DEFINE: %{references} = %{cc1} -fcontract-evaluation-semantic=ignore -o %t.references.ref.o %S/p4405-shadow-same-code-references.cpp && \
// DEFINE:   %{cc1} %{shadow} -o %t.references.shadow.o %S/p4405-shadow-same-code-references.cpp && cmp %t.references.ref.o %t.references.shadow.o && \
// DEFINE:   %{cc1} %{shadow} -fno-contracts-prove-shadow -o %t.references.noshadow.o %S/p4405-shadow-same-code-references.cpp && cmp %t.references.ref.o %t.references.noshadow.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -o %t.references.level0.o %S/p4405-shadow-same-code-references.cpp && cmp %t.references.ref.o %t.references.level0.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -o %t.references.explicit0.o %S/p4405-shadow-same-code-references.cpp && cmp %t.references.ref.o %t.references.explicit0.o
// DEFINE: %{unit-growth} = %{cc1} -fcontract-evaluation-semantic=ignore -o %t.unit-growth.ref.o %S/p4405-shadow-same-code-unit-growth.cpp && \
// DEFINE:   %{cc1} %{shadow} -o %t.unit-growth.shadow.o %S/p4405-shadow-same-code-unit-growth.cpp && cmp %t.unit-growth.ref.o %t.unit-growth.shadow.o && \
// DEFINE:   %{cc1} %{shadow} -fno-contracts-prove-shadow -o %t.unit-growth.noshadow.o %S/p4405-shadow-same-code-unit-growth.cpp && cmp %t.unit-growth.ref.o %t.unit-growth.noshadow.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -o %t.unit-growth.level0.o %S/p4405-shadow-same-code-unit-growth.cpp && cmp %t.unit-growth.ref.o %t.unit-growth.level0.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -o %t.unit-growth.explicit0.o %S/p4405-shadow-same-code-unit-growth.cpp && cmp %t.unit-growth.ref.o %t.unit-growth.explicit0.o
// DEFINE: %{vtable} = %{cc1} -fcontract-evaluation-semantic=ignore -o %t.vtable.ref.o %S/p4405-shadow-same-code-vtable.cpp && \
// DEFINE:   %{cc1} %{shadow} -o %t.vtable.shadow.o %S/p4405-shadow-same-code-vtable.cpp && cmp %t.vtable.ref.o %t.vtable.shadow.o && \
// DEFINE:   %{cc1} %{shadow} -fno-contracts-prove-shadow -o %t.vtable.noshadow.o %S/p4405-shadow-same-code-vtable.cpp && cmp %t.vtable.ref.o %t.vtable.noshadow.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -o %t.vtable.level0.o %S/p4405-shadow-same-code-vtable.cpp && cmp %t.vtable.ref.o %t.vtable.level0.o && \
// DEFINE:   %{cc1} %{shadow} -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -o %t.vtable.explicit0.o %S/p4405-shadow-same-code-vtable.cpp && cmp %t.vtable.ref.o %t.vtable.explicit0.o
// RUN: %{sc}
// RUN: %{called-once}
// RUN: %{comdat}
// RUN: %{frequency}
// RUN: %{icf}
// RUN: %{ipa-cp}
// RUN: %{ipa-sra}
// RUN: %{numbering}
// RUN: %{references}
// RUN: %{unit-growth}
// RUN: %{vtable}
// REDEFINE: %{O} = -Og
// RUN: %{sc}
// RUN: %{called-once}
// RUN: %{comdat}
// RUN: %{frequency}
// RUN: %{icf}
// RUN: %{ipa-cp}
// RUN: %{ipa-sra}
// RUN: %{numbering}
// RUN: %{references}
// RUN: %{unit-growth}
// RUN: %{vtable}
// REDEFINE: %{O} = -Os
// RUN: %{sc}
// RUN: %{called-once}
// RUN: %{comdat}
// RUN: %{frequency}
// RUN: %{icf}
// RUN: %{ipa-cp}
// RUN: %{ipa-sra}
// RUN: %{numbering}
// RUN: %{references}
// RUN: %{unit-growth}
// RUN: %{vtable}
// REDEFINE: %{O} = -O2
// RUN: %{sc}
// RUN: %{called-once}
// RUN: %{comdat}
// RUN: %{frequency}
// RUN: %{icf}
// RUN: %{ipa-cp}
// RUN: %{ipa-sra}
// RUN: %{numbering}
// RUN: %{references}
// RUN: %{unit-growth}
// RUN: %{vtable}
// REQUIRES: x86-registered-target

// D4405's proof optimization level (notadragon_wg21 DECISIONS.md E162-E168,
// E166): the object code is the same with the proof shadows as with every
// evaluation ignore, without the shadows, at proof level 0 and with every
// strategy but assume given explicitly at level 0, for the shapes of
// p4405-shadow-same-code.cpp and X12's p4405-shadow-same-code-*.cpp
// (GCC-907's and GCC-995's), and CLANG-914's speculative vtable
// (p4405-shadow-same-code-vtable.cpp): at -O0 and -Og (proofs at level 2,
// above the build's), at -Os (level 2, without -Os's size attributes) and
// at -O2 (the build's own level).  p4405-prove-optimize-same-code.cpp
// compares the IR at a fixed level across -O1, -O3 and -Oz.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove-optimize-same-code-called-once.C,
// -same-code-comdat.C, -same-code-frequency.C, -same-code-icf.C,
// -same-code-ipa-cp.C, -same-code-ipa-sra.C, -same-code-numbering.C,
// -same-code-unit-growth.C

int unused_p4405_prove_optimize_same_code_levels;
