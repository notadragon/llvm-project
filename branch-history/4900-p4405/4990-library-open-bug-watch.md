---
id: 4990-library-open-bug-watch
subject: '[clang][contracts][P3400] watch tests for the open <contracts> library bugs'
depends: [4200-p3400-core, 4800-p4298, 4900-p4405]
regenerates: []
fixes: []
---

## Rationale
Watch tests for places where libc++'s `<contracts>` and the P3400 facet
reader still differ from the P3400 library wording.  Each is a whole-file
`XFAIL` in `OpenBugs/`, written to the wording, so it starts passing (and
the suite says so) when the library is fixed.  `open-issues/` carries the
writeups.

`identification-label-string-view-range.cpp`,
`identification-label-string-view-groups.cpp` and
`identification-label-pointer-array-combine.cpp` (CLANG-826) use labels
whose `group_names` is a range of `std::string_view`, which P3400 makes an
identification label, and combine labels whose `group_names` is an array of
pointers to strings, whose combined `group_names` P3400 makes the union of
the constituents'.  One file per failing behaviour: the concept, the groups
Sema reads, and the merge.

## Compile gap
None.  A pure test addition.

## Contents
- clang/test/Contracts/OpenBugs/identification-label-pointer-array-combine.cpp : *
- clang/test/Contracts/OpenBugs/identification-label-string-view-groups.cpp : *
- clang/test/Contracts/OpenBugs/identification-label-string-view-range.cpp : *
