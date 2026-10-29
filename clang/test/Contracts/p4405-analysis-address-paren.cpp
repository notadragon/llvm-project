// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -fsyntax-only -ferror-limit=0 -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("Taking the Address of a Function"): a parenthesized function
// name takes its address just as the name does -- bound to a reference,
// converted to a pointer, the operand of the unary &, an argument, deduced
// by auto or decltype(auto), however many parentheses, in a template's
// instantiation and in a default argument -- so an always_prove or
// always_diagnose precondition cannot be proven for a call through the
// address.  A parenthesized qualified name, template-id and closure are
// diagnosed too; a parenthesized direct call is not taking the address,
// nor is an unevaluated operand (CLANG-867).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-address-paren.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

void fp(int x) pre<always_prove>(x > 0);    // expected-note 17 {{precondition here}}
void fd(int x) pre<always_diagnose>(x > 0); // expected-note 2 {{precondition here}}
void fn(int x) pre(x > 0);

struct S {
  static void sp(int x) pre<always_prove>(x > 0); // expected-note 2 {{precondition here}}
};

template <typename T>
void tp(T x) pre<always_prove>(x > 0); // expected-note {{precondition here}}

void take(void (*)(int));
void take_ref(void (&)(int));

void (&gr)(int) = (fp); // expected-error {{cannot be proven for a call through the address of 'fp'}}

void use(S &s) {
  void (&r1)(int) = (fp);    // expected-error {{contract assertion cannot be proven for a call through the address of 'fp'}}
  void (&r2)(int) = ((fp));  // expected-error {{cannot be proven for a call through the address}}
  void (&&r3)(int) = (fp);   // expected-error {{cannot be proven for a call through the address}}
  void (*p1)(int) = (fp);    // expected-error {{cannot be proven for a call through the address}}
  void (*p2)(int) = ((fp));  // expected-error {{cannot be proven for a call through the address}}
  void (*p3)(int) = &(fp);   // expected-error {{cannot be proven for a call through the address}}
  void (*p4)(int) = &((fp)); // expected-error {{cannot be proven for a call through the address}}
  void (*p5)(int) = (&fp);   // expected-error {{cannot be proven for a call through the address}}
  take((fp));                // expected-error {{cannot be proven for a call through the address}}
  take_ref((fp));            // expected-error {{cannot be proven for a call through the address}}
  auto p6 = (fp);            // expected-error {{cannot be proven for a call through the address}}
  auto &r4 = (fp);           // expected-error {{cannot be proven for a call through the address}}
  decltype(auto) r5 = (fp);  // expected-error {{cannot be proven for a call through the address}}
  void (&r6)(int) = (fd);    // expected-warning {{contract assertion cannot be proven for a call through the address of 'fd'}}
  void (*p7)(int) = &(fd);   // expected-warning {{cannot be proven for a call through the address}}
  void (&r7)(int) = (fn);
  void (*p8)(int) = (S::sp); // expected-error {{cannot be proven for a call through the address of 'sp'}}
  void (&r8)(int) = (s.sp);  // expected-error {{cannot be proven for a call through the address of 'sp'}}
  void (*p9)(int) = (tp<int>); // expected-error {{cannot be proven for a call through the address of 'tp<int>'}}
  auto l = [](int x) pre<always_prove>(x > 0) { return x; }; // expected-note 2 {{precondition here}}
  int (*p10)(int) = (l);     // expected-error {{cannot be proven for a call through the address of 'operator()'}}
  int (*p11)(int) = ((l));   // expected-error {{cannot be proven for a call through the address of 'operator()'}}
  decltype(&(fp)) q = nullptr;
  (void)sizeof(&(fp));
  (fp)(1);
  ((fp))(1);
  (void)r1; (void)r2; (void)r3; (void)p1; (void)p2; (void)p3;
  (void)p4; (void)p5; (void)p6; (void)r4; (void)r5; (void)r6;
  (void)p7; (void)r7; (void)p8; (void)r8; (void)p9; (void)p10;
  (void)p11; (void)q;
}

template <typename T>
void inst(T) {
  void (&r)(int) = (fp); // expected-error {{cannot be proven for a call through the address of 'fp'}}
  void (*p)(int) = &(fp); // expected-error {{cannot be proven for a call through the address of 'fp'}}
  (void)r; (void)p;
}

template void inst(int); // expected-note {{in instantiation of function template specialization 'inst<int>' requested here}}

void dflt(void (&)(int) = (fp)); // expected-error {{cannot be proven for a call through the address of 'fp'}}
