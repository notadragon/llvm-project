// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -Wno-unused-command-line-argument %libcxx_flags -fsyntax-only 2>&1 \
// RUN:   | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -Wno-contract-invalid-label-facet -Wno-unused-command-line-argument \
// RUN:   %libcxx_flags -fsyntax-only 2>&1 \
// RUN:   | FileCheck --check-prefix=QUIET --allow-empty %s

// P3400: warn when a control object has a member that almost provides a facet.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-facet-near-miss.C)
//
// A facet is absent when its concept is not satisfied, and absence is silent
// by design -- p3400-facet-inaccessible.cpp pins that.  Silence is right for a
// type that never meant to provide the facet, and a poor answer for one that
// plainly did and got a detail wrong: the label compiles, the contract
// compiles, and the handler simply never runs.
//
// The warning must not fire on a member that merely shares a facet's name.
// D3400R5 is explicit that a label may carry private helpers named after a
// facet -- tag-dispatch overloads among them -- so the test is signature
// based: relax exactly one dimension (access, or constness) and ask whether
// that alone makes the call viable.  A helper of a different shape is viable
// under neither, so it stays quiet.

#include <contracts>
#include <cstddef>

using std::contracts::contract_violation;
using std::contracts::evaluation_semantic;

// ---------------------------------------------------------------------------
// Inaccessible members: the signature fits, only access stands in the way.
// ---------------------------------------------------------------------------

struct priv_handler_t {
  using assertion_control_object = priv_handler_t;

private:
  void handle_contract_violation(const contract_violation &) const {}
};
constexpr priv_handler_t priv_handler{};

struct priv_query_t {
  using assertion_control_object = priv_query_t;

private:
  void *query(const void *, std::size_t) const { return nullptr; }
};
constexpr priv_query_t priv_query{};

struct priv_comment_t {
  using assertion_control_object = priv_comment_t;

private:
  constexpr const char *compute_comment(const char *) const { return "x"; }
};
constexpr priv_comment_t priv_comment{};

struct priv_message_t {
  using assertion_control_object = priv_message_t;

private:
  constexpr const char *compute_message(const char *) const { return "x"; }
};
constexpr priv_message_t priv_message{};

struct priv_semantic_t {
  using assertion_control_object = priv_semantic_t;

private:
  constexpr evaluation_semantic compute_semantic(evaluation_semantic s) const {
    return s;
  }
};
constexpr priv_semantic_t priv_semantic{};

// CHECK: warning: assertion-control object 'priv_handler_t' does not provide the handle_contract_violation facet because that member is inaccessible
void f1(int x) pre<priv_handler>(x > 0) {}
// CHECK: warning: assertion-control object 'priv_query_t' does not provide the query facet because that member is inaccessible
void f2(int x) pre<priv_query>(x > 0) {}
// CHECK: warning: assertion-control object 'priv_comment_t' does not provide the compute_comment facet because that member is inaccessible
void f3(int x) pre<priv_comment>(x > 0) {}
// CHECK: warning: assertion-control object 'priv_message_t' does not provide the compute_message facet because that member is inaccessible
void f4(int x) pre<priv_message>(x > 0) {}
// CHECK: warning: assertion-control object 'priv_semantic_t' does not provide the compute_semantic facet because that member is inaccessible
void f5(int x) pre<priv_semantic>(x > 0) {}

// ---------------------------------------------------------------------------
// Non-const members: a facet is always invoked on a constexpr, therefore
// const, control object, so a non-const member can never be one.
// ---------------------------------------------------------------------------

struct mut_handler_t {
  using assertion_control_object = mut_handler_t;
  void handle_contract_violation(const contract_violation &) {}
};
constexpr mut_handler_t mut_handler{};

struct mut_query_t {
  using assertion_control_object = mut_query_t;
  void *query(const void *, std::size_t) { return nullptr; }
};
constexpr mut_query_t mut_query{};

struct mut_comment_t {
  using assertion_control_object = mut_comment_t;
  constexpr const char *compute_comment(const char *) { return "x"; }
};
constexpr mut_comment_t mut_comment{};

// CHECK: warning: assertion-control object 'mut_handler_t' does not provide the handle_contract_violation facet because that member is not 'const'
void g1(int x) pre<mut_handler>(x > 0) {}
// CHECK: warning: assertion-control object 'mut_query_t' does not provide the query facet because that member is not 'const'
void g2(int x) pre<mut_query>(x > 0) {}
// CHECK: warning: assertion-control object 'mut_comment_t' does not provide the compute_comment facet because that member is not 'const'
void g3(int x) pre<mut_comment>(x > 0) {}

// ---------------------------------------------------------------------------
// Not near misses.  Nothing below may warn, which the trailing CHECK-NOT and
// the QUIET run together enforce.
// ---------------------------------------------------------------------------

// A private helper that merely shares a facet's name: neither relaxation makes
// it viable, so the tag-dispatch idiom stays quiet.
struct dispatch_t {
  using assertion_control_object = dispatch_t;

private:
  void handle_contract_violation(int) const {}
  void *query(int) const { return nullptr; }
};
constexpr dispatch_t dispatch{};
void h1(int x) pre<dispatch>(x > 0) {}

// A public member of the wrong shape, likewise.
struct wrong_shape_t {
  using assertion_control_object = wrong_shape_t;
  void handle_contract_violation() const {}
  constexpr int compute_comment(int) const { return 0; }
};
constexpr wrong_shape_t wrong_shape{};
void h2(int x) pre<wrong_shape>(x > 0) {}

// Facets that are actually provided.
struct good_t {
  using assertion_control_object = good_t;
  void handle_contract_violation(const contract_violation &) const {}
  void *query(const void *, std::size_t) const { return nullptr; }
  constexpr const char *compute_comment(const char *) const { return "x"; }
  constexpr const char *compute_message(const char *) const { return "x"; }
  constexpr evaluation_semantic compute_semantic(evaluation_semantic s) const {
    return s;
  }
};
constexpr good_t good{};
void h3(int x) pre<good>(x > 0) {}

// A static facet member is const-correct by construction.
struct static_t {
  using assertion_control_object = static_t;
  static void handle_contract_violation(const contract_violation &) {}
  static constexpr const char *compute_comment(const char *) { return "x"; }
};
constexpr static_t stat{};
void h4(int x) pre<stat>(x > 0) {}

// An inherited facet is a facet.
struct HandlerBase {
  void handle_contract_violation(const contract_violation &) const {}
};
struct inherited_t : HandlerBase {
  using assertion_control_object = inherited_t;
};
constexpr inherited_t inherited{};
void h5(int x) pre<inherited>(x > 0) {}

// A label with no facets at all has nothing to be near.
struct bare_t {
  using assertion_control_object = bare_t;
};
constexpr bare_t bare{};
void h6(int x) pre<bare>(x > 0) {}

// ---------------------------------------------------------------------------
// Once per label type, not once per contract.  Three more contracts on
// mut_handler must add no further warnings.
// ---------------------------------------------------------------------------

void r1(int x) pre<mut_handler>(x > 0) {}
void r2(int x) pre<mut_handler>(x > 0) {}
void r3(int x) post<mut_handler>(true) { (void)x; }

// CHECK-NOT: warning:
// CHECK-NOT: error:

// QUIET-NOT: warning:
// QUIET-NOT: error:
