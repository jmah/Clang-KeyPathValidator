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
#include "clang/AST/NSAPI.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace {

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
};


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

  bool VisitObjCMessageExpr(ObjCMessageExpr *E) {
    if (E->getNumArgs() != 1 || !E->isInstanceMessage())
      return true;

    Selector Sel = E->getSelector();
    if (Sel != VFKSelector && Sel != VFKPathSelector)
      return true;

    ObjCStringLiteral *KeyPathLiteral = dyn_cast<ObjCStringLiteral>(E->getArg(0));
    if (!KeyPathLiteral)
      return true;

    bool path = (Sel == VFKPathSelector);
    StringRef KeyPathString = KeyPathLiteral->getString()->getString();
    const size_t KeyCount = 1 + (path ? KeyPathString.count('.') : 0);

    StringRef *Keys = new StringRef[KeyCount];
    if (path) {
      typedef std::pair<StringRef,StringRef> StringPair;
      size_t i = 0;
      for (StringPair KeyAndPath = KeyPathString.split('.');
          KeyAndPath.first.size() > 0;
          KeyAndPath = KeyAndPath.second.split('.'))
        Keys[i++] = KeyAndPath.first;
    } else {
      Keys[0] = KeyPathString;
    }

    ASTContext &Ctx = Compiler.getASTContext();
    QualType ObjType = Ctx.getObjCObjectPointerType(Ctx.getObjCInterfaceType(E->getReceiverInterface()));
    size_t Offset = path ? 2 : 0; // @"  (but whole string for non-path, TODO: Set to 2 and properly adjust end of range)
    for (size_t i = 0; i < KeyCount; i++) {
      StringRef Key = Keys[i];

      bool Valid = Consumer->CheckKeyType(ObjType, Key);
      if (!Valid) {
        SourceRange KeyRange = KeyPathLiteral->getSourceRange();
        SourceLocation KeyStart = KeyRange.getBegin().getLocWithOffset(Offset);
        KeyRange.setBegin(KeyStart);
        if (path)
          KeyRange.setEnd(KeyStart.getLocWithOffset(1));

        Compiler.getDiagnostics().Report(KeyStart, Consumer->KeyDiagID)
          << Key << ObjType->getPointeeType().getAsString()
          << KeyRange << E->getReceiverRange();
        break;
      }
      Offset += Key.size() + 1;
    }

    delete[] Keys;
    return true;
  }
};


class FBBinderVisitor : public RecursiveASTVisitor<FBBinderVisitor> {
  KeyPathValidationConsumer *Consumer;
  const CompilerInstance &Compiler;
  Selector BindSelector;

public:
  FBBinderVisitor(KeyPathValidationConsumer *Consumer, const CompilerInstance &Compiler)
    : Consumer(Consumer)
    , Compiler(Compiler)
  {
    ASTContext &Ctx = Compiler.getASTContext();
    IdentifierTable &IDs = Ctx.Idents;
    IdentifierInfo *BindIIs[3] = {&IDs.get("bindToModel"), &IDs.get("keyPath"), &IDs.get("change")};
    BindSelector = Ctx.Selectors.getSelector(3, BindIIs);
  }

  bool shouldVisitTemplateInstantiations() const { return false; }
  bool shouldWalkTypesOfTypeLocs() const { return false; }

  bool VisitObjCMessageExpr(ObjCMessageExpr *E) {
    if (E->getNumArgs() != 3 || !E->isInstanceMessage())
      return true;

    if (E->getSelector() != BindSelector)
      return true;

    Expr *ModelArg = E->getArg(0);
    QualType ModelType = ModelArg->IgnoreImplicit()->getType();
    ObjCStringLiteral *KeyPathLiteral = dyn_cast<ObjCStringLiteral>(E->getArg(1));

    if (!KeyPathLiteral)
      return true;

    QualType ObjType = ModelType;
    size_t Offset = 2; // @"
    typedef std::pair<StringRef,StringRef> StringPair;
    for (StringPair KeyAndPath = KeyPathLiteral->getString()->getString().split('.'); KeyAndPath.first.size() > 0; KeyAndPath = KeyAndPath.second.split('.')) {
      StringRef Key = KeyAndPath.first;
      bool Valid = Consumer->CheckKeyType(ObjType, Key);
      if (!Valid) {
        SourceRange KeyRange = KeyPathLiteral->getSourceRange();
        SourceLocation KeyStart = KeyRange.getBegin().getLocWithOffset(Offset);
        KeyRange.setBegin(KeyStart);
        KeyRange.setEnd(KeyStart.getLocWithOffset(1));

        Compiler.getDiagnostics().Report(KeyStart, Consumer->KeyDiagID)
          << Key << ObjType->getPointeeType().getAsString()
          << KeyRange << ModelArg->getSourceRange();
        break;
      }
      Offset += Key.size() + 1;
    }
    return true;
  }
};


void KeyPathValidationConsumer::HandleTranslationUnit(ASTContext &Context) {
  ValueForKeyVisitor(this, Compiler).TraverseDecl(Context.getTranslationUnitDecl());
  FBBinderVisitor(this, Compiler).TraverseDecl(Context.getTranslationUnitDecl());
}

bool KeyPathValidationConsumer::CheckKeyType(QualType &ObjTypeInOut, StringRef &Key) {
  const ObjCObjectPointerType *ObjPointerType = ObjTypeInOut->getAsObjCInterfacePointerType();
  if (!ObjPointerType)
    return false;

  const ObjCInterfaceDecl *ObjInterface = ObjPointerType->getInterfaceDecl();
  if (!ObjInterface)
    return false;

  // Special case keys
  if (Key.equals("self"))
    return true; // leave ObjTypeInOut unchanged

  // TODO: Special case ObjType: NSArray, NSSet, NSDictionary, etc.
  Selector Sel = Context.Selectors.getNullarySelector(&Context.Idents.get(Key));
  ObjCMethodDecl *Method = ObjInterface->lookupInstanceMethod(Sel);
  if (!Method)
    return false;

  ObjTypeInOut = Method->getReturnType();
  if (!ObjTypeInOut->isObjCObjectPointerType())
    if (NSAPIObj->getNSNumberFactoryMethodKind(ObjTypeInOut).hasValue()) {
      IdentifierInfo *NSNumberId = NSAPIObj->getNSClassId(NSAPI::ClassId_NSNumber);
      TranslationUnitDecl *TUD = Context.getTranslationUnitDecl();
      DeclContext::lookup_result R = TUD->lookup(NSNumberId);
      if (R.size() > 0) {
        ObjCInterfaceDecl *NSNumberDecl = dyn_cast<ObjCInterfaceDecl>(R[0]);
        ObjTypeInOut = Context.getObjCObjectPointerType(Context.getObjCInterfaceType(NSNumberDecl));
      }
    }

  // TODO: Primitives to NSValue / NSNumber
  return true;
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
