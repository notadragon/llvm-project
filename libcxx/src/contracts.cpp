#include "../include/contracts"
#include <__config>
#include <__contracts/abi.h>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>

using namespace std::contracts;
using namespace __cxxabiv1;

extern "C" __attribute__((weak)) void __handle_contract_violation(const std::contracts::contract_violation&);

namespace {

static void __default_violation_handler(const contract_violation& violation) {
  using namespace std::contracts;
  // D4301: emit the producer-supplied report() text, when present, ahead of the
  // basic diagnostic line.  A producer (e.g. a routed sanitizer) can attach a
  // fuller diagnostic that shows up even without a custom
  // handle_contract_violation.  When report() is null (the common case -- no
  // report field on the violation chain), behavior is unchanged.
  if (const char* __r = violation.report(); __r && __r[0] != '\0')
    std::cerr << __r << '\n';
  std::cerr << violation.location().file_name() << ":" << violation.location().line() << ": ";
  auto assert_str = [&]() -> std::pair<const char*, const char*> {
    switch (violation.kind()) {
    case _AssertKind::pre:
      return {"pre(", ")"};
    case _AssertKind::post:
      return {"post(", ")"};
    case _AssertKind::assert:
      return {"contract_assert(", ")"};
    case _AssertKind::cassert:
      return {"assert(", ")"};
    case _AssertKind::post_capture:
      return {"post capture(", ")"};
    case _AssertKind::implicit:
      return {"implicit(", ")"};
    case _AssertKind::manual:
    default:
      return {"", ""};
    }
  }();
  std::cerr << assert_str.first << violation.comment() << assert_str.second;
  if (violation.detection_mode() == _DetectionMode::predicate_false) {
    std::cerr << " failed";
  } else {
    std::cerr << " exited via exception";
  }
  const char* msg = violation.message();
  if (msg && msg[0] != '\0' && std::strcmp(msg, violation.comment()) != 0) {
    std::cerr << ": " << msg;
  }
  std::cerr << std::endl;
}

void __run_violation_handler(const std::contracts::contract_violation& violation) {
  if (::__handle_contract_violation)
    ::__handle_contract_violation(violation);
  else
    invoke_default_contract_violation_handler(violation);

  // When the handler returns normally for an enforced violation, terminate
  // via std::abort() (not std::terminate()): abort() is not affected by a
  // user-installed std::terminate handler, giving predictable, uniform
  // contract-termination (the P3290 assert integration relies on this).
  if (violation.semantic() == evaluation_semantic::enforce)
    std::abort();
}

void __run_nothrow_violation_handler(const std::contracts::contract_violation& violation) noexcept {
#if _LIBCPP_HAS_EXCEPTIONS
  try {
#endif
    __run_violation_handler(violation);
#if _LIBCPP_HAS_EXCEPTIONS
  } catch (...) {
    // A handler that exits via an exception at a noexcept entry point
    // terminates via std::terminate() (the exception is still active).
    std::terminate();
  }
#endif
}

} // end namespace

void std::contracts::invoke_default_contract_violation_handler(const contract_violation& violation) noexcept {
  __default_violation_handler(violation);
}

// C-linkage always-default entry point (contracts ABI spec section 8.4).  This
// is the C form of std::contracts::invoke_default_contract_violation_handler:
// it always invokes the implementation default handler, bypassing any user
// replacement.  libcontracts (pure C) calls it for its dispatch fallback when
// no user handler (__handle_contract_violation) is present, and C violation
// handlers can call it to emit the default diagnostics.  The argument is a
// pointer to a contract_violation object (ABI-identical to a const reference).
extern "C" _LIBCPP_EXPORTED_FROM_ABI void __contract_invoke_default_handler(const std::contracts::contract_violation&);

extern "C" _LIBCPP_EXPORTED_FROM_ABI void
__contract_invoke_default_handler(const std::contracts::contract_violation& __violation) {
  std::contracts::invoke_default_contract_violation_handler(__violation);
}

// P3290 API support: build data blocks on the stack and call
// __cxa_contract_violation, same pattern as GCC's contract26.cc.

namespace {

struct __p3290_data_block_t {
  const __cxa_descriptor_table_t* descriptor;
  const __cxa_contract_data_block* next;
  __cxa_source_location location;
  const char* comment;
  __UINT8_TYPE__ kind;
  __UINT8_TYPE__ semantic;
  __UINT8_TYPE__ mode;
};

struct __p3290_desc_t {
  __UINT8_TYPE__ header;
  __UINT8_TYPE__ num_entries;
  __UINT8_TYPE__ fid[5];
  __UINT8_TYPE__ pad[1];
  __cxa_descriptor_data_t data[5];
};

static const __p3290_desc_t __p3290_desc = {
    static_cast<__UINT8_TYPE__>((1u << 4) | CXA_VENDOR_CLANG),
    5,
    {CXA_FIELD_SOURCE_LOCATION,
     CXA_FIELD_COMMENT,
     CXA_FIELD_ASSERTION_KIND,
     CXA_FIELD_EVALUATION_SEMANTIC,
     CXA_FIELD_DETECTION_MODE},
    {0},
    {
        {offsetof(__p3290_data_block_t, location)},
        {offsetof(__p3290_data_block_t, comment)},
        {offsetof(__p3290_data_block_t, kind)},
        {offsetof(__p3290_data_block_t, semantic)},
        {offsetof(__p3290_data_block_t, mode)},
    }};

} // anonymous namespace

void std::contracts::__handle_manual_contract_violation(
    assertion_kind __kind,
    evaluation_semantic __semantic,
    detection_mode __mode,
    const char* __comment,
    std::source_location __loc,
    bool __can_throw) {
  __p3290_data_block_t __data = {
      reinterpret_cast<const __cxa_descriptor_table_t*>(&__p3290_desc),
      nullptr,
      {__loc.file_name(), __loc.function_name(), __loc.line(), __loc.column()},
      __comment ? __comment : "",
      static_cast<__UINT8_TYPE__>(__kind),
      static_cast<__UINT8_TYPE__>(__semantic),
      static_cast<__UINT8_TYPE__>(__mode),
  };

  contract_violation __cv{reinterpret_cast<const __cxa_contract_data_block*>(&__data)};

  if (!__can_throw)
    __run_nothrow_violation_handler(__cv);
  else
    __run_violation_handler(__cv);
}

// Shared assert-integration entry point (P3290).  Both libstdc++ and libc++
// expand the <cassert> `assert` macro (under __STDC_WANT_ASSERT_USES_CONTRACTS__) to a call
// to this single symbol.  It reports evaluation_semantic=enforce and
// assertion_kind=cassert to the handler.  Unlike the general contract entry
// points, *any* completion of the handler results in std::abort(): a normal
// return aborts via the enforce post-action inside the ABI, and an escaping
// exception is caught here and turned into std::abort() as well.  This matches
// classic assert() termination semantics regardless of how the handler exits.
// The user-facing declaration lives in <__cassert/assert_contract.h>, which is
// not included here; declare it explicitly so the definition has a prototype.
extern "C++" [[noreturn]] _LIBCPP_EXPORTED_FROM_ABI void
__cxa_handle_cassert_violation(const char*, std::source_location) noexcept;

extern "C++" [[noreturn]] _LIBCPP_EXPORTED_FROM_ABI void
__cxa_handle_cassert_violation(const char* __comment, std::source_location __loc) noexcept {
  __p3290_data_block_t __data = {
      reinterpret_cast<const __cxa_descriptor_table_t*>(&__p3290_desc),
      nullptr,
      {__loc.file_name(), __loc.function_name(), __loc.line(), __loc.column()},
      __comment ? __comment : "",
      CXA_AK_CASSERT,
      CXA_ES_ENFORCE,
      CXA_DM_PREDICATE_FALSE,
  };
#if _LIBCPP_HAS_EXCEPTIONS
  try {
#endif
    __cxa_contract_violation(const_cast<void*>(static_cast<const void*>(&__data)));
#if _LIBCPP_HAS_EXCEPTIONS
  } catch (...) {
    std::abort();
  }
#endif
  std::abort();
}
