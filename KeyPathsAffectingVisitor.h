//
// KeyPathsAffectingVisitor.h
// Created by Jonathon Mah on 2014-05-11.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef CLANG_KPV_KEY_PATHS_AFFECTING_VISITOR_H
#define CLANG_KPV_KEY_PATHS_AFFECTING_VISITOR_H

#include "KeyPathValidationConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;


class KeyPathsAffectingVisitor : public RecursiveASTVisitor<KeyPathsAffectingVisitor> {
  KeyPathValidationConsumer *Consumer;
  const CompilerInstance &Compiler;
  std::string Prefix;
  llvm::SmallSet<Selector, 2> SetConstructorSelectors;
  //RecursiveASTVisitor Visitor;

public:
  KeyPathsAffectingVisitor(KeyPathValidationConsumer *Consumer, const CompilerInstance &Compiler)
    : Consumer(Consumer)
    , Compiler(Compiler)
    , Prefix("keyPathsForValuesAffecting")
  {
    ASTContext &Ctx = Compiler.getASTContext();
    SetConstructorSelectors.insert(Ctx.Selectors.getUnarySelector(&Ctx.Idents.get("setWithObject")));
    SetConstructorSelectors.insert(Ctx.Selectors.getUnarySelector(&Ctx.Idents.get("setWithObjects")));
  }

  bool shouldVisitTemplateInstantiations() const { return false; }
  bool shouldWalkTypesOfTypeLocs() const { return false; }

  bool VisitObjCMethodDecl(ObjCMethodDecl *D);
};

#endif
