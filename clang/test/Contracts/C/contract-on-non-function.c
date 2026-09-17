// A D4299 contract written on something that is not a function declaration
// must be diagnosed rather than crash the compiler.  Two distinct assertions
// are reachable, depending on whether the declarator carries a function chunk
// at all:
//
//   int x _Pre(1);              -> getFunctionTypeInfo()'s "Not a function
//                                  declarator!", if the eager parse reaches
//                                  for parameters that do not exist;
//   int (*p)(void) _Pre(1);     -> Declarator::clear()'s "Late-parsed
//   typedef int F(void) _Pre(1);   contracts unhandled", if tokens are cached
//                                  on the strength of the function chunk and
//                                  then never replayed, because what the
//                                  declarator declares is a variable or a
//                                  typedef.
//
// (A struct member takes neither path -- C parses struct declarators
// elsewhere and already rejects a contract there as a plain syntax error.)
//
// Same family as param-declarator-contract.c: a contract with nothing to
// attach to must be an error, not a crash.

// RUN: %clang_cc1 -fcontracts-p4299 -fsyntax-only -verify %s

int gv _Pre(1); // expected-error {{'_Pre' can only appear on a function declaration}}

int gv_post _Post(r : 1); // expected-error {{'_Post' can only appear on a function declaration}}

int (*gfp)(void) _Pre(1); // expected-error {{'_Pre' can only appear on a function declaration}}

typedef int GF(void) _Pre(1); // expected-error {{'_Pre' can only appear on a function declaration}}

// A contract on a real function declaration is still accepted, and the
// declarations around the rejected ones still parse.
int ok(int a) _Pre(a > 0);
int after;
