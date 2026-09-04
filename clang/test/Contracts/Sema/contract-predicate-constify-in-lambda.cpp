// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s -fcontracts

// [expr.prim.id.unqual]/3+d reaches inside a lambda that appears in the
// predicate: the paragraph's own example marks `++n`, on a namespace-scope
// `int n`, an error inside `pre([=,&i,*this] mutable {...})`.
//
// Clang gets the variable cases right as a consequence of llvm 31b784c4d29f,
// which removed the automatic-storage restriction from
// getContractConstification.  GCC needed a dedicated fix for the lambda case
// (gnu_gcc 5442adee87a), because its gate asked whether the INNERMOST binding
// level was the contract scope, which stops being true the moment a lambda
// pushes its own.  So most of this file is a regression pin on the Clang side
// of a bug that was real on the other.
//
// TWO SHAPES ARE NOT HERE, because Clang is wrong about them.  They are
// XFAILed in contract-predicate-constify-in-lambda-gaps.cpp: a member reached
// through a captured `*this` (which the paper marks OK and Clang rejects) and
// a function-local static (which the paper makes an error and Clang accepts,
// though it gets that one right when the static is named directly).
//
// GCC mirror: g++.dg/contracts/cpp26/contract-predicate-constify-lambda.C

int n = 0;
thread_local int t_n = 0;

struct HasStatic {
  static int s;
};
int HasStatic::s = 0;

struct X {
  bool m();
};

struct Y {
  void f(int i, int *p, int &r, X x, X *px)
      pre([=, &i]() mutable {
        // expected-error@+1 {{cannot assign to variable 'n' because it is considered 'const' inside of a contract}}
        ++n;
        // expected-error@+1 {{cannot assign to a variable captured by reference which was captured as const because it is inside a contract}}
        ++i;
        ++p; // OK: a member of the closure type
        ++r; // OK: a non-reference member of the closure type
        (void)x;
        (void)px;

        int j = 17; // declared INSIDE the predicate, so not constified
        ++j;        // OK

        [&]() {
          int k = 34;
          // expected-error@+1 {{cannot assign to a variable captured by reference which was captured as const because it is inside a contract}}
          ++i;
          ++j; // OK
          ++k; // OK
        }();
        return true;
      }()) {}
};

// The storage durations Clang does constify, named from inside a lambda in the
// predicate rather than directly.  Each is "a variable declared outside of C".
void from_lambda() {
  // expected-error@+1 {{cannot assign to variable 'n' because it is considered 'const' inside of a contract}}
  contract_assert([]() { ++n; return true; }());
  // expected-error@+1 {{cannot assign to variable 't_n' because it is considered 'const' inside of a contract}}
  contract_assert([]() { ++t_n; return true; }());
  // expected-error@+1 {{cannot assign to variable 's' because it is considered 'const' inside of a contract}}
  contract_assert([]() { ++HasStatic::s; return true; }());
}

// CONTROL: named directly in the predicate, all of them including a
// function-local static -- which is the case that makes the lambda gap in the
// companion file a lambda gap rather than a storage-duration one.
void named_directly() {
  static int local_static = 0;
  // expected-error@+1 {{cannot assign to variable 'n' because it is considered 'const' inside of a contract}}
  contract_assert(++n);
  // expected-error@+1 {{cannot assign to variable 'local_static' because it is considered 'const' inside of a contract}}
  contract_assert(++local_static);
}

// CONTROL: outside a contract entirely, a lambda constifies nothing.
void not_in_a_contract() {
  []() {
    ++n;
    ++t_n;
    ++HasStatic::s;
  }();
}
