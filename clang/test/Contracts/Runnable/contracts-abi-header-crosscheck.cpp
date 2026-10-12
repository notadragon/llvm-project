// RUN: rm -rf %t && mkdir -p %t
// RUN: %clang -x c -std=c11 -I%S/../../../../libcontracts %s -o %t/c
// RUN: %t/c > %t/c.txt
// RUN: %clangxx -std=c++26 %libcxx_flags %s -o %t/cxx
// RUN: %t/cxx > %t/cxx.txt
// RUN: diff %t/c.txt %t/cxx.txt

// The contracts ABI is spelled twice in this tree: libcontracts' C header
// (libcontracts/contracts-abi.h) and libc++'s <__contracts/abi.h>.  Nothing
// shares the two, so this file is compiled once against each -- as C with
// the first, as C++ with the second -- and the two programs must print the
// same thing: every CXA_* value and the size and member offsets of every
// wire-format type.  (libstdc++'s <bits/contracts_abi.h> is GCC's to check.)

#ifdef __cplusplus
#include <__contracts/abi.h>
using namespace __cxxabiv1;
#define ALIGNOF alignof
#else
#include "contracts-abi.h"
#define ALIGNOF _Alignof
#endif

#include <stddef.h>
#include <stdio.h>

#define V(e) printf("%s %lu\n", #e, (unsigned long)(e));
#define S(t) V(sizeof(t)) V(ALIGNOF(t))
#define O(t, m) V(offsetof(t, m))

int main(void) {
  V(CXA_AK_UNSPECIFIED) V(CXA_AK_PRE) V(CXA_AK_POST) V(CXA_AK_ASSERT)
  V(CXA_AK_MANUAL) V(CXA_AK_CASSERT) V(CXA_AK_POST_CAPTURE) V(CXA_AK_IMPLICIT)

  V(CXA_ES_UNSPECIFIED) V(CXA_ES_IGNORE) V(CXA_ES_OBSERVE) V(CXA_ES_ENFORCE)
  V(CXA_ES_QUICK_ENFORCE) V(CXA_ES_NOEXCEPT_OBSERVE) V(CXA_ES_NOEXCEPT_ENFORCE)

  V(CXA_DM_UNSPECIFIED) V(CXA_DM_PREDICATE_FALSE)
  V(CXA_DM_EVALUATION_EXCEPTION)

  V(CXA_FIELD_SOURCE_LOCATION) V(CXA_FIELD_COMMENT) V(CXA_FIELD_MESSAGE)
  V(CXA_FIELD_LOCAL_HANDLER) V(CXA_FIELD_QUERY_FUNCTION) V(CXA_FIELD_LABEL_PTR)
  V(CXA_FIELD_ASSERTION_KIND) V(CXA_FIELD_EVALUATION_SEMANTIC)
  V(CXA_FIELD_DETECTION_MODE) V(CXA_FIELD_EXCEPTION_PTR) V(CXA_FIELD_REPORT)
  V(CXA_FIELD_EXTENDED)

  V(CXA_VENDOR_GENERIC) V(CXA_VENDOR_GCC) V(CXA_VENDOR_CLANG)
  V(CXA_VENDOR_MSVC)

  S(__cxa_source_location)
  O(__cxa_source_location, file_name) O(__cxa_source_location, function_name)
  O(__cxa_source_location, line) O(__cxa_source_location, column)

  S(union __cxa_descriptor_data_t)
  O(union __cxa_descriptor_data_t, offset)
  O(union __cxa_descriptor_data_t, pointer)

  S(struct __cxa_descriptor_table_t)
  O(struct __cxa_descriptor_table_t, header)
  O(struct __cxa_descriptor_table_t, num_entries)
  O(struct __cxa_descriptor_table_t, field_ids)

  S(struct __cxa_contract_data_block)
  O(struct __cxa_contract_data_block, descriptor)
  O(struct __cxa_contract_data_block, next)

  S(struct __cxa_contract_report_populator)
  O(struct __cxa_contract_report_populator, populate)
  O(struct __cxa_contract_report_populator, ctx)
  return 0;
}
