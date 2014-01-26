//
// KeyPathValidator.cpp
// Created by Jonathon Mah on 2014-01-24.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace {

class KeyPathValidationConsumer : public ASTConsumer {
public:
  KeyPathValidationConsumer(const CompilerInstance &compiler)
    : ASTConsumer()
    , Compiler(compiler)
  { }

  virtual void HandleTranslationUnit(clang::ASTContext &Context);

private:
  const CompilerInstance &Compiler;
};


class ValueForKeyVisitor : public RecursiveASTVisitor<ValueForKeyVisitor> {
  const CompilerInstance &Compiler;

public:
  ValueForKeyVisitor(const CompilerInstance &compiler)
    : Compiler(compiler)
  { }

  bool shouldVisitTemplateInstantiations() const { return false; }
  bool shouldWalkTypesOfTypeLocs() const { return false; }

  bool VisitObjCMessageExpr(ObjCMessageExpr *E) {
    if (E->getNumArgs() == 1 && E->isInstanceMessage() && E->getSelector().getAsString() == "valueForKey:") {
      Expr *A1 = E->getArg(0);

      if (ObjCStringLiteral *SL = dyn_cast<ObjCStringLiteral>(A1)) {
        ObjCInterfaceDecl *RecvID = E->getReceiverInterface();
        ASTContext &Ctx = Compiler.getASTContext();

        std::string KeyString = SL->getString()->getString().str();
        Selector Sel = Ctx.Selectors.getNullarySelector(&Ctx.Idents.get(KeyString));

        if (!RecvID->lookupInstanceMethod(Sel)) {
          DiagnosticsEngine &de = Compiler.getDiagnostics();
          unsigned id = de.getCustomDiagID(DiagnosticsEngine::Warning, "test vfk " + KeyString);
          DiagnosticBuilder B = de.Report(E->getLocStart(), id);
        }
      }
    }
    return true;
  }
};


class FBBinderVisitor : public RecursiveASTVisitor<FBBinderVisitor> {
  const CompilerInstance &Compiler;

public:
  FBBinderVisitor(const CompilerInstance &compiler)
    : Compiler(compiler)
  { }

  bool shouldVisitTemplateInstantiations() const { return false; }
  bool shouldWalkTypesOfTypeLocs() const { return false; }

  bool VisitObjCMessageExpr(ObjCMessageExpr *E) {
    if (E->getNumArgs() == 3 && E->isInstanceMessage() && E->getSelector().getAsString() == "bindToModel:keyPath:change:") {
      QualType ModelType = E->getArg(0)->IgnoreImplicit()->getType();
      ObjCStringLiteral *KeyPathLiteral = dyn_cast<ObjCStringLiteral>(E->getArg(1));

      const ObjCObjectPointerType *ModelPointerType = ModelType->getAsObjCInterfacePointerType();

      //llvm::outs() << "Checking... " << ModelType.getAsString() << " " << (ModelPointerType ? "MPT" : "") << "\n";
      if (!ModelPointerType || !KeyPathLiteral)
        return true;

      const ObjCInterfaceDecl *ModelIntDecl = ModelPointerType->getInterfaceDecl();
      ASTContext &Ctx = Compiler.getASTContext();

      std::string KeyPathString = KeyPathLiteral->getString()->getString().str();
      Selector Sel = Ctx.Selectors.getNullarySelector(&Ctx.Idents.get(KeyPathString));

      if (!ModelIntDecl->lookupInstanceMethod(Sel)) {
        DiagnosticsEngine &de = Compiler.getDiagnostics();
        unsigned id = de.getCustomDiagID(DiagnosticsEngine::Warning, "Static key path '" + KeyPathString + "' not found as instance method on model class " + ModelType->getPointeeType().getAsString());
        DiagnosticBuilder B = de.Report(KeyPathLiteral->getLocStart(), id);
      }
    }
    return true;
  }
};


void KeyPathValidationConsumer::HandleTranslationUnit(ASTContext &Context) {
  ValueForKeyVisitor(Compiler).TraverseDecl(Context.getTranslationUnitDecl());
  FBBinderVisitor(Compiler).TraverseDecl(Context.getTranslationUnitDecl());
}


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
