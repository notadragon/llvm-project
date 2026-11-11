// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3099 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3400 x templates: labels on members of a class template whose facets
// differ per instantiation of the class.  The label is the class's own
// static data member, of a type chosen by the class's parameter: one
// instantiation's label computes ignore, one handles the violation locally,
// one computes the message, and one has no facets at all.  A member function
// template and a member of a nested class template use the same label.
// Each instantiation must apply its own label's facets, not another's.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-class-template-facets.C

#include <contracts>
#include <cstring>

using namespace std::contracts;

static int local_calls, global_calls;
static const char *message;

struct ignore_t {
  using assertion_control_object = ignore_t;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return evaluation_semantic::ignore;
  }
};
struct local_t {
  using assertion_control_object = local_t;
  violation_handled
  handle_contract_violation(const contract_violation &) const {
    ++local_calls;
    return violation_handled::handled;
  }
};
struct message_t {
  using assertion_control_object = message_t;
  constexpr const char *compute_message(const char *) const {
    return "from label";
  }
};
struct plain_t {
  using assertion_control_object = plain_t;
};

template <class T> struct pick { using type = plain_t; };
template <> struct pick<char> { using type = ignore_t; };
template <> struct pick<int> { using type = local_t; };
template <> struct pick<long long> { using type = message_t; };

void handle_contract_violation(const contract_violation &v) {
  ++global_calls;
  message = v.message();
}

template <class T> struct S {
  static constexpr typename pick<T>::type lab{};
  void f(int x) pre<lab>(x > 0, "written") {}
  template <class U> void g(U x) pre<lab>(x > 0, "written") {}
  template <class U> struct N {
    void h(int x) pre<lab>(x > 0, "written") {}
  };
};

static bool counts(int local, int global, const char *msg) {
  bool ok = local_calls == local && global_calls == global &&
            (msg ? message && std::strcmp(message, msg) == 0 : true);
  local_calls = global_calls = 0;
  message = nullptr;
  return ok;
}

template <class T> static bool run(int local, int global, const char *msg) {
  S<T>().f(0);
  S<T>().g(0.0);
  typename S<T>::template N<void>().h(0);
  return counts(local, global, msg);
}

int main() {
  if (!run<char>(0, 0, nullptr))
    __builtin_abort();
  if (!run<int>(3, 0, nullptr))
    __builtin_abort();
  if (!run<long long>(0, 3, "from label"))
    __builtin_abort();
  if (!run<short>(0, 3, "written"))
    __builtin_abort();
}
