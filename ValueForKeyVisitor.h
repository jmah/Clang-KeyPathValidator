//
// ValueForKeyVisitor.h
// Created by Jonathon Mah on 2014-01-24.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef CLANG_KPV_VALUE_FOR_KEY_VISITOR_H
#define CLANG_KPV_VALUE_FOR_KEY_VISITOR_H

#include "KeyPathValidationConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;


class ValueForKeyVisitor : public RecursiveASTVisitor<ValueForKeyVisitor> {
  KeyPathValidationConsumer *Consumer;
  const CompilerInstance &Compiler;
  Selector VFKSelector, VFKPathSelector;

public:
  ValueForKeyVisitor(KeyPathValidationConsumer *Consumer, const CompilerInstance &Compiler)
    : Consumer(Consumer)
    , Compiler(Compiler)
  {
    ASTContext &Ctx = Compiler.getASTContext();
    VFKSelector = Ctx.Selectors.getUnarySelector(&Ctx.Idents.get("valueForKey"));
    VFKPathSelector = Ctx.Selectors.getUnarySelector(&Ctx.Idents.get("valueForKeyPath"));
  }

  bool shouldVisitTemplateInstantiations() const { return false; }
  bool shouldWalkTypesOfTypeLocs() const { return false; }

  bool VisitObjCMessageExpr(ObjCMessageExpr *E);
};

#endif
