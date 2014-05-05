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
#include "clang/AST/Attr.h"
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

  QualType NSNumberPtrType;

  // Hard-coded set of KVC containers (can't add attributes in a category)
  ObjCInterfaceDecl *NSDictionaryInterface, *NSArrayInterface, *NSSetInterface, *NSOrderedSetInterface;

  void cacheNSTypes();
  bool isKVCContainer(QualType type);
};


void KeyPathValidationConsumer::cacheNSTypes() {
  TranslationUnitDecl *TUD = Context.getTranslationUnitDecl();

  { DeclContext::lookup_result R = TUD->lookup(&Context.Idents.get("NSNumber"));
    if (R.size() > 0) {
      ObjCInterfaceDecl *NSNumberDecl = dyn_cast<ObjCInterfaceDecl>(R[0]);
      NSNumberPtrType = Context.getObjCObjectPointerType(Context.getObjCInterfaceType(NSNumberDecl));
    } }

  { DeclContext::lookup_result R = TUD->lookup(&Context.Idents.get("NSDictionary"));
    if (R.size() > 0)
      NSDictionaryInterface = dyn_cast<ObjCInterfaceDecl>(R[0]); }

  { DeclContext::lookup_result R = TUD->lookup(&Context.Idents.get("NSArray"));
    if (R.size() > 0)
      NSArrayInterface = dyn_cast<ObjCInterfaceDecl>(R[0]); }

  { DeclContext::lookup_result R = TUD->lookup(&Context.Idents.get("NSSet"));
    if (R.size() > 0)
      NSSetInterface = dyn_cast<ObjCInterfaceDecl>(R[0]); }

  { DeclContext::lookup_result R = TUD->lookup(&Context.Idents.get("NSOrderedSet"));
    if (R.size() > 0)
      NSOrderedSetInterface = dyn_cast<ObjCInterfaceDecl>(R[0]); }
}


bool KeyPathValidationConsumer::CheckKeyType(QualType &ObjTypeInOut, StringRef &Key) {
  if (isKVCContainer(ObjTypeInOut)) {
    ObjTypeInOut = Context.getObjCIdType();
    return true;
  }

  // Special case keys
  if (Key.equals("self"))
    return true; // leave ObjTypeInOut unchanged


  std::vector<const ObjCContainerDecl *> ContainerDecls;
  if (const ObjCObjectPointerType *ObjType = ObjTypeInOut->getAs<ObjCObjectPointerType>()) {
    if (const ObjCInterfaceDecl *Interface = ObjType->getInterfaceDecl()) {
      ContainerDecls.push_back(Interface);
    }
    std::copy(ObjType->qual_begin(), ObjType->qual_end(), std::back_inserter(ContainerDecls));
  }

  Selector Sel = Context.Selectors.getNullarySelector(&Context.Idents.get(Key));
  StringRef IsKey = ("is" + Key.substr(0, 1).upper() + Key.substr(1)).str();
  Selector IsSel = Context.Selectors.getNullarySelector(&Context.Idents.get(IsKey));

  ObjCMethodDecl *Method = NULL;
  for (std::vector<const ObjCContainerDecl *>::iterator Decl = ContainerDecls.begin(), DeclEnd = ContainerDecls.end();
      Decl != DeclEnd; ++Decl) {
    if ((Method = (*Decl)->getInstanceMethod(Sel)))
      break;
    if ((Method = (*Decl)->getInstanceMethod(IsSel)))
      break;
  }
  if (!Method)
    return false;

  ObjTypeInOut = Method->getResultType();
  if (!ObjTypeInOut->isObjCObjectPointerType())
    if (NSAPIObj->getNSNumberFactoryMethodKind(ObjTypeInOut).hasValue())
      ObjTypeInOut = NSNumberPtrType;

  // TODO: Primitives to NSValue
  return true;
}

bool KeyPathValidationConsumer::isKVCContainer(QualType Type) {
  if (Type->isObjCIdType())
    return true;

  const ObjCInterfaceDecl *ObjInterface = NULL;
  if (const ObjCObjectPointerType *ObjPointerType = Type->getAsObjCInterfacePointerType())
    ObjInterface = ObjPointerType->getInterfaceDecl();

  // Foundation built-ins
  if (NSDictionaryInterface->isSuperClassOf(ObjInterface) ||
      NSArrayInterface->isSuperClassOf(ObjInterface) ||
      NSSetInterface ->isSuperClassOf(ObjInterface)||
      NSOrderedSetInterface->isSuperClassOf(ObjInterface))
    return true;

  // Check for attribute
  while (ObjInterface) {
    for (Decl::attr_iterator Attr = ObjInterface->attr_begin(), AttrEnd = ObjInterface->attr_end();
        Attr != AttrEnd; ++Attr) {
      if (AnnotateAttr *AA = dyn_cast<AnnotateAttr>(*Attr))
        if (AA->getAnnotation().equals("objc_kvc_container"))
          return true;
    }

    ObjInterface = ObjInterface->getSuperClass();
  }
  return false;
}


//
// ----
//

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

    QualType ObjType = E->getReceiverType();

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
private:
  KeyPathValidationConsumer *Consumer;
  const CompilerInstance &Compiler;
  Selector BindSelector;
  Selector BindMultipleSelector;
  unsigned BindMultipleCountMismatchDiagID;

public:
  FBBinderVisitor(KeyPathValidationConsumer *Consumer, const CompilerInstance &Compiler)
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

  bool shouldVisitTemplateInstantiations() const { return false; }
  bool shouldWalkTypesOfTypeLocs() const { return false; }

  bool VisitObjCMessageExpr(ObjCMessageExpr *E) {
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

  void ValidateModelAndKeyPath(const Expr *ModelExpr, const Expr *KeyPathExpr) {
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
};


void KeyPathValidationConsumer::HandleTranslationUnit(ASTContext &Context) {
  cacheNSTypes();

  ValueForKeyVisitor(this, Compiler).TraverseDecl(Context.getTranslationUnitDecl());
  FBBinderVisitor(this, Compiler).TraverseDecl(Context.getTranslationUnitDecl());
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
