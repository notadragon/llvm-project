// CLANG-10: naming a function parameter inside a requires-expression in a
// contract predicate crashes the compiler.
//
//   clang++ -std=c++26 -fcontracts -fsyntax-only \
//       clang-10-requires-expr-in-contract-ice.cpp
//
//   SemaContract.cpp:1680: ScopeWalker::nextFuncScope():
//     Assertion `FunctionScopeIndex < FunctionScopes.size()' failed.
//
// The crash aborts the translation unit, so the controls below are only
// reached once the offending line is removed.  They are what bound the
// problem: it is the parameter lookup, not requires-expressions as such, and
// not the const rule.

int g;

// THE CRASH.  `pre` does it too, and so does a const parameter -- delete this
// line and uncomment either of the next two to confirm.
void post_case (int p) post (requires { +p; }) { }

// void pre_case (int p) pre (requires { +p; }) { }
// void const_case (const int p) post (requires { +p; }) { }

// CONTROL: a requires-expression naming something that is NOT a parameter
// compiles, which is what points at parameter lookup rather than at
// requires-expressions in general.
void global_case (int p) post (requires { +g; }) { }

// CONTROL: the same parameter named outside a requires-expression compiles.
void plain_case (const int p) post (p > 0) { }

int
main ()
{
  global_case (1);
  plain_case (1);
}
