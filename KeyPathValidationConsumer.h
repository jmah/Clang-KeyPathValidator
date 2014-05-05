//
// KeyPathValidationConsumer.h
// Created by Jonathon Mah on 2014-01-24.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef CLANG_KPV_KEY_PATH_VALIDATION_CONSUMER_H
#define CLANG_KPV_KEY_PATH_VALIDATION_CONSUMER_H

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/NSAPI.h"
#include "clang/AST/Attr.h"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;

class KeyPathValidationConsumer : public ASTConsumer {
public:
  KeyPathValidationConsumer(const CompilerInstance &Compiler)
    : ASTConsumer()
    , Compiler(Compiler)
    , Context(Compiler.getASTContext())
  {
    NSAPIObj.reset(new NSAPI(Context));
    KeyDiagID = Compiler.getDiagnostics().getCustomDiagID(DiagnosticsEngine::Warning, "key '%0' not found on type %1");
  }

  virtual void HandleTranslationUnit(ASTContext &Context);

  bool CheckKeyType(QualType &ObjTypeInOut, StringRef &Key);

  unsigned KeyDiagID;

private:
  const CompilerInstance &Compiler;
  ASTContext &Context;
  OwningPtr<NSAPI> NSAPIObj;

  QualType NSNumberPtrType;

  // Hard-coded set of KVC containers (can't add attributes in a category)
  ObjCInterfaceDecl *NSDictionaryInterface, *NSArrayInterface, *NSSetInterface, *NSOrderedSetInterface;

  void cacheNSTypes();
  bool isKVCContainer(QualType type);
};

#endif
