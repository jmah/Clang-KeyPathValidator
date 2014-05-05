//
// FitbitFBBinderVisitor.cpp
// Created by Jonathon Mah on 2014-01-25.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "FitbitFBBinderVisitor.h"

using namespace clang;


FBBinderVisitor::FBBinderVisitor(KeyPathValidationConsumer *Consumer, const CompilerInstance &Compiler)
    : Consumer(Consumer)
    , Compiler(Compiler)
{
  ASTContext &Ctx = Compiler.getASTContext();
  IdentifierTable &IDs = Ctx.Idents;
  IdentifierInfo *BindIIs[3] = {&IDs.get("bindToModel"), &IDs.get("keyPath"), &IDs.get("change")};
  BindSelector = Ctx.Selectors.getSelector(3, BindIIs);

  IdentifierInfo *BindMultipleIIs[3] = {&IDs.get("bindToModels"), &IDs.get("keyPaths"), &IDs.get("change")};
  BindMultipleSelector = Ctx.Selectors.getSelector(3, BindMultipleIIs);
  BindMultipleCountMismatchDiagID = Compiler.getDiagnostics().getCustomDiagID(DiagnosticsEngine::Error, "model and key path arrays must have same number of elements");
}

bool FBBinderVisitor::VisitObjCMessageExpr(ObjCMessageExpr *E) {
  if (E->getNumArgs() != 3 || !E->isInstanceMessage())
    return true;

  if (E->getSelector() == BindSelector)
    ValidateModelAndKeyPath(E->getArg(0), E->getArg(1));

  if (E->getSelector() == BindMultipleSelector) {
    ObjCArrayLiteral *ModelsLiteral = dyn_cast<ObjCArrayLiteral>(E->getArg(0)->IgnoreImplicit());
    ObjCArrayLiteral *KeyPathsLiterals = dyn_cast<ObjCArrayLiteral>(E->getArg(1)->IgnoreImplicit());

    if (ModelsLiteral && KeyPathsLiterals) {
      if (ModelsLiteral->getNumElements() == KeyPathsLiterals->getNumElements()) {
        for (unsigned ModelIdx = 0, ModelCount = ModelsLiteral->getNumElements();
        ModelIdx < ModelCount; ++ModelIdx) {
          Expr *ModelExpr = ModelsLiteral->getElement(ModelIdx);
          Expr *KeyPathsExpr = KeyPathsLiterals->getElement(ModelIdx);

          if (ObjCArrayLiteral *KeyPathsLiteral = dyn_cast<ObjCArrayLiteral>(KeyPathsExpr->IgnoreImplicit())) {
            for (unsigned KeyPathIdx = 0, KeyPathCount = KeyPathsLiteral->getNumElements();
            KeyPathIdx < KeyPathCount; ++KeyPathIdx) {
              Expr *KeyPathExpr = KeyPathsLiteral->getElement(KeyPathIdx);
              ValidateModelAndKeyPath(ModelExpr, KeyPathExpr);
            }
          }
        }

      } else {
        Compiler.getDiagnostics().Report(ModelsLiteral->getLocStart(), BindMultipleCountMismatchDiagID)
          << ModelsLiteral << KeyPathsLiterals;
      }
    }
  }

  return true;
}

void FBBinderVisitor::ValidateModelAndKeyPath(const Expr *ModelExpr, const Expr *KeyPathExpr) {
  QualType ModelType = ModelExpr->IgnoreImplicit()->getType();
  const ObjCStringLiteral *KeyPathLiteral = dyn_cast<const ObjCStringLiteral>(KeyPathExpr->IgnoreImplicit());

  if (!KeyPathLiteral)
    return;

  QualType ObjType = ModelType;
  size_t Offset = 2; // @"
  typedef std::pair<StringRef,StringRef> StringPair;
  for (StringPair KeyAndPath = KeyPathLiteral->getString()->getString().split('.'); KeyAndPath.first.size() > 0; KeyAndPath = KeyAndPath.second.split('.')) {
    StringRef Key = KeyAndPath.first;
    bool Valid = Consumer->CheckKeyType(ObjType, Key);
    if (!Valid) {
      SourceRange KeyRange = KeyPathExpr->getSourceRange();
      SourceLocation KeyStart = KeyRange.getBegin().getLocWithOffset(Offset);
      KeyRange.setBegin(KeyStart);
      KeyRange.setEnd(KeyStart.getLocWithOffset(1));

      Compiler.getDiagnostics().Report(KeyStart, Consumer->KeyDiagID)
        << Key << ObjType->getPointeeType().getAsString()
          << KeyRange << ModelExpr->getSourceRange();
      break;
    }
    Offset += Key.size() + 1;
  }
}
