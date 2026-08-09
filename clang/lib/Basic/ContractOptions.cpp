//===- Contracts.cpp - C Language Family Language Options -----------------===//
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
#include "clang/Basic/ContractConfig.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/Decl.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/VirtualFileSystem.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

std::optional<ContractEvaluationSemantic> semanticFromString(StringRef Str) {
  return llvm::StringSwitch<std::optional<ContractEvaluationSemantic>>(Str)
      .Case("ignore", ContractEvaluationSemantic::Ignore)
      .Case("enforce", ContractEvaluationSemantic::Enforce)
      .Case("observe", ContractEvaluationSemantic::Observe)
      .Case("quick_enforce", ContractEvaluationSemantic::QuickEnforce)
      .Case("assume", ContractEvaluationSemantic::Assume)
      .Case("noexcept_enforce", ContractEvaluationSemantic::NoexceptEnforce)
      .Case("noexcept_observe", ContractEvaluationSemantic::NoexceptObserve)
      .Default(std::nullopt);
}

void ContractOptions::addUnparsedContractGroup(
    StringRef Group, const ContractOptions::DiagnoseGroupFunc &Diagnoser) {
  if (Group.empty())
    return Diagnoser(ContractGroupDiagnostic::Empty, "", "");

  const bool ParseAsValueOnly = not Group.contains(":");
  auto [Key, Value] = Group.split(':');
  if (ParseAsValueOnly) {
    assert(Value.empty() && "Value should be empty");
    std::swap(Key, Value);
  } else {
    if (!validateContractGroup(Key, Diagnoser))
      return;
  }

  std::optional<ContractEvaluationSemantic> Sem = semanticFromString(Value);
  if (!Sem)
    return Diagnoser(ContractGroupDiagnostic::InvalidSemantic, Group, Value);

  // A bare "semantic" sets the catch-all default.  A "group:semantic" entry is
  // recorded as a configuration source by the caller (addConfigSource) and
  // turned into a ContractConfigEntry by initConfig; nothing to store here.
  if (ParseAsValueOnly)
    DefaultSemantic = Sem.value();
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

// Namespace prefix matching on "::" delimiter.
static bool namespaceMatches(llvm::StringRef EntryNS, llvm::StringRef QueryNS) {
  if (!QueryNS.starts_with(EntryNS))
    return false;
  if (QueryNS.size() == EntryNS.size())
    return true;
  return QueryNS.size() > EntryNS.size() + 1 &&
         QueryNS[EntryNS.size()] == ':' &&
         QueryNS[EntryNS.size() + 1] == ':';
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

// Parse a location string: "file" or "file:N-M,P-Q".
static void parseLocationString(llvm::StringRef LocStr,
                                std::string &OutFile,
                                llvm::SmallVector<ContractLineRange> &OutRanges) {
  auto ColonPos = LocStr.rfind(':');
  if (ColonPos == llvm::StringRef::npos || ColonPos == 0) {
    OutFile = LocStr.str();
    return;
  }
  llvm::StringRef After = LocStr.substr(ColonPos + 1);
  if (After.empty() || !isdigit(After[0])) {
    OutFile = LocStr.str();
    return;
  }
  OutFile = LocStr.substr(0, ColonPos).str();
  llvm::SmallVector<llvm::StringRef> RangeParts;
  After.split(RangeParts, ',');
  for (auto &Part : RangeParts) {
    auto [StartStr, EndStr] = Part.split('-');
    int Start = 0, End = 0;
    StartStr.getAsInteger(10, Start);
    if (EndStr.empty())
      End = Start;
    else
      EndStr.getAsInteger(10, End);
    OutRanges.push_back({Start, End});
  }
}

static const llvm::StringRef KnownMatchKeys[] = {"kind", "group", "caller",
                                                  "constexpr", "namespace",
                                                  "location"};
static const llvm::StringRef KnownOutputKeys[] = {"semantic", "dynamic"};
static const llvm::StringRef KnownCallerKeys[] = {"location", "namespace"};
static const llvm::StringRef KnownDynamicKeys[] = {"linkage", "name",
                                                    "provideweak"};

static void warnUnknownKeys(DiagnosticsEngine &Diags,
                            const llvm::json::Object &Obj,
                            llvm::StringRef ObjName,
                            llvm::ArrayRef<llvm::StringRef> KnownKeys,
                            llvm::StringRef SourceDesc) {
  for (const auto &KV : Obj) {
    bool Found = false;
    for (const auto &K : KnownKeys)
      if (llvm::StringRef(KV.first) == K) {
        Found = true;
        break;
      }
    if (!Found)
      Diags.Report(diag::warn_contract_config_unknown_key)
          << llvm::StringRef(KV.first) << ObjName << SourceDesc;
  }
}

void ContractOptions::parseConfigJSON(DiagnosticsEngine &Diags,
                                      llvm::StringRef JSON,
                                      llvm::StringRef SourceDesc) {
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
          << (unsigned)I << SourceDesc;
      continue;
    }

    const auto *OutputVal = EntryObj->get("output");
    if (!OutputVal) {
      Diags.Report(diag::err_contract_config_missing_output)
          << (unsigned)I << SourceDesc;
      continue;
    }
    const auto *OutputObj = OutputVal->getAsObject();
    if (!OutputObj) {
      Diags.Report(diag::err_contract_config_output_not_object)
          << (unsigned)I << SourceDesc;
      continue;
    }

    ContractConfigEntry E;

    if (auto SemStr = OutputObj->getString("semantic")) {
      auto Sem = semanticFromString(*SemStr);
      if (!Sem) {
        Diags.Report(diag::err_contract_config_invalid_semantic)
            << *SemStr << (unsigned)I << SourceDesc;
        continue;
      }
      E.Semantic = *Sem;
      E.HasSemantic = true;
    }

    if (const auto *DynVal = OutputObj->get("dynamic")) {
      const auto *DynObj = DynVal->getAsObject();
      if (!DynObj) {
        Diags.Report(diag::err_contract_config_dynamic_not_object)
            << (unsigned)I << SourceDesc;
        continue;
      }

      if (auto NameStr = DynObj->getString("name")) {
        E.DynName = NameStr->str();
      } else {
        Diags.Report(diag::err_contract_config_dynamic_missing_name)
            << (unsigned)I << SourceDesc;
        continue;
      }

      if (auto LinkageStr = DynObj->getString("linkage")) {
        if (*LinkageStr == "C++") {
          E.DynLinkage = 0;
        } else if (*LinkageStr == "C") {
          E.DynLinkage = 1;
        } else {
          Diags.Report(diag::err_contract_config_dynamic_invalid_linkage)
              << *LinkageStr << (unsigned)I << SourceDesc;
          continue;
        }
      }

      bool ProvideWeakSpecified = false;
      if (auto PwBool = DynObj->getBoolean("provideweak")) {
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
              << (unsigned)I << SourceDesc;
          continue;
        }
        E.DynProvideWeak = false;
      }

      warnUnknownKeys(Diags, *DynObj, "dynamic", KnownDynamicKeys,
                      SourceDesc);
    }

    if (!E.HasSemantic && E.DynName.empty()) {
      Diags.Report(diag::err_contract_config_output_needs_semantic_or_dynamic)
          << (unsigned)I << SourceDesc;
      continue;
    }

    warnUnknownKeys(Diags, *OutputObj, "output", KnownOutputKeys, SourceDesc);

    if (const auto *MatchVal = EntryObj->get("match")) {
      const auto *MatchObj = MatchVal->getAsObject();
      if (!MatchObj) {
        Diags.Report(diag::err_contract_config_match_not_object)
            << (unsigned)I << SourceDesc;
        continue;
      }

      if (auto KindStr = MatchObj->getString("kind")) {
        auto K = kindFromString(*KindStr);
        if (!K) {
          Diags.Report(diag::err_contract_config_invalid_kind)
              << *KindStr << (unsigned)I << SourceDesc;
          continue;
        }
        E.Kind = static_cast<int>(*K);
      }

      if (auto GroupStr = MatchObj->getString("group"))
        E.Group = GroupStr->str();

      if (const auto *CallerVal = MatchObj->get("caller")) {
        if (auto CallerBool = CallerVal->getAsBoolean()) {
          E.CallerSide = *CallerBool ? 1 : 0;
        } else if (const auto *CallerObj = CallerVal->getAsObject()) {
          E.CallerSide = 1;
          if (auto LocStr = CallerObj->getString("location"))
            parseLocationString(*LocStr, E.CallerLocationFile,
                                E.CallerLocationLines);
          if (auto NsStr = CallerObj->getString("namespace"))
            E.CallerNamespace = NsStr->str();
          warnUnknownKeys(Diags, *CallerObj, "caller", KnownCallerKeys,
                          SourceDesc);
        } else {
          Diags.Report(diag::err_contract_config_caller_not_bool_or_object)
              << (unsigned)I << SourceDesc;
          continue;
        }
      }

      if (auto CEVal = MatchObj->getBoolean("constexpr"))
        E.ConstexprEval = *CEVal ? 1 : 0;

      if (auto NsStr = MatchObj->getString("namespace"))
        E.Namespace = NsStr->str();

      if (auto LocStr = MatchObj->getString("location"))
        parseLocationString(*LocStr, E.LocationFile, E.LocationLines);

      warnUnknownKeys(Diags, *MatchObj, "match", KnownMatchKeys, SourceDesc);
    }

    ensureConfig().Entries.push_back(std::move(E));
  }
}

void ContractOptions::parseConfigFile(DiagnosticsEngine &Diags,
                                      llvm::vfs::FileSystem &VFS,
                                      llvm::StringRef Path) {
  auto File = VFS.getBufferForFile(Path);
  if (!File) {
    Diags.Report(diag::err_contract_config_cannot_open_file)
        << Path << File.getError().message();
    return;
  }
  parseConfigJSON(Diags, File.get()->getBuffer(), Path);
}

void ContractOptions::initConfig(DiagnosticsEngine *Diags,
                                 llvm::vfs::FileSystem *VFS) const {
  ContractConfigData &Data = ensureConfig();
  if (Data.Initialized)
    return;
  Data.Initialized = true;

  // The configuration is always parsed eagerly, with a DiagnosticsEngine and a
  // VFS, from CompilerInstance before any lazy resolution occurs, so Diags is
  // present the first (and only) time initialization actually runs.
  assert(Diags && "contract configuration initialized without a "
                  "DiagnosticsEngine");

  // Walk ordered config sources.
  for (const auto &Src : Data.Sources) {
    switch (Src.Kind) {
    case ContractConfigSourceKind::GroupSemantic: {
      llvm::StringRef Arg = Src.Arg;
      auto [Key, Value] = Arg.split(':');
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
      const_cast<ContractOptions *>(this)->parseConfigJSON(
          *Diags, Src.Arg, "<command-line>");
      break;
    case ContractConfigSourceKind::JSONFile:
      assert(VFS && "contract configuration file read without a VFS");
      const_cast<ContractOptions *>(this)->parseConfigFile(*Diags, *VFS,
                                                           Src.Arg);
      break;
    }
  }

  // P3100: implicit contract assertions guarding core-language UB default to
  // the "assume" semantic (today's behaviour: no check, the UB is preserved).
  // This builtin entry is placed after any user-provided sources (so a user
  // config can still override it, first-match-wins) but before the global
  // default catch-all, so implicit assertions do not pick up the global default.
  // It is injected unconditionally: it can only ever match a Kind=Implicit
  // query, which the compiler emits only under -fcontracts-p3100, so it is inert
  // otherwise.  Like GCC, implicit "assume" is not gated on
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
    Diags->Report(diag::warn_contract_config_caller_unimplemented);
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
  if (Entry.Kind != -1 &&
      Entry.Kind != static_cast<int>(Query.Kind))
    return false;

  if (Entry.CallerSide == -1) {
    if (Query.CallerSide)
      return false;
  } else if ((Entry.CallerSide == 1) != Query.CallerSide)
    return false;

  if (Entry.ConstexprEval != -1 &&
      Entry.ConstexprEval != (int)Query.InConstantEvaluation)
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
  }

  return true;
}

// The safety level of a semantic: higher is "safer" (more checking).  Two
// semantics share a level when they differ only in throwing-ness.
static int semanticLevel(ContractEvaluationSemantic S) {
  switch (S) {
  case ContractEvaluationSemantic::Assume:
    return 0;
  case ContractEvaluationSemantic::Ignore:
    return 1;
  case ContractEvaluationSemantic::Observe:
  case ContractEvaluationSemantic::NoexceptObserve:
    return 2;
  case ContractEvaluationSemantic::Enforce:
  case ContractEvaluationSemantic::NoexceptEnforce:
    return 3;
  case ContractEvaluationSemantic::QuickEnforce:
    return 4;
  }
  return -1;
}

// Return the semantic present in Mask at safety level Lvl, trying the two
// variants (levels 2 and 3) in an order that prefers the throwing variant when
// PreferThrowing, else the noexcept one.  Value 0 (not a valid semantic) means
// the level has nothing in Mask.
static ContractEvaluationSemantic
semanticAtLevel(int Lvl, unsigned Mask, bool PreferThrowing) {
  using CES = ContractEvaluationSemantic;
  CES A = static_cast<CES>(0), B = static_cast<CES>(0);
  switch (Lvl) {
  case 0:
    A = CES::Assume;
    break;
  case 1:
    A = CES::Ignore;
    break;
  case 2:
    A = PreferThrowing ? CES::Observe : CES::NoexceptObserve;
    B = PreferThrowing ? CES::NoexceptObserve : CES::Observe;
    break;
  case 3:
    A = PreferThrowing ? CES::Enforce : CES::NoexceptEnforce;
    B = PreferThrowing ? CES::NoexceptEnforce : CES::Enforce;
    break;
  case 4:
    A = CES::QuickEnforce;
    break;
  default:
    return static_cast<CES>(0);
  }
  if (static_cast<unsigned>(A) && (Mask & (1u << static_cast<unsigned>(A))))
    return A;
  if (static_cast<unsigned>(B) && (Mask & (1u << static_cast<unsigned>(B))))
    return B;
  return static_cast<CES>(0);
}

ContractEvaluationSemantic
ContractOptions::clampToAllowed(ContractEvaluationSemantic Candidate,
                                unsigned AllowedMask) {
  // Walk the safety levels outward from Candidate's own level -- same level,
  // then upward (nearest safer), then downward (safest available) -- returning
  // the first semantic present in AllowedMask, preferring at each two-variant
  // level the variant matching Candidate's throwing-ness (a potentially-throwing
  // Candidate prefers observe/enforce; a non-throwing one prefers the noexcept_
  // variant).  This subsumes the old assume->ignore special case (assume is
  // level 0, so the upward walk reaches ignore first).
  using CES = ContractEvaluationSemantic;
  int L = semanticLevel(Candidate);
  if (L < 0)
    return Candidate;
  bool PreferThrowing = (Candidate == CES::Observe || Candidate == CES::Enforce);
  const CES None = static_cast<CES>(0);

  if (CES R = semanticAtLevel(L, AllowedMask, PreferThrowing); R != None)
    return R;
  for (int Lvl = L + 1; Lvl <= 4; ++Lvl)
    if (CES R = semanticAtLevel(Lvl, AllowedMask, PreferThrowing); R != None)
      return R;
  for (int Lvl = L - 1; Lvl >= 0; --Lvl)
    if (CES R = semanticAtLevel(Lvl, AllowedMask, PreferThrowing); R != None)
      return R;

  // Empty allowed set: an empty label mask is diagnosed in Sema
  // (applyLabelFacets) before resolution, and every other mask contains a
  // semantic, so this is unreachable in practice.  Return the candidate rather
  // than inventing an out-of-set semantic.
  return Candidate;
}

ContractEvaluationSemantic
ContractOptions::resolveContractSemantic(const ContractQuery &Query) const {
  initConfig(); // ensures Config is allocated and Entries is built

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
  initConfig(); // ensures Config is allocated and Entries is built

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
    const ContractOptions::DiagnoseGroupFunc &Diagnoser) {
  using CGD = ContractGroupDiagnostic;
  if (GroupName.empty()) {
    Diagnoser(CGD::Empty, GroupName, "");
    return false;
  }

  if (auto Pos = GroupName.find_first_not_of("abcdefghijklmnopqrstuvwxyz"
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
