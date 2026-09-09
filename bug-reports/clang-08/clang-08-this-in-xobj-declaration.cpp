// clang-08-this-in-xobj-declaration.cpp                             -*-C++-*-
//
// CLANG-8: `this` is incorrectly accepted in the declaration of an
// explicit-object member function (trailing return type and
// noexcept-specifier).  No contracts involved, plain C++23.
//
// [expr.prim.this]/3 (as amended by P0847R7): the keyword `this` "shall not
// appear within the declaration of either a static member function or an
// EXPLICIT OBJECT member function of the current class (although its type and
// value category are defined within such member functions as they are within
// an implicit object member function)".
//
// One sentence, two kinds of function.  A trailing return type, a
// noexcept-specifier and a requires-clause are all within the declaration, so
// `this` is ill-formed in each of them for BOTH kinds.  The static half is the
// control: a compiler that rejects the static case and accepts the
// explicit-object case is applying half of one sentence.
//
// Select with -DCASE=n.

struct S
{
  int x;
  static int s;

  // ---- explicit object member function -------------------------------
#if CASE == 1
  // Trailing return type.
  auto f (this S &self) -> decltype (this->x);
#elif CASE == 2
  // noexcept-specifier.
  void f (this S &self) noexcept (noexcept (this->x));
#elif CASE == 3
  // requires-clause.  NOTE: stock clang-trunk rejects this row already, but
  // for an unrelated reason ("non-templated function cannot have a requires
  // clause") that has nothing to do with `this` -- not usable as evidence
  // either way for this bug.
  void f (this S &self)
    requires (sizeof (this) > 0);
#elif CASE == 4
  // Body.  Both compilers already reject this.
  void
  f (this S &self)
  {
    (void) this;
  }

  // ---- static member function: the control ---------------------------
#elif CASE == 5
  // Trailing return type.
  static auto g () -> decltype (this->x);
#elif CASE == 6
  // noexcept-specifier.
  static void g () noexcept (noexcept (this->x));
#elif CASE == 7
  // requires-clause.  NOTE: same unrelated confound as CASE 3 -- stock
  // clang-trunk rejects a non-templated function's requires-clause
  // regardless of `this`, so this row isn't clean evidence either.
  static void g ()
    requires (sizeof (this) > 0);
#elif CASE == 8
  // Body.  Both compilers already reject this.
  static void
  g ()
  {
    (void) this;
  }

  // ---- implicit object member function: must all be ACCEPTED ---------
  // (except CASE 11, the requires-clause row: stock clang-trunk rejects it
  // for an unrelated reason -- "non-templated function cannot have a
  // requires clause" -- independent of `this`, so it is not part of this
  // bug's clean control set.)
#elif CASE == 9
  auto h () -> decltype (this->x);
#elif CASE == 10
  void h () noexcept (noexcept (this->x));
#elif CASE == 11
  // Excluded from the control set -- see note above.
  void h ()
    requires (sizeof (this) > 0);
#else
#error "define CASE"
#endif
};
