//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//


#include "contracts-abi.h"
#include <stdio.h>
#include <stdlib.h>

/* --------------------------------------------------------------------- */
/* Chain-walking field lookup                                            */
/* --------------------------------------------------------------------- */

/* Return the descriptor's data[] array, accounting for alignment padding
   after the flexible field_ids[] member.  */
static const union __cxa_descriptor_data_t *
desc_data (const __cxa_descriptor_table_t *desc)
{
  uintptr_t p = (uintptr_t) (desc->field_ids + desc->num_entries);
  uintptr_t a = (uintptr_t) _Alignof (union __cxa_descriptor_data_t);
  p = (p + a - 1) & ~(a - 1);
  return (const union __cxa_descriptor_data_t *) p;
}

const void *
__cxa_find_field (const __cxa_contract_data_block *chain,
		  const uint8_t *ids, uint8_t num_ids)
{
  const __cxa_contract_data_block *block;

  for (block = chain; block; block = block->next)
    {
      const __cxa_descriptor_table_t *desc = block->descriptor;
      const union __cxa_descriptor_data_t *fdata;
      uint8_t i, j;

      if (!desc)
	continue;
      fdata = desc_data (desc);
      for (i = 0; i < desc->num_entries; ++i)
	for (j = 0; j < num_ids; ++j)
	  if (desc->field_ids[i] == ids[j])
	    {
	      if (desc->field_ids[i] >= CXA_FIELD_EXTENDED)
		return fdata[i].pointer;
	      return (const char *) block + fdata[i].offset;
	    }
    }
  return NULL;
}

static const void *
find_field1 (const __cxa_contract_data_block *chain, uint8_t id)
{
  uint8_t ids[1];
  ids[0] = id;
  return __cxa_find_field (chain, ids, 1);
}

/* --------------------------------------------------------------------- */
/* Override descriptor (prepends kind/semantic/mode to the chain)        */
/* --------------------------------------------------------------------- */

struct __override_desc_t
{
  uint8_t header;
  uint8_t num_entries;
  uint8_t field_ids[3];
  uint8_t padding[3];			/* align data[] to 8 bytes */
  union __cxa_descriptor_data_t data[3];
};

/* Override data block layout:
     offset 0:  descriptor pointer   (8 bytes)
     offset 8:  next pointer         (8 bytes)
     offset 16: kind      (uint8_t)
     offset 17: semantic  (uint8_t)
     offset 18: mode      (uint8_t)  */
static const struct __override_desc_t __override_desc = {
  (uint8_t) ((1u << 4) | CXA_VENDOR_GENERIC),	/* version=1, vendor=GENERIC */
  3,
  { CXA_FIELD_ASSERTION_KIND, CXA_FIELD_EVALUATION_SEMANTIC,
    CXA_FIELD_DETECTION_MODE },
  { 0, 0, 0 },
  {
    { 2 * sizeof (void *) },		/* kind     at offset 16 */
    { 2 * sizeof (void *) + 1 },	/* semantic at offset 17 */
    { 2 * sizeof (void *) + 2 },	/* mode     at offset 18 */
  }
};

struct __override_block_t
{
  const __cxa_descriptor_table_t         *descriptor;
  const __cxa_contract_data_block        *next;
  uint8_t kind;
  uint8_t semantic;
  uint8_t mode;
};

/* --------------------------------------------------------------------- */
/* Default handler fallback (libc only)                                  */
/* --------------------------------------------------------------------- */

/* Minimal default handler used only when neither a user handler nor a C++
   runtime (which would supply __contract_invoke_default_handler) is linked.
   Prints a diagnostic and returns; termination for enforcing semantics is
   performed by the dispatch core's post-handler action, not here.  */
static void
__contract_minimal_default (const __cxa_contract_data_block *chain)
{
  const void *cp = find_field1 (chain, CXA_FIELD_COMMENT);
  const char *comment = cp ? *(const char *const *) cp : "";
  const void *lp = find_field1 (chain, CXA_FIELD_SOURCE_LOCATION);
  const char *file = "";
  const char *func = "";
  unsigned line = 0;

  if (lp)
    {
      const __cxa_source_location *loc = (const __cxa_source_location *) lp;
      file = loc->file_name ? loc->file_name : "";
      func = loc->function_name ? loc->function_name : "";
      line = loc->line;
    }

  fprintf (stderr,
	   "contract violation in function %s at %s:%u: %s\n",
	   func, file, line, comment);
}

/* --------------------------------------------------------------------- */
/* Core dispatch                                                         */
/* --------------------------------------------------------------------- */

void
__contract_dispatch_core (const __cxa_contract_data_block *chain,
			  uint8_t semantic)
{
  /* A contract_violation object is just a leading chain pointer; build one
     on the stack to pass to the handler (C or C++).  */
  struct { const void *chain; } cv;
  const void *hp;
  const void *lpp;
  __cxa_local_handler_fn_t handler_fn = NULL;
  const void *label_ptr = NULL;
  int handled = 0;

  cv.chain = chain;

  /* Local (per-assertion) handler dispatch.  */
  hp = find_field1 (chain, CXA_FIELD_LOCAL_HANDLER);
  if (hp)
    handler_fn = *(__cxa_local_handler_fn_t const *) hp;
  lpp = find_field1 (chain, CXA_FIELD_LABEL_PTR);
  if (lpp)
    label_ptr = *(const void *const *) lpp;

  if (handler_fn && handler_fn (label_ptr, &cv) == 1)
    handled = 1;

  /* Global handler dispatch: user replacement, else the C++ runtime's
     always-default handler, else the libc-only fallback.  */
  if (!handled)
    {
      if (__handle_contract_violation)
	__handle_contract_violation (&cv);
      else if (__contract_invoke_default_handler)
	__contract_invoke_default_handler (&cv);
      else
	__contract_minimal_default (chain);
    }

  /* Post-handler action: enforcing semantics terminate via abort() when the
     handler returns normally.  (abort() is unaffected by any std::terminate
     handler, giving uniform contract-termination.)  */
  if (semantic == CXA_ES_ENFORCE || semantic == CXA_ES_NOEXCEPT_ENFORCE)
    abort ();
}

void
__dispatch_with_override_core (void *data, uint8_t kind,
			       uint8_t semantic, uint8_t mode)
{
  struct __override_block_t ovr;

  ovr.descriptor = (const __cxa_descriptor_table_t *) &__override_desc;
  ovr.next = (const __cxa_contract_data_block *) data;
  ovr.kind = kind;
  ovr.semantic = semantic;
  ovr.mode = mode;
  __contract_dispatch_core ((const __cxa_contract_data_block *) &ovr, semantic);
}

/* --------------------------------------------------------------------- */
/* Entry points (non-noexcept; the _noexcept variants live in the C++    */
/* runtime, which wraps these cores in a terminate-on-throw barrier)     */
/* --------------------------------------------------------------------- */

void
__cxa_contract_violation (void *data)
{
  const __cxa_contract_data_block *chain
    = (const __cxa_contract_data_block *) data;
  const void *sp = find_field1 (chain, CXA_FIELD_EVALUATION_SEMANTIC);
  uint8_t sem = sp ? *(const uint8_t *) sp : (uint8_t) CXA_ES_UNSPECIFIED;
  __contract_dispatch_core (chain, sem);
}

#define CXA_OBSERVE(kn, kv, mn, mv)					\
  void									\
  __cxa_contract_violation_##kn##_observe_##mn (void *data)		\
  {									\
    __dispatch_with_override_core (data, kv, CXA_ES_OBSERVE, mv);	\
  }

CXA_OBSERVE (pre,          CXA_AK_PRE,          pf, CXA_DM_PREDICATE_FALSE)
CXA_OBSERVE (pre,          CXA_AK_PRE,          ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_OBSERVE (post,         CXA_AK_POST,         pf, CXA_DM_PREDICATE_FALSE)
CXA_OBSERVE (post,         CXA_AK_POST,         ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_OBSERVE (assert,       CXA_AK_ASSERT,       pf, CXA_DM_PREDICATE_FALSE)
CXA_OBSERVE (assert,       CXA_AK_ASSERT,       ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_OBSERVE (post_capture, CXA_AK_POST_CAPTURE, pf, CXA_DM_PREDICATE_FALSE)
CXA_OBSERVE (post_capture, CXA_AK_POST_CAPTURE, ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_OBSERVE (implicit,     CXA_AK_IMPLICIT,     pf, CXA_DM_PREDICATE_FALSE)
CXA_OBSERVE (implicit,     CXA_AK_IMPLICIT,     ex, CXA_DM_EVALUATION_EXCEPTION)

#undef CXA_OBSERVE

#define CXA_ENFORCE(kn, kv, mn, mv)					\
  void									\
  __cxa_contract_violation_##kn##_enforce_##mn (void *data)		\
  {									\
    __dispatch_with_override_core (data, kv, CXA_ES_ENFORCE, mv);	\
    __builtin_unreachable ();						\
  }

CXA_ENFORCE (pre,          CXA_AK_PRE,          pf, CXA_DM_PREDICATE_FALSE)
CXA_ENFORCE (pre,          CXA_AK_PRE,          ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_ENFORCE (post,         CXA_AK_POST,         pf, CXA_DM_PREDICATE_FALSE)
CXA_ENFORCE (post,         CXA_AK_POST,         ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_ENFORCE (assert,       CXA_AK_ASSERT,       pf, CXA_DM_PREDICATE_FALSE)
CXA_ENFORCE (assert,       CXA_AK_ASSERT,       ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_ENFORCE (post_capture, CXA_AK_POST_CAPTURE, pf, CXA_DM_PREDICATE_FALSE)
CXA_ENFORCE (post_capture, CXA_AK_POST_CAPTURE, ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_ENFORCE (implicit,     CXA_AK_IMPLICIT,     pf, CXA_DM_PREDICATE_FALSE)
CXA_ENFORCE (implicit,     CXA_AK_IMPLICIT,     ex, CXA_DM_EVALUATION_EXCEPTION)

#undef CXA_ENFORCE
