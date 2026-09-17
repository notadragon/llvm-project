// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4301 %libcxx_flags -o %t
// RUN: %t

// D4301: contract_violation::report() is noexcept -- a populator that throws
// must not escape; report() returns the fixed "Error generating report" string
// instead.  No shipping producer throws, so this exercises the try/catch
// fallback white-box: it hand-builds an ABI data block whose CXA_FIELD_REPORT
// populator throws and drives report() over it.
// (GCC mirror: g++.dg/contracts/cpp26/p4301-report-noexcept-fallback.C.)

#include <contracts>
#include <__contracts/abi.h>
#include <cstring>

using namespace __cxxabiv1;

static const char *throwing_populate(const void *) { throw 42; }

// A descriptor table describing a single offset-encoded CXA_FIELD_REPORT field.
// Mirrors __cxa_descriptor_table_t's binary layout: after field_ids[num_entries]
// the data[] array is aligned to alignof(__cxa_descriptor_data_t), which this
// struct's natural layout reproduces (header+num_entries+one id -> 3 bytes,
// padded to 8).
struct my_desc {
  __UINT8_TYPE__ header;       // table_version 0, vendor 0
  __UINT8_TYPE__ num_entries;  // 1
  __UINT8_TYPE__ field_ids[1]; // { CXA_FIELD_REPORT }
  __cxa_descriptor_data_t data[1];
};

// A data block with the report populator embedded (standard field 0x0B is
// offset-encoded, so the field data lives inside the block).
struct my_block {
  __cxa_contract_data_block base;      // descriptor, next
  __cxa_contract_report_populator pop; // the CXA_FIELD_REPORT field data
};

int main() {
  static my_desc desc = {0x00, 1, {CXA_FIELD_REPORT}, {}};
  desc.data[0].offset = __builtin_offsetof(my_block, pop);

  static my_block block;
  block.base.descriptor =
      reinterpret_cast<const __cxa_descriptor_table_t *>(&desc);
  block.base.next = nullptr;
  block.pop.populate = &throwing_populate;
  block.pop.ctx = nullptr;

  // A contract_violation is a single leading data-block-chain pointer (the same
  // representation the C accessors rely on), so view a pointer holding the chain
  // head as a contract_violation.
  const __cxa_contract_data_block *chain = &block.base;
  const std::contracts::contract_violation *cv =
      reinterpret_cast<const std::contracts::contract_violation *>(&chain);

  const char *r = cv->report();
  if (!r || std::strcmp(r, "Error generating report") != 0)
    __builtin_abort();
  return 0;
}
