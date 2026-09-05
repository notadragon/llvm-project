# Open Issues on This Branch

What is currently broken on our Clang contracts branch -- whatever its origin.
If you hit something while using this compiler, look here first: a row means
we already know, and tells you whether there is a way around it.

A row is removed, and any file it points to in this directory deleted, as soon
as the issue is **fixed on this branch**. Git history is the record; nothing
is archived in place.

This is a different question from the one
[`../bug-reports/`](../bug-reports/README.md) answers. That directory tracks
bugs that reproduce on **stock upstream** Clang, including ones we have
already fixed here, and a row there survives until *upstream* fixes it. An
upstream bug we have not yet fixed appears in both; its single writeup lives
in `bug-reports/` and the row below links to it.

Contracts are not upstream in Clang, so a contracts-dependent bug has nothing
upstream to reproduce against and is not tracked in `bug-reports/`. Such
issues are branch-only by definition and their writeups live here.

**Next ID:** CLANG-12

IDs come from one sequence per compiler, shared with `bug-reports/`, and are
never reused. Allocate from the line above and increment it. Both tables
delete rows, so the highest ID visible in either is not a reliable counter.

**Kind** is one of `defect` (wrong, and we intend to fix it), `deferred`
(known, deliberately not fixed yet) or `divergence` (GCC and Clang disagree
and the standard does not clearly settle which is right).

| ID | Symptom | Kind | Workaround | Upstream | Details |
|----|---------|------|------------|----------|---------|
| CLANG-2 | Under `-fsanitize=bool -fsanitize-recover=bool`, an invalid `_Bool` load keeps its raw bits and reads as true; GCC coerces it to `false` | divergence | do not rely on the recovered value; the report itself is identical on both | UNKNOWN | [../bug-reports/clang-02-ubsan-bool-recover.md](../bug-reports/clang-02-ubsan-bool-recover.md) |
| CLANG-5 | A by-value return object is never destroyed when a local's destructor throws after it is built | defect | none known; GCC does destroy it | UNKNOWN | [../bug-reports/clang-05-retval-not-destroyed-on-throwing-cleanup.md](../bug-reports/clang-05-retval-not-destroyed-on-throwing-cleanup.md) |
| CLANG-8 | `this` is accepted in the declaration of an explicit-object member function, where it is ill-formed | defect | name the object parameter instead | UNKNOWN | [../bug-reports/clang-08-this-in-xobj-declaration.md](../bug-reports/clang-08-this-in-xobj-declaration.md) |
| CLANG-9 | Constant evaluation accepts forming a member's or non-virtual base's address before its constructor begins | defect | none; the program is accepted silently | UNKNOWN | [../bug-reports/clang-09-member-address-before-ctor.md](../bug-reports/clang-09-member-address-before-ctor.md) |
| CLANG-11 | A redeclaration whose parameter type is dependent escapes the postcondition `const` rule | defect | declare the parameter `const` on every declaration | -- | [clang-11-postcondition-redecl-dependent-param.md](clang-11-postcondition-redecl-dependent-param.md) |
