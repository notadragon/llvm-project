//===- ContractOptions.cpp - C++ Contract Options -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file defines the classes from ContractOptions.h
//
//===----------------------------------------------------------------------===//

#include "clang/Basic/ContractOptions.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/Basic/ContractConfig.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/VirtualFileSystem.h"

using namespace clang;

// Delegates to contractSemanticFromName (ContractOptions.h), the single source
// of truth for semantic-name spelling.
static std::optional<ContractEvaluationSemantic>
semanticFromString(StringRef Str) {
  ContractEvaluationSemantic Sem;
  if (contractSemanticFromName(Str, Sem))
    return Sem;
  return std::nullopt;
}

void ContractOptions::addUnparsedContractGroup(
    StringRef Group, const ContractOptions::DiagnoseGroupFunc &Diagnoser) {
  if (Group.empty())
    return Diagnoser(ContractGroupDiagnostic::Empty, "", "");

  // The format is group:semantic; the default semantic has its own option.
  if (!Group.contains(":"))
    return Diagnoser(ContractGroupDiagnostic::MissingGroup, Group, "");

  // The semantic follows the last colon: a P3100 group name (ub:...)
  // contains one itself.
  auto [Key, Value] = Group.rsplit(':');
  if (!validateContractGroup(Key, Diagnoser, /*AllowColon=*/true))
    return;

  // The entry is recorded as a configuration source by the caller
  // (addConfigSource) and turned into a ContractConfigEntry by initConfig;
  // nothing to store here.
  if (!semanticFromString(Value))
    return Diagnoser(ContractGroupDiagnostic::InvalidSemantic, Group, Value);
}

ContractConfigData &ContractOptions::ensureConfig() const {
  if (!Config)
    Config = std::make_shared<ContractConfigData>();
  return *Config;
}

llvm::ArrayRef<ContractConfigSource> ContractOptions::getConfigSources() const {
  if (!Config)
    return {};
  return Config->Sources;
}

void ContractOptions::addConfigSource(ContractConfigSourceKind Kind,
                                      llvm::StringRef Arg) {
  ensureConfig().Sources.push_back({Kind, Arg.str()});
}

static std::optional<ContractKind> kindFromString(llvm::StringRef Name) {
  return llvm::StringSwitch<std::optional<ContractKind>>(Name)
      .Case("pre", ContractKind::Pre)
      .Case("post", ContractKind::Post)
      .Case("contract_assert", ContractKind::Assert)
      .Case("implicit", ContractKind::Implicit)
      .Default(std::nullopt);
}

// Lazy namespace computation.
llvm::StringRef ContractQuery::getNamespace() const {
  if (!NamespaceComputed) {
    NamespaceComputed = true;
    if (FnContext) {
      llvm::SmallVector<llvm::StringRef> Parts;
      for (const DeclContext *Ctx = FnContext; Ctx; Ctx = Ctx->getParent()) {
        if (auto *NS = llvm::dyn_cast<NamespaceDecl>(Ctx)) {
          if (!NS->isAnonymousNamespace())
            Parts.push_back(NS->getName());
        }
      }
      if (!Parts.empty()) {
        std::string Result;
        for (int I = Parts.size() - 1; I >= 0; --I) {
          if (!Result.empty())
            Result += "::";
          Result += Parts[I];
        }
        CachedNamespace = std::move(Result);
      }
    }
  }
  return CachedNamespace;
}

// Lazy location computation.
llvm::StringRef ContractQuery::getLocationFile() const {
  if (!LocationComputed) {
    LocationComputed = true;
    if (SM && Loc.isValid()) {
      auto PLoc = SM->getPresumedLoc(Loc);
      if (PLoc.isValid()) {
        CachedLocationFile = PLoc.getFilename();
        CachedLocationLine = PLoc.getLine();
        CachedLocationColumn = PLoc.getColumn();
      }
    }
  }
  return CachedLocationFile;
}

int ContractQuery::getLocationLine() const {
  if (!LocationComputed)
    getLocationFile();
  return CachedLocationLine;
}

int ContractQuery::getLocationColumn() const {
  if (!LocationComputed)
    getLocationFile();
  return CachedLocationColumn;
}

// Namespace prefix matching on "::" delimiter.
static bool namespaceMatches(llvm::StringRef EntryNS, llvm::StringRef QueryNS) {
  if (!QueryNS.starts_with(EntryNS))
    return false;
  if (QueryNS.size() == EntryNS.size())
    return true;
  return QueryNS.size() > EntryNS.size() + 1 &&
         QueryNS[EntryNS.size()] == ':' && QueryNS[EntryNS.size() + 1] == ':';
}

// Filename suffix matching.
static bool filenameSuffixMatches(llvm::StringRef EntryFile,
                                  llvm::StringRef QueryFile) {
  if (!QueryFile.ends_with(EntryFile))
    return false;
  if (QueryFile.size() == EntryFile.size())
    return true;
  char Before = QueryFile[QueryFile.size() - EntryFile.size() - 1];
  return Before == '/' || Before == '\\';
}

// Parse a location string: "file", "file:N-M,P-Q" or "file:N-M,P-Q:C" (P3595
// allows a column "for the rare occasions where such precision is needed").
// A trailing ":column" is recognised only after a ":ranges" component, so a
// file name ending in ":digits" is still read as ranges.
// Returns false for a malformed line-range list (trailing junk, an empty or
// non-numeric bound, or an end before its start), which the caller diagnoses.
static bool
parseLocationString(llvm::StringRef LocStr, std::string &OutFile,
                    llvm::SmallVector<ContractLineRange> &OutRanges,
                    int &OutColumn) {
  OutColumn = 0;
  auto LastColon = LocStr.rfind(':');
  if (LastColon != llvm::StringRef::npos && LastColon != 0) {
    llvm::StringRef Tail = LocStr.substr(LastColon + 1);
    llvm::StringRef Head = LocStr.substr(0, LastColon);
    auto PrevColon = Head.rfind(':');
    if (!Tail.empty() && llvm::all_of(Tail, llvm::isDigit) &&
        PrevColon != llvm::StringRef::npos) {
      llvm::StringRef Ranges = Head.substr(PrevColon + 1);
      if (!Ranges.empty() && llvm::isDigit(Ranges[0]) &&
          Ranges.find_first_not_of("0123456789-,") == llvm::StringRef::npos) {
        Tail.getAsInteger(10, OutColumn);
        LocStr = Head;
      }
    }
  }
  auto ColonPos = LocStr.rfind(':');
  if (ColonPos == llvm::StringRef::npos || ColonPos == 0) {
    OutFile = LocStr.str();
    return true;
  }
  llvm::StringRef After = LocStr.substr(ColonPos + 1);
  if (After.empty() || !llvm::isDigit(After[0])) {
    OutFile = LocStr.str();
    return true;
  }
  OutFile = LocStr.substr(0, ColonPos).str();
  auto IsNumber = [](llvm::StringRef S) {
    return !S.empty() && llvm::all_of(S, llvm::isDigit);
  };
  llvm::SmallVector<llvm::StringRef> RangeParts;
  After.split(RangeParts, ',');
  for (auto &Part : RangeParts) {
    auto [StartStr, EndStr] = Part.split('-');
    bool HasEnd = Part.contains('-');
    int Start = 0, End = 0;
    if (!IsNumber(StartStr) || StartStr.getAsInteger(10, Start))
      return false;
    if (!HasEnd)
      End = Start;
    else if (!IsNumber(EndStr) || EndStr.getAsInteger(10, End) || End < Start)
      return false;
    OutRanges.push_back({Start, End});
  }
  return true;
}

/// Return true if \p S is an identifier: a letter, '_' or '$' (or a byte of
/// a UTF-8 sequence) followed by those or digits.  A dynamic selector's name
/// is emitted as (or mangled into) a symbol, so anything else -- a
/// template-id, an operator-function-id, a parameter list, a space -- would
/// reach the object file as an invalid symbol.
static bool isSelectorIdentifier(llvm::StringRef S) {
  if (S.empty())
    return false;
  for (size_t I = 0, E = S.size(); I != E; ++I) {
    unsigned char C = S[I];
    if (C >= 0x80 || C == '_' || C == '$' || llvm::isAlpha(C))
      continue;
    if (I > 0 && llvm::isDigit(C))
      continue;
    return false;
  }
  return true;
}

/// Return true if \p Name (any leading "::" already removed) is a valid
/// dynamic selector name: identifiers separated by "::" for C++ linkage, a
/// single identifier for C.
static bool isValidSelectorName(llvm::StringRef Name, bool CXX) {
  if (!CXX)
    return isSelectorIdentifier(Name);
  llvm::SmallVector<llvm::StringRef, 4> Parts;
  Name.split(Parts, "::");
  return llvm::all_of(Parts, isSelectorIdentifier);
}

static const llvm::StringRef KnownMatchKeys[] = {
    "kind", "group", "caller", "constexpr", "namespace", "location"};
static const llvm::StringRef KnownOutputKeys[] = {"semantic", "dynamic"};
static const llvm::StringRef KnownCallerKeys[] = {"location", "namespace"};
static const llvm::StringRef KnownDynamicKeys[] = {"linkage", "name",
                                                   "provideweak"};

// Warn about each unknown key in Obj, and return whether there was one: an
// entry that specifies an unsupported input or output is skipped (P3595,
// "File Format"), rather than applied with that criterion dropped.
static bool warnUnknownKeys(DiagnosticsEngine &Diags,
                            const llvm::json::Object &Obj,
                            llvm::StringRef ObjName,
                            llvm::ArrayRef<llvm::StringRef> KnownKeys,
                            llvm::StringRef SourceDesc) {
  bool Unknown = false;
  for (const auto &KV : Obj) {
    bool Found = false;
    for (const auto &K : KnownKeys)
      if (llvm::StringRef(KV.first) == K) {
        Found = true;
        break;
      }
    if (!Found) {
      Diags.Report(diag::warn_contract_config_unknown_key)
          << llvm::StringRef(KV.first) << ObjName << SourceDesc;
      Unknown = true;
    }
  }
  return Unknown;
}

void ContractOptions::parseConfigJSON(DiagnosticsEngine &Diags,
                                      llvm::StringRef JSON,
                                      llvm::StringRef SourceDesc) const {
  auto Parsed = llvm::json::parse(JSON);
  if (!Parsed) {
    Diags.Report(diag::err_contract_config_invalid_json)
        << SourceDesc << llvm::toString(Parsed.takeError());
    return;
  }

  const auto *Arr = Parsed->getAsArray();
  if (!Arr) {
    Diags.Report(diag::err_contract_config_not_array) << SourceDesc;
    return;
  }

  for (size_t I = 0; I < Arr->size(); ++I) {
    const auto *EntryObj = (*Arr)[I].getAsObject();
    if (!EntryObj) {
      Diags.Report(diag::err_contract_config_entry_not_object)
          << static_cast<unsigned>(I) << SourceDesc;
      continue;
    }

    const auto *OutputVal = EntryObj->get("output");
    if (!OutputVal) {
      Diags.Report(diag::err_contract_config_missing_output)
          << static_cast<unsigned>(I) << SourceDesc;
      continue;
    }
    const auto *OutputObj = OutputVal->getAsObject();
    if (!OutputObj) {
      Diags.Report(diag::err_contract_config_output_not_object)
          << static_cast<unsigned>(I) << SourceDesc;
      continue;
    }

    ContractConfigEntry E;

    // A known key carrying the wrong JSON type is worth reporting rather than
    // ignoring: dropping a match criterion makes the entry *less* selective,
    // so a mistyped "kind" silently turns a narrowly-scoped entry into a
    // catch-all that matches every contract in the translation unit.  GCC
    // diagnoses each of these (gcc/c-family/contracts-config.cc); match it.
    enum ExpectedJSONType { EJT_String = 0, EJT_Bool = 1 };
    auto reportWrongType = [&](llvm::StringRef Key, ExpectedJSONType Ty) {
      Diags.Report(diag::err_contract_config_wrong_type)
          << Key << static_cast<unsigned>(I) << SourceDesc
          << static_cast<unsigned>(Ty);
    };

    if (const auto *SemVal = OutputObj->get("semantic")) {
      auto SemStr = SemVal->getAsString();
      if (!SemStr) {
        reportWrongType("semantic", EJT_String);
        continue;
      }
      auto Sem = semanticFromString(*SemStr);
      if (!Sem) {
        Diags.Report(diag::err_contract_config_invalid_semantic)
            << *SemStr << static_cast<unsigned>(I) << SourceDesc;
        continue;
      }
      E.Semantic = *Sem;
      E.HasSemantic = true;
      if (!NoexceptSemanticsAvailable &&
          (*Sem == ContractEvaluationSemantic::NoexceptEnforce ||
           *Sem == ContractEvaluationSemantic::NoexceptObserve))
        Diags.Report(diag::warn_contract_noexcept_semantic_needs_p4298)
            << 1 << *SemStr
            << (*Sem == ContractEvaluationSemantic::NoexceptEnforce
                    ? "enforce"
                    : "observe");
    }

    if (const auto *DynVal = OutputObj->get("dynamic")) {
      const auto *DynObj = DynVal->getAsObject();
      if (!DynObj) {
        Diags.Report(diag::err_contract_config_dynamic_not_object)
            << static_cast<unsigned>(I) << SourceDesc;
        continue;
      }

      if (const auto *NameVal = DynObj->get("name")) {
        auto NameStr = NameVal->getAsString();
        if (!NameStr) {
          reportWrongType("name", EJT_String);
          continue;
        }
        E.DynName = NameStr->str();
      } else {
        Diags.Report(diag::err_contract_config_dynamic_missing_name)
            << static_cast<unsigned>(I) << SourceDesc;
        continue;
      }

      if (const auto *LinkageVal = DynObj->get("linkage")) {
        auto LinkageStr = LinkageVal->getAsString();
        if (!LinkageStr) {
          reportWrongType("linkage", EJT_String);
          continue;
        }
        if (*LinkageStr == "C++") {
          E.DynLinkage = ContractDynamicLinkage::CXX;
        } else if (*LinkageStr == "C") {
          E.DynLinkage = ContractDynamicLinkage::C;
        } else {
          Diags.Report(diag::err_contract_config_dynamic_invalid_linkage)
              << *LinkageStr << static_cast<unsigned>(I) << SourceDesc;
          continue;
        }
      }

      // A "C++" name is a qualified name: a leading `::' names the global
      // namespace and is dropped, and every component must then be an
      // identifier.  A "C" name is a single identifier.
      {
        bool CXX = E.DynLinkage == ContractDynamicLinkage::CXX;
        llvm::StringRef Name = E.DynName;
        if (CXX)
          Name.consume_front("::");
        if (!isValidSelectorName(Name, CXX)) {
          Diags.Report(CXX ? diag::err_contract_config_dynamic_bad_name
                           : diag::err_contract_config_dynamic_bad_c_name)
              << E.DynName << static_cast<unsigned>(I) << SourceDesc;
          continue;
        }
        E.DynName = Name.str();
      }

      bool ProvideWeakSpecified = false;
      if (const auto *PwVal = DynObj->get("provideweak")) {
        auto PwBool = PwVal->getAsBoolean();
        if (!PwBool) {
          reportWrongType("provideweak", EJT_Bool);
          continue;
        }
        E.DynProvideWeak = *PwBool;
        ProvideWeakSpecified = true;
      }

      // semantic / provideweak interaction (design P3595 dynamic section 2.1):
      // "provideweak" needs a "semantic" value to hand back from the weak
      // definition.  Without "semantic": explicit provideweak:true is an
      // error, and an unspecified provideweak is treated as false.
      if (!E.HasSemantic) {
        if (ProvideWeakSpecified && E.DynProvideWeak) {
          Diags.Report(
              diag::err_contract_config_dynamic_provideweak_needs_semantic)
              << static_cast<unsigned>(I) << SourceDesc;
          continue;
        }
        E.DynProvideWeak = false;
      }

      if (warnUnknownKeys(Diags, *DynObj, "dynamic", KnownDynamicKeys,
                          SourceDesc))
        continue;
    }

    if (!E.HasSemantic && E.DynName.empty()) {
      Diags.Report(diag::err_contract_config_output_needs_semantic_or_dynamic)
          << static_cast<unsigned>(I) << SourceDesc;
      continue;
    }

    if (warnUnknownKeys(Diags, *OutputObj, "output", KnownOutputKeys,
                        SourceDesc))
      continue;

    if (const auto *MatchVal = EntryObj->get("match")) {
      const auto *MatchObj = MatchVal->getAsObject();
      if (!MatchObj) {
        Diags.Report(diag::err_contract_config_match_not_object)
            << static_cast<unsigned>(I) << SourceDesc;
        continue;
      }

      if (const auto *KindVal = MatchObj->get("kind")) {
        auto KindStr = KindVal->getAsString();
        if (!KindStr) {
          reportWrongType("kind", EJT_String);
          continue;
        }
        auto K = kindFromString(*KindStr);
        if (!K) {
          Diags.Report(diag::err_contract_config_invalid_kind)
              << *KindStr << static_cast<unsigned>(I) << SourceDesc;
          continue;
        }
        E.Kind = static_cast<int>(*K);
      }

      if (const auto *GroupVal = MatchObj->get("group")) {
        auto GroupStr = GroupVal->getAsString();
        if (!GroupStr) {
          reportWrongType("group", EJT_String);
          continue;
        }
        E.Group = GroupStr->str();
      }

      if (const auto *CallerVal = MatchObj->get("caller")) {
        if (auto CallerBool = CallerVal->getAsBoolean()) {
          E.CallerSide = *CallerBool ? 1 : 0;
        } else if (const auto *CallerObj = CallerVal->getAsObject()) {
          E.CallerSide = 1;
          if (const auto *LocVal = CallerObj->get("location")) {
            auto LocStr = LocVal->getAsString();
            if (!LocStr) {
              reportWrongType("location", EJT_String);
              continue;
            }
            if (!parseLocationString(*LocStr, E.CallerLocationFile,
                                     E.CallerLocationLines,
                                     E.CallerLocationColumn)) {
              Diags.Report(diag::err_contract_config_malformed_location)
                  << *LocStr << 1 << static_cast<unsigned>(I) << SourceDesc;
              continue;
            }
          }
          if (const auto *NsVal = CallerObj->get("namespace")) {
            auto NsStr = NsVal->getAsString();
            if (!NsStr) {
              reportWrongType("namespace", EJT_String);
              continue;
            }
            if (NsStr->empty()) {
              Diags.Report(diag::err_contract_config_empty_namespace)
                  << 1 << static_cast<unsigned>(I) << SourceDesc;
              continue;
            }
            E.CallerNamespace = NsStr->str();
          }
          if (warnUnknownKeys(Diags, *CallerObj, "caller", KnownCallerKeys,
                              SourceDesc))
            continue;
        } else {
          Diags.Report(diag::err_contract_config_caller_not_bool_or_object)
              << static_cast<unsigned>(I) << SourceDesc;
          continue;
        }
      }

      if (const auto *CEValue = MatchObj->get("constexpr")) {
        auto CEVal = CEValue->getAsBoolean();
        if (!CEVal) {
          reportWrongType("constexpr", EJT_Bool);
          continue;
        }
        E.ConstexprEval = *CEVal ? 1 : 0;
      }

      if (const auto *NsVal = MatchObj->get("namespace")) {
        auto NsStr = NsVal->getAsString();
        if (!NsStr) {
          reportWrongType("namespace", EJT_String);
          continue;
        }
        if (NsStr->empty()) {
          Diags.Report(diag::err_contract_config_empty_namespace)
              << 0 << static_cast<unsigned>(I) << SourceDesc;
          continue;
        }
        E.Namespace = NsStr->str();
      }

      if (const auto *LocVal = MatchObj->get("location")) {
        auto LocStr = LocVal->getAsString();
        if (!LocStr) {
          reportWrongType("location", EJT_String);
          continue;
        }
        if (!parseLocationString(*LocStr, E.LocationFile, E.LocationLines,
                                 E.LocationColumn)) {
          Diags.Report(diag::err_contract_config_malformed_location)
              << *LocStr << 0 << static_cast<unsigned>(I) << SourceDesc;
          continue;
        }
      }

      if (warnUnknownKeys(Diags, *MatchObj, "match", KnownMatchKeys,
                          SourceDesc))
        continue;
    }

    ensureConfig().Entries.push_back(std::move(E));
  }
}

void ContractOptions::parseConfigFile(DiagnosticsEngine &Diags,
                                      llvm::vfs::FileSystem &VFS,
                                      llvm::StringRef Path) const {
  auto File = VFS.getBufferForFile(Path);
  if (!File) {
    Diags.Report(diag::err_contract_config_cannot_open_file)
        << Path << File.getError().message();
    return;
  }
  parseConfigJSON(Diags, File.get()->getBuffer(), Path);
}

void ContractOptions::initConfig(DiagnosticsEngine &Diags,
                                 llvm::vfs::FileSystem &VFS) const {
  ContractConfigData &Data = ensureConfig();
  if (Data.Initialized)
    return;
  Data.Initialized = true;

  // Walk ordered config sources.
  for (const auto &Src : Data.Sources) {
    switch (Src.Kind) {
    case ContractConfigSourceKind::GroupSemantic: {
      llvm::StringRef Arg = Src.Arg;
      auto [Key, Value] = Arg.rsplit(':');
      bool IsDefault = !Arg.contains(':');
      if (IsDefault)
        std::swap(Key, Value);
      auto Sem = semanticFromString(Value);
      if (!Sem)
        continue;
      if (IsDefault) {
        // Handled as DefaultSemantic, skip config entry.
        continue;
      }
      ContractConfigEntry E;
      E.Kind = -1;
      E.CallerSide = -1;
      E.Group = Key.str();
      E.Semantic = *Sem;
      E.HasSemantic = true;
      Data.Entries.push_back(std::move(E));
      break;
    }
    case ContractConfigSourceKind::JSONInline:
      parseConfigJSON(Diags, Src.Arg, "<command-line>");
      break;
    case ContractConfigSourceKind::JSONFile:
      parseConfigFile(Diags, VFS, Src.Arg);
      break;
    }
  }

  // P3100: implicit contract assertions guarding core-language UB default to
  // the "assume" semantic (today's behaviour: no check, the UB is preserved).
  // This builtin entry is placed after any user-provided sources (so a user
  // config can still override it, first-match-wins) but before the global
  // default catch-all, so implicit assertions do not pick up the global
  // default. It is injected unconditionally: it can only ever match a
  // Kind=Implicit query, which the compiler emits only under -fcontracts-p3100,
  // so it is inert otherwise.  Like GCC, implicit "assume" is not gated on
  // -fcontracts-allow-assume: it introduces no new UB.
  {
    ContractConfigEntry ImplicitEntry;
    ImplicitEntry.Kind = static_cast<int>(ContractKind::Implicit);
    ImplicitEntry.CallerSide = -1;
    ImplicitEntry.Semantic = ContractEvaluationSemantic::Assume;
    ImplicitEntry.HasSemantic = true;
    Data.Entries.push_back(std::move(ImplicitEntry));
  }

  // Catch-all entry.
  ContractConfigEntry Catchall;
  Catchall.Kind = -1;
  Catchall.CallerSide = -1;
  Catchall.Semantic = DefaultSemantic;
  Catchall.HasSemantic = true;
  Data.Entries.push_back(std::move(Catchall));

  // P3595: Clang does not yet implement caller-side contract checking
  // (see the class-level comment on configRequestsCallerSideChecks()).
  // Warn once, here, if the effective configuration actually requests it.
  if (configRequestsCallerSideChecks())
    Diags.Report(diag::warn_contract_config_caller_unimplemented);
}

llvm::SmallVector<llvm::StringRef>
ContractOptions::getCXXDynamicSelectorNames() const {
  llvm::SmallVector<llvm::StringRef> Names;
  if (!Config)
    return Names;
  llvm::StringSet<> Seen;
  for (const auto &Entry : Config->Entries)
    if (!Entry.DynName.empty() &&
        Entry.DynLinkage == ContractDynamicLinkage::CXX &&
        Seen.insert(Entry.DynName).second)
      Names.push_back(Entry.DynName);
  return Names;
}

bool ContractOptions::configRequestsCallerSideChecks() const {
  if (!Config)
    return false;
  for (const auto &Entry : Config->Entries) {
    // A dynamic-only entry (no explicit "semantic") has no compile-time
    // Semantic to read; conservatively treat it as "not ignore" rather than
    // reading the field's default-initialized value.
    if (Entry.CallerSide == 1 &&
        (!Entry.HasSemantic ||
         Entry.Semantic != ContractEvaluationSemantic::Ignore))
      return true;
  }
  return false;
}

static bool groupMatches(StringRef EntryGroup, StringRef QueryGroup) {
  if (QueryGroup.starts_with(EntryGroup)) {
    if (QueryGroup.size() == EntryGroup.size())
      return true;
    if (QueryGroup[EntryGroup.size()] == '.')
      return true;
  }
  return false;
}

static bool configEntryMatches(const ContractConfigEntry &Entry,
                               const ContractQuery &Query) {
  if (Entry.Kind != -1 && Entry.Kind != static_cast<int>(Query.Kind))
    return false;

  if (Entry.CallerSide == -1) {
    if (Query.CallerSide)
      return false;
  } else if ((Entry.CallerSide == 1) != Query.CallerSide)
    return false;

  if (Entry.ConstexprEval != -1 &&
      Entry.ConstexprEval != static_cast<int>(Query.InConstantEvaluation))
    return false;

  // P3595 dynamic selection, constant-evaluation rule: a dynamic selector
  // function is never called during constant evaluation.  An entry that
  // carries "dynamic" but no explicit "semantic" has no compile-time value
  // to fall back to, so treat it as non-matching in a constant-evaluation
  // query (as if "constexpr: false" had been specified) and let the scan
  // continue to the next entry.  An entry with both "dynamic" and
  // "semantic" still matches; its Semantic is used and the descriptor is
  // unused (see resolveContractDynamic).
  if (Query.InConstantEvaluation && !Entry.DynName.empty() &&
      !Entry.HasSemantic)
    return false;

  if (!Entry.Group.empty()) {
    if (Query.Groups.empty())
      return false;
    bool Found = false;
    for (const auto &QG : Query.Groups) {
      if (groupMatches(Entry.Group, QG)) {
        Found = true;
        break;
      }
    }
    if (!Found)
      return false;
  }

  if (!Entry.Namespace.empty()) {
    auto QNS = Query.getNamespace();
    if (QNS.empty() || !namespaceMatches(Entry.Namespace, QNS))
      return false;
  }

  if (!Entry.LocationFile.empty()) {
    auto QFile = Query.getLocationFile();
    if (QFile.empty() || !filenameSuffixMatches(Entry.LocationFile, QFile))
      return false;
    if (!Entry.LocationLines.empty()) {
      int QLine = Query.getLocationLine();
      bool InRange = false;
      for (const auto &R : Entry.LocationLines) {
        if (QLine >= R.Start && QLine <= R.End) {
          InRange = true;
          break;
        }
      }
      if (!InRange)
        return false;
    }
    if (Entry.LocationColumn &&
        Query.getLocationColumn() != Entry.LocationColumn)
      return false;
  }

  return true;
}

ContractEvaluationSemantic
ContractOptions::resolveContractSemantic(const ContractQuery &Query) const {
  assert(Config && Config->Initialized &&
         "contract resolved before initConfig built the configuration");

  ContractEvaluationSemantic Candidate = ContractEvaluationSemantic::Enforce;
  bool Matched = false;

  for (const auto &Entry : Config->Entries) {
    if (configEntryMatches(Entry, Query)) {
      // A dynamic-only entry (no explicit "semantic") has no compile-time
      // Semantic to read.  Leave Candidate at its pre-loop default rather
      // than reading the field's default-initialized value; dynamic
      // dispatch for such entries is resolved via resolveContractDynamic /
      // codegen (P3595), not here.
      if (Entry.HasSemantic)
        Candidate = Entry.Semantic;
      Matched = true;
      break;
    }
  }

  if (!Matched && Query.CallerSide)
    return ContractEvaluationSemantic::Ignore;

  // Clamp the compile-time candidate into the allowed set (P3100 assume rule +
  // fallback order).  The same helper is reused by the P3595 dynamic transform
  // table so both paths clamp identically (see ContractStmt dynamic support /
  // applyLabelFacets).
  return clampToAllowed(Candidate, Query.AllowedMask);
}

ContractDynamicResult
ContractOptions::resolveContractDynamic(const ContractQuery &Query) const {
  assert(Config && Config->Initialized &&
         "contract resolved before initConfig built the configuration");

  for (const auto &Entry : Config->Entries) {
    if (!configEntryMatches(Entry, Query))
      continue;

    // configEntryMatches already applies the constant-evaluation skip rule:
    // a dynamic-only entry never matches when Query.InConstantEvaluation.
    // An entry with both "dynamic" and "semantic" can still match in that
    // case, but per P3595's constant-evaluation rule its descriptor is
    // unused there -- the caller gets the entry's Semantic from
    // resolveContractSemantic instead.  Guard on InConstantEvaluation here
    // too so this function's contract holds independently of that rule
    // living in the shared matcher.
    if (Entry.DynName.empty() || Query.InConstantEvaluation)
      return {};

    return {Entry.DynName, Entry.DynLinkage, Entry.DynProvideWeak, true};
  }

  return {};
}

bool ContractOptions::validateContractGroup(
    llvm::StringRef GroupName,
    const ContractOptions::DiagnoseGroupFunc &Diagnoser, bool AllowColon) {
  using CGD = ContractGroupDiagnostic;
  if (GroupName.empty()) {
    Diagnoser(CGD::Empty, GroupName, "");
    return false;
  }

  // A P3100 group name (ub:...) named on the command line contains ':'.
  if (auto Pos = GroupName.find_first_not_of(
          AllowColon ? "abcdefghijklmnopqrstuvwxyz"
                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                       "0123456789"
                       "_.-:"
                     : "abcdefghijklmnopqrstuvwxyz"
                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                       "0123456789"
                       "_.-");
      Pos != StringRef::npos) {
    Diagnoser(CGD::InvalidChar, GroupName, GroupName.substr(Pos, 1));
    return false;
  }

  if (GroupName[0] == '.') {
    Diagnoser(CGD::InvalidFirstChar, GroupName, ".");
    return false;
  }
  if (GroupName.back() == '.') {
    Diagnoser(CGD::InvalidLastChar, GroupName, ".");
    return false;
  }
  // Diagnose empty subgroups. i.e. "a..b"
  if (GroupName.find("..", 0) != StringRef::npos) {
    Diagnoser(CGD::EmptySubGroup, GroupName, "..");
    return false;
  }
  return true;
}
