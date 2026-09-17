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

