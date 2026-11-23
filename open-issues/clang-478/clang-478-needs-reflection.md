# CLANG-478: Work waiting for Clang reflection support

**Kind:** deferred (user, 2026-09-25)
**Revisit:** on the first rebase that brings `-freflection` into Clang.

Everything here is blocked only because this branch's Clang has no
reflection.  When a rebase brings it in, work through the list, then delete
this row.  Add to the list, rather than opening new rows, whenever something
else turns out to need reflection.

| Item | What to do |
|------|------------|
| GCC's `g++.dg/contracts/cpp26/p3400-facet-comment-append.C` | Mirror it in `clang/test/Contracts/` (it builds its comment with `-freflection`) |
