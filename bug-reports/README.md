# Open Upstream Bugs

Bugs found during this implementation that reproduce on stock upstream
Clang, independent of anything in this branch. Each links to a
self-contained report-ready writeup plus a reproducer. A row is removed
(and its file deleted) once the bug is fixed on upstream main, regardless
of who fixed it or whether it was ever formally filed.

| Bug | Summary | Status | Upstream Link | Details |
|-----|---------|--------|----------------|---------|
| CLANG-1 | Constant evaluator accepts converting to a virtual base through an object outside its lifetime | Fixed here | UNKNOWN | [clang-01-constexpr-vbase-lifetime.md](clang-01-constexpr-vbase-lifetime.md) |
| CLANG-5 | Returned object is not destroyed when a local's destructor throws during a return statement | Open | UNKNOWN | [clang-05-retval-not-destroyed-on-throwing-cleanup.md](clang-05-retval-not-destroyed-on-throwing-cleanup.md) |
| CLANG-8 | `this` accepted in the declaration of an explicit-object member function | Open | UNKNOWN | [clang-08-this-in-xobj-declaration.md](clang-08-this-in-xobj-declaration.md) |
| CLANG-9 | Constexpr evaluator accepts forming a non-virtual-base or direct member's address before its non-trivial constructor begins | Open | UNKNOWN | [clang-09-member-address-before-ctor.md](clang-09-member-address-before-ctor.md) |
