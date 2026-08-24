// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3850 -fsyntax-only -verify %s
// expected-no-diagnostics

// [class.mem.general]p7: a contract assertion on a member function is a
// complete-class context, so its predicate may name members declared later in
// the class -- exactly as a member function body may.  That works by caching
// the contract's tokens and replaying them at the end of the class.
//
// The caching decision is made in ParseFunctionDeclarator, which looks at the
// token sitting directly after the declarator.  A virt-specifier-seq and a
// trailing requires-clause are both parsed *after* that point, so a contract
// written behind either of them was reached by a different path that parsed it
// eagerly -- and every member declared after it became "use of undeclared
// identifier".
//
// The tests are therefore organised around WHAT SEPARATES THE DECLARATOR FROM
// THE CONTRACT, not around virtualness: `Virtual` below is a virtual function
// whose contract was always accepted, and `Override` is the same function with
// `override` written on it.  Each case names a member declared after the
// contract, since that is the whole point; a case that named an earlier member
// would pass whether the contract was deferred or not.

struct Base {
  virtual ~Base();
  virtual int vf() const;
  virtual int wf() const;
};

// ---------------------------------------------------------------------------
// Nothing between the declarator and the contract.  These always worked and
// are here as controls: a fix that deferred too eagerly, or not at all, breaks
// them rather than the cases below.
// ---------------------------------------------------------------------------

struct Plain {
  int f() const pre(g() >= 0);
  int g() const;
};

struct PlainDtor {
  ~PlainDtor() pre(g() >= 0);
  int g() const;
};

struct Virtual : Base {
  virtual int vf() const pre(g() >= 0);
  int g() const;
};

// ---------------------------------------------------------------------------
// A virt-specifier between the declarator and the contract.
// ---------------------------------------------------------------------------

struct Override : Base {
  int vf() const override pre(g() >= 0);
  int g() const;
};

struct Final : Base {
  int vf() const final pre(g() >= 0);
  int g() const;
};

struct OverrideDtor : Base {
  ~OverrideDtor() override pre(g() >= 0);
  int g() const;
};

struct OverridePost : Base {
  int vf() const override post(r : r >= h());
  int h() const;
};

struct OverrideBoth : Base {
  int vf() const override pre(g() >= 0) post(r : r <= h());
  int g() const;
  int h() const;
};

struct OverrideNoexcept : Base {
  int vf() const noexcept override pre(g() >= 0);
  int g() const;
};

struct OverrideTrailingReturn : Base {
  auto vf() const -> int override pre(g() >= 0);
  int g() const;
};

struct OverrideDefinition : Base {
  int vf() const override pre(g() >= 0) { return g(); }
  int g() const;
};

struct OverridePure : Base {
  int vf() const override pre(g() >= 0) = 0;
  int g() const;
};

// The deferral is per-declarator, so a member-declarator-list must not lose it
// for the second and later declarators, whose Declarator is recycled.
struct OverrideAfterOtherMembers : Base {
  int a, b;
  int vf() const override pre(g() >= 0);
  int g() const;
};

template <class X>
struct OverrideInClassTemplate : Base {
  int vf() const override pre(g() >= 0);
  int g() const;
};

template struct OverrideInClassTemplate<int>;

// Two overriders in one class, so the second is parsed with the first already
// cached.
struct TwoOverriders : Base {
  int vf() const override pre(g() >= 0);
  int wf() const override post(r : r <= h());
  int g() const;
  int h() const;
};

// ---------------------------------------------------------------------------
// A trailing requires-clause between the declarator and the contract.  Not a
// virt-specifier and not a complete-class context itself, but it sits in the
// other arm of the same if/else and so was broken the same way.
// ---------------------------------------------------------------------------

struct Requires {
  template <class T>
  int f() const requires (sizeof(T) > 0) pre(g() >= 0);
  int g() const;
};

struct RequiresPost {
  template <class T>
  int f() const requires (sizeof(T) > 0) post(r : r <= h());
  int h() const;
};

// ---------------------------------------------------------------------------
// POSITIVE CONTROL: same virt-specifier, but the named member comes first, so
// no deferral is needed to find it.  This isolates the defect to WHEN the
// contract is parsed rather than to what is visible from it.
// ---------------------------------------------------------------------------

struct MemberDeclaredFirst : Base {
  int k() const;
  int vf() const override pre(k() >= 0);
};
