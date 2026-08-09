//===- c_api.c - libcontracts P3290 C contract API ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//


#include "contracts-abi.h"
#include <stdlib.h>

/* Stack-constructed data block matching the compiler-generated layout, used
   by the P3290 C API and the C contract-check helpers.  */
struct c_p3290_data_block_t
{
  const __cxa_descriptor_table_t  *descriptor;
  const __cxa_contract_data_block *next;
  __cxa_source_location            location;
  const char                      *comment;
  uint8_t                          kind;
  uint8_t                          semantic;
  uint8_t                          mode;
};

struct c_p3290_desc_t
{
  uint8_t header;
  uint8_t num_entries;
  uint8_t fid[5];
  uint8_t pad[1];
  union __cxa_descriptor_data_t data[5];
};

static const struct c_p3290_desc_t c_p3290_desc = {
  (uint8_t) ((1u << 4) | CXA_VENDOR_GCC),
  5,
  { CXA_FIELD_SOURCE_LOCATION, CXA_FIELD_COMMENT, CXA_FIELD_ASSERTION_KIND,
    CXA_FIELD_EVALUATION_SEMANTIC, CXA_FIELD_DETECTION_MODE },
  { 0 },
  {
    { offsetof (struct c_p3290_data_block_t, location) },
    { offsetof (struct c_p3290_data_block_t, comment) },
    { offsetof (struct c_p3290_data_block_t, kind) },
    { offsetof (struct c_p3290_data_block_t, semantic) },
    { offsetof (struct c_p3290_data_block_t, mode) },
  }
};

/* A second data block carrying a lazy report populator (CXA_FIELD_REPORT).
   The populator struct is embedded inline so the standard field's byte offset
   locates it; the block is chained after the primary block, so
   contract_violation::report() finds it via the chain walk.  */
struct c_report_data_block_t
{
  const __cxa_descriptor_table_t   *descriptor;
  const __cxa_contract_data_block  *next;
  __cxa_contract_report_populator   report;
};

struct c_report_desc_t
{
  uint8_t header;
  uint8_t num_entries;
  uint8_t fid[1];
  uint8_t pad[1];
  union __cxa_descriptor_data_t data[1];
};

static const struct c_report_desc_t c_report_desc = {
  (uint8_t) ((1u << 4) | CXA_VENDOR_GCC),
  1,
  { CXA_FIELD_REPORT },
  { 0 },
  {
    { offsetof (struct c_report_data_block_t, report) },
  }
};

static void
c_build_and_dispatch (const char *comment, const char *file, const char *func,
		      unsigned line, uint8_t kind, uint8_t semantic,
		      uint8_t mode)
{
  struct c_p3290_data_block_t data;

  data.descriptor = (const __cxa_descriptor_table_t *) &c_p3290_desc;
  data.next = NULL;
  data.location.file_name = file ? file : "";
  data.location.function_name = func ? func : "";
  data.location.line = line;
  data.location.column = 0;
  data.comment = comment ? comment : "";
  data.kind = kind;
  data.semantic = semantic;
  data.mode = mode;
  __cxa_contract_violation (&data);
}

/* --------------------------------------------------------------------- */
/* P3290 C API                                                           */
/* --------------------------------------------------------------------- */

void
stdc_handle_enforced_contract_violation_explicit (const char *comment,
						  const char *file,
						  const char *func,
						  unsigned line)
{
  c_build_and_dispatch (comment, file, func, line, CXA_AK_MANUAL,
			CXA_ES_ENFORCE, CXA_DM_UNSPECIFIED);
  /* Unreachable backstop: enforced dispatch terminates via abort().  */
  abort ();
}

void
stdc_handle_observed_contract_violation_explicit (const char *comment,
						  const char *file,
						  const char *func,
						  unsigned line)
{
  c_build_and_dispatch (comment, file, func, line, CXA_AK_MANUAL,
			CXA_ES_OBSERVE, CXA_DM_UNSPECIFIED);
}

void
stdc_handle_quick_enforced_contract_violation_explicit (const char *comment,
							const char *file,
							const char *func,
							unsigned line)
{
  (void) comment;
  (void) file;
  (void) func;
  (void) line;
  /* Quick-enforce terminates immediately without invoking the handler.  */
  abort ();
}

/* --------------------------------------------------------------------- */
/* P3100 Task 2.2 / CL2: sanitizer-report routing entry point            */
/* --------------------------------------------------------------------- */

/* A sanitizer runtime (compiler-rt asan) detected an error and calls this to
   report it through the contract-violation handler as an implicit
   (CXA_AK_IMPLICIT) contract violation.  It builds a data block from the
   discrete fields the runtime has and dispatches through
   __contract_dispatch_core.

   __semantic is the ASan descriptor wire value: 2 => noexcept_enforce,
   anything else (1 = noexcept_observe, 0 = stock) => noexcept_observe (the
   non-terminating default).  Routing to the handler is only ever the NON-
   throwing (D4298 noexcept) semantics -- a throwing handler cannot propagate
   from asan's noexcept report path (P3100 Task 4.1) -- so the block records the
   NOEXCEPT ABI semantic (CXA_ES_NOEXCEPT_ENFORCE = 7 / CXA_ES_NOEXCEPT_OBSERVE
   = 6), NOT the plain throwing CXA_ES_ENFORCE/OBSERVE, so
   contract_violation.semantic() reports what the routed check actually is.
   (wire 3 = quick_enforce never calls this entry point.)  The core is invoked
   with CXA_ES_OBSERVE so it NEVER performs its post-handler abort() --
   termination for the enforce case is the caller's job (the sanitizer runtime's
   Die()), so this entry point ALWAYS returns.

   'report' is an optional lazy report populator (CXA_FIELD_REPORT): when
   non-NULL with a non-NULL populate function, a data block carrying it is
   chained onto the violation so contract_violation::report() invokes it on
   demand (the sanitizer prints nothing itself on the routed path).  Pass NULL
   for no report field.  Must be ABI-identical to GCC's libcontracts.  */

void
__cxa_contract_violation_sanitizer (const char *comment, const char *file,
				    unsigned line, unsigned char semantic,
				    const __cxa_contract_report_populator
				      *report)
{
  struct c_p3290_data_block_t data;
  struct c_report_data_block_t report_block;
  uint8_t sem = (semantic == 2) ? (uint8_t) CXA_ES_NOEXCEPT_ENFORCE
			        : (uint8_t) CXA_ES_NOEXCEPT_OBSERVE;

  data.descriptor = (const __cxa_descriptor_table_t *) &c_p3290_desc;
  data.next = NULL;
  data.location.file_name = file ? file : "";
  data.location.function_name = "";
  data.location.line = line;
  data.location.column = 0;
  data.comment = comment ? comment : "";
  data.kind = CXA_AK_IMPLICIT;
  data.semantic = sem;
  data.mode = CXA_DM_PREDICATE_FALSE;

  /* When the sanitizer provides a lazy report populator, chain a second block
     carrying CXA_FIELD_REPORT so contract_violation::report() can find it and
     invoke the populator on demand.  The populator is copied by value into a
     block that outlives the dispatch call.  */
  if (report && report->populate)
    {
      report_block.descriptor
	= (const __cxa_descriptor_table_t *) &c_report_desc;
      report_block.next = NULL;
      report_block.report = *report;
      data.next = (const __cxa_contract_data_block *) &report_block;
    }

  /* Dispatch with a non-terminating core semantic (OBSERVE) so libcontracts
     never aborts; the block still carries the true semantic for the handler.  */
  __contract_dispatch_core ((const __cxa_contract_data_block *) &data,
			    (uint8_t) CXA_ES_OBSERVE);
}

/* --------------------------------------------------------------------- */
/* Compiler-emitted C contract-check helpers (D4299)                     */
/* --------------------------------------------------------------------- */

void
__c_contract_check_enforce (const char *comment, const char *file,
			    const char *func, unsigned line,
			    unsigned char kind)
{
  c_build_and_dispatch (comment, file, func, line, (uint8_t) kind,
			CXA_ES_ENFORCE, CXA_DM_PREDICATE_FALSE);
  /* Unreachable backstop: enforced dispatch terminates via abort().  */
  abort ();
}

void
__c_contract_check_observe (const char *comment, const char *file,
			    const char *func, unsigned line,
			    unsigned char kind)
{
  c_build_and_dispatch (comment, file, func, line, (uint8_t) kind,
			CXA_ES_OBSERVE, CXA_DM_PREDICATE_FALSE);
}
