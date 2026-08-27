// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3400: an inaccessible facet member means the type does not participate in
// that facet.  It is not an error.
//
// The facet concepts are requires-expressions, and one is false for a member
// the caller cannot name, so "absent" is the answer the library gives and the
// front end has to give the same one.  D3400R5 is explicit that this must not
// be ill-formed: a label may carry private helpers that happen to share a
// facet's name -- tag-dispatch overloads, say -- and those must not break the
// program.
//
// Previously every one of these was a hard error, reported against the
// contract.  The cause was subtle: contracts are parsed as part of a
// declarator, where Clang *delays* access checking so it can be redone once
// the declaration's context is known.  Detection ran its probe inside an
// SFINAE trap and saw no access failure at all -- the check was not skipped,
// it was parked, and it fired later, after the trap was gone.  Detection now
// drops inaccessible candidates before probing, so no such check is ever
// queued.
//
// Each case is pinned twice: the concept's own answer via static_assert, and
// the observed behaviour, so the two cannot drift apart.  A private
// assertion_control_object is a different matter and stays ill-formed -- such
// a type is not a control object at all.

#include <contracts>
#include <cstddef>
#include <cstring>

using std::contracts::contract_violation;
using std::contracts::evaluation_semantic;
namespace lbl = std::contracts::labels;

static int key = 1;
static int global_calls = 0;
static int local_calls = 0;
static const char *last_comment = nullptr;
static const char *last_message = nullptr;
static bool last_query = false;

void handle_contract_violation(const contract_violation &v) {
  ++global_calls;
  last_comment = v.comment();
  last_message = v.message();
  last_query = v.query_control_object(&key, 0) != nullptr;
}

#define PRIVATE_LABEL(NAME, MEMBER)                                            \
  struct NAME {                                                                \
    using assertion_control_object = NAME;                                     \
                                                                               \
  private:                                                                     \
    MEMBER                                                                     \
  };
#define PUBLIC_LABEL(NAME, MEMBER)                                             \
  struct NAME {                                                                \
    using assertion_control_object = NAME;                                     \
    MEMBER                                                                     \
  };

#define COMMENT_M                                                              \
  constexpr const char *compute_comment(const char *) const { return "FIRED"; }
#define MESSAGE_M                                                              \
  constexpr const char *compute_message(const char *) const { return "FIRED"; }
#define QUERY_M                                                                \
  void *query(const void *k, std::size_t) const {                              \
    return k == &key ? (void *)&key : nullptr;                                 \
  }
#define HANDLER_M                                                              \
  void handle_contract_violation(const contract_violation &) const {           \
    ++local_calls;                                                             \
  }
#define SEM_M                                                                  \
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {  \
    return evaluation_semantic::ignore;                                        \
  }

PRIVATE_LABEL(priv_comment_t, COMMENT_M) PUBLIC_LABEL(pub_comment_t, COMMENT_M)
PRIVATE_LABEL(priv_message_t, MESSAGE_M) PUBLIC_LABEL(pub_message_t, MESSAGE_M)
PRIVATE_LABEL(priv_query_t, QUERY_M) PUBLIC_LABEL(pub_query_t, QUERY_M)
PRIVATE_LABEL(priv_handler_t, HANDLER_M) PUBLIC_LABEL(pub_handler_t, HANDLER_M)
PRIVATE_LABEL(priv_sem_t, SEM_M) PUBLIC_LABEL(pub_sem_t, SEM_M)

// Ground truth: the concepts say a private member is not a facet, and the
// public counterpart is.  Every behavioural check below must agree with these.
static_assert(!lbl::compute_comment_label<priv_comment_t>);
static_assert(lbl::compute_comment_label<pub_comment_t>);
static_assert(!lbl::compute_message_label<priv_message_t>);
static_assert(lbl::compute_message_label<pub_message_t>);
static_assert(!lbl::queryable_label<priv_query_t>);
static_assert(lbl::queryable_label<pub_query_t>);
static_assert(!lbl::local_violation_label<priv_handler_t>);
static_assert(lbl::local_violation_label<pub_handler_t>);
static_assert(!lbl::semantic_computation_label<priv_sem_t>);
static_assert(lbl::semantic_computation_label<pub_sem_t>);

constexpr priv_comment_t priv_comment{};
constexpr pub_comment_t pub_comment{};
constexpr priv_message_t priv_message{};
constexpr pub_message_t pub_message{};
constexpr priv_query_t priv_query{};
constexpr pub_query_t pub_query{};
constexpr priv_handler_t priv_handler{};
constexpr pub_handler_t pub_handler{};
constexpr priv_sem_t priv_sem{};
constexpr pub_sem_t pub_sem{};

// Merely declaring these labels on a contract must compile.
void f_priv_comment(int x) pre<priv_comment>(x > 0) {}
void f_pub_comment(int x) pre<pub_comment>(x > 0) {}
void f_priv_message(int x) pre<priv_message>(x > 0) {}
void f_pub_message(int x) pre<pub_message>(x > 0) {}
void f_priv_query(int x) pre<priv_query>(x > 0) {}
void f_pub_query(int x) pre<pub_query>(x > 0) {}
void f_priv_handler(int x) pre<priv_handler>(x > 0) {}
void f_pub_handler(int x) pre<pub_handler>(x > 0) {}
void f_priv_sem(int x) pre<priv_sem>(x > 0) {}
void f_pub_sem(int x) pre<pub_sem>(x > 0) {}

// A private helper merely sharing a facet's name -- the tag-dispatch shape
// D3400R5 calls out -- must also be fine.
struct dispatch_t {
  using assertion_control_object = dispatch_t;

private:
  void handle_contract_violation(int) const {}
  void *query(int) const { return nullptr; }
};
static_assert(!lbl::local_violation_label<dispatch_t>);
static_assert(!lbl::queryable_label<dispatch_t>);
constexpr dispatch_t dispatch{};
void f_dispatch(int x) pre<dispatch>(x > 0) {}

static void reset() {
  global_calls = local_calls = 0;
  last_comment = last_message = nullptr;
  last_query = false;
}

int main() {
  // compute_comment: private is not applied, public is.
  reset();
  f_priv_comment(-1);
  if (last_comment && !strcmp(last_comment, "FIRED"))
    __builtin_abort();
  reset();
  f_pub_comment(-1);
  if (!last_comment || strcmp(last_comment, "FIRED"))
    __builtin_abort();

  // compute_message: likewise.
  reset();
  f_priv_message(-1);
  if (last_message && !strcmp(last_message, "FIRED"))
    __builtin_abort();
  reset();
  f_pub_message(-1);
  if (!last_message || strcmp(last_message, "FIRED"))
    __builtin_abort();

  // query: a private one leaves the handler unable to reach it.
  reset();
  f_priv_query(-1);
  if (last_query)
    __builtin_abort();
  reset();
  f_pub_query(-1);
  if (!last_query)
    __builtin_abort();

  // handle_contract_violation: a private one is never dispatched to, and the
  // global handler still runs.
  reset();
  f_priv_handler(-1);
  if (local_calls != 0 || global_calls != 1)
    __builtin_abort();
  reset();
  f_pub_handler(-1);
  if (local_calls != 1)
    __builtin_abort();

  // compute_semantic: the public one maps observe to ignore, so no violation
  // is reported at all; the private one leaves the semantic alone.
  reset();
  f_priv_sem(-1);
  if (global_calls != 1)
    __builtin_abort();
  reset();
  f_pub_sem(-1);
  if (global_calls != 0)
    __builtin_abort();

  // The tag-dispatch label provides neither facet.
  reset();
  f_dispatch(-1);
  if (local_calls != 0 || global_calls != 1 || last_query)
    __builtin_abort();

  return 0;
}
