# CLANG-18: The AST matchers this branch adds are missing from the generated reference

**Kind:** deferred
**Status:** Open
**Affects:** `clang/test/AST/ast_matchers_updated.test`, which fails on this
branch in any configuration
**Workaround:** none needed for using the compiler; this is a documentation
artefact, not behaviour.

## Symptom

`ast_matchers_updated.test` regenerates the AST-matcher reference from the
matcher registry and diffs it against the checked-in
`clang/docs/LibASTMatchersReference.html`.  This branch registers two matchers
that the checked-in HTML does not mention, so the diff is non-empty and the
test fails:

```
- <tr>... contractSpecifierDecl ... Matcher<ContractSpecifierDecl>...</tr>
- <tr>... resultNameDecl        ... Matcher<ResultNameDecl>...</tr>
```

## Why it matters

It is the only thing in the tree that checks the matcher registry and its
published reference agree, and it is a hard requirement for upstreaming: a
patch adding a matcher without regenerating the reference does not land.
Fixing it is mechanical -- regenerate the HTML from the registry -- but it has
to be redone whenever the matcher set changes, so it wants doing once the
matcher set is final rather than now.

## Why it went unnoticed

`bin/clang/test.sh --all` runs `clang/test/Contracts/` and nothing else, so
until `--clang-full` was added (2026-09-16) the whole 26,171-test clang suite
had never been run against this branch at all.  This is one of two failures it
found immediately; the other is CLANG-19.

Ours, and there is nothing to file -- the matchers are this branch's.
