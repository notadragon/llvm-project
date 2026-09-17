// clang-05b-retval-leak-all-return-forms.cpp                         -*-C++-*-
//
// CLANG-5, scope control.  The primary reproducer
// (`clang-05-retval-not-destroyed-on-throwing-cleanup.cpp') uses a named
// return object, so on its own it leaves open the two questions upstream
// will ask first: is this an NRVO artifact, and does the optimizer paper
// over it?  Neither.  All three return forms leak, at every -O level.
//
//   clang++ -std=c++17 clang-05b-retval-leak-all-return-forms.cpp
//   ./a.out; echo $?      -> 7   (all three leak)
//
//   g++     -std=c++17 clang-05b-retval-leak-all-return-forms.cpp
//   ./a.out; echo $?      -> 0
//
// The exit code is a bitmask so one run reports all three independently:
//
//   bit 0 (1)  `return r;'          -- named local, NRVO candidate
//   bit 1 (2)  `return Counted();'  -- prvalue, guaranteed elision (C++17)
//   bit 2 (4)  `return s;'          -- copy from a reference, no elision possible
//
// MEASURED 2026-09-02:
//
//   clang++-trunk 24.0.0git 4c176c47d8be   -O0 -> 7    -O2 -> 7
//   g++-trunk     17.0.0 20260901          -O0 -> 0    -O2 -> 0
//
// So the defect is in how Clang scopes the returned object's cleanup, not in
// any elision path in particular.  See
// clang-05-retval-not-destroyed-on-throwing-cleanup.md (CLANG-5) in this
// directory, for the [except.ctor]/2 citation and the cost analysis of a fix.

int live = 0;

struct Counted {
  Counted () { ++live; }
  Counted (const Counted &) { ++live; }
  ~Counted () { --live; }
};

struct Throw {
  ~Throw () noexcept (false) { throw 42; }
};

Counted nrvo    ()                  { Throw g; Counted r; return r; }
Counted prvalue ()                  { Throw g; return Counted (); }
Counted copy    (const Counted &s)  { Throw g; return s; }

int
main ()
{
  int bad = 0;

  try { nrvo (); }    catch (int) { }  if (live) { bad |= 1; live = 0; }
  try { prvalue (); } catch (int) { }  if (live) { bad |= 2; live = 0; }

  {
    Counted s;                  // `s' itself stays live for the whole block,
    try { copy (s); }           // so the leaked return object shows up as a
    catch (int) { }             // count of 2 rather than 1.
    if (live != 1)
      bad |= 4;
  }

  return bad;                   // GCC: 0.  Clang: 7.
}
