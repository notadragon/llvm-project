// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4283 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// P3400: a label on a *member function*'s contract must accept the same
// expressions a free function's does.
//
// A member's contract specifier is token-cached and re-parsed once the class
// is complete, and the caching loop in LateParseFunctionContractSpecifier
// consumed every token of the label with plain ConsumeToken().  That asserts
// on "special" tokens -- braces, parens, brackets, string literals and
// annotations -- so any label with punctuation in it crashed the parser:
//
//   pre<L{}>  pre<make_label()>  pre<L{"tag"}>  pre<TL<1>{}>  post<L{}>
//
// Only a bare named label survived.  A free function is parsed eagerly and
// was unaffected, which is why p3400-label-prvalue.cpp never caught this.
//
// The same loop also counted a '<' or '>' appearing *inside* a parenthesized
// subexpression as a template-argument-list delimiter, which would have ended
// the label early.  That defect was never observable on its own -- the '('
// crashed first -- but caching the group whole fixes both at once, so it is
// covered here.  LateParseContractRequiresClause's cacheAngles had the same
// pair of defects, so a requires-clause naming a template-id with a brace or
// a paren among its arguments crashed as well.
//
// Found while mirroring gnu_gcc ddc6c726bae (p3400-label-prvalue-template.C),
// whose class-template-member case tripped this on the way past.  The two
// bugs are unrelated: that one was about materializing a prvalue label inside
// a template, this one is about caching a member's tokens at all.

#include <contracts>
#include <cstddef>
#include <cstring>

using std::contracts::contract_violation;
using std::contracts::evaluation_semantic;
using std::contracts::violation_handled;

static int calls = 0;
static const char *last_tag = nullptr;

struct L {
  using assertion_control_object = L;
  static constexpr int key = 0;
  const char *tag = "plain";
  constexpr L() {}
  constexpr L(const char *t) : tag(t) {}
  void *query(const void *k, std::size_t i) const {
    return (i == 0 && k == &key) ? (void *)tag : nullptr;
  }
};

template <int N> struct TL {
  using assertion_control_object = TL;
  static constexpr int key = 0;
  void *query(const void *k, std::size_t i) const {
    return (i == 0 && k == &key) ? (void *)(N == 1 ? "TL1" : "TL2") : nullptr;
  }
};

constexpr L named{"named"};
constexpr L alt{"alt"};
constexpr L make_label() { return L{"made"}; }

template <class T> concept Anything = true;

void handle_contract_violation(const contract_violation &v) {
  ++calls;
  last_tag = (const char *)v.query_control_object(&L::key);
  if (!last_tag)
    last_tag = (const char *)v.query_control_object(&TL<1>::key);
}

// Every contract below is on a member function, so every one is late-parsed.
struct S {
  // Control: a bare named label, the only form that used to survive.
  static void bare(int x) pre<named>(x > 0) {}

  // Braces, parens, string literals, nested template-ids.
  static void brace(int x) pre<L{}>(x > 0) {}
  static void paren(int x) pre<make_label()>(x > 0) {}
  static void str(int x) pre<L{"tagged"}>(x > 0) {}
  static void tmplid(int x) pre<TL<1>{}>(x > 0) {}
  static int post_brace(int x) post<L{}>(r : r > 0) { return x; }

  // A '>' and a '<' inside a parenthesized subexpression are operators, not
  // the end of the label.  Picking the second operand proves the whole
  // expression was cached rather than truncated at the inner '>'.
  static void gt_in_paren(int x) pre<(1 > 2 ? named : alt)>(x > 0) {}
  static void lt_in_paren(int x) pre<(1 < 2 ? named : alt)>(x > 0) {}

  // A ';' inside a lambda body must not end the caching either.
  static void lambda(int x) pre<([] { return L{"iife"}; }())>(x > 0) {}
};

// The same, as members of a class template -- which is also where the
// requires-clause cases have to live, since P4283 only permits a
// requires-clause on a contract of a templated function.
template <class T> struct TS {
  static void brace(T x) pre<L{}>(x > 0) {}
  static void tmplid(T x) pre<TL<1>{}>(x > 0) {}

  // A requires-clause whose template-id arguments contain a brace / a paren:
  // cacheAngles had both defects the label loop did.
  static void req_brace(T x)
      pre<named> requires Anything<decltype(L{})>(x > 0) {}
  static void req_paren(T x)
      pre<named> requires Anything<int (*)()>(x > 0) {}
};

static void expect(const char *want) {
  if (calls != 1)
    __builtin_abort();
  if (!last_tag || std::strcmp(last_tag, want) != 0)
    __builtin_abort();
}

#define CHECK(CALL, WANT)                                                      \
  do {                                                                         \
    calls = 0;                                                                 \
    last_tag = nullptr;                                                        \
    CALL;                                                                      \
    expect(WANT);                                                              \
  } while (0)

int main() {
  CHECK(S::bare(-1), "named");
  CHECK(S::brace(-1), "plain");
  CHECK(S::paren(-1), "made");
  CHECK(S::str(-1), "tagged");
  CHECK(S::tmplid(-1), "TL1");
  CHECK(S::post_brace(-1), "plain");

  // 1 > 2 is false and 1 < 2 is true, so these must select different labels.
  CHECK(S::gt_in_paren(-1), "alt");
  CHECK(S::lt_in_paren(-1), "named");

  CHECK(S::lambda(-1), "iife");

  CHECK(TS<int>::brace(-1), "plain");
  CHECK(TS<int>::tmplid(-1), "TL1");
  CHECK(TS<int>::req_brace(-1), "named");
  CHECK(TS<int>::req_paren(-1), "named");

  return 0;
}
