//
// KeyPathsAffectingVisitor.cpp
// Created by Jonathon Mah on 2014-05-11.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "KeyPathsAffectingVisitor.h"

using namespace clang;


class ReturnSetVisitor : public RecursiveASTVisitor<ReturnSetVisitor> {
  KeyPathValidationConsumer *Consumer;
  QualType Type;
  llvm::SmallSet<Selector, 2> *SetConstructorSelectors;

public:
  ReturnSetVisitor(KeyPathValidationConsumer *Consumer, QualType Type, llvm::SmallSet<Selector, 2> *Selectors)
    : Consumer(Consumer)
    , Type(Type)
    , SetConstructorSelectors(Selectors)
  {}

  bool VisitStmt(const Stmt *Node);
};


bool KeyPathsAffectingVisitor::VisitObjCMethodDecl(ObjCMethodDecl *D) {
  if (!D->isClassMethod())
    return true;

  std::string Name = D->getNameAsString();
  if (Name.length() <= Prefix.length() || !std::equal(Prefix.begin(), Prefix.end(), Name.begin()))
    return true;

  ASTContext &Context = Compiler.getASTContext();
  QualType Type = Context.getObjCObjectPointerType(Context.getObjCInterfaceType(D->getClassInterface()));
  ReturnSetVisitor(Consumer, Type, &SetConstructorSelectors).TraverseDecl(D);

  return true;
}


bool ReturnSetVisitor::VisitStmt(const Stmt *Node) {
  const ReturnStmt *Return = dyn_cast<ReturnStmt>(Node);
  if (!Return)
    return true;

  const ObjCMessageExpr *E = dyn_cast<ObjCMessageExpr>(Return->getRetValue()->IgnoreImplicit());
  if (!E)
    return true;

  QualType ClassReceiver = E->getClassReceiver();
  if (ClassReceiver.isNull() || ClassReceiver.getAsString() != "NSSet")
    return true;

  if (!SetConstructorSelectors->count(E->getSelector()))
    return true;

  for (unsigned I = 0, N = E->getNumArgs(); I < N; ++I)
  {
    const Expr *Arg = E->getArg(I)->IgnoreImplicit();
    const ObjCStringLiteral *KeyPathLiteral = dyn_cast<ObjCStringLiteral>(Arg);
    if (!KeyPathLiteral)
      continue;

    Consumer->emitDiagnosticsForTypeAndKeyPath(Type, Arg, true);

    const StringRef KeyPathString = KeyPathLiteral->getString()->getString();
    llvm::outs() << KeyPathString << ", ";
  }

  return true;
}
