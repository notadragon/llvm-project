// CLANG-14: a pure virtual in a class template whose contract predicate is
// value-dependent at parse time reaches CodeGen still dependent.
//
//   clang++ -std=c++26 -fcontracts -fcontracts-p3097 -c <this file>
//   ExprConstant.cpp:22215: Assertion `!isValueDependent() &&
//     "Expression evaluator can't be called on a dependent expression."'
//
// -fsyntax-only is NOT enough: the crash is in CodeGen, so the whole
// syntax-only probe matrix that found CLANG-13 walked straight past this.
//
// The predicate has to be value-dependent when parsed.  `n >= 0` qualifies (n
// is a member of a dependent class) and so does `sizeof (T) == 4`; `f () == 0`
// and `true` do not, and compile clean.

template <class T> struct A {
  virtual int get() const pre(n >= 0) = 0;
  int n;
};

struct D : A<int> {
  int get() const override { return n; }
};

int main() {
  D d;
  A<int> &a = d;
  return a.get();
}
