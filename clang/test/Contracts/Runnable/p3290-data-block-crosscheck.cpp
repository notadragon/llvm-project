// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 %libcxx_flags -o %t && %t

// P3290: the violation data block behind a manual violation is built in two
// places -- libcontracts' c_fill_block, for the C API
// (stdc_handle_*_contract_violation_explicit), and libc++'s
// __handle_manual_contract_violation, for the C++ one -- and nothing shares
// the two (notadragon_wg21 DECISIONS.md E40).  This pins that they agree:
// the same descriptor, byte for byte (header, entry count, field IDs and
// offsets), a single block, and every field reading back as passed, both
// through the offsets libc++'s __cxa_descriptor_table_t::data() locates and
// through libcontracts' C accessors, whose lookup finds data[] its own way.

#include <contracts>
#include <cstring>

extern "C" {
void stdc_handle_observed_contract_violation_explicit(const char *comment,
                                                      const char *file,
                                                      const char *func,
                                                      unsigned line);
const char *stdc_contract_violation_comment(const void *cv);
const char *stdc_contract_violation_file(const void *cv);
const char *stdc_contract_violation_function(const void *cv);
unsigned stdc_contract_violation_line(const void *cv);
int stdc_contract_violation_kind(const void *cv);
int stdc_contract_violation_semantic(const void *cv);
int stdc_contract_violation_detection_mode(const void *cv);
}

using namespace __cxxabiv1;

struct Snapshot {
  unsigned char header, num_entries;
  unsigned char field_ids[8];
  unsigned long offsets[8];
  const char *comment, *file, *function;
  unsigned line;
  int kind, semantic, mode;
};

static Snapshot snaps[2];
static int calls;

static void check(bool b) {
  if (!b)
    __builtin_abort();
}

void handle_contract_violation(const std::contracts::contract_violation &v) {
  check(calls < 2);
  Snapshot &s = snaps[calls++];

  // A contract_violation is a single leading chain pointer.
  const __cxa_contract_data_block *chain;
  std::memcpy(&chain, &v, sizeof chain);
  check(chain && !chain->next);

  const __cxa_descriptor_table_t *d = chain->descriptor;
  s.header = d->header;
  s.num_entries = d->num_entries;
  check(s.num_entries <= 8);
  for (unsigned i = 0; i != s.num_entries; ++i) {
    s.field_ids[i] = d->field_ids[i];
    s.offsets[i] = d->data()[i].offset;
  }

  // The fields at libc++'s data() offsets.
  const char *base = reinterpret_cast<const char *>(chain);
  for (unsigned i = 0; i != s.num_entries; ++i) {
    const char *f = base + s.offsets[i];
    switch (s.field_ids[i]) {
    case CXA_FIELD_SOURCE_LOCATION: {
      const auto *loc = reinterpret_cast<const __cxa_source_location *>(f);
      s.file = loc->file_name;
      s.function = loc->function_name;
      s.line = loc->line;
      break;
    }
    case CXA_FIELD_COMMENT:
      s.comment = *reinterpret_cast<const char *const *>(f);
      break;
    case CXA_FIELD_ASSERTION_KIND:
      s.kind = *reinterpret_cast<const unsigned char *>(f);
      break;
    case CXA_FIELD_EVALUATION_SEMANTIC:
      s.semantic = *reinterpret_cast<const unsigned char *>(f);
      break;
    case CXA_FIELD_DETECTION_MODE:
      s.mode = *reinterpret_cast<const unsigned char *>(f);
      break;
    default:
      check(false);
    }
  }

  // The same fields through libcontracts' lookup and through libc++'s
  // accessors.
  check(stdc_contract_violation_comment(&v) == s.comment);
  check(stdc_contract_violation_file(&v) == s.file);
  check(stdc_contract_violation_function(&v) == s.function);
  check(stdc_contract_violation_line(&v) == s.line);
  check(stdc_contract_violation_kind(&v) == s.kind);
  check(stdc_contract_violation_semantic(&v) == s.semantic);
  check(stdc_contract_violation_detection_mode(&v) == s.mode);
  check(v.comment() == s.comment);
  check(v.location().file_name() == s.file);
  check(v.location().line() == s.line);
  check(static_cast<int>(v.kind()) == s.kind);
  check(static_cast<int>(v.semantic()) == s.semantic);
  check(static_cast<int>(v.detection_mode()) == s.mode);
}

static const char comment[] = "crosscheck";
static const char file[] = "crosscheck.c";
static const char function[] = "c_function";

int main() {
  stdc_handle_observed_contract_violation_explicit(comment, file, function, 42);
  const char *cxx_file = std::source_location::current().file_name();
  unsigned cxx_line = std::source_location::current().line() + 1;
  std::contracts::handle_observed_contract_violation(comment);
  check(calls == 2);

  const Snapshot &c = snaps[0], &cxx = snaps[1];
  check(c.header == cxx.header);
  check(c.num_entries == 5 && cxx.num_entries == 5);
  check(!std::memcmp(c.field_ids, cxx.field_ids, c.num_entries));
  check(!std::memcmp(c.offsets, cxx.offsets, c.num_entries * sizeof *c.offsets));

  check(c.comment == comment && cxx.comment == comment);
  check(c.file == file && c.function == function && c.line == 42);
  check(!std::strcmp(cxx.file, cxx_file) && cxx.line == cxx_line);
  check(c.kind == CXA_AK_MANUAL && cxx.kind == CXA_AK_MANUAL);
  check(c.semantic == CXA_ES_OBSERVE && cxx.semantic == CXA_ES_OBSERVE);
  check(c.mode == CXA_DM_UNSPECIFIED && cxx.mode == CXA_DM_UNSPECIFIED);
}
