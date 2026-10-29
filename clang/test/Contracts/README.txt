Tests for C++26 contracts (P2900) and the extensions this branch implements
(P3097, P3098, P3099, P3100, P3290, P3400, P3595, P4283, P4298, P4299, P4301).

Layout
  *.cpp, *.c   Compile-only tests (-verify, FileCheck on IR or AST), and
               tests that link and run (older ones; new ones go in Runnable/).
  Runnable/    Every test builds a program and runs it.  The directory is
               unsupported unless the libc++ and contracts runtime built with
               this compiler are present and the target runs natively
               (Runnable/lit.local.cfg).
  C/           Contracts in C (-fcontracts-p4299).
  Sema/        Semantic-analysis tests; those named for a section of the
               working draft (decl.contract.func.cpp) check its examples.
  OpenBugs/    Watch tests for open defects, XFAIL: * until fixed.  lit marks
               a whole file XFAIL, so a control belongs in a passing file next
               to the watch (for example this-in-member-declaration-controls.cpp),
               never inside it.  Each has a GCC mirror; see open-issues/ at the
               top of the repository.

Substitutions (lit.local.cfg)
  %libcxx_flags   -stdlib=libc++ plus -L and -rpath for the runtimes directory
                  of this build (lib/<target triple>), which holds libc++ and
                  libcontracts.  A test that uses it outside Runnable/ says
                  `REQUIRES: contracts-libcxx`, and `native` as well if it runs
                  what it built; both lines sit at the end of the file so that
                  they do not move the line numbers other checks rely on.
  %libcxx_libdir  That directory alone.

Generated files
  matrix-*.cpp are written by contract-matrix-gen.py (in the notadragon_wg21
  repository, src/pubs/impl/p3850impl/); regenerate them rather than editing
  them by hand.  They are the only tests here that pass
  -verify-ignore-unexpected=note.

Conventions
  * Pin the evaluation semantic on the RUN line
    (-fcontract-evaluation-semantic=...) when the test depends on it.
  * A run test that expects termination checks that its handler ran (a marker
    on stderr, matched by FileCheck), so that some other trap cannot pass it.
  * A run test reports a failed check with __builtin_abort() (or a non-zero
    exit), never assert(): -DNDEBUG removes it, and P3290 turns it into a
    contract assertion whose semantic and handler the test may be changing.
  * A test that mirrors one in the GCC fork names it in a comment
    ("GCC mirror: g++.dg/contracts/cpp26/...").
