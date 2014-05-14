//
// ValidateKeyPathsAction.cpp
// Created by Jonathon Mah on 2014-01-24.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "KeyPathValidationConsumer.h"
#include "ValueForKeyVisitor.h"
#include "KeyPathsAffectingVisitor.h"
#include "FitbitFBBinderVisitor.h"

using namespace clang;


void KeyPathValidationConsumer::HandleTranslationUnit(ASTContext &Context) {
  cacheNSTypes();

  ValueForKeyVisitor(this, Compiler).TraverseDecl(Context.getTranslationUnitDecl());
  KeyPathsAffectingVisitor(this, Compiler).TraverseDecl(Context.getTranslationUnitDecl());
  FBBinderVisitor(this, Compiler).TraverseDecl(Context.getTranslationUnitDecl());
}


namespace {

class NullConsumer : public ASTConsumer {
public:
  NullConsumer()
    : ASTConsumer()
  { }
};


class ValidateKeyPathsAction : public PluginASTAction {
protected:
  ASTConsumer *CreateASTConsumer(CompilerInstance &compiler, llvm::StringRef) {
    LangOptions const opts = compiler.getLangOpts();
    if (opts.ObjC1 || opts.ObjC2)
      return new KeyPathValidationConsumer(compiler);
    else
      return new NullConsumer();
  }

  bool ParseArgs(const CompilerInstance &compiler,
                 const std::vector<std::string>& args) {
    return true;
  }
};

}


static FrontendPluginRegistry::Add<ValidateKeyPathsAction>
X("validate-key-paths", "warn if static key paths seem invalid");
