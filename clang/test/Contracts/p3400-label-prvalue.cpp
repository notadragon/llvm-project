// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// P3400: a facet label written as a prvalue -- pre<L{}> rather than
// pre<named> -- must have its facets applied.
//
// Reading a facet means constant-evaluating a member call built on the
// label expression.  For a prvalue that materializes a temporary, and the
// resulting cleanup state escaped into the enclosing function.  Two
// symptoms, depending on whether the label expression got transformed a
// second time:
//
//   * post<L{}> with an explicit return type silently applied no facet at
//     all -- the handler simply never ran, with no diagnostic;
//   * with a deduced return type, or with a dependent prvalue label in a
//     template, it crashed outright in ActOnFinishFunctionBody with
//     "Unaccounted cleanups in function".
//
// pre<L{}> and contract_assert<L{}> were unaffected, which is why this
// went unnoticed.
//
// Found while mirroring the GCC tests, where the same
// construct failed differently: GCC dropped the facet for pre as well as
// post, and its crash needed all three of template, prvalue label and
// deduced return type together.

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::violation_handled;

int calls = 0;

struct L {
  using assertion_control_object = L;
  violation_handled handle_contract_violation(const contract_violation &) const {
    ++calls;
    return violation_handled::handled;
  }
};

constexpr L named{};

template <class T> struct wrap {
  using type = L;
};

// Control: the named-label forms, which always worked.
void named_pre(int x) pre<named>(x > 0) {}
int named_post(int x) post<named>(r : r > 0) { return x; }

// pre and contract_assert with a prvalue label: also always worked.
void prvalue_pre(int x) pre<L{}>(x > 0) {}
void prvalue_assert(int x) { contract_assert<L{}>(x > 0); }

// post with a prvalue label, explicit return type: silently applied no facet.
int prvalue_post(int x) post<L{}>(r : r > 0) { return x; }

// post with a prvalue label, deduced return type: crashed.
auto prvalue_post_auto(int x) post<L{}>(r : r > 0) { return x; }

// Dependent prvalue label in a template, explicit return type: crashed.
template <class T>
int tmpl_prvalue(T x) post<typename wrap<T>::type{}>(r : r > 0) {
  return x;
}

// Dependent prvalue label with a deduced return type as well.
template <class T>
auto tmpl_prvalue_auto(T x) post<typename wrap<T>::type{}>(r : r > 0) {
  return x;
}

#define CHECK(CALL)                                                            \
  do {                                                                         \
    calls = 0;                                                                 \
    CALL;                                                                      \
    if (calls != 1)                                                            \
      __builtin_abort();                                                       \
  } while (0)

int main() {
  CHECK(named_pre(-1));
  CHECK(named_post(-1));
  CHECK(prvalue_pre(-1));
  CHECK(prvalue_assert(-1));
  CHECK(prvalue_post(-1));
  CHECK(prvalue_post_auto(-1));
  CHECK(tmpl_prvalue<int>(-1));
  CHECK(tmpl_prvalue_auto<int>(-1));
  return 0;
}
