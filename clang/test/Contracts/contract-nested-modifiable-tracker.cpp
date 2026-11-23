// RUN: %clang_cc1 -std=c++26 -fcontracts -Wno-assume -fsyntax-only -verify %s
// expected-no-diagnostics

// A contract assertion evaluated inside another evaluation whose
// modifications must not survive -- a contract predicate, or the operand of
// an [[assume]] -- must not disturb the enclosing one.  The cases cover
// several shapes of inner assertion (constant and non-constant predicates,
// through a call and through a lambda) because the property being tested is
// that none of them does.
//
// Mirror of gnu_gcc's contract-nested-modifiable-tracker.C.

struct string {
  const char *p;
  int i;
  constexpr string(const char *p) : p(p), i(0) {}
  constexpr int length() {
    ++i;
    return __builtin_strlen(p);
  }
};

constexpr bool ctr_const(const char *) {
  contract_assert(true);
  return true;
}
constexpr bool ctr_dyn(const char *p) {
  contract_assert(p != nullptr);
  return true;
}

// A contract predicate that modifies the enclosing evaluation.  The
// modification is permitted -- [basic.contract.eval] has this construct as a
// worked example -- so every case must be accepted, and the modification is
// undone once the predicate's value is known, so every case must leave
// i == 0, silently, whatever the shape of the inner assertion.  The
// const_cast is required: the predicate const-ifies the id-expression, not the
// object.

constexpr int plain() {
  string s("foobar");
  contract_assert(const_cast<string &>(s).length() > 0);
  return s.i;
}
static_assert(plain() == 0);

constexpr int via_call_const() {
  string s("foobar");
  contract_assert(ctr_const(s.p) && const_cast<string &>(s).length() > 0);
  return s.i;
}
static_assert(via_call_const() == 0);

constexpr int via_call_dyn() {
  string s("foobar");
  contract_assert(ctr_dyn(s.p) && const_cast<string &>(s).length() > 0);
  return s.i;
}
static_assert(via_call_dyn() == 0);

constexpr int via_lambda() {
  string s("foobar");
  contract_assert([] { contract_assert(true); return true; }() &&
                  const_cast<string &>(s).length() > 0);
  return s.i;
}
static_assert(via_lambda() == 0);

constexpr int via_lambda_capture() {
  string s("foobar");
  contract_assert([&] { contract_assert(true); return true; }() &&
                  const_cast<string &>(s).length() > 0);
  return s.i;
}
static_assert(via_lambda_capture() == 0);

// The mirror image.  An [[assume]] operand is not evaluated, so x must stay 0,
// whatever contract assertion is reached while evaluating it.

constexpr bool bump(unsigned *p) {
  *p += 1;
  return true;
}
constexpr bool a_ctr_const(unsigned *) {
  contract_assert(true);
  return true;
}
constexpr bool a_ctr_dyn(unsigned *p) {
  contract_assert(p != nullptr);
  return true;
}
constexpr bool a_assume(unsigned *p) {
  [[assume(*p < 100)]];
  return true;
}

constexpr unsigned assume_plain() {
  unsigned x = 0;
  [[assume(bump(&x))]];
  return x;
}
static_assert(assume_plain() == 0);

constexpr unsigned assume_in_assume() {
  unsigned x = 0;
  [[assume(a_assume(&x) && bump(&x))]];
  return x;
}
static_assert(assume_in_assume() == 0);

constexpr unsigned contract_in_assume_const() {
  unsigned x = 0;
  [[assume(a_ctr_const(&x) && bump(&x))]];
  return x;
}
static_assert(contract_in_assume_const() == 0);

constexpr unsigned contract_in_assume_dyn() {
  unsigned x = 0;
  [[assume(a_ctr_dyn(&x) && bump(&x))]];
  return x;
}
static_assert(contract_in_assume_dyn() == 0);

constexpr unsigned contract_in_assume_lambda() {
  unsigned x = 0;
  [[assume([&] { contract_assert(true); return true; }() && bump(&x))]];
  return x;
}
static_assert(contract_in_assume_lambda() == 0);

// The value is a constant, so getting it wrong changes object layout too.
char probe[contract_in_assume_const() + 1];
static_assert(sizeof(probe) == 1);
