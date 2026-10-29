// RUN: %contracts_verify_test -fsyntax-only

// A contract_assert inside a lambda, inside a function template, whose
// predicate is itself an immediately-invoked lambda capturing both an
// enclosing local and one of the enclosing lambda's parameters.  Pins that
// the construct is accepted at instantiation.
//
// The only diagnostics are incidental to the shape: the outer lambda is
// converted to a function pointer by the unary + and then discarded, which
// is an unused expression, and the warning carries an instantiation
// backtrace.

template <class T>
void foo () {
  +[](int p, int pppp) { // expected-warning {{expression result unused}}
    int local = 202;

    contract_assert(
        [&]
        (int p2) { return

        local; }
        (
            p)
            );
    return 0;

  };
}
template void foo<int>(); // expected-note {{in instantiation of function template specialization 'foo<int>' requested here}}
