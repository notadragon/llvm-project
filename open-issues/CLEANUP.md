# Cleanup Owed on This Branch

Code that works but should be tidied: style, comments, naming, dead code,
duplication, organization.  Nothing here changes behaviour.  Once an item
turns out to be wrong behaviour it stops being a cleanup: record it as a
defect in [`README.md`](README.md) (with a writeup and a watch test) and
delete it here.

IDs come from the same sequence as `README.md` and `../bug-reports/`, and are
never reused.  A row is deleted when the cleanup lands; git history is the
record.  **Where** gives the file and, where one was recorded, a line number
as of the `360ffd98d8e0` head -- lines drift, so search by content.  Rows marked
"(unverified)" were recorded by a review that could not confirm the
observation still holds; check before acting.

| ID | Cleanup | Where |
|----|---------|-------|
