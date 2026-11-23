// A system header for p4405-analysis-system-header.cpp and its -sys
// companion: a forwarding template that calls a closure the program passes
// it, and a function whose own precondition is analyzed at a call in this
// header.  The line numbers of the calls are named by the tests.
#pragma clang system_header

#include <contracts>

struct sys_always_diagnose_t {
  using assertion_control_object = sys_always_diagnose_t;
  static constexpr std::contracts::evaluation_semantic_set
    allowed_semantics{std::contracts::evaluation_semantic::diagnose};
};
constexpr sys_always_diagnose_t sys_always_diagnose{};

template <typename F>
void
sys_invoke(F &&f, int a)
{
  f(a);  // line 20
}

inline void
sys_callee(int x) pre<sys_always_diagnose>(x > 0)
{
}

template <typename T>
void
sys_caller(T x)
{
  sys_callee(x);  // line 32
}

inline void
sys_lambda_caller(int a)
{
  auto l = [](int x) pre<sys_always_diagnose>(x > 0) {};
  sys_invoke(l, a);
}
