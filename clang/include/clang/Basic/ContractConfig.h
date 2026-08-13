//===- ContractConfig.h - P3595 contract config storage ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
/// \file
/// Defines the parsed-configuration storage types for the P3595 contract
/// configuration system: the ordered list of ContractConfigEntry rules, the
/// command-line ContractConfigSource records, and the ContractConfigData that
/// aggregates them.
///
/// These types are deliberately kept out of ContractOptions.h (which is pulled
/// in transitively by LangOptions.h, and therefore by most of Clang).
/// ContractConfigEntry in particular grows as new match criteria are added, so
/// only the two translation units that actually build and resolve the
/// configuration -- ContractOptions.cpp and CompilerInvocation.cpp -- include
/// this header.  ContractOptions holds a ContractConfigData behind a pointer.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_BASIC_CONTRACT_CONFIG_H
#define LLVM_CLANG_BASIC_CONTRACT_CONFIG_H

#include "clang/Basic/ContractOptions.h"
#include "llvm/ADT/SmallVector.h"
#include <string>

namespace clang {

/// One source of contract configuration, recorded in command-line order.
struct ContractConfigSource {
  ContractConfigSourceKind Kind;
  std::string Arg;
};

/// A line range for location matching.
struct ContractLineRange {
  int Start;
  int End;
};

/// One rule in the P3595 ordered contract configuration list.
struct ContractConfigEntry {
  int Kind = -1;            // ContractKind (Pre/Post/Assert) or -1 (any)
  int CallerSide = -1;      // 1=caller, 0=callee, -1=any
  int ConstexprEval = -1;   // 1=CE-only, 0=runtime-only, -1=both (default)
  std::string Group;        // Group prefix to match, or empty (any)
  std::string Namespace;    // Namespace prefix to match, or empty (any)
  std::string LocationFile; // Filename suffix to match, or empty (any)
  llvm::SmallVector<ContractLineRange> LocationLines; // Line ranges
  std::string CallerNamespace;    // Caller-context namespace prefix, or empty
  std::string CallerLocationFile; // Caller-context filename suffix, or empty
  llvm::SmallVector<ContractLineRange> CallerLocationLines; // Line ranges
  ContractEvaluationSemantic Semantic = ContractEvaluationSemantic::Enforce;

  // output.dynamic descriptor.  DynName empty means no dynamic selection.
  std::string DynName;
  int DynLinkage = 0; // 0 = "C++", 1 = "C"
  bool DynProvideWeak = true;
  bool HasSemantic = false; // was "semantic" explicitly given?
};

/// The parsed P3595 configuration for a translation unit: the ordered command
/// line sources and the resolved entry list built from them by
/// ContractOptions::initConfig().  Held behind a pointer by ContractOptions.
struct ContractConfigData {
  /// Configuration sources in command-line order.
  llvm::SmallVector<ContractConfigSource> Sources;

  /// The ordered entry list, built lazily from Sources.
  llvm::SmallVector<ContractConfigEntry, 0> Entries;

  /// Whether Entries has been built from Sources yet.
  bool Initialized = false;
};

} // namespace clang

#endif // LLVM_CLANG_BASIC_CONTRACT_CONFIG_H
