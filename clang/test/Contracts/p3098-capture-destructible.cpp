// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s

// P3098: a postcondition capture is an object with automatic storage
// duration.  CodeGen destroys it when the call ends -- Runnable/p3098-dtor.cpp
// pins the exact order -- so the captured type must be destructible from the
// context of the capture, not merely copyable.
//
// The capture's copy-initialization catches a non-copyable type; its
// destructor has to be checked as well, as for a lambda's capture, or an
// inaccessible one is accepted and then called.
//
// Each group below pairs the destructibility case with the copyability case
// that already worked, so a fix that accidentally routes both through the same
// check still has to keep both diagnostics.

bool ok(int);

// -- Inaccessible destructor -----------------------------------------------

struct PrivateDtor {
  int v = 1;
  PrivateDtor() = default;
  PrivateDtor(const PrivateDtor &) = default;

private:
  ~PrivateDtor() = default; // expected-note 2 {{declared private here}}
};

void param_private_dtor(PrivateDtor p)
    post [p] (ok(p.v)) // expected-error {{variable of type 'PrivateDtor' has private destructor}}
{}

void init_private_dtor(PrivateDtor p)
    post [q = p] (ok(q.v)) // expected-error {{variable of type 'PrivateDtor' has private destructor}}
{}

// -- Deleted destructor ----------------------------------------------------

struct DeletedDtor {
  int v = 1;
  DeletedDtor() = default;
  DeletedDtor(const DeletedDtor &) = default;
  ~DeletedDtor() = delete; // expected-note {{explicitly marked deleted here}}
};

void param_deleted_dtor(DeletedDtor p)
    post [p] (ok(p.v)) // expected-error {{attempt to use a deleted function}}
{}

// -- Non-copyable: the half that already worked ----------------------------

struct NoCopy {
  int v = 1;
  NoCopy() = default;
  NoCopy(const NoCopy &) = delete; // expected-note {{has been explicitly marked deleted here}}
};

void param_noncopyable(NoCopy p)
    post [p] (ok(p.v)) // expected-error {{call to deleted constructor of 'NoCopy'}}
{}

// -- explicit-only copy constructor ----------------------------------------
// A capture is COPY-initialized, not direct-initialized, so an explicit copy
// constructor is not a candidate.  This is the row that tells the two apart.

struct ExplicitCopy {
  int v = 1;
  ExplicitCopy() = default; // expected-note {{candidate constructor not viable: requires 0 arguments, but 1 was provided}}
  explicit ExplicitCopy(const ExplicitCopy &) = default; // expected-note {{explicit constructor is not a candidate}}
};

void param_explicit_copy(ExplicitCopy p)
    post [p] (ok(p.v)) // expected-error {{no matching constructor for initialization of 'ExplicitCopy'}}
{}

// -- Destructible and copyable: no diagnostic ------------------------------

struct Fine {
  int v = 1;
  Fine() = default;
  Fine(const Fine &) = default;
  ~Fine() = default;
};

void param_fine(Fine p) post [p] (ok(p.v)) {}
void init_fine(Fine p) post [q = p] (ok(q.v)) {}

// A capture of scalar type has no destructor to check at all.
void param_scalar(int p) post [p] (ok(p)) {}
