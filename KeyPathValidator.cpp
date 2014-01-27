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
          unsigned id = de.getCustomDiagID(DiagnosticsEngine::Warning, "test vfk %0");
          de.Report(E->getLocStart(), id) << KeyString;
        }
      }
    }
    return true;
  }
};


class FBBinderVisitor : public RecursiveASTVisitor<FBBinderVisitor> {
  const CompilerInstance &Compiler;
  unsigned DiagID;

public:
  FBBinderVisitor(const CompilerInstance &compiler)
    : Compiler(compiler)
  {
    DiagID = compiler.getDiagnostics().getCustomDiagID(DiagnosticsEngine::Warning, "key path '%0' not found on model %1");
  }

  bool shouldVisitTemplateInstantiations() const { return false; }
  bool shouldWalkTypesOfTypeLocs() const { return false; }

  bool VisitObjCMessageExpr(ObjCMessageExpr *E) {
    if (E->getNumArgs() == 3 && E->isInstanceMessage() && E->getSelector().getAsString() == "bindToModel:keyPath:change:") {
      Expr *ModelArg = E->getArg(0);
      QualType ModelType = ModelArg->IgnoreImplicit()->getType();
      ObjCStringLiteral *KeyPathLiteral = dyn_cast<ObjCStringLiteral>(E->getArg(1));

      if (!KeyPathLiteral)
        return true;

      QualType PrevType = ModelType;
      size_t Offset = 2; // @"
      typedef std::pair<StringRef,StringRef> StringPair;
      for (StringPair KeyAndPath = KeyPathLiteral->getString()->getString().split('.'); KeyAndPath.first.size() > 0; KeyAndPath = KeyAndPath.second.split('.')) {
        StringRef Key = KeyAndPath.first;
        QualType NextType = CheckKeyPathElement(PrevType, Key);
        if (NextType.isNull()) {
          SourceRange KeyRange = KeyPathLiteral->getSourceRange();
          SourceLocation KeyStart = KeyRange.getBegin().getLocWithOffset(Offset);
          KeyRange.setBegin(KeyStart);
          KeyRange.setEnd(KeyStart.getLocWithOffset(1));

          Compiler.getDiagnostics().Report(KeyStart, DiagID)
            << Key << PrevType->getPointeeType().getAsString()
            << KeyRange << ModelArg->getSourceRange();
          break;
        }
        PrevType = NextType;
        Offset += Key.size() + 1;
      }
    }
    return true;
  }


  QualType CheckKeyPathElement(QualType ObjType, StringRef &Key) {
    // TODO: Primitives to NSValue / NSNumber
    ASTContext &Ctx = Compiler.getASTContext();
    const ObjCObjectPointerType *ObjPointerType = ObjType->getAsObjCInterfacePointerType();
    if (!ObjPointerType)
      return QualType();

    const ObjCInterfaceDecl *ObjInterface = ObjPointerType->getInterfaceDecl();
    if (!ObjInterface)
      return QualType();

    Selector Sel = Ctx.Selectors.getNullarySelector(&Ctx.Idents.get(Key));
    ObjCMethodDecl *Method = ObjInterface->lookupInstanceMethod(Sel);
    return Method ? Method->getReturnType() : QualType();
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
