// DEFINE: %{run} = %clangxx -std=c++26 %s %libcxx_flags -o %t
//
// RUN: %{run} -fcontracts-p4451 -fcontract-evaluation-semantic=ignore && %t
// RUN: %{run} -fcontracts-p4451 -fcontract-evaluation-semantic=observe && %t
// RUN: %{run} -fcontracts-p3400 -fcontract-evaluation-semantic=ignore && %t
// RUN: %{run} -fcontracts-p3400 -fcontracts-p4451 -fcontract-evaluation-semantic=observe && %t

// P4451: a terminating contract assertion is enforced under a configured
// ignore or observe in the forms p4451-run.cpp does not cover: after a
// trailing return type, with a deduced return type, on a class template's
// members (defined in and out of class), a generic lambda, a function
// template with a deduced return type, and a contract_assert in a function
// template.  Under -fcontracts-p4451 alone with ignore and with observe,
// -fcontracts-p3400 alone (the P3400 label) with ignore, and both with
// observe.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-run-forms.C (and its
// -observe, -p3400 and -both variants)

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;

static int handled;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.semantic() != evaluation_semantic::enforce)
    std::abort();
  ++handled;
  throw handled;
}

auto tr(int x) -> int post<terminating>(r : r > 0) { return x; }
auto ded(int x) post<terminating>(r : r > 0) { return x; }

template <class T> struct CT {
  T m(T x) pre<terminating>(x > 0) { return x; }
  T d(T x) pre<terminating>(x > 0);
};
template <class T> T CT<T>::d(T x) pre<terminating>(x > 0) { return x; }

auto glam = [](auto x) pre<terminating>(x > 0) { return x; };
template <class T> auto tded(T x) post<terminating>(r : r > 0) { return x; }
template <class T> void tassert(T x) { contract_assert<terminating>(x > 0); }

#define CHECK(E)                                                               \
  do {                                                                         \
    try {                                                                      \
      (void)(E);                                                               \
      std::abort();                                                            \
    } catch (int) {                                                            \
    }                                                                          \
  } while (0)

int main() {
  CHECK(tr(-1));
  CHECK(ded(-1));
  CT<int> c;
  CHECK(c.m(-1));
  CHECK(c.d(-1));
  CHECK(glam(-1));
  CHECK(glam(-1L));
  CHECK(tded(-1));
  CHECK((tassert(-1), 0));
  if (handled != 8)
    std::abort();
  tr(1);
  ded(1);
  c.m(1);
  c.d(1);
  glam(1);
  tded(1);
  tassert(1);
  return handled == 8 ? 0 : 1;
}
