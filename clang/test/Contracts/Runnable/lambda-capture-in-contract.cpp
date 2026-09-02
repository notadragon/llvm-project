// [expr.prim.lambda.capture]/3.3 allows a capture-default or simple-capture in
// a lambda-introducer when the lambda appears within a contract assertion and
// its innermost enclosing scope is the corresponding contract-assertion scope.
// So a lambda written in a contract predicate may capture the enclosing
// function's parameters.
//
// Every case is checked BY THE VALUE THE PREDICATE SAW, not by whether it
// compiles: before this was fixed Clang accepted three of these forms with no
// capture recorded at all and then crashed in CodeGen
// ("DeclRefExpr for Decl not entered in LocalDeclMap?"), so "it compiles" was
// never evidence of anything. A contract predicate cannot assign to a variable
// it names, so the value is recorded through a called function.
//
// RUN: %clangxx -std=c++26 %s -fcontracts %libcxx_flags -o %t && %t

int g_seen = -1;

bool record(int v) {
  g_seen = v;
  return true;
}

// Free functions: the four legal capture forms. These are the ones that were
// broken -- the predicate is parsed off the declarator, before the parameters
// are reparented onto the function.
void copy_cap(int x) pre([x] { return record(x); }()) {}
void ref_cap(int x) pre([&x] { return record(x); }()) {}
void default_ref(int x) pre([&] { return record(x); }()) {}
void default_copy(int x) pre([=] { return record(x); }()) {}

// A postcondition. The parameter must be const: capturing it odr-uses it, and
// [dcl.contract.func] requires a non-reference parameter odr-used by a
// postcondition predicate to have const type.
int post_cap(const int x) post(r : [x] { return record(x); }()) { return x + 1; }

// Member functions, which already worked -- kept as the reference behaviour
// the free-function cases above must now match.
struct S {
  int m = 7;

  void mem(int x) pre([x] { return record(x); }()) {}
  void mem_default(int x) pre([&] { return record(x); }()) {}

  // A parameter captured alongside 'this', which reaches the member through
  // the remapped dummy rather than through a capture.
  void mem_this(int x) pre([this, x] { return record(x + m); }()) {}
};

// A lambda's own precondition capturing that lambda's parameter.
void lambda_own_pre() {
  auto l = [](int x) pre([x] { return record(x); }()) {};
  l(66);
}

int main() {
  copy_cap(11);
  if (g_seen != 11)
    __builtin_abort();

  ref_cap(22);
  if (g_seen != 22)
    __builtin_abort();

  default_ref(33);
  if (g_seen != 33)
    __builtin_abort();

  default_copy(44);
  if (g_seen != 44)
    __builtin_abort();

  post_cap(55);
  if (g_seen != 55)
    __builtin_abort();

  S s;

  s.mem(77);
  if (g_seen != 77)
    __builtin_abort();

  s.mem_default(88);
  if (g_seen != 88)
    __builtin_abort();

  s.mem_this(99);
  if (g_seen != 99 + 7)
    __builtin_abort();

  lambda_own_pre();
  if (g_seen != 66)
    __builtin_abort();

  return 0;
}
