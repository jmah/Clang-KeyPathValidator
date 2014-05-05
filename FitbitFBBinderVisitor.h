//
// FitbitFBBinderVisitor.h
// Created by Jonathon Mah on 2014-01-25.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef LLVM_CLANG_FITBIT_FBBINDER_VISITOR_H
#define LLVM_CLANG_FITBIT_FBBINDER_VISITOR_H

#include "KeyPathValidationConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;


class FBBinderVisitor : public RecursiveASTVisitor<FBBinderVisitor> {
private:
  KeyPathValidationConsumer *Consumer;
  const CompilerInstance &Compiler;
  Selector BindSelector;
  Selector BindMultipleSelector;
  unsigned BindMultipleCountMismatchDiagID;

public:
  FBBinderVisitor(KeyPathValidationConsumer *Consumer, const CompilerInstance &Compiler);

  bool shouldVisitTemplateInstantiations() const { return false; }
  bool shouldWalkTypesOfTypeLocs() const { return false; }

  bool VisitObjCMessageExpr(ObjCMessageExpr *E);
  void ValidateModelAndKeyPath(const Expr *ModelExpr, const Expr *KeyPathExpr);
};

#endif
