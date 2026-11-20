// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4299 -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// -fcontracts-p4299 in C++: _Pre, _Post and _ContractAssert mean pre, post and
// contract_assert on a member function, a function template, a lambda, and
// with a P3400 label, each checked; ../p4299-in-cxx.cpp covers a plain
// function.  (From the EDG fork's p4299/spellings_run.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4299-in-cxx-forms.C

#include <contracts>

static int violations = 0;
static unsigned lines[16];
void handle_contract_violation(const std::contracts::contract_violation &v) {
  lines[violations++] = v.location().line();
}

struct label_t { using assertion_control_object = label_t; };
constexpr label_t lbl{};

struct S {
  int m(int x) const _Pre(x != 1) _Post(r : r != 1) { return x; } // line 22
};
template <class T> T t(T x) _Pre(x != 2) _Post(r : r != 2) { // line 24
  _ContractAssert(x != 2);                                    // line 25
  return x;
}
int lab(int x) _Pre<lbl>(x != 3) _Post<lbl>(r : r != 3) { // line 28
  _ContractAssert<lbl>(x != 3);                            // line 29
  return x;
}

int main() {
  auto l = [](int x) _Pre(x != 4) _Post(r : r != 4) { // line 34
    _ContractAssert(x != 4);                          // line 35
    return x;
  };
  S().m(0);
  t(0);
  lab(0);
  l(0);
  if (violations != 0)
    __builtin_abort();
  S().m(1);
  if (violations != 2 || lines[0] != 22 || lines[1] != 22)
    __builtin_abort();
  t(2);
  if (violations != 5 || lines[2] != 24 || lines[3] != 25 || lines[4] != 24)
    __builtin_abort();
  lab(3);
  if (violations != 8 || lines[5] != 28 || lines[6] != 29 || lines[7] != 28)
    __builtin_abort();
  l(4);
  if (violations != 11 || lines[8] != 34 || lines[9] != 35 || lines[10] != 34)
    __builtin_abort();
}
